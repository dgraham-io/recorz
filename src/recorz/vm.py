"""Phase 1 interpreter with explicit contexts and processes."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from typing import Any, Optional, Union

from .compiler import compile_do_it
from .model import (
    ActivationRequest,
    BlockClosure,
    Cell,
    CompiledBlock,
    CompiledMethod,
    ExecutionContext,
    InvalidNonLocalReturn,
    LexicalEnvironment,
    MessageNotUnderstood,
    Process,
    RecorzClass,
    RecorzInstance,
    RecorzRuntimeError,
    RuntimeImage,
    SendSite,
    SymbolAtom,
)


Primitive = Callable[["VirtualMachine", Optional[ExecutionContext], Any, list[Any]], Union[Any, ActivationRequest]]


@dataclass
class DebugTrace:
    selector: str
    defining_class: str


class VirtualMachine:
    def __init__(self, image: RuntimeImage):
        self.image = image
        self.primitives: dict[str, Primitive] = {}
        self.processes: list[Process] = []

    def register_primitive(self, name: str, function: Primitive) -> None:
        self.primitives[name] = function

    def class_of(self, value: Any) -> RecorzClass:
        if value is None:
            return self.image.class_named("UndefinedObject")
        if value is True:
            return self.image.class_named("True")
        if value is False:
            return self.image.class_named("False")
        if isinstance(value, int):
            return self.image.class_named("SmallInteger")
        if isinstance(value, str):
            return self.image.class_named("String")
        if isinstance(value, SymbolAtom):
            return self.image.class_named("Symbol")
        if isinstance(value, RecorzInstance):
            return value.cls
        if isinstance(value, RecorzClass):
            if value.is_metaclass:
                return self.image.class_named("Metaclass")
            if value.metaclass is None:
                return self.image.class_named("Class")
            return value.metaclass
        if isinstance(value, BlockClosure):
            return self.image.class_named("BlockClosure")
        if isinstance(value, CompiledMethod):
            return self.image.class_named("CompiledMethod")
        if isinstance(value, ExecutionContext):
            return self.image.class_named("Context")
        if isinstance(value, Process):
            return self.image.class_named("Process")
        raise RecorzRuntimeError(f"No class mapping for value {value!r}")

    def spawn_do_it(self, source: str) -> Process:
        compiled = compile_do_it(source, "Object", self.image.class_named("Object").all_instance_variables())
        receiver = RecorzInstance.new(self.image.class_named("Object"))
        process = self._spawn_method(receiver, compiled, [])
        return process

    def evaluate(self, source: str) -> Any:
        process = self.spawn_do_it(source)
        return self.run_process(process)

    def send(
        self,
        receiver: Any,
        selector: str,
        arguments: list[Any],
        sender: Optional[ExecutionContext] = None,
        super_send: bool = False,
    ) -> Any:
        method = self._lookup_method(receiver, selector, sender, super_send)
        if method.primitive is not None:
            primitive = self.primitives[method.primitive]
            outcome = primitive(self, sender, receiver, arguments)
            if isinstance(outcome, ActivationRequest):
                return self._run_context(outcome.context)
            return outcome
        context = self._activate_method(method, receiver, arguments, sender)
        return self._run_context(context)

    def run_process(self, process: Process) -> Any:
        process.state = "running"
        try:
            result = self._run_context(process.active_context)
            process.state = "terminated"
            process.result = result
            return result
        except Exception:
            process.state = "terminated"
            raise

    def stack_trace(self, context: Optional[ExecutionContext]) -> list[DebugTrace]:
        traces: list[DebugTrace] = []
        current = context
        while current is not None:
            selector = current.code.selector if isinstance(current.code, CompiledMethod) else "<block>"
            traces.append(DebugTrace(selector=selector, defining_class=current.code.defining_class))
            current = current.sender
        return traces

    def _spawn_method(self, receiver: Any, method: CompiledMethod, arguments: list[Any]) -> Process:
        context = self._activate_method(method, receiver, arguments, sender=None)
        process = Process(name=method.selector, active_context=context)
        self.processes.append(process)
        return process

    def _run_context(self, context: ExecutionContext) -> Any:
        current = context
        while True:
            instruction = current.code.instructions[current.pc]
            current.pc += 1
            opcode = instruction.opcode
            if opcode == "push_literal":
                current.stack.append(current.code.literals[instruction.operand])
                continue
            if opcode == "push_self":
                current.stack.append(current.receiver)
                continue
            if opcode == "push_nil":
                current.stack.append(None)
                continue
            if opcode == "push_true":
                current.stack.append(True)
                continue
            if opcode == "push_false":
                current.stack.append(False)
                continue
            if opcode == "push_this_context":
                current.stack.append(current)
                continue
            if opcode == "push_binding":
                current.stack.append(self._read_binding(current, instruction.operand))
                continue
            if opcode == "store_binding":
                value = current.stack.pop()
                self._write_binding(current, instruction.operand, value)
                continue
            if opcode == "duplicate":
                current.stack.append(current.stack[-1])
                continue
            if opcode == "pop":
                current.stack.pop()
                continue
            if opcode == "make_block":
                compiled_block = current.code.literals[instruction.operand]
                current.stack.append(
                    BlockClosure(
                        compiled_block=compiled_block,
                        receiver=current.receiver,
                        outer_environment=current.lexical_environment,
                        home_context=current.home_context or current,
                    )
                )
                continue
            if opcode == "send":
                send_site: SendSite = instruction.operand
                arguments = [current.stack.pop() for _ in range(send_site.argument_count)]
                arguments.reverse()
                receiver = current.stack.pop()
                method = self._lookup_method(receiver, send_site.selector, current, send_site.super_send)
                if method.primitive is not None:
                    primitive = self.primitives[method.primitive]
                    outcome = primitive(self, current, receiver, arguments)
                    if isinstance(outcome, ActivationRequest):
                        current = outcome.context
                    else:
                        current.stack.append(outcome)
                    continue
                current = self._activate_method(method, receiver, arguments, current)
                continue
            if opcode == "return_local":
                value = current.stack.pop() if current.stack else None
                current.completed = True
                if current.sender is None:
                    return value
                current.sender.stack.append(value)
                current = current.sender
                continue
            if opcode == "return_non_local":
                value = current.stack.pop() if current.stack else None
                target = current.home_context
                if target is None or target.completed:
                    raise InvalidNonLocalReturn("Block attempted to return to a dead home context")
                target.completed = True
                if target.sender is None:
                    return value
                target.sender.stack.append(value)
                current = target.sender
                continue
            raise RecorzRuntimeError(f"Unknown opcode {opcode}")

    def _activate_method(
        self,
        method: CompiledMethod,
        receiver: Any,
        arguments: list[Any],
        sender: Optional[ExecutionContext],
    ) -> ExecutionContext:
        if len(arguments) != len(method.arg_names):
            raise RecorzRuntimeError(
                f"Selector {method.selector} expected {len(method.arg_names)} arguments but received {len(arguments)}"
            )
        values = {name: Cell(value) for name, value in zip(method.arg_names, arguments)}
        values.update({name: Cell(None) for name in method.temp_names})
        environment = LexicalEnvironment(values=values)
        return ExecutionContext(
            code=method,
            receiver=receiver,
            lexical_environment=environment,
            sender=sender,
            arguments=list(arguments),
        )

    def _activate_block(self, closure: BlockClosure, arguments: list[Any], sender: ExecutionContext) -> ActivationRequest:
        block = closure.compiled_block
        if len(arguments) != len(block.arg_names):
            raise RecorzRuntimeError(
                f"Block expected {len(block.arg_names)} arguments but received {len(arguments)}"
            )
        values = {name: Cell(value) for name, value in zip(block.arg_names, arguments)}
        values.update({name: Cell(None) for name in block.temp_names})
        environment = LexicalEnvironment(values=values, parent=closure.outer_environment)
        context = ExecutionContext(
            code=block,
            receiver=closure.receiver,
            lexical_environment=environment,
            sender=sender,
            home_context=closure.home_context,
            arguments=list(arguments),
        )
        return ActivationRequest(context)

    def _lookup_method(
        self,
        receiver: Any,
        selector: str,
        sender: Optional[ExecutionContext],
        super_send: bool,
    ) -> CompiledMethod:
        defining_class_name = sender.code.defining_class if sender is not None else None
        receiver_class = self.class_of(receiver)
        lookup_class = receiver_class
        if super_send:
            if defining_class_name is None:
                raise RecorzRuntimeError("super send requires an active defining class")
            lookup_class = self.image.class_named(defining_class_name).superclass
            if lookup_class is None:
                raise MessageNotUnderstood(f"super send of {selector} has no superclass to search")
        method = lookup_class.lookup(selector) if lookup_class is not None else None
        if method is not None:
            return method
        dnu = receiver_class.lookup("doesNotUnderstand:")
        if dnu is None or selector == "doesNotUnderstand:":
            raise MessageNotUnderstood(f"{receiver_class.name} does not understand {selector}")
        if dnu.primitive is not None:
            primitive = self.primitives[dnu.primitive]
            outcome = primitive(self, sender, receiver, [SymbolAtom(selector)])
            if isinstance(outcome, ActivationRequest):
                raise RecorzRuntimeError("doesNotUnderstand: must not activate a block in the seed runtime")
            raise MessageNotUnderstood(str(outcome))
        return dnu

    def _read_binding(self, context: ExecutionContext, binding: Any) -> Any:
        if binding.kind == "lexical":
            cell = context.lexical_environment.lookup(binding.name)
            if cell is None:
                raise RecorzRuntimeError(f"Unknown lexical binding {binding.name}")
            return cell.value
        if binding.kind == "instance":
            if not isinstance(context.receiver, RecorzInstance):
                raise RecorzRuntimeError(f"Receiver has no instance variables: {context.receiver!r}")
            return context.receiver.fields[binding.name]
        if binding.kind == "global":
            if binding.name not in self.image.globals:
                raise RecorzRuntimeError(f"Unknown global {binding.name}")
            return self.image.globals[binding.name]
        raise RecorzRuntimeError(f"Unknown binding kind {binding.kind}")

    def _write_binding(self, context: ExecutionContext, binding: Any, value: Any) -> None:
        if binding.kind == "lexical":
            cell = context.lexical_environment.lookup(binding.name)
            if cell is None:
                raise RecorzRuntimeError(f"Unknown lexical binding {binding.name}")
            cell.value = value
            return
        if binding.kind == "instance":
            if not isinstance(context.receiver, RecorzInstance):
                raise RecorzRuntimeError(f"Receiver has no instance variables: {context.receiver!r}")
            context.receiver.fields[binding.name] = value
            return
        if binding.kind == "global":
            self.image.globals[binding.name] = value
            return
        raise RecorzRuntimeError(f"Unknown binding kind {binding.kind}")


def primitive_class(vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, _arguments: list[Any]) -> Any:
    return vm.class_of(receiver)


def primitive_new(vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, _arguments: list[Any]) -> Any:
    if not isinstance(receiver, RecorzClass):
        raise RecorzRuntimeError("new must be sent to a class")
    return RecorzInstance.new(receiver)


def primitive_class_name(_vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, _arguments: list[Any]) -> Any:
    if not isinstance(receiver, RecorzClass):
        raise RecorzRuntimeError("name must be sent to a class")
    return receiver.name


def primitive_class_superclass(_vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, _arguments: list[Any]) -> Any:
    if not isinstance(receiver, RecorzClass):
        raise RecorzRuntimeError("superclass must be sent to a class")
    return receiver.superclass


def primitive_identity(_vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, arguments: list[Any]) -> Any:
    return receiver is arguments[0]


def primitive_equals(_vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, arguments: list[Any]) -> Any:
    return receiver == arguments[0]


def primitive_print_string(vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, _arguments: list[Any]) -> Any:
    if receiver is None:
        return "nil"
    if receiver is True:
        return "true"
    if receiver is False:
        return "false"
    if isinstance(receiver, SymbolAtom):
        return str(receiver)
    if isinstance(receiver, RecorzClass):
        return receiver.name
    if isinstance(receiver, RecorzInstance):
        return f"a {receiver.cls.name}"
    if isinstance(receiver, ExecutionContext):
        selector = receiver.code.selector if isinstance(receiver.code, CompiledMethod) else "<block>"
        return f"Context({selector})"
    return str(receiver)


def primitive_dnu(_vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, arguments: list[Any]) -> Any:
    selector = arguments[0]
    return f"{receiver!r} does not understand {selector}"


def primitive_small_integer_add(_vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, arguments: list[Any]) -> Any:
    return receiver + arguments[0]


def primitive_small_integer_subtract(_vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, arguments: list[Any]) -> Any:
    return receiver - arguments[0]


def primitive_small_integer_multiply(_vm: VirtualMachine, _context: Optional[ExecutionContext], receiver: Any, arguments: list[Any]) -> Any:
    return receiver * arguments[0]


def primitive_true_if_true(
    vm: VirtualMachine, context: Optional[ExecutionContext], _receiver: Any, arguments: list[Any]
) -> Union[Any, ActivationRequest]:
    if context is None:
        raise RecorzRuntimeError("Block activation requires an active context")
    return vm._activate_block(arguments[0], [], context)


def primitive_true_if_false(
    _vm: VirtualMachine, _context: Optional[ExecutionContext], _receiver: Any, _arguments: list[Any]
) -> Any:
    return None


def primitive_false_if_true(
    _vm: VirtualMachine, _context: Optional[ExecutionContext], _receiver: Any, _arguments: list[Any]
) -> Any:
    return None


def primitive_false_if_false(
    vm: VirtualMachine, context: Optional[ExecutionContext], _receiver: Any, arguments: list[Any]
) -> Union[Any, ActivationRequest]:
    if context is None:
        raise RecorzRuntimeError("Block activation requires an active context")
    return vm._activate_block(arguments[0], [], context)


def primitive_true_if_true_if_false(
    vm: VirtualMachine, context: Optional[ExecutionContext], _receiver: Any, arguments: list[Any]
) -> Union[Any, ActivationRequest]:
    if context is None:
        raise RecorzRuntimeError("Block activation requires an active context")
    return vm._activate_block(arguments[0], [], context)


def primitive_false_if_true_if_false(
    vm: VirtualMachine, context: Optional[ExecutionContext], _receiver: Any, arguments: list[Any]
) -> Union[Any, ActivationRequest]:
    if context is None:
        raise RecorzRuntimeError("Block activation requires an active context")
    return vm._activate_block(arguments[1], [], context)


def primitive_true_not(
    _vm: VirtualMachine, _context: Optional[ExecutionContext], _receiver: Any, _arguments: list[Any]
) -> Any:
    return False


def primitive_false_not(
    _vm: VirtualMachine, _context: Optional[ExecutionContext], _receiver: Any, _arguments: list[Any]
) -> Any:
    return True


def primitive_block_value(
    vm: VirtualMachine, context: Optional[ExecutionContext], receiver: Any, arguments: list[Any]
) -> Union[Any, ActivationRequest]:
    if context is None:
        raise RecorzRuntimeError("Block activation requires an active context")
    if not isinstance(receiver, BlockClosure):
        raise RecorzRuntimeError("value must be sent to a block")
    return vm._activate_block(receiver, arguments, context)
