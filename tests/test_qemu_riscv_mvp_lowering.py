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

    def test_loads_kernel_methods_from_class_source_files(self) -> None:
        sources = mvp.load_kernel_method_sources()
        transcript_show = sources["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"]
        display_default_form = sources["RECORZ_MVP_METHOD_ENTRY_DISPLAY_DEFAULT_FORM"]
        bitblt_fill_form_color = sources["RECORZ_MVP_METHOD_ENTRY_BITBLT_FILL_FORM_COLOR"]
        form_width = sources["RECORZ_MVP_METHOD_ENTRY_FORM_WIDTH"]
        form_newline = sources["RECORZ_MVP_METHOD_ENTRY_FORM_NEWLINE"]

        self.assertEqual(len(sources), len(mvp.METHOD_ENTRY_DEFINITIONS))
        self.assertEqual(
            {source.relative_path for source in sources.values()},
            {
                "BitBlt.rz",
                "Bitmap.rz",
                "BitmapFactory.rz",
                "Class.rz",
                "Display.rz",
                "Form.rz",
                "FormFactory.rz",
                "Glyphs.rz",
                "Transcript.rz",
            },
        )
        self.assertEqual(transcript_show.class_name, "Transcript")
        self.assertEqual(transcript_show.entry_name, "RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW")
        self.assertEqual(transcript_show.instance_variables, ())
        self.assertEqual(transcript_show.relative_path, "Transcript.rz")
        self.assertEqual(transcript_show.selector, "show:")
        self.assertEqual(transcript_show.argument_count, 1)
        self.assertEqual(transcript_show.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_COMPILED)
        self.assertIsNone(transcript_show.primitive_binding)
        self.assertIn("Display defaultForm writeString: text.", transcript_show.source_text)
        self.assertEqual(display_default_form.class_name, "Display")
        self.assertEqual(display_default_form.entry_name, "RECORZ_MVP_METHOD_ENTRY_DISPLAY_DEFAULT_FORM")
        self.assertEqual(display_default_form.instance_variables, ())
        self.assertEqual(display_default_form.relative_path, "Display.rz")
        self.assertEqual(display_default_form.selector, "defaultForm")
        self.assertEqual(display_default_form.argument_count, 0)
        self.assertEqual(display_default_form.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_COMPILED)
        self.assertIsNone(display_default_form.primitive_binding)
        self.assertEqual(display_default_form.source_text, "defaultForm\n    ^Display defaultForm")
        self.assertEqual(bitblt_fill_form_color.class_name, "BitBlt")
        self.assertEqual(bitblt_fill_form_color.entry_name, "RECORZ_MVP_METHOD_ENTRY_BITBLT_FILL_FORM_COLOR")
        self.assertEqual(bitblt_fill_form_color.instance_variables, ())
        self.assertEqual(bitblt_fill_form_color.relative_path, "BitBlt.rz")
        self.assertEqual(bitblt_fill_form_color.selector, "fillForm:color:")
        self.assertEqual(bitblt_fill_form_color.argument_count, 2)
        self.assertEqual(bitblt_fill_form_color.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE)
        self.assertEqual(bitblt_fill_form_color.primitive_binding, "bitbltFillFormColor")
        self.assertEqual(
            bitblt_fill_form_color.source_text,
            "fillForm: form color: color\n    <primitive: #bitbltFillFormColor>",
        )
        self.assertEqual(form_width.class_name, "Form")
        self.assertEqual(form_width.entry_name, "RECORZ_MVP_METHOD_ENTRY_FORM_WIDTH")
        self.assertEqual(form_width.instance_variables, ("bits",))
        self.assertEqual(form_width.relative_path, "Form.rz")
        self.assertEqual(form_width.selector, "width")
        self.assertEqual(form_width.argument_count, 0)
        self.assertEqual(form_width.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_COMPILED)
        self.assertIsNone(form_width.primitive_binding)
        self.assertEqual(form_width.source_text, "width\n    ^bits width")
        self.assertEqual(form_newline.class_name, "Form")
        self.assertEqual(form_newline.entry_name, "RECORZ_MVP_METHOD_ENTRY_FORM_NEWLINE")
        self.assertEqual(form_newline.selector, "newline")
        self.assertEqual(form_newline.argument_count, 0)
        self.assertEqual(form_newline.implementation_kind, mvp.KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE)
        self.assertEqual(form_newline.primitive_binding, "formNewline")
        self.assertEqual(form_newline.source_text, "newline\n    <primitive: #formNewline>")

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
                "fillForm: form color: color\n    <primitive: #bitbltFillFormColor>",
            ),
            ("fillForm:color:", 2, mvp.KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE, "bitbltFillFormColor"),
        )

    def test_derives_kernel_method_entry_names(self) -> None:
        self.assertEqual(
            mvp.kernel_method_entry_name("Transcript", "show:"),
            "RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW",
        )
        self.assertEqual(
            mvp.kernel_method_entry_name("Display", "defaultForm"),
            "RECORZ_MVP_METHOD_ENTRY_DISPLAY_DEFAULT_FORM",
        )
        self.assertEqual(
            mvp.kernel_method_entry_name("BitBlt", "copyBitmap:toForm:x:y:scale:"),
            "RECORZ_MVP_METHOD_ENTRY_BITBLT_COPY_BITMAP_TO_FORM_X_Y_SCALE",
        )
        self.assertEqual(
            mvp.kernel_method_entry_name("FormFactory", "fromBits:"),
            "RECORZ_MVP_METHOD_ENTRY_FORM_FACTORY_FROM_BITS",
        )

    def test_derives_primitive_binding_ids_from_kernel_sources(self) -> None:
        self.assertEqual(mvp.PRIMITIVE_BINDING_VALUES["bitbltFillFormColor"], 1)
        self.assertEqual(mvp.PRIMITIVE_BINDING_VALUES["glyphsAt"], 5)
        self.assertEqual(mvp.PRIMITIVE_BINDING_VALUES["formClear"], 8)
        self.assertEqual(
            mvp.PRIMITIVE_BINDING_BY_ENTRY_NAME["RECORZ_MVP_METHOD_ENTRY_FORM_WRITE_STRING"],
            mvp.PRIMITIVE_BINDING_VALUES["formWriteString"],
        )
        self.assertEqual(
            mvp.PRIMITIVE_BINDING_BY_ENTRY_NAME["RECORZ_MVP_METHOD_ENTRY_FORM_NEWLINE"],
            mvp.PRIMITIVE_BINDING_VALUES["formNewline"],
        )

    def test_derives_global_and_selector_maps_from_specs(self) -> None:
        self.assertEqual(
            mvp.GLOBAL_SPECS,
            [
                ("Transcript", "RECORZ_MVP_GLOBAL_TRANSCRIPT"),
                ("Display", "RECORZ_MVP_GLOBAL_DISPLAY"),
                ("BitBlt", "RECORZ_MVP_GLOBAL_BITBLT"),
                ("Glyphs", "RECORZ_MVP_GLOBAL_GLYPHS"),
                ("Form", "RECORZ_MVP_GLOBAL_FORM"),
                ("Bitmap", "RECORZ_MVP_GLOBAL_BITMAP"),
            ],
        )
        self.assertEqual(mvp.GLOBAL_IDS["Transcript"], "RECORZ_MVP_GLOBAL_TRANSCRIPT")
        self.assertEqual(mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_BITMAP"], 6)
        self.assertEqual(mvp.GLOBAL_DEFINITIONS[-1], ("RECORZ_MVP_GLOBAL_BITMAP", 6))
        self.assertEqual(mvp.SELECTOR_IDS["writeString:"], "RECORZ_MVP_SELECTOR_WRITE_STRING")
        self.assertEqual(mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_INSTANCE_KIND"], 22)
        self.assertEqual(
            mvp.SELECTOR_DEFINITIONS[16],
            ("RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE_COLOR", 17),
        )

    def test_derives_seed_object_and_root_maps_from_specs(self) -> None:
        self.assertEqual(
            mvp.SEED_FIELD_KIND_SPECS,
            [
                ("nil", "RECORZ_MVP_SEED_FIELD_NIL"),
                ("small_integer", "RECORZ_MVP_SEED_FIELD_SMALL_INTEGER"),
                ("object_index", "RECORZ_MVP_SEED_FIELD_OBJECT_INDEX"),
            ],
        )
        self.assertEqual(mvp.SEED_FIELD_KIND_DEFINITIONS[2], ("RECORZ_MVP_SEED_FIELD_OBJECT_INDEX", 2))
        self.assertEqual(mvp.OBJECT_KIND_SPECS[0], ("Transcript", "RECORZ_MVP_OBJECT_TRANSCRIPT"))
        self.assertEqual(mvp.OBJECT_KIND_SPECS[-1], ("CompiledMethod", "RECORZ_MVP_OBJECT_COMPILED_METHOD"))
        self.assertEqual(mvp.KERNEL_CLASS_SPECS[0], mvp.KernelClassSpec("Class", source_boot_order=8))
        self.assertEqual(mvp.KERNEL_CLASS_SPECS[-1], mvp.KernelClassSpec("CompiledMethod"))
        self.assertEqual(mvp.KERNEL_CLASS_NAME_TO_OBJECT_KIND["BitBlt"], mvp.SEED_OBJECT_BITBLT)
        self.assertEqual(mvp.OBJECT_KIND_DEFINITIONS[-1], ("RECORZ_MVP_OBJECT_COMPILED_METHOD", 22))
        self.assertEqual(
            mvp.SEED_ROOT_SPECS,
            [
                ("default_form", "RECORZ_MVP_SEED_ROOT_DEFAULT_FORM"),
                ("framebuffer_bitmap", "RECORZ_MVP_SEED_ROOT_FRAMEBUFFER_BITMAP"),
                ("transcript_behavior", "RECORZ_MVP_SEED_ROOT_TRANSCRIPT_BEHAVIOR"),
                ("transcript_layout", "RECORZ_MVP_SEED_ROOT_TRANSCRIPT_LAYOUT"),
                ("transcript_style", "RECORZ_MVP_SEED_ROOT_TRANSCRIPT_STYLE"),
                ("transcript_metrics", "RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS"),
            ],
        )
        self.assertEqual(mvp.SEED_ROOT_VALUES["RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS"], 6)
        self.assertEqual(mvp.SEED_ROOT_DEFINITIONS[0], ("RECORZ_MVP_SEED_ROOT_DEFAULT_FORM", 1))

    def test_derives_method_entry_order_from_boot_class_order_and_source_chunks(self) -> None:
        self.assertEqual(
            mvp.KERNEL_CLASS_BOOT_ORDER,
            [
                "Transcript",
                "Display",
                "BitBlt",
                "Glyphs",
                "FormFactory",
                "BitmapFactory",
                "Bitmap",
                "Form",
                "Class",
            ],
        )
        self.assertEqual(
            [spec.class_name for spec in mvp.KERNEL_SOURCE_CLASS_SPECS],
            mvp.KERNEL_CLASS_BOOT_ORDER,
        )
        self.assertEqual(
            mvp.METHOD_ENTRY_ORDER[:6],
            [
                "RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW",
                "RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_CR",
                "RECORZ_MVP_METHOD_ENTRY_DISPLAY_DEFAULT_FORM",
                "RECORZ_MVP_METHOD_ENTRY_DISPLAY_CLEAR",
                "RECORZ_MVP_METHOD_ENTRY_DISPLAY_WRITE_STRING",
                "RECORZ_MVP_METHOD_ENTRY_DISPLAY_NEWLINE",
            ],
        )
        self.assertEqual(
            mvp.METHOD_ENTRY_ORDER[-4:],
            [
                "RECORZ_MVP_METHOD_ENTRY_FORM_BITS",
                "RECORZ_MVP_METHOD_ENTRY_FORM_WIDTH",
                "RECORZ_MVP_METHOD_ENTRY_FORM_HEIGHT",
                "RECORZ_MVP_METHOD_ENTRY_CLASS_INSTANCE_KIND",
            ],
        )

    def test_derives_seed_bindings_from_boot_object_names(self) -> None:
        self.assertEqual(
            mvp.GLOBAL_NAME_TO_BOOT_OBJECT_NAME,
            {
                "Transcript": "Transcript",
                "Display": "Display",
                "BitBlt": "BitBlt",
                "Glyphs": "Glyphs",
                "Form": "FormFactory",
                "Bitmap": "BitmapFactory",
            },
        )
        self.assertEqual(
            mvp.SEED_ROOT_NAME_TO_BOOT_OBJECT_NAME,
            {
                "default_form": "DefaultForm",
                "framebuffer_bitmap": "FramebufferBitmap",
                "transcript_behavior": "TranscriptBehavior",
                "transcript_layout": "TranscriptLayout",
                "transcript_style": "TranscriptStyle",
                "transcript_metrics": "TranscriptMetrics",
            },
        )
        self.assertEqual(mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS[0].global_exports, ("Transcript",))
        self.assertEqual(mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS[4].global_exports, ("Form",))
        self.assertEqual(mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS[6].root_exports, ("transcript_layout",))
        self.assertEqual(mvp.BOOT_OBJECT_SPECS_AFTER_GLYPHS[0].root_exports, ("transcript_metrics",))
        self.assertEqual(mvp.GLYPH_FALLBACK_CODE, 32)
        self.assertEqual(
            mvp.build_named_object_bindings(
                ["Transcript", "Bitmap"],
                mvp.GLOBAL_IDS,
                mvp.GLOBAL_VALUES,
                mvp.GLOBAL_NAME_TO_BOOT_OBJECT_NAME,
                {
                    "Transcript": 0,
                    "Display": 1,
                    "BitBlt": 2,
                    "Glyphs": 3,
                    "FormFactory": 4,
                    "BitmapFactory": 5,
                    "DefaultForm": 9,
                },
            ),
            [
                (mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRANSCRIPT"], 0),
                (mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_BITMAP"], 5),
            ],
        )

    def test_declares_boot_object_specs_and_materializes_fields(self) -> None:
        self.assertEqual(
            list(mvp.BOOT_OBJECT_FAMILY_SPECS_BY_NAME),
            [
                "before_glyphs",
                "glyph_bitmaps",
                "after_glyphs",
            ],
        )
        self.assertEqual(
            [spec.name for spec in mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS],
            [
                "Transcript",
                "Display",
                "BitBlt",
                "Glyphs",
                "FormFactory",
                "BitmapFactory",
                "TranscriptLayout",
                "TranscriptStyle",
                "FramebufferBitmap",
                "DefaultForm",
            ],
        )
        self.assertEqual(
            [spec.name for spec in mvp.BOOT_OBJECT_SPECS_AFTER_GLYPHS],
            [
                "TranscriptMetrics",
                "TranscriptBehavior",
            ],
        )
        self.assertEqual(
            mvp.materialize_boot_object_fields(
                mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS[-1].field_specs,
                {"FramebufferBitmap": 8},
                [],
            ),
            [(mvp.SEED_FIELD_OBJECT_INDEX, 8)],
        )
        self.assertEqual(
            mvp.materialize_boot_object_fields(
                mvp.BOOT_OBJECT_SPECS_AFTER_GLYPHS[-1].field_specs,
                {"TranscriptBehavior": 139},
                [100 + glyph_index for glyph_index in range(len(mvp.GLYPH_BITMAP_BOOT_SPECS))],
            ),
            [
                (mvp.SEED_FIELD_OBJECT_INDEX, 132),
                (mvp.SEED_FIELD_SMALL_INTEGER, 1),
            ],
        )

    def test_declares_boot_object_families(self) -> None:
        self.assertEqual(
            mvp.BOOT_OBJECT_FAMILY_NAMES,
            [
                "before_glyphs",
                "glyph_bitmaps",
                "after_glyphs",
            ],
        )
        self.assertIs(mvp.BOOT_OBJECT_FAMILY_SPECS_BY_NAME["before_glyphs"], mvp.BOOT_OBJECT_FAMILY_SPECS[0])
        self.assertIs(mvp.BOOT_OBJECT_FAMILY_SPECS_BY_NAME["glyph_bitmaps"], mvp.BOOT_OBJECT_FAMILY_SPECS[1])
        self.assertIs(mvp.BOOT_OBJECT_FAMILY_SPECS_BY_NAME["after_glyphs"], mvp.BOOT_OBJECT_FAMILY_SPECS[2])
        self.assertEqual(len(mvp.BOOT_OBJECT_FAMILY_SPECS[0].object_specs), 10)
        self.assertTrue(mvp.BOOT_OBJECT_FAMILY_SPECS[1].collect_object_indices)
        self.assertEqual(len(mvp.BOOT_OBJECT_FAMILY_SPECS[1].object_specs), len(mvp.GLYPH_BITMAP_BOOT_SPECS))
        self.assertEqual(len(mvp.BOOT_OBJECT_FAMILY_SPECS[2].object_specs), 2)
        self.assertEqual(mvp.BOOT_OBJECT_FIXED_COUNT, 140)

    def test_builds_fixed_boot_seed_objects_from_declared_families(self) -> None:
        seed_objects, seed_object_indices_by_name, glyph_object_indices = mvp.build_fixed_boot_seed_objects()

        self.assertEqual(len(seed_objects), mvp.BOOT_OBJECT_FIXED_COUNT)
        self.assertEqual(seed_object_indices_by_name["Transcript"], 0)
        self.assertEqual(seed_object_indices_by_name["DefaultForm"], 9)
        self.assertEqual(seed_object_indices_by_name["TranscriptBehavior"], 139)
        self.assertEqual(glyph_object_indices[0], 10)
        self.assertEqual(glyph_object_indices[-1], 137)
        self.assertEqual(
            seed_objects[glyph_object_indices[0]].fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, 5),
                (mvp.SEED_FIELD_SMALL_INTEGER, 7),
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.BITMAP_STORAGE_GLYPH_MONO),
                (mvp.SEED_FIELD_SMALL_INTEGER, 0),
            ],
        )

    def test_declares_glyph_bitmap_boot_specs(self) -> None:
        self.assertEqual(mvp.GLYPH_BITMAP_NAME_PREFIX, "GlyphBitmap")
        self.assertEqual(mvp.GLYPH_BITMAP_WIDTH, 5)
        self.assertEqual(mvp.GLYPH_BITMAP_HEIGHT, 7)
        self.assertEqual(len(mvp.GLYPH_BITMAP_BOOT_SPECS), mvp.GLYPH_BITMAP_CODE_COUNT)
        self.assertEqual(mvp.GLYPH_BITMAP_BOOT_SPECS[0].name, "GlyphBitmap0")
        self.assertEqual(mvp.GLYPH_BITMAP_BOOT_SPECS[-1].name, "GlyphBitmap127")
        self.assertEqual(
            mvp.GLYPH_BITMAP_BOOT_SPECS[0].field_specs,
            (
                (mvp.FIELD_SPEC_SMALL_INTEGER, 5),
                (mvp.FIELD_SPEC_SMALL_INTEGER, 7),
                (mvp.FIELD_SPEC_SMALL_INTEGER, mvp.BITMAP_STORAGE_GLYPH_MONO),
                (mvp.FIELD_SPEC_SMALL_INTEGER, 0),
            ),
        )
        self.assertEqual(
            mvp.GLYPH_BITMAP_BOOT_SPECS[-1].field_specs,
            (
                (mvp.FIELD_SPEC_SMALL_INTEGER, 5),
                (mvp.FIELD_SPEC_SMALL_INTEGER, 7),
                (mvp.FIELD_SPEC_SMALL_INTEGER, mvp.BITMAP_STORAGE_GLYPH_MONO),
                (mvp.FIELD_SPEC_SMALL_INTEGER, 127),
            ),
        )

    def test_declares_class_descriptor_order_and_materializes_class_seed_objects(self) -> None:
        self.assertEqual(
            mvp.CLASS_DESCRIPTOR_KIND_NAMES,
            [
                "Class",
                "Transcript",
                "Display",
                "BitBlt",
                "Glyphs",
                "FormFactory",
                "BitmapFactory",
                "TextLayout",
                "TextStyle",
                "Bitmap",
                "Form",
                "TextMetrics",
                "TextBehavior",
                "MethodDescriptor",
                "MethodEntry",
                "Selector",
                "CompiledMethod",
            ],
        )
        self.assertEqual(
            [spec.class_name for spec in mvp.KERNEL_CLASS_SPECS],
            mvp.CLASS_DESCRIPTOR_KIND_NAMES,
        )
        self.assertEqual(
            mvp.CLASS_DESCRIPTOR_KIND_ORDER[:4],
            [
                mvp.SEED_OBJECT_CLASS,
                mvp.SEED_OBJECT_TRANSCRIPT,
                mvp.SEED_OBJECT_DISPLAY,
                mvp.SEED_OBJECT_BITBLT,
            ],
        )
        class_indices, class_seed_objects = mvp.build_class_seed_objects(
            mvp.CLASS_DESCRIPTOR_KIND_ORDER,
            140,
            {mvp.SEED_OBJECT_TRANSCRIPT: 208},
            {mvp.SEED_OBJECT_TRANSCRIPT: 2},
        )
        self.assertEqual(class_indices[mvp.SEED_OBJECT_TRANSCRIPT], 141)
        self.assertEqual(class_indices[mvp.SEED_OBJECT_COMPILED_METHOD], 156)
        self.assertEqual(
            class_seed_objects[1].fields,
            [
                (mvp.SEED_FIELD_NIL, 0),
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SEED_OBJECT_TRANSCRIPT),
                (mvp.SEED_FIELD_OBJECT_INDEX, 208),
                (mvp.SEED_FIELD_SMALL_INTEGER, 2),
            ],
        )

    def test_declares_seed_layout_sections_and_derives_layout(self) -> None:
        self.assertEqual(
            mvp.SEED_LAYOUT_SECTION_NAMES,
            [
                "class_descriptors",
                "selectors",
                "compiled_methods",
                "method_entries",
                "method_descriptors",
            ],
        )
        layout = mvp.build_seed_layout(140, mvp.CLASS_DESCRIPTOR_KIND_ORDER)
        self.assertEqual(layout["class_descriptors"], mvp.SeedLayoutSection(140, 17))
        self.assertEqual(layout["selectors"], mvp.SeedLayoutSection(157, 17))
        self.assertEqual(layout["compiled_methods"], mvp.SeedLayoutSection(174, 12))
        self.assertEqual(layout["method_entries"], mvp.SeedLayoutSection(186, 22))
        self.assertEqual(layout["method_descriptors"], mvp.SeedLayoutSection(208, 22))

    def test_builds_dynamic_seed_sections_from_fixed_boot_graph(self) -> None:
        seed_objects, _seed_object_indices_by_name, _glyph_object_indices = mvp.build_fixed_boot_seed_objects()
        dynamic_sections = mvp.build_dynamic_seed_sections(seed_objects)

        self.assertEqual(dynamic_sections.seed_layout["class_descriptors"], mvp.SeedLayoutSection(140, 17))
        self.assertEqual(dynamic_sections.class_indices[mvp.SEED_OBJECT_TRANSCRIPT], 141)
        self.assertEqual(dynamic_sections.class_indices[mvp.SEED_OBJECT_COMPILED_METHOD], 156)
        self.assertEqual(seed_objects[0].class_index, 141)
        self.assertEqual(len(dynamic_sections.class_seed_objects), 17)
        self.assertEqual(len(dynamic_sections.selector_seed_objects), 17)
        self.assertEqual(len(dynamic_sections.compiled_method_seed_objects), 12)
        self.assertEqual(len(dynamic_sections.method_entry_seed_objects), 22)
        self.assertEqual(len(dynamic_sections.method_seed_objects), 22)
        self.assertEqual(
            dynamic_sections.class_seed_objects[0].fields,
            [
                (mvp.SEED_FIELD_NIL, 0),
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SEED_OBJECT_CLASS),
                (mvp.SEED_FIELD_OBJECT_INDEX, 208),
                (mvp.SEED_FIELD_SMALL_INTEGER, 1),
            ],
        )

    def test_builds_seed_bindings_and_encodes_seed_manifest(self) -> None:
        seed_objects, seed_object_indices_by_name, glyph_object_indices = mvp.build_fixed_boot_seed_objects()
        dynamic_sections = mvp.build_dynamic_seed_sections(seed_objects)
        seed_objects.extend(dynamic_sections.class_seed_objects)
        seed_objects.extend(dynamic_sections.selector_seed_objects)
        seed_objects.extend(dynamic_sections.compiled_method_seed_objects)
        seed_objects.extend(dynamic_sections.method_entry_seed_objects)
        seed_objects.extend(dynamic_sections.method_seed_objects)

        bindings = mvp.build_seed_bindings(seed_object_indices_by_name)
        self.assertEqual(bindings.global_bindings[0], (mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRANSCRIPT"], 0))
        self.assertEqual(bindings.root_bindings[0], (mvp.SEED_ROOT_VALUES["RECORZ_MVP_SEED_ROOT_DEFAULT_FORM"], 9))

        manifest = mvp.encode_seed_manifest(seed_objects, bindings, glyph_object_indices)
        self.assertEqual(struct.unpack_from(mvp.SEED_HEADER_FORMAT, manifest, 0), (mvp.SEED_MAGIC, mvp.SEED_VERSION, 230, 6, 6, 128, 0))

    def test_declares_method_seed_orders_and_materializes_method_seed_objects(self) -> None:
        self.assertEqual(
            mvp.SELECTOR_VALUE_ORDER,
            [1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 16, 17, 18, 19, 20, 22],
        )
        self.assertEqual(
            mvp.COMPILED_METHOD_ENTRY_ORDER[:4],
            [
                "RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW",
                "RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_CR",
                "RECORZ_MVP_METHOD_ENTRY_DISPLAY_DEFAULT_FORM",
                "RECORZ_MVP_METHOD_ENTRY_DISPLAY_CLEAR",
            ],
        )
        class_indices = mvp.build_class_index_map(mvp.CLASS_DESCRIPTOR_KIND_ORDER, 140)
        selector_indices, selector_seed_objects = mvp.build_selector_seed_objects(157, class_indices[mvp.SEED_OBJECT_SELECTOR])
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SHOW"]], 157)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_INSTANCE_KIND"]], 173)
        self.assertEqual(
            selector_seed_objects[0].fields,
            [(mvp.SEED_FIELD_SMALL_INTEGER, mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SHOW"])],
        )
        compiled_method_indices, compiled_method_seed_objects = mvp.build_compiled_method_seed_objects(
            174,
            class_indices[mvp.SEED_OBJECT_COMPILED_METHOD],
        )
        self.assertEqual(compiled_method_indices["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"], 174)
        self.assertEqual(compiled_method_indices["RECORZ_MVP_METHOD_ENTRY_CLASS_INSTANCE_KIND"], 185)
        self.assertEqual(
            compiled_method_seed_objects[0].fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, instruction)
                for instruction in mvp.COMPILED_METHOD_PROGRAM_BY_ENTRY_NAME["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"]
            ],
        )
        method_entry_indices, method_entry_seed_objects = mvp.build_method_entry_seed_objects(
            186,
            class_indices[mvp.SEED_OBJECT_METHOD_ENTRY],
            compiled_method_indices,
        )
        self.assertEqual(
            method_entry_seed_objects[0].fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.METHOD_ENTRY_VALUES["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"]),
                (mvp.SEED_FIELD_OBJECT_INDEX, 174),
            ],
        )
        bitblt_fill_entry_index = mvp.METHOD_ENTRY_ORDER.index("RECORZ_MVP_METHOD_ENTRY_BITBLT_FILL_FORM_COLOR")
        self.assertEqual(
            method_entry_seed_objects[bitblt_fill_entry_index].fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.METHOD_ENTRY_VALUES["RECORZ_MVP_METHOD_ENTRY_BITBLT_FILL_FORM_COLOR"]),
                (
                    mvp.SEED_FIELD_SMALL_INTEGER,
                    mvp.PRIMITIVE_BINDING_BY_ENTRY_NAME["RECORZ_MVP_METHOD_ENTRY_BITBLT_FILL_FORM_COLOR"],
                ),
            ],
        )
        method_start_by_kind, method_count_by_kind, method_seed_objects = mvp.build_method_descriptor_seed_objects(
            mvp.CLASS_DESCRIPTOR_KIND_ORDER,
            class_indices[mvp.SEED_OBJECT_METHOD_DESCRIPTOR],
            selector_indices,
            method_entry_indices,
            208,
        )
        self.assertEqual(method_start_by_kind[mvp.SEED_OBJECT_CLASS], 208)
        self.assertEqual(method_count_by_kind[mvp.SEED_OBJECT_FORM], 6)
        self.assertEqual(
            method_seed_objects[0].fields,
            [
                (mvp.SEED_FIELD_OBJECT_INDEX, 173),
                (mvp.SEED_FIELD_SMALL_INTEGER, 0),
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SEED_OBJECT_CLASS),
                (mvp.SEED_FIELD_OBJECT_INDEX, 207),
            ],
        )

    def test_renders_generated_runtime_bindings_header(self) -> None:
        header = mvp.render_generated_runtime_bindings_header()

        self.assertIn("#ifndef RECORZ_QEMU_RISCV64_GENERATED_RUNTIME_BINDINGS_H", header)
        self.assertIn("enum recorz_mvp_opcode {", header)
        self.assertIn("RECORZ_MVP_OP_STORE_LEXICAL = 9,", header)
        self.assertIn("enum recorz_mvp_global {", header)
        self.assertIn("RECORZ_MVP_GLOBAL_BITMAP = 6,", header)
        self.assertIn("enum recorz_mvp_selector {", header)
        self.assertIn("RECORZ_MVP_SELECTOR_INSTANCE_KIND = 22,", header)
        self.assertIn("enum recorz_mvp_literal_kind {", header)
        self.assertIn("RECORZ_MVP_LITERAL_SMALL_INTEGER = 2,", header)
        self.assertIn("enum recorz_mvp_object_kind {", header)
        self.assertIn("RECORZ_MVP_OBJECT_COMPILED_METHOD = 22,", header)
        self.assertIn("enum recorz_mvp_seed_field_kind {", header)
        self.assertIn("RECORZ_MVP_SEED_FIELD_OBJECT_INDEX = 2,", header)
        self.assertIn("enum recorz_mvp_seed_root {", header)
        self.assertIn("RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS = 6,", header)
        self.assertIn("enum recorz_mvp_method_entry {", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW = 1,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_CLASS_INSTANCE_KIND = 22,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_COUNT = 23,", header)
        self.assertIn("enum recorz_mvp_primitive_binding {", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_BITBLT_FILL_FORM_COLOR = 1,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_FORM_NEWLINE = 10,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_COUNT = 11,", header)
        self.assertIn("#define RECORZ_MVP_GENERATED_PRIMITIVE_BINDING_HANDLERS \\", header)
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_BITBLT_COPY_BITMAP_REGION_TO_FORM_X_Y_SCALE_COLOR] = "
            "execute_entry_bitblt_copy_bitmap_region_to_form_x_y_scale_color,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_FORM_NEWLINE] = execute_entry_form_newline,",
            header,
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
        self.assertEqual(glyph_code_count, len(mvp.GLYPH_BITMAP_BOOT_SPECS))
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
