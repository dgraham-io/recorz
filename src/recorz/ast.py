"""Small, inspectable AST nodes for the Phase 1 compiler."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class SourceSpan:
    start: int
    end: int


@dataclass(frozen=True)
class Identifier:
    name: str
    span: SourceSpan


@dataclass(frozen=True)
class IntegerLiteral:
    value: int
    span: SourceSpan


@dataclass(frozen=True)
class StringLiteral:
    value: str
    span: SourceSpan


@dataclass(frozen=True)
class SymbolLiteral:
    value: str
    span: SourceSpan


@dataclass(frozen=True)
class Message:
    selector: str
    arguments: tuple[object, ...]
    span: SourceSpan


@dataclass(frozen=True)
class MessageSend:
    receiver: object
    selector: str
    arguments: tuple[object, ...]
    span: SourceSpan


@dataclass(frozen=True)
class Cascade:
    receiver: object
    messages: tuple[Message, ...]
    span: SourceSpan


@dataclass(frozen=True)
class Assignment:
    name: str
    value: object
    span: SourceSpan


@dataclass(frozen=True)
class Return:
    value: object
    span: SourceSpan


@dataclass(frozen=True)
class Block:
    parameters: tuple[str, ...]
    temporaries: tuple[str, ...]
    body: "Sequence"
    span: SourceSpan


@dataclass(frozen=True)
class Sequence:
    statements: tuple[object, ...]
    span: SourceSpan


@dataclass(frozen=True)
class MethodDefinition:
    selector: str
    parameters: tuple[str, ...]
    temporaries: tuple[str, ...]
    body: Sequence
    source: str
    span: SourceSpan


@dataclass(frozen=True)
class DoItDefinition:
    temporaries: tuple[str, ...]
    body: Sequence
    source: str
    span: SourceSpan
