"""Recursive descent parser for the untyped Recorz Phase 1 subset."""

from __future__ import annotations

from dataclasses import dataclass

from . import ast
from .model import ParseError


@dataclass(frozen=True)
class Token:
    kind: str
    value: str
    start: int
    end: int


_BINARY_CHARS = set("+-*/\\~<>=@%&?,")


def tokenize(source: str) -> list[Token]:
    tokens: list[Token] = []
    index = 0
    length = len(source)
    while index < length:
        char = source[index]
        if char.isspace():
            index += 1
            continue
        if char == '"':
            index += 1
            while index < length and source[index] != '"':
                index += 1
            if index >= length:
                raise ParseError("Unterminated comment")
            index += 1
            continue
        if source.startswith(":=", index):
            tokens.append(Token("ASSIGN", ":=", index, index + 2))
            index += 2
            continue
        if char == "^":
            tokens.append(Token("RETURN", char, index, index + 1))
            index += 1
            continue
        if char == ".":
            tokens.append(Token("PERIOD", char, index, index + 1))
            index += 1
            continue
        if char == ";":
            tokens.append(Token("SEMICOLON", char, index, index + 1))
            index += 1
            continue
        if char == "(":
            tokens.append(Token("LPAREN", char, index, index + 1))
            index += 1
            continue
        if char == ")":
            tokens.append(Token("RPAREN", char, index, index + 1))
            index += 1
            continue
        if char == "[":
            tokens.append(Token("LBRACKET", char, index, index + 1))
            index += 1
            continue
        if char == "]":
            tokens.append(Token("RBRACKET", char, index, index + 1))
            index += 1
            continue
        if char == "|":
            tokens.append(Token("PIPE", char, index, index + 1))
            index += 1
            continue
        if char == ":":
            tokens.append(Token("COLON", char, index, index + 1))
            index += 1
            continue
        if char == "'":
            start = index
            index += 1
            pieces: list[str] = []
            while index < length:
                if source[index] == "'" and index + 1 < length and source[index + 1] == "'":
                    pieces.append("'")
                    index += 2
                    continue
                if source[index] == "'":
                    index += 1
                    break
                pieces.append(source[index])
                index += 1
            else:
                raise ParseError("Unterminated string literal")
            tokens.append(Token("STRING", "".join(pieces), start, index))
            continue
        if char == "#":
            start = index
            index += 1
            if index < length and source[index] == "'":
                string_token = tokenize(source[index:])[0]
                if string_token.kind != "STRING":
                    raise ParseError("Expected string after #")
                token_end = index + string_token.end
                tokens.append(Token("SYMBOL", string_token.value, start, token_end))
                index = token_end
                continue
            symbol_start = index
            while index < length and (source[index].isalnum() or source[index] == "_" or source[index] == ":"):
                index += 1
            if symbol_start == index:
                raise ParseError("Expected symbol literal")
            tokens.append(Token("SYMBOL", source[symbol_start:index], start, index))
            continue
        if char.isdigit():
            start = index
            while index < length and source[index].isdigit():
                index += 1
            tokens.append(Token("INTEGER", source[start:index], start, index))
            continue
        if char.isalpha() or char == "_":
            start = index
            while index < length and (source[index].isalnum() or source[index] == "_"):
                index += 1
            value = source[start:index]
            if index < length and source[index] == ":":
                tokens.append(Token("KEYWORD", value + ":", start, index + 1))
                index += 1
            else:
                tokens.append(Token("IDENT", value, start, index))
            continue
        if char in _BINARY_CHARS:
            start = index
            while index < length and source[index] in _BINARY_CHARS:
                index += 1
            tokens.append(Token("BINARY", source[start:index], start, index))
            continue
        raise ParseError(f"Unexpected character {char!r}")
    tokens.append(Token("EOF", "", length, length))
    return tokens


@dataclass
class _Chain:
    expression: object
    cascade_receiver: object
    cascade_messages: list[ast.Message]


