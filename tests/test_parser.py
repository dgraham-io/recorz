import unittest

from recorz import ast
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


if __name__ == "__main__":
    unittest.main()
