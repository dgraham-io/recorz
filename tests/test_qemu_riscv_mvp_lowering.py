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

    def test_lowers_heap_object_class_introspection(self) -> None:
        program = mvp.build_program(
            "| form | form := Display defaultForm. form class instanceKind printString"
        )
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        self.assertEqual(program.lexical_count, 1)
        self.assertIn("RECORZ_MVP_SELECTOR_CLASS", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_INSTANCE_KIND", selectors)
        self.assertIn("RECORZ_MVP_SELECTOR_PRINT_STRING", selectors)

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

    def test_builds_image_manifest_with_entry_program_and_seed_sections(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        manifest = mvp.build_image_manifest(program)
        magic, version, section_count, feature_flags, checksum, profile = struct.unpack_from(mvp.IMAGE_HEADER_FORMAT, manifest, 0)
        entry_section = struct.unpack_from(
            mvp.IMAGE_SECTION_FORMAT,
            manifest,
            struct.calcsize(mvp.IMAGE_HEADER_FORMAT),
        )
        program_section = struct.unpack_from(
            mvp.IMAGE_SECTION_FORMAT,
            manifest,
            struct.calcsize(mvp.IMAGE_HEADER_FORMAT) + struct.calcsize(mvp.IMAGE_SECTION_FORMAT),
        )
        seed_section = struct.unpack_from(
            mvp.IMAGE_SECTION_FORMAT,
            manifest,
            struct.calcsize(mvp.IMAGE_HEADER_FORMAT) + (2 * struct.calcsize(mvp.IMAGE_SECTION_FORMAT)),
        )
        entry_payload = manifest[entry_section[2] : entry_section[2] + entry_section[3]]
        entry_magic, entry_version, entry_kind, entry_flags, entry_program_section, entry_argument_count, entry_reserved = (
            struct.unpack_from(mvp.IMAGE_ENTRY_FORMAT, entry_payload, 0)
        )

        self.assertEqual(magic, mvp.IMAGE_MAGIC)
        self.assertEqual(version, mvp.IMAGE_VERSION)
        self.assertEqual(section_count, 3)
        self.assertEqual(feature_flags, mvp.IMAGE_FEATURE_FNV1A32)
        self.assertEqual(checksum, mvp.fnv1a32(manifest[struct.calcsize(mvp.IMAGE_HEADER_FORMAT) :]))
        self.assertEqual(profile, mvp.IMAGE_PROFILE)
        self.assertEqual(entry_section[0], mvp.IMAGE_SECTION_ENTRY)
        self.assertEqual(program_section[0], mvp.IMAGE_SECTION_PROGRAM)
        self.assertEqual(seed_section[0], mvp.IMAGE_SECTION_SEED)
        self.assertEqual(entry_magic, mvp.IMAGE_ENTRY_MAGIC)
        self.assertEqual(entry_version, mvp.IMAGE_ENTRY_VERSION)
        self.assertEqual(entry_kind, mvp.IMAGE_ENTRY_KIND_DOIT)
        self.assertEqual(entry_flags, 0)
        self.assertEqual(entry_program_section, mvp.IMAGE_SECTION_PROGRAM)
        self.assertEqual(entry_argument_count, 0)
        self.assertEqual(entry_reserved, 0)
        self.assertGreater(entry_section[2], 0)
        self.assertGreater(entry_section[3], 0)
        self.assertEqual(program_section[2], entry_section[2] + entry_section[3])
        self.assertGreater(program_section[2], 0)
        self.assertGreater(program_section[3], 0)
        self.assertEqual(seed_section[2], program_section[2] + program_section[3])
        self.assertGreater(seed_section[3], 0)

    def test_compiles_kernel_method_source_to_tiny_compiled_method(self) -> None:
        self.assertEqual(
            mvp.compile_kernel_method_program(
                "Bitmap",
                ["width", "height", "storageKind", "storageId"],
                "width ^width",
            ),
            [
                mvp.encode_compiled_method_instruction("push_field", 0),
                mvp.encode_compiled_method_instruction("return_top"),
            ],
        )
        self.assertEqual(
            mvp.compile_kernel_method_program(
                "Transcript",
                [],
                "show: text Display defaultForm writeString: text. ^self",
            ),
            [
                mvp.encode_compiled_method_instruction("push_root", mvp.SEED_ROOT_DEFAULT_FORM),
                mvp.encode_compiled_method_instruction("push_argument", 0),
                mvp.encode_compiled_method_instruction(
                    "send",
                    mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_WRITE_STRING"],
                    1,
                ),
                mvp.encode_compiled_method_instruction("return_receiver"),
            ],
        )
        self.assertEqual(
            mvp.compile_kernel_method_program(
                "Display",
                [],
                "defaultForm ^Display defaultForm",
            ),
            [
                mvp.encode_compiled_method_instruction("push_root", mvp.SEED_ROOT_DEFAULT_FORM),
                mvp.encode_compiled_method_instruction("return_top"),
            ],
        )

    def test_loads_kernel_methods_from_manifest_files(self) -> None:
        sources = mvp.load_kernel_method_sources()
        transcript_show = sources["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"]
        display_default_form = sources["RECORZ_MVP_METHOD_ENTRY_DISPLAY_DEFAULT_FORM"]
        bitblt_fill_form_color = sources["RECORZ_MVP_METHOD_ENTRY_BITBLT_FILL_FORM_COLOR"]
        form_width = sources["RECORZ_MVP_METHOD_ENTRY_FORM_WIDTH"]
        form_newline = sources["RECORZ_MVP_METHOD_ENTRY_FORM_NEWLINE"]

        self.assertEqual(transcript_show.class_name, "Transcript")
        self.assertEqual(transcript_show.instance_variables, ())
        self.assertEqual(transcript_show.relative_path, "Transcript.rz")
        self.assertEqual(transcript_show.selector, "show:")
        self.assertEqual(transcript_show.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_COMPILED)
        self.assertIn("Display defaultForm writeString: text.", transcript_show.source_text)
        self.assertEqual(display_default_form.class_name, "Display")
        self.assertEqual(display_default_form.instance_variables, ())
        self.assertEqual(display_default_form.relative_path, "Display.rz")
        self.assertEqual(display_default_form.selector, "defaultForm")
        self.assertEqual(display_default_form.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_COMPILED)
        self.assertEqual(display_default_form.source_text, "defaultForm\n    ^Display defaultForm")
        self.assertEqual(bitblt_fill_form_color.class_name, "BitBlt")
        self.assertEqual(bitblt_fill_form_color.instance_variables, ())
        self.assertEqual(bitblt_fill_form_color.relative_path, "BitBlt.rz")
        self.assertEqual(bitblt_fill_form_color.selector, "fillForm:color:")
        self.assertEqual(bitblt_fill_form_color.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE)
        self.assertEqual(bitblt_fill_form_color.source_text, "fillForm: form color: color\n    <primitive>")
        self.assertEqual(form_width.class_name, "Form")
        self.assertEqual(form_width.instance_variables, ("bits",))
        self.assertEqual(form_width.relative_path, "Form.rz")
        self.assertEqual(form_width.selector, "width")
        self.assertEqual(form_width.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_COMPILED)
        self.assertEqual(form_width.source_text, "width\n    ^bits width")
        self.assertEqual(form_newline.class_name, "Form")
        self.assertEqual(form_newline.selector, "newline")
        self.assertEqual(form_newline.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE)
        self.assertEqual(form_newline.source_text, "newline\n    <primitive>")

    def test_splits_class_file_chunks_on_bang_lines(self) -> None:
        self.assertEqual(
            mvp.split_kernel_method_chunks(
                "width\n    ^width\n!\nheight\n    ^height\n!\n"
            ),
            [
                "width\n    ^width",
                "height\n    ^height",
            ],
        )

    def test_parses_kernel_class_header(self) -> None:
        self.assertEqual(
            mvp.parse_kernel_class_header(
                "RecorzKernelClass: #Form instanceVariableNames: 'bits'",
                "Form.rz",
            ),
            mvp.KernelClassHeader("Form", ("bits",)),
        )
        self.assertEqual(
            mvp.parse_kernel_class_header(
                "RecorzKernelClass: #Display instanceVariableNames: ''",
                "Display.rz",
            ),
            mvp.KernelClassHeader("Display", ()),
        )

    def test_parses_primitive_kernel_method_chunk(self) -> None:
        self.assertEqual(
            mvp.parse_kernel_method_chunk(
                "BitBlt",
                [],
                "fillForm: form color: color\n    <primitive>",
            ),
            ("fillForm:color:", 2, mvp.KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE),
        )

    def test_builds_seed_manifest_with_expected_header(self) -> None:
        manifest = mvp.build_seed_manifest()
        (
            magic,
            version,
            object_count,
            global_binding_count,
            root_binding_count,
            glyph_code_count,
            reserved,
        ) = struct.unpack_from(mvp.SEED_HEADER_FORMAT, manifest, 0)
        object_size = struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT) + (4 * struct.calcsize(mvp.SEED_FIELD_FORMAT))
        object_bytes = object_count * object_size
        global_binding_offset = struct.calcsize(mvp.SEED_HEADER_FORMAT) + object_bytes
        first_global_binding = struct.unpack_from(mvp.SEED_BINDING_FORMAT, manifest, global_binding_offset)
        first_root_binding = struct.unpack_from(
            mvp.SEED_BINDING_FORMAT,
            manifest,
            global_binding_offset + (global_binding_count * struct.calcsize(mvp.SEED_BINDING_FORMAT)),
        )
        first_object_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT),
        )
        class_class_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (140 * object_size),
        )
        first_selector_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (157 * object_size),
        )
        first_selector_fields = [
            struct.unpack_from(
                mvp.SEED_FIELD_FORMAT,
                manifest,
                struct.calcsize(mvp.SEED_HEADER_FORMAT)
                + (157 * object_size)
                + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
                + (field_index * struct.calcsize(mvp.SEED_FIELD_FORMAT)),
            )
            for field_index in range(1)
        ]
        first_compiled_method_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (174 * object_size),
        )
        first_compiled_method_fields = [
            struct.unpack_from(
                mvp.SEED_FIELD_FORMAT,
                manifest,
                struct.calcsize(mvp.SEED_HEADER_FORMAT)
                + (174 * object_size)
                + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
                + (field_index * struct.calcsize(mvp.SEED_FIELD_FORMAT)),
            )
            for field_index in range(4)
        ]
        first_method_entry_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (186 * object_size),
        )
        first_method_entry_fields = [
            struct.unpack_from(
                mvp.SEED_FIELD_FORMAT,
                manifest,
                struct.calcsize(mvp.SEED_HEADER_FORMAT)
                + (186 * object_size)
                + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
                + (field_index * struct.calcsize(mvp.SEED_FIELD_FORMAT)),
            )
            for field_index in range(2)
        ]
        first_method_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (208 * object_size),
        )
        first_method_fields = [
            struct.unpack_from(
                mvp.SEED_FIELD_FORMAT,
                manifest,
                struct.calcsize(mvp.SEED_HEADER_FORMAT)
                + (208 * object_size)
                + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
                + (field_index * struct.calcsize(mvp.SEED_FIELD_FORMAT)),
            )
            for field_index in range(4)
        ]
        first_glyph_object_index = struct.unpack_from(
            "<H",
            manifest,
            global_binding_offset
            + ((global_binding_count + root_binding_count) * struct.calcsize(mvp.SEED_BINDING_FORMAT)),
        )[0]

        self.assertEqual(struct.calcsize(mvp.SEED_HEADER_FORMAT), 16)
        self.assertEqual(magic, mvp.SEED_MAGIC)
        self.assertEqual(version, mvp.SEED_VERSION)
        self.assertEqual(object_count, 230)
        self.assertEqual(global_binding_count, 6)
        self.assertEqual(root_binding_count, 6)
        self.assertEqual(glyph_code_count, 128)
        self.assertEqual(reserved, 0)
        self.assertEqual(first_object_header, (mvp.SEED_OBJECT_TRANSCRIPT, 0, 141))
        self.assertEqual(class_class_header, (mvp.SEED_OBJECT_CLASS, 4, 140))
        self.assertEqual(first_selector_header, (mvp.SEED_OBJECT_SELECTOR, 1, 155))
        self.assertEqual(
            first_selector_fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SHOW"]),
            ],
        )
        self.assertEqual(first_compiled_method_header, (mvp.SEED_OBJECT_COMPILED_METHOD, 4, 156))
        self.assertEqual(
            first_compiled_method_fields,
            [
                (
                    mvp.SEED_FIELD_SMALL_INTEGER,
                    mvp.encode_compiled_method_instruction("push_root", mvp.SEED_ROOT_DEFAULT_FORM),
                ),
                (
                    mvp.SEED_FIELD_SMALL_INTEGER,
                    mvp.encode_compiled_method_instruction("push_argument", 0),
                ),
                (
                    mvp.SEED_FIELD_SMALL_INTEGER,
                    mvp.encode_compiled_method_instruction(
                        "send",
                        mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_WRITE_STRING"],
                        1,
                    ),
                ),
                (
                    mvp.SEED_FIELD_SMALL_INTEGER,
                    mvp.encode_compiled_method_instruction("return_receiver"),
                ),
            ],
        )
        self.assertEqual(first_method_entry_header, (mvp.SEED_OBJECT_METHOD_ENTRY, 2, 154))
        self.assertEqual(
            first_method_entry_fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.METHOD_ENTRY_VALUES["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"]),
                (mvp.SEED_FIELD_OBJECT_INDEX, 174),
            ],
        )
        self.assertEqual(first_method_header, (mvp.SEED_OBJECT_METHOD_DESCRIPTOR, 4, 153))
        self.assertEqual(
            first_method_fields,
            [
                (mvp.SEED_FIELD_OBJECT_INDEX, 173),
                (mvp.SEED_FIELD_SMALL_INTEGER, 0),
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SEED_OBJECT_CLASS),
                (
                    mvp.SEED_FIELD_OBJECT_INDEX,
                    186 + (mvp.METHOD_ENTRY_VALUES["RECORZ_MVP_METHOD_ENTRY_CLASS_INSTANCE_KIND"] - 1),
                ),
            ],
        )
        self.assertEqual(first_global_binding, (mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRANSCRIPT"], 0))
        self.assertEqual(first_root_binding, (mvp.SEED_ROOT_DEFAULT_FORM, 9))
        self.assertEqual(first_glyph_object_index, 10)

    def test_rejects_unsupported_globals(self) -> None:
        with self.assertRaises(mvp.LoweringError):
            mvp.build_program("Object new")

    def test_rejects_unsupported_compiler_features(self) -> None:
        with self.assertRaises(mvp.LoweringError):
            mvp.build_program("[ 1 ] value")


if __name__ == "__main__":
    unittest.main()
