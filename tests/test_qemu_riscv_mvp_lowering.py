import sys
import struct
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools"))
if str(ROOT / "src") not in sys.path:
    sys.path.insert(0, str(ROOT / "src"))

import build_qemu_riscv_mvp_image as mvp  # noqa: E402


class QemuRiscvMvpLoweringTests(unittest.TestCase):
    def test_lowers_transcript_show_and_cr(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        self.assertEqual([(literal.kind, literal.value) for literal in program.literals], [(mvp.LITERAL_STRING, "HELLO")])
        self.assertEqual(
            [instruction.opcode for instruction in program.instructions],
            [
                mvp.OP_PUSH_GLOBAL,
                mvp.OP_DUP,
                mvp.OP_PUSH_LITERAL,
                mvp.OP_SEND,
                mvp.OP_POP,
                mvp.OP_SEND,
                mvp.OP_RETURN,
            ],
        )

    def test_lowers_display_service_from_compiler_output(self) -> None:
        program = mvp.build_program("Display writeString: 'HELLO'; newline")
        self.assertEqual([(literal.kind, literal.value) for literal in program.literals], [(mvp.LITERAL_STRING, "HELLO")])
        self.assertEqual(program.instructions[0].operand_a, "RECORZ_MVP_GLOBAL_DISPLAY")
        self.assertEqual(program.instructions[3].operand_a, "RECORZ_MVP_SELECTOR_WRITE_STRING")
        self.assertEqual(program.instructions[5].operand_a, "RECORZ_MVP_SELECTOR_NEWLINE")

    def test_lowers_workspace_temporaries_and_integer_arithmetic(self) -> None:
        program = mvp.build_program("| pixels | pixels := 640 * 480. Transcript show: pixels printString")
        self.assertEqual(program.lexical_count, 1)
        self.assertIn((mvp.LITERAL_SMALL_INTEGER, 640), [(literal.kind, literal.value) for literal in program.literals])
        self.assertIn((mvp.LITERAL_SMALL_INTEGER, 480), [(literal.kind, literal.value) for literal in program.literals])
        self.assertIn(mvp.OP_STORE_LEXICAL, [instruction.opcode for instruction in program.instructions])
        self.assertIn(mvp.OP_PUSH_LEXICAL, [instruction.opcode for instruction in program.instructions])
        self.assertIn("RECORZ_MVP_SELECTOR_MULTIPLY", [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND])
        self.assertIn("RECORZ_MVP_SELECTOR_PRINT_STRING", [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND])

    def test_lowers_display_default_form_and_clear(self) -> None:
        program = mvp.build_program("| form | form := Display defaultForm. form clear. form writeString: 'HELLO'. form newline")
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        self.assertEqual(program.lexical_count, 1)
        self.assertIn("RECORZ_MVP_SELECTOR_DEFAULT_FORM", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_CLEAR", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_WRITE_STRING", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_NEWLINE", selectors)

    def test_lowers_form_bits_and_dimension_access(self) -> None:
        program = mvp.build_program(
            "| form bits width height | form := Display defaultForm. bits := form bits. width := bits width. height := bits height. width * height"
        )
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        self.assertEqual(program.lexical_count, 4)
        self.assertIn("RECORZ_MVP_SELECTOR_BITS", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_WIDTH", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_HEIGHT", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_MULTIPLY", selectors)

    def test_lowers_bitmap_and_form_factories(self) -> None:
        program = mvp.build_program(
            "| scratch | scratch := Form fromBits: (Bitmap monoWidth: 24 height: 24). BitBlt fillForm: scratch color: 0"
        )
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        globals_used = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_PUSH_GLOBAL]
        self.assertEqual(program.lexical_count, 1)
        self.assertIn("RECORZ_MVP_SELECTOR_MONO_WIDTH_HEIGHT", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_FROM_BITS", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_FILL_FORM_COLOR", selectors)
        self.assertIn("RECORZ_MVP_GLOBAL_FORM", globals_used)
        self.assertIn("RECORZ_MVP_GLOBAL_BITMAP", globals_used)

    def test_lowers_bitblt_fill_form_color(self) -> None:
        program = mvp.build_program("| form | form := Display defaultForm. BitBlt fillForm: form color: 0")
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        self.assertEqual(program.lexical_count, 1)
        self.assertIn("RECORZ_MVP_SELECTOR_FILL_FORM_COLOR", selectors)
        self.assertEqual(program.instructions[5].operand_a, "RECORZ_MVP_GLOBAL_BITBLT")

    def test_lowers_bitblt_copy_bitmap_to_form(self) -> None:
        program = mvp.build_program(
            "| form | form := Display defaultForm. BitBlt copyBitmap: (Glyphs at: 82) toForm: form x: 560 y: 24 scale: 4"
        )
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        globals_used = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_PUSH_GLOBAL]
        self.assertEqual(program.lexical_count, 1)
        self.assertIn("RECORZ_MVP_SELECTOR_AT", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE", selectors)
        self.assertIn("RECORZ_MVP_GLOBAL_GLYPHS", globals_used)

    def test_lowers_bitblt_copy_bitmap_to_form_with_color(self) -> None:
        program = mvp.build_program(
            "| form | form := Display defaultForm. BitBlt copyBitmap: (Glyphs at: 82) toForm: form x: 560 y: 24 scale: 4 color: 16711680"
        )
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        self.assertEqual(program.lexical_count, 1)
        self.assertIn("RECORZ_MVP_SELECTOR_AT", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE_COLOR", selectors)

    def test_lowers_bitblt_copy_bitmap_region_to_form_with_color(self) -> None:
        program = mvp.build_program(
            "| form | form := Display defaultForm. BitBlt copyBitmap: (Glyphs at: 82) sourceX: 1 sourceY: 0 width: 4 height: 7 toForm: form x: 560 y: 24 scale: 4 color: 16711680"
        )
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        globals_used = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_PUSH_GLOBAL]
        self.assertEqual(program.lexical_count, 1)
        self.assertIn("RECORZ_MVP_SELECTOR_AT", selectors)
        self.assertIn(
            "RECORZ_MVP_SELECTOR_COPY_BITMAP_SOURCE_X_SOURCE_Y_WIDTH_HEIGHT_TO_FORM_X_Y_SCALE_COLOR",
            selectors,
        )
        self.assertIn("RECORZ_MVP_GLOBAL_GLYPHS", globals_used)

    def test_builds_program_manifest_with_expected_header(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        manifest = mvp.build_program_manifest(program)
        magic, version, instruction_count, literal_count, lexical_count = struct.unpack_from(
            mvp.PROGRAM_HEADER_FORMAT,
            manifest,
            0,
        )
        opcode, operand_a, operand_b = struct.unpack_from(
            mvp.PROGRAM_INSTRUCTION_FORMAT,
            manifest,
            struct.calcsize(mvp.PROGRAM_HEADER_FORMAT),
        )

        self.assertEqual(struct.calcsize(mvp.PROGRAM_HEADER_FORMAT), 12)
        self.assertEqual(magic, mvp.PROGRAM_MAGIC)
        self.assertEqual(version, mvp.PROGRAM_VERSION)
        self.assertEqual((instruction_count, literal_count, lexical_count), (7, 1, 0))
        self.assertEqual(opcode, mvp.OPCODE_VALUES[mvp.OP_PUSH_GLOBAL])
        self.assertEqual(operand_a, mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRANSCRIPT"])
        self.assertEqual(operand_b, 0)

    def test_builds_image_manifest_with_program_and_seed_sections(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        manifest = mvp.build_image_manifest(program)
        magic, version, section_count = struct.unpack_from(mvp.IMAGE_HEADER_FORMAT, manifest, 0)
        program_section = struct.unpack_from(
            mvp.IMAGE_SECTION_FORMAT,
            manifest,
            struct.calcsize(mvp.IMAGE_HEADER_FORMAT),
        )
        seed_section = struct.unpack_from(
            mvp.IMAGE_SECTION_FORMAT,
            manifest,
            struct.calcsize(mvp.IMAGE_HEADER_FORMAT) + struct.calcsize(mvp.IMAGE_SECTION_FORMAT),
        )

        self.assertEqual(magic, mvp.IMAGE_MAGIC)
        self.assertEqual(version, mvp.IMAGE_VERSION)
        self.assertEqual(section_count, 2)
        self.assertEqual(program_section[0], mvp.IMAGE_SECTION_PROGRAM)
        self.assertEqual(seed_section[0], mvp.IMAGE_SECTION_SEED)
        self.assertGreater(program_section[2], 0)
        self.assertGreater(program_section[3], 0)
        self.assertEqual(seed_section[2], program_section[2] + program_section[3])
        self.assertGreater(seed_section[3], 0)

    def test_builds_seed_manifest_with_expected_header(self) -> None:
        manifest = mvp.build_seed_manifest()
        (
            magic,
            version,
            object_count,
            nil_global,
            transcript_global,
            display_global,
            bitblt_global,
            glyphs_global,
            form_global,
            bitmap_global,
            default_form_index,
            framebuffer_bitmap_index,
            glyph_bitmap_start_index,
            glyph_code_count,
            glyph_fallback_offset,
            reserved,
        ) = struct.unpack_from(mvp.SEED_HEADER_FORMAT, manifest, 0)

        self.assertEqual(struct.calcsize(mvp.SEED_HEADER_FORMAT), 32)
        self.assertEqual(magic, mvp.SEED_MAGIC)
        self.assertEqual(version, mvp.SEED_VERSION)
        self.assertEqual(object_count, 136)
        self.assertEqual(
            (nil_global, transcript_global, display_global, bitblt_global, glyphs_global, form_global, bitmap_global),
            (0, 0, 1, 2, 3, 4, 5),
        )
        self.assertEqual(default_form_index, 7)
        self.assertEqual(framebuffer_bitmap_index, 6)
        self.assertEqual(glyph_bitmap_start_index, 8)
        self.assertEqual(glyph_code_count, 128)
        self.assertEqual(glyph_fallback_offset, 32)
        self.assertEqual(reserved, 0)

    def test_rejects_unsupported_globals(self) -> None:
        with self.assertRaises(mvp.LoweringError):
            mvp.build_program("Object new")

    def test_rejects_unsupported_compiler_features(self) -> None:
        with self.assertRaises(mvp.LoweringError):
            mvp.build_program("[ 1 ] value")


if __name__ == "__main__":
    unittest.main()