class Parser:
    def __init__(self, source: str):
        self.source = source
        self.tokens = tokenize(source)
        self.index = 0

    def parse_method(self) -> ast.MethodDefinition:
        header_start = self.current.start
        selector, parameters = self.parse_method_header()
        temporaries = self.parse_temporaries()
        body = self.parse_sequence({"EOF"})
        span = ast.SourceSpan(header_start, body.span.end)
        self.expect("EOF")
        return ast.MethodDefinition(
            selector=selector,
            parameters=tuple(parameters),
            temporaries=tuple(temporaries),
            body=body,
            source=self.source,
            span=span,
        )

    def parse_do_it(self) -> ast.DoItDefinition:
        start = self.current.start
        temporaries = self.parse_temporaries()
        body = self.parse_sequence({"EOF"})
        self.expect("EOF")
        return ast.DoItDefinition(
            temporaries=tuple(temporaries),
            body=body,
            source=self.source,
            span=ast.SourceSpan(start, body.span.end),
        )

    def parse_method_header(self) -> tuple[str, list[str]]:
        if self.match("BINARY"):
            selector = self.consume("BINARY").value
            argument = self.consume("IDENT").value
            return selector, [argument]
        if self.match("KEYWORD"):
            parts: list[str] = []
            parameters: list[str] = []
            while self.match("KEYWORD"):
                parts.append(self.consume("KEYWORD").value)
                parameters.append(self.consume("IDENT").value)
            return "".join(parts), parameters
        selector = self.consume("IDENT").value
        return selector, []

    def parse_temporaries(self) -> list[str]:
        if not self.match("PIPE"):
            return []
        self.consume("PIPE")
        temporaries: list[str] = []
        while not self.match("PIPE"):
            temporaries.append(self.consume("IDENT").value)
        self.consume("PIPE")
        return temporaries

    def parse_sequence(self, terminators: set[str]) -> ast.Sequence:
        statements: list[object] = []
        while not self.match_any(terminators):
            statements.append(self.parse_statement())
            if self.match("PERIOD"):
                self.consume("PERIOD")
                while self.match("PERIOD"):
                    self.consume("PERIOD")
                continue
            break
        start = statements[0].span.start if statements else self.current.start
        end = statements[-1].span.end if statements else self.current.start
        return ast.Sequence(tuple(statements), ast.SourceSpan(start, end))

    def parse_statement(self) -> object:
        if self.match("RETURN"):
            token = self.consume("RETURN")
            value = self.parse_expression()
            return ast.Return(value, ast.SourceSpan(token.start, value.span.end))
        if self.match("IDENT") and self.peek(1).kind == "ASSIGN":
            name_token = self.consume("IDENT")
            self.consume("ASSIGN")
            value = self.parse_expression()
            return ast.Assignment(name_token.value, value, ast.SourceSpan(name_token.start, value.span.end))
        return self.parse_expression()

    def parse_expression(self) -> object:
        chain = self.parse_keyword_expression()
        if self.match("SEMICOLON"):
            messages = list(chain.cascade_messages)
            receiver = chain.cascade_receiver
            while self.match("SEMICOLON"):
                self.consume("SEMICOLON")
                messages.append(self.parse_cascade_message())
            span = ast.SourceSpan(receiver.span.start, messages[-1].span.end)
            return ast.Cascade(receiver, tuple(messages), span)
        return chain.expression

    def parse_keyword_expression(self) -> _Chain:
        chain = self.parse_binary_expression()
        if not self.match("KEYWORD"):
            return chain
        selector_parts: list[str] = []
        arguments: list[object] = []
        start = self.current.start
        while self.match("KEYWORD"):
            selector_parts.append(self.consume("KEYWORD").value)
            arguments.append(self.parse_binary_expression().expression)
        message = ast.Message("".join(selector_parts), tuple(arguments), ast.SourceSpan(start, arguments[-1].span.end))
        expression = ast.MessageSend(chain.expression, message.selector, message.arguments, ast.SourceSpan(chain.expression.span.start, message.span.end))
        return _Chain(expression, chain.expression, [message])

    def parse_binary_expression(self) -> _Chain:
        chain = self.parse_unary_expression()
        if not self.match("BINARY"):
            return chain
        receiver = chain.expression
        current_expression = receiver
        messages: list[ast.Message] = []
        while self.match("BINARY"):
            selector = self.consume("BINARY")
            argument = self.parse_unary_expression().expression
            message = ast.Message(selector.value, (argument,), ast.SourceSpan(selector.start, argument.span.end))
            messages.append(message)
            current_expression = ast.MessageSend(current_expression, message.selector, message.arguments, ast.SourceSpan(receiver.span.start, message.span.end))
        return _Chain(current_expression, receiver, messages)

    def parse_unary_expression(self) -> _Chain:
        receiver = self.parse_primary()
        current_expression = receiver
        messages: list[ast.Message] = []
        while self.match("IDENT"):
            selector = self.consume("IDENT")
            message = ast.Message(selector.value, (), ast.SourceSpan(selector.start, selector.end))
            messages.append(message)
            current_expression = ast.MessageSend(current_expression, message.selector, message.arguments, ast.SourceSpan(receiver.span.start, selector.end))
        return _Chain(current_expression, receiver, messages)

    def parse_primary(self) -> object:
        if self.match("INTEGER"):
            token = self.consume("INTEGER")
            return ast.IntegerLiteral(int(token.value), ast.SourceSpan(token.start, token.end))
        if self.match("STRING"):
            token = self.consume("STRING")
            return ast.StringLiteral(token.value, ast.SourceSpan(token.start, token.end))
        if self.match("SYMBOL"):
            token = self.consume("SYMBOL")
            return ast.SymbolLiteral(token.value, ast.SourceSpan(token.start, token.end))
        if self.match("IDENT"):
            token = self.consume("IDENT")
            return ast.Identifier(token.value, ast.SourceSpan(token.start, token.end))
        if self.match("LPAREN"):
            self.consume("LPAREN")
            expression = self.parse_expression()
            self.expect("RPAREN")
            self.consume("RPAREN")
            return expression
        if self.match("LBRACKET"):
            return self.parse_block()
        raise ParseError(f"Unexpected token {self.current.kind}")

    def parse_block(self) -> ast.Block:
        start = self.consume("LBRACKET").start
        parameters: list[str] = []
        while self.match("COLON"):
            self.consume("COLON")
            parameters.append(self.consume("IDENT").value)
        if parameters:
            self.consume("PIPE")
        temporaries = self.parse_temporaries()
        body = self.parse_sequence({"RBRACKET"})
        end = self.consume("RBRACKET").end
        return ast.Block(tuple(parameters), tuple(temporaries), body, ast.SourceSpan(start, end))

    def parse_cascade_message(self) -> ast.Message:
        if self.match("IDENT"):
            token = self.consume("IDENT")
            return ast.Message(token.value, (), ast.SourceSpan(token.start, token.end))
        if self.match("BINARY"):
            selector = self.consume("BINARY")
            argument = self.parse_unary_expression().expression
            return ast.Message(selector.value, (argument,), ast.SourceSpan(selector.start, argument.span.end))
        if self.match("KEYWORD"):
            parts: list[str] = []
            arguments: list[object] = []
            start = self.current.start
            while self.match("KEYWORD"):
                parts.append(self.consume("KEYWORD").value)
                arguments.append(self.parse_binary_expression().expression)
            return ast.Message("".join(parts), tuple(arguments), ast.SourceSpan(start, arguments[-1].span.end))
        raise ParseError("Expected cascade message")

    def match(self, kind: str) -> bool:
        return self.current.kind == kind

    def match_any(self, kinds: set[str]) -> bool:
        return self.current.kind in kinds

    def peek(self, offset: int) -> Token:
        return self.tokens[self.index + offset]

    @property
    def current(self) -> Token:
        return self.tokens[self.index]

    def expect(self, kind: str) -> None:
        if not self.match(kind):
            raise ParseError(f"Expected {kind}, found {self.current.kind}")

    def consume(self, kind: str) -> Token:
        self.expect(kind)
        token = self.current
        self.index += 1
        return token


def parse_method(source: str) -> ast.MethodDefinition:
    return Parser(source).parse_method()


def parse_do_it(source: str) -> ast.DoItDefinition:
    return Parser(source).parse_do_it()
