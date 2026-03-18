import unittest

from recorz import ast
from recorz.model import ParseError
from recorz.parser import parse_do_it, parse_method


class ParserTests(unittest.TestCase):
    def test_keyword_header_and_precedence(self) -> None:
        method = parse_method(
            """sum: a with: b
    ^a + b
"""
        )
        self.assertEqual(method.selector, "sum:with:")
        self.assertEqual(method.parameters, ("a", "b"))

    def test_cascade_receiver_is_preserved(self) -> None:
        expression = parse_do_it("stream nextPutAll: 'abc'; cr").body.statements[0]
        self.assertIsInstance(expression, ast.Cascade)
        self.assertIsInstance(expression.receiver, ast.Identifier)
        self.assertEqual(expression.receiver.name, "stream")
        self.assertEqual([message.selector for message in expression.messages], ["nextPutAll:", "cr"])

    def test_unterminated_string_literal_raises_parse_error(self) -> None:
        with self.assertRaisesRegex(ParseError, "Unterminated string literal"):
            parse_do_it("'unterminated")

    def test_unterminated_comment_raises_parse_error(self) -> None:
        with self.assertRaisesRegex(ParseError, "Unterminated comment"):
            parse_do_it('"unterminated')

    def test_unexpected_character_raises_parse_error(self) -> None:
        with self.assertRaisesRegex(ParseError, r"Unexpected character '!'"):
            parse_do_it("!")


if __name__ == "__main__":
    unittest.main()
