"""Inspectable runtime objects for the Recorz seed implementation."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Optional, Union


class RecorzError(Exception):
    """Base exception for the seed implementation."""


class ParseError(RecorzError):
    """Raised for parse failures."""


class CompilerError(RecorzError):
    """Raised for compiler failures."""


class RecorzRuntimeError(RecorzError):
    """Raised for runtime failures."""


class MessageNotUnderstood(RecorzRuntimeError):
    """Raised after the doesNotUnderstand: path fails."""


class InvalidNonLocalReturn(RecorzRuntimeError):
    """Raised when a block returns to a dead home context."""


@dataclass(frozen=True)
class SymbolAtom:
    name: str

    def __str__(self) -> str:
        return f"#{self.name}"


@dataclass(frozen=True)
class BindingRef:
    kind: str
    name: str


@dataclass(frozen=True)
class SendSite:
    selector: str
    argument_count: int
    super_send: bool = False


@dataclass(frozen=True)
class Instruction:
    opcode: str
    operand: Any = None


@dataclass
class CompiledCode:
    arg_names: list[str]
    temp_names: list[str]
    instructions: list[Instruction]
    literals: list[Any]
    source: str
    defining_class: str


@dataclass
class CompiledMethod(CompiledCode):
    selector: str = ""
    primitive: Optional[str] = None
    category: str = "default"


@dataclass
class CompiledBlock(CompiledCode):
    pass


@dataclass
class Cell:
    value: Any


@dataclass
class LexicalEnvironment:
    values: dict[str, Cell]
    parent: Optional["LexicalEnvironment"] = None

    def lookup(self, name: str) -> Optional[Cell]:
        environment: Optional["LexicalEnvironment"] = self
        while environment is not None:
            cell = environment.values.get(name)
            if cell is not None:
                return cell
            environment = environment.parent
        return None


@dataclass
class RecorzClass:
    name: str
    superclass: Optional["RecorzClass"] = None
    instance_variables: list[str] = field(default_factory=list)
    methods: dict[str, CompiledMethod] = field(default_factory=dict)
    metaclass: Optional["RecorzClass"] = None
    is_metaclass: bool = False

    def all_instance_variables(self) -> list[str]:
        inherited = [] if self.superclass is None else self.superclass.all_instance_variables()
        return inherited + list(self.instance_variables)

    def lookup(self, selector: str) -> Optional[CompiledMethod]:
        current: Optional["RecorzClass"] = self
        while current is not None:
            method = current.methods.get(selector)
            if method is not None:
                return method
            current = current.superclass
        return None


@dataclass
class RecorzInstance:
    cls: RecorzClass
    fields: dict[str, Any]

    @classmethod
    def new(cls, recorz_class: RecorzClass, nil_object: Any = None) -> "RecorzInstance":
        field_map = {name: nil_object for name in recorz_class.all_instance_variables()}
        return cls(cls=recorz_class, fields=field_map)


@dataclass
class BlockClosure:
    compiled_block: CompiledBlock
    receiver: Any
    outer_environment: LexicalEnvironment
    home_context: "ExecutionContext"


@dataclass
class ExecutionContext:
    code: Union[CompiledMethod, CompiledBlock]
    receiver: Any
    lexical_environment: LexicalEnvironment
    sender: Optional["ExecutionContext"] = None
    home_context: Optional["ExecutionContext"] = None
    stack: list[Any] = field(default_factory=list)
    arguments: list[Any] = field(default_factory=list)
    pc: int = 0
    completed: bool = False

    def __post_init__(self) -> None:
        if self.home_context is None:
            self.home_context = self


@dataclass
class Process:
    name: str
    active_context: ExecutionContext
    state: str = "new"
    priority: int = 50
    waiting_reason: Optional[str] = None
    result: Any = None


@dataclass
class ActivationRequest:
    context: ExecutionContext


@dataclass
class RuntimeImage:
    classes: dict[str, RecorzClass] = field(default_factory=dict)
    globals: dict[str, Any] = field(default_factory=dict)
    metadata: dict[str, Any] = field(default_factory=dict)

    def class_named(self, name: str) -> RecorzClass:
        return self.classes[name]

    def install_method(self, class_name: str, method: CompiledMethod) -> None:
        self.classes[class_name].methods[method.selector] = method
