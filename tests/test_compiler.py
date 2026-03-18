import unittest

from recorz.compiler import compile_do_it, compile_method
from recorz.model import BindingRef, CompilerError


class CompilerTests(unittest.TestCase):
    def test_rejects_bare_super_identifier(self) -> None:
        with self.assertRaisesRegex(CompilerError, "super may only be used as a message receiver"):
            compile_do_it("super", "Object", [])

    def test_compiles_super_send_as_super_send_site(self) -> None:
        compiled = compile_method(
            """foo
    ^super bar
""",
            "Object",
            [],
        )

        self.assertEqual([instruction.opcode for instruction in compiled.instructions], ["push_self", "send", "return_local"])
        send_site = compiled.instructions[1].operand
        self.assertEqual(send_site.selector, "bar")
        self.assertTrue(send_site.super_send)

    def test_compiles_nested_block_with_capture_and_non_local_return(self) -> None:
        compiled = compile_do_it(
            """| x |
    [ ^x ] value
""",
            "Object",
            [],
        )

        self.assertEqual([instruction.opcode for instruction in compiled.instructions], ["make_block", "send", "return_local"])
        block = compiled.literals[0]
        self.assertEqual([instruction.opcode for instruction in block.instructions], ["push_binding", "return_non_local"])
        self.assertEqual(block.instructions[0].operand, BindingRef("lexical", "x"))
        self.assertEqual(block.source, " ^x ")


if __name__ == "__main__":
    unittest.main()
