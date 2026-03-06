"""Compiler from the AST to a small inspectable instruction list."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional

from . import ast
from .model import BindingRef, CompiledBlock, CompiledMethod, CompilerError, Instruction, SendSite, SymbolAtom
from .parser import parse_do_it, parse_method


@dataclass
class _Scope:
    lexical_names: set[str]
    instance_variables: set[str]
    parent: Optional["_Scope"] = None
    in_block: bool = False

    def is_lexical(self, name: str) -> bool:
        scope: Optional["_Scope"] = self
        while scope is not None:
            if name in scope.lexical_names:
                return True
            scope = scope.parent
        return False


@dataclass
class _Emitter:
    instructions: list[Instruction] = field(default_factory=list)
    literals: list[object] = field(default_factory=list)

    def emit(self, opcode: str, operand: object = None) -> None:
        self.instructions.append(Instruction(opcode, operand))

    def literal_index(self, value: object) -> int:
        for index, literal in enumerate(self.literals):
            if literal == value:
                return index
        self.literals.append(value)
        return len(self.literals) - 1


class Compiler:
    def __init__(self, class_name: str, instance_variables: list[str]):
        self.class_name = class_name
        self.instance_variables = set(instance_variables)

    def compile_method(self, source: str) -> CompiledMethod:
        method_definition = parse_method(source)
        emitter = _Emitter()
        scope = _Scope(
            lexical_names=set(method_definition.parameters) | set(method_definition.temporaries),
            instance_variables=self.instance_variables,
        )
        self._compile_sequence(method_definition.body, emitter, scope, final_position=True)
        if not emitter.instructions or emitter.instructions[-1].opcode not in {"return_local", "return_non_local"}:
            emitter.emit("push_nil")
            emitter.emit("return_local")
        return CompiledMethod(
            selector=method_definition.selector,
            defining_class=self.class_name,
            arg_names=list(method_definition.parameters),
            temp_names=list(method_definition.temporaries),
            instructions=emitter.instructions,
            literals=emitter.literals,
            source=source,
        )

    def compile_do_it(self, source: str) -> CompiledMethod:
        do_it = parse_do_it(source)
        emitter = _Emitter()
        scope = _Scope(lexical_names=set(do_it.temporaries), instance_variables=self.instance_variables)
        self._compile_sequence(do_it.body, emitter, scope, final_position=True)
        if not emitter.instructions or emitter.instructions[-1].opcode not in {"return_local", "return_non_local"}:
            emitter.emit("return_local")
        return CompiledMethod(
            selector="__doIt__",
            defining_class=self.class_name,
            arg_names=[],
            temp_names=list(do_it.temporaries),
            instructions=emitter.instructions,
            literals=emitter.literals,
            source=source,
        )

    def _compile_sequence(self, sequence: ast.Sequence, emitter: _Emitter, scope: _Scope, final_position: bool) -> None:
        if not sequence.statements:
            emitter.emit("push_nil")
            if final_position:
                emitter.emit("return_local")
            return
        for index, statement in enumerate(sequence.statements):
            is_last = index == len(sequence.statements) - 1
            self._compile_node(statement, emitter, scope, final_position=is_last and final_position)
            if not is_last and emitter.instructions and emitter.instructions[-1].opcode not in {"return_local", "return_non_local"}:
                emitter.emit("pop")

    def _compile_node(self, node: object, emitter: _Emitter, scope: _Scope, final_position: bool) -> None:
        if isinstance(node, ast.IntegerLiteral):
            emitter.emit("push_literal", emitter.literal_index(node.value))
            if final_position:
                return
            return
        if isinstance(node, ast.StringLiteral):
            emitter.emit("push_literal", emitter.literal_index(node.value))
            return
        if isinstance(node, ast.SymbolLiteral):
            emitter.emit("push_literal", emitter.literal_index(SymbolAtom(node.value)))
            return
        if isinstance(node, ast.Identifier):
            self._compile_identifier(node, emitter, scope)
            return
        if isinstance(node, ast.Assignment):
            self._compile_node(node.value, emitter, scope, final_position=False)
            emitter.emit("duplicate")
            emitter.emit("store_binding", self._binding_for(node.name, scope))
            return
        if isinstance(node, ast.Return):
            self._compile_node(node.value, emitter, scope, final_position=False)
            emitter.emit("return_non_local" if scope.in_block else "return_local")
            return
        if isinstance(node, ast.Block):
            self._compile_block(node, emitter, scope)
            return
        if isinstance(node, ast.MessageSend):
            self._compile_send(node.receiver, node.selector, list(node.arguments), emitter, scope)
            return
        if isinstance(node, ast.Cascade):
            self._compile_cascade(node, emitter, scope)
            return
        if isinstance(node, ast.Sequence):
            self._compile_sequence(node, emitter, scope, final_position=final_position)
            return
        raise CompilerError(f"Unsupported AST node {type(node)!r}")

    def _compile_identifier(self, identifier: ast.Identifier, emitter: _Emitter, scope: _Scope) -> None:
        name = identifier.name
        if name == "self":
            emitter.emit("push_self")
            return
        if name == "nil":
            emitter.emit("push_nil")
            return
        if name == "true":
            emitter.emit("push_true")
            return
        if name == "false":
            emitter.emit("push_false")
            return
        if name == "thisContext":
            emitter.emit("push_this_context")
            return
        if name == "super":
            raise CompilerError("super may only be used as a message receiver")
        emitter.emit("push_binding", self._binding_for(name, scope))

    def _compile_block(self, block: ast.Block, emitter: _Emitter, scope: _Scope) -> None:
        block_emitter = _Emitter()
        block_scope = _Scope(
            lexical_names=set(block.parameters) | set(block.temporaries),
            instance_variables=scope.instance_variables,
            parent=scope,
            in_block=True,
        )
        self._compile_sequence(block.body, block_emitter, block_scope, final_position=True)
        if not block_emitter.instructions or block_emitter.instructions[-1].opcode not in {"return_local", "return_non_local"}:
            block_emitter.emit("return_local")
        compiled_block = CompiledBlock(
            defining_class=self.class_name,
            arg_names=list(block.parameters),
            temp_names=list(block.temporaries),
            instructions=block_emitter.instructions,
            literals=block_emitter.literals,
            source=self._source_for(block),
        )
        emitter.emit("make_block", emitter.literal_index(compiled_block))

    def _compile_send(
        self,
        receiver: object,
        selector: str,
        arguments: list[object],
        emitter: _Emitter,
        scope: _Scope,
    ) -> None:
        super_send = isinstance(receiver, ast.Identifier) and receiver.name == "super"
        if super_send:
            emitter.emit("push_self")
        else:
            self._compile_node(receiver, emitter, scope, final_position=False)
        for argument in arguments:
            self._compile_node(argument, emitter, scope, final_position=False)
        emitter.emit("send", SendSite(selector, len(arguments), super_send))

    def _compile_cascade(self, cascade: ast.Cascade, emitter: _Emitter, scope: _Scope) -> None:
        super_send = isinstance(cascade.receiver, ast.Identifier) and cascade.receiver.name == "super"
        if super_send:
            emitter.emit("push_self")
        else:
            self._compile_node(cascade.receiver, emitter, scope, final_position=False)
        for index, message in enumerate(cascade.messages):
            if index < len(cascade.messages) - 1:
                emitter.emit("duplicate")
            for argument in message.arguments:
                self._compile_node(argument, emitter, scope, final_position=False)
            emitter.emit("send", SendSite(message.selector, len(message.arguments), super_send))
            if index < len(cascade.messages) - 1:
                emitter.emit("pop")

    def _binding_for(self, name: str, scope: _Scope) -> BindingRef:
        if scope.is_lexical(name):
            return BindingRef("lexical", name)
        if name in self.instance_variables:
            return BindingRef("instance", name)
        return BindingRef("global", name)

    def _source_for(self, node: object) -> str:
        return ""


def compile_method(source: str, class_name: str, instance_variables: list[str]) -> CompiledMethod:
    return Compiler(class_name, instance_variables).compile_method(source)


def compile_do_it(source: str, class_name: str, instance_variables: list[str]) -> CompiledMethod:
    return Compiler(class_name, instance_variables).compile_do_it(source)
