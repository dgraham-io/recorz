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
        self.assertEqual(program.lexical_names, ["pixels"])
        self.assertIn((mvp.LITERAL_SMALL_INTEGER, 640), [(literal.kind, literal.value) for literal in program.literals])
        self.assertIn((mvp.LITERAL_SMALL_INTEGER, 480), [(literal.kind, literal.value) for literal in program.literals])
        self.assertIn(mvp.OP_STORE_LEXICAL, [instruction.opcode for instruction in program.instructions])
        self.assertIn(mvp.OP_PUSH_LEXICAL, [instruction.opcode for instruction in program.instructions])
        self.assertIn("RECORZ_MVP_SELECTOR_MULTIPLY", [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND])
        self.assertIn("RECORZ_MVP_SELECTOR_PRINT_STRING", [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND])

    def test_builds_program_manifest_with_top_level_lexical_names(self) -> None:
        program = mvp.build_program("| x | x := 4. [ x + 3 ] value")
        manifest = mvp.build_program_manifest(program)

        self.assertEqual(program.lexical_names, ["x"])
        self.assertTrue(manifest.endswith(b"x\x00"))

    def test_lowers_this_context_in_workspace_programs(self) -> None:
        program = mvp.build_program("thisContext detail")
        self.assertEqual(
            [instruction.opcode for instruction in program.instructions],
            [
                mvp.OP_PUSH_THIS_CONTEXT,
                mvp.OP_SEND,
                mvp.OP_RETURN,
            ],
        )
        self.assertEqual(program.instructions[1].operand_a, "RECORZ_MVP_SELECTOR_DETAIL")

    def test_lowers_top_level_block_literals_and_boolean_globals(self) -> None:
        program = mvp.build_program("true ifTrue: [ :x | x + 4 ] ifFalse: [ :x | x + 5 ]")
        literal_strings = [literal.value.strip() for literal in program.literals if literal.kind == mvp.LITERAL_STRING]
        selectors = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_SEND]
        globals_used = [instruction.operand_a for instruction in program.instructions if instruction.opcode == mvp.OP_PUSH_GLOBAL]

        self.assertIn("RECORZ_MVP_GLOBAL_TRUE", globals_used)
        self.assertIn(mvp.OP_PUSH_BLOCK_LITERAL, [instruction.opcode for instruction in program.instructions])
        self.assertIn("RECORZ_MVP_SELECTOR_IF_TRUE_IF_FALSE", selectors)
        self.assertIn(":x | x + 4", literal_strings)
        self.assertIn(":x | x + 5", literal_strings)

    def test_lowers_top_level_self_as_a_real_receiver(self) -> None:
        program = mvp.build_program("self contents")

        self.assertEqual(
            [instruction.opcode for instruction in program.instructions],
            [
                mvp.OP_PUSH_SELF,
                mvp.OP_SEND,
                mvp.OP_RETURN,
            ],
        )
        self.assertEqual(program.instructions[1].operand_a, "RECORZ_MVP_SELECTOR_CONTENTS")

    def test_boot_program_seeds_workspace_contents_with_original_source(self) -> None:
        program = mvp.build_boot_program("Transcript show: 'BOOT'")

        self.assertEqual(
            [instruction.opcode for instruction in program.instructions[:4]],
            [
                mvp.OP_PUSH_SELF,
                mvp.OP_PUSH_LITERAL,
                mvp.OP_SEND,
                mvp.OP_POP,
            ],
        )
        self.assertEqual(program.instructions[2].operand_a, "RECORZ_MVP_SELECTOR_SET_CONTENTS")
        self.assertEqual(program.literals[program.instructions[1].operand_b], mvp.Literal(mvp.LITERAL_STRING, "Transcript show: 'BOOT'"))

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
        self.assertEqual(
            mvp.compile_kernel_method_program(
                "Object",
                [],
                "detail ^thisContext detail",
            ),
            [
                mvp.encode_compiled_method_instruction("push_this_context"),
                mvp.encode_compiled_method_instruction(
                    "send",
                    mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_DETAIL"],
                    0,
                ),
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
                "Context.rz",
                "Display.rz",
                "Form.rz",
                "FormFactory.rz",
                "Glyphs.rz",
                "KernelInstaller.rz",
                "Transcript.rz",
                "Workspace.rz",
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
                "RecorzKernelClass: #Form descriptorOrder: 10 objectKindOrder: 2 sourceBootOrder: 7 instanceVariableNames: 'bits'",
                "Form.rz",
            ),
            mvp.KernelClassHeader("Form", 10, 2, 7, ("bits",)),
        )
        self.assertEqual(
            mvp.parse_kernel_class_header(
                "RecorzKernelClass: #Display descriptorOrder: 2 objectKindOrder: 1 sourceBootOrder: 1 instanceVariableNames: ''",
                "Display.rz",
            ),
            mvp.KernelClassHeader("Display", 2, 1, 1, ()),
        )
        self.assertEqual(
            mvp.parse_kernel_class_header(
                "RecorzKernelClass: #CompiledMethod descriptorOrder: 16 objectKindOrder: 16 instanceVariableNames: 'word0 word1 word2 word3'",
                "CompiledMethod.rz",
            ),
            mvp.KernelClassHeader("CompiledMethod", 16, 16, None, ("word0", "word1", "word2", "word3")),
        )

    def test_parses_kernel_boot_object_chunk(self) -> None:
        self.assertEqual(
            mvp.parse_kernel_boot_object_chunk(
                "\n".join(
                    [
                        "RecorzKernelBootObject: #DefaultForm family: #beforeGlyphs order: 9 class: #Form",
                        "fields: 'object:FramebufferBitmap'",
                        "rootExports: 'default_form'",
                    ]
                ),
                "Form.rz",
            ),
            mvp.KernelBootObjectDeclaration(
                "DefaultForm",
                "before_glyphs",
                9,
                "Form",
                ((mvp.FIELD_SPEC_OBJECT_REF, "FramebufferBitmap"),),
                (),
                ("default_form",),
                "Form.rz",
            ),
        )
        self.assertEqual(
            mvp.parse_kernel_boot_object_chunk(
                "\n".join(
                    [
                        "RecorzKernelBootObject: #Transcript family: #beforeGlyphs order: 0 class: #Transcript",
                        "globalExports: 'Transcript'",
                    ]
                ),
                "Transcript.rz",
            ),
            mvp.KernelBootObjectDeclaration(
                "Transcript",
                "before_glyphs",
                0,
                "Transcript",
                (),
                ("Transcript",),
                (),
                "Transcript.rz",
            ),
        )

    def test_parses_kernel_glyph_bitmap_family_chunk(self) -> None:
        self.assertEqual(
            mvp.parse_kernel_glyph_bitmap_family_chunk(
                "RecorzKernelGlyphBitmapFamily: #GlyphBitmap family: #glyphBitmaps class: #Bitmap width: 5 height: 7 storageKind: 2 count: 128",
                "Bitmap.rz",
            ),
            mvp.KernelGlyphBitmapFamilyDeclaration(
                "GlyphBitmap",
                "glyph_bitmaps",
                "Bitmap",
                5,
                7,
                2,
                128,
                "Bitmap.rz",
            ),
        )

    def test_parses_kernel_selector_chunk(self) -> None:
        self.assertEqual(
            mvp.parse_kernel_selector_chunk(
                "RecorzKernelSelector: #copyBitmap:sourceX:sourceY:width:height:toForm:x:y:scale:color: order: 17",
                "Selector.rz",
            ),
            mvp.KernelSelectorDeclaration(
                "copyBitmap:sourceX:sourceY:width:height:toForm:x:y:scale:color:",
                17,
                "Selector.rz",
            ),
        )
        self.assertEqual(
            mvp.parse_kernel_selector_chunk(
                "RecorzKernelSelector: #+ order: 9",
                "Selector.rz",
            ),
            mvp.KernelSelectorDeclaration("+", 9, "Selector.rz"),
        )

    def test_parses_kernel_root_chunk(self) -> None:
        self.assertEqual(
            mvp.parse_kernel_root_chunk(
                "RecorzKernelRoot: #default_form object: #DefaultForm order: 0",
                "Form.rz",
            ),
            mvp.KernelRootDeclaration("default_form", "DefaultForm", 0, "Form.rz"),
        )
        self.assertEqual(
            mvp.parse_kernel_root_chunk(
                "RecorzKernelRoot: #transcript_metrics object: #TranscriptMetrics order: 5",
                "TextMetrics.rz",
            ),
            mvp.KernelRootDeclaration("transcript_metrics", "TranscriptMetrics", 5, "TextMetrics.rz"),
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

    def test_loads_runtime_format_constants_from_spec(self) -> None:
        self.assertEqual(mvp.RUNTIME_SPEC_PATH.name, "runtime_spec.json")
        self.assertEqual(mvp.RUNTIME_SPEC["program"]["magic"], "RCZP")
        self.assertEqual(mvp.RUNTIME_SPEC["image"]["profile"], "RV64MVP1")
        self.assertEqual(mvp.RUNTIME_SPEC["seed"]["version"], 16)
        self.assertEqual(
            mvp.OPCODE_VALUES,
            mvp.build_constant_value_map_from_explicit_specs(mvp.OPCODE_SPEC),
        )
        self.assertEqual(
            mvp.LITERAL_VALUES,
            mvp.build_constant_value_map_from_explicit_specs(mvp.LITERAL_KIND_SPEC),
        )
        self.assertEqual(
            mvp.SEED_FIELD_KIND_DEFINITIONS,
            mvp.build_named_constant_maps_from_explicit_specs(mvp.SEED_FIELD_KIND_SPEC)[2],
        )
        self.assertEqual(
            mvp.COMPILED_METHOD_OPCODE_VALUES,
            mvp.build_named_constant_maps_from_explicit_specs(mvp.COMPILED_METHOD_OPCODE_SPEC)[1],
        )
        self.assertEqual(
            mvp.METHOD_IMPLEMENTATION_DEFINITIONS,
            mvp.build_named_constant_maps_from_explicit_specs(mvp.METHOD_IMPLEMENTATION_RUNTIME_SPEC)[2],
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
            mvp.PRIMITIVE_BINDING_VALUES["kernelInstallerInstallCompiledMethodOnClassSelectorIdArgumentCount"],
            13,
        )
        self.assertEqual(
            mvp.PRIMITIVE_BINDING_BY_ENTRY_NAME["RECORZ_MVP_METHOD_ENTRY_FORM_WRITE_STRING"],
            mvp.PRIMITIVE_BINDING_VALUES["formWriteString"],
        )
        self.assertEqual(
            mvp.PRIMITIVE_BINDING_BY_ENTRY_NAME["RECORZ_MVP_METHOD_ENTRY_FORM_NEWLINE"],
            mvp.PRIMITIVE_BINDING_VALUES["formNewline"],
        )

    def test_derives_global_maps_from_boot_object_exports_and_selector_maps_from_source(self) -> None:
        self.assertEqual(
            mvp.GLOBAL_SPECS,
            [
                ("Transcript", "RECORZ_MVP_GLOBAL_TRANSCRIPT"),
                ("Display", "RECORZ_MVP_GLOBAL_DISPLAY"),
                ("BitBlt", "RECORZ_MVP_GLOBAL_BITBLT"),
                ("Glyphs", "RECORZ_MVP_GLOBAL_GLYPHS"),
                ("Form", "RECORZ_MVP_GLOBAL_FORM"),
                ("Bitmap", "RECORZ_MVP_GLOBAL_BITMAP"),
                ("KernelInstaller", "RECORZ_MVP_GLOBAL_KERNEL_INSTALLER"),
                ("Workspace", "RECORZ_MVP_GLOBAL_WORKSPACE"),
                ("true", "RECORZ_MVP_GLOBAL_TRUE"),
                ("false", "RECORZ_MVP_GLOBAL_FALSE"),
            ],
        )
        self.assertEqual(
            mvp.GLOBAL_SPECS,
            mvp.build_global_specs_from_boot_object_exports(mvp.FIXED_BOOT_GRAPH_SPEC),
        )
        self.assertEqual(
            mvp.SELECTOR_SPECS,
            mvp.build_selector_specs_from_declarations(mvp.KERNEL_SELECTOR_DECLARATIONS_IN_ORDER),
        )
        self.assertEqual(mvp.GLOBAL_IDS["Transcript"], "RECORZ_MVP_GLOBAL_TRANSCRIPT")
        self.assertEqual(mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_BITMAP"], 6)
        self.assertEqual(mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_KERNEL_INSTALLER"], 7)
        self.assertEqual(mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_WORKSPACE"], 8)
        self.assertEqual(mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRUE"], 9)
        self.assertEqual(mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_FALSE"], 10)
        self.assertEqual(mvp.GLOBAL_DEFINITIONS[-1], ("RECORZ_MVP_GLOBAL_FALSE", 10))
        self.assertEqual(mvp.SELECTOR_IDS["writeString:"], "RECORZ_MVP_SELECTOR_WRITE_STRING")
        self.assertEqual(
            mvp.KERNEL_SELECTOR_DECLARATIONS_BY_SELECTOR["+"],
            mvp.KernelSelectorDeclaration("+", 9, "Selector.rz"),
        )
        self.assertEqual(mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_INSTANCE_KIND"], 22)
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_INSTALL_COMPILED_METHOD_ON_CLASS_SELECTOR_ID_ARGUMENT_COUNT"],
            24,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_INSTALL_METHOD_SOURCE_ON_CLASS"],
            25,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_FILE_IN_METHOD_CHUNKS_ON_CLASS"],
            26,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_FILE_IN_CLASS_CHUNKS"],
            27,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_NEW"],
            28,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_CLASS_NAMED"],
            29,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_VALUE"],
            30,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SET_VALUE"],
            31,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_DETAIL"],
            32,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SET_DETAIL"],
            33,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_REMEMBER_OBJECT_NAMED"],
            34,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_OBJECT_NAMED"],
            35,
        )
        self.assertEqual(
            mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SAVE_SNAPSHOT"],
            36,
        )
        self.assertEqual(
            mvp.SELECTOR_DEFINITIONS[16],
            ("RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE_COLOR", 17),
        )
        self.assertEqual(
            mvp.SELECTOR_DEFINITIONS[-9:],
            mvp.SELECTOR_DEFINITIONS[-9:],
        )
        self.assertEqual(
            mvp.SELECTOR_DEFINITIONS[-24:],
            [
                ("RECORZ_MVP_SELECTOR_BROWSE_CLASS_METHOD_OF_CLASS_NAMED", 53),
                ("RECORZ_MVP_SELECTOR_FILE_OUT_CLASS_NAMED", 54),
                ("RECORZ_MVP_SELECTOR_MEMORY_REPORT", 55),
                ("RECORZ_MVP_SELECTOR_BROWSE_PROTOCOLS_FOR_CLASS_NAMED", 56),
                ("RECORZ_MVP_SELECTOR_BROWSE_PROTOCOL_OF_CLASS_NAMED", 57),
                ("RECORZ_MVP_SELECTOR_BROWSE_CLASS_PROTOCOLS_FOR_CLASS_NAMED", 58),
                ("RECORZ_MVP_SELECTOR_BROWSE_CLASS_PROTOCOL_OF_CLASS_NAMED", 59),
                ("RECORZ_MVP_SELECTOR_FILE_OUT_PACKAGE_NAMED", 60),
                ("RECORZ_MVP_SELECTOR_BROWSE_PACKAGES", 61),
                ("RECORZ_MVP_SELECTOR_BROWSE_PACKAGE_NAMED", 62),
                ("RECORZ_MVP_SELECTOR_SUM", 63),
                ("RECORZ_MVP_SELECTOR_SET_LEFT_RIGHT", 64),
                ("RECORZ_MVP_SELECTOR_TRUTH", 65),
                ("RECORZ_MVP_SELECTOR_FALSITY", 66),
                ("RECORZ_MVP_SELECTOR_IF_TRUE", 67),
                ("RECORZ_MVP_SELECTOR_IF_FALSE", 68),
                ("RECORZ_MVP_SELECTOR_IF_TRUE_IF_FALSE", 69),
                ("RECORZ_MVP_SELECTOR_EQUAL", 70),
                ("RECORZ_MVP_SELECTOR_LESS_THAN", 71),
                ("RECORZ_MVP_SELECTOR_GREATER_THAN", 72),
                ("RECORZ_MVP_SELECTOR_SENDER", 73),
                ("RECORZ_MVP_SELECTOR_RECEIVER", 74),
                ("RECORZ_MVP_SELECTOR_ALIVE", 75),
                ("RECORZ_MVP_SELECTOR_VALUE_ARG", 76),
            ],
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
        self.assertEqual(mvp.OBJECT_KIND_SPECS[-1], ("Context", "RECORZ_MVP_OBJECT_CONTEXT"))
        self.assertEqual(
            mvp.KERNEL_CLASS_HEADERS_IN_DESCRIPTOR_ORDER[0],
            mvp.KernelClassHeader("Class", 0, 12, 8, ("superclass", "instanceKind", "methodStart", "methodCount")),
        )
        self.assertEqual(
            [header.class_name for header in mvp.KERNEL_CLASS_HEADERS_IN_OBJECT_KIND_ORDER],
            [
                "Transcript",
                "Display",
                "Form",
                "Bitmap",
                "BitBlt",
                "Glyphs",
                "FormFactory",
                "BitmapFactory",
                "TextLayout",
                "TextStyle",
                "TextMetrics",
                "TextBehavior",
                "Class",
                "MethodDescriptor",
                "MethodEntry",
                "Selector",
                "CompiledMethod",
                "KernelInstaller",
                "Object",
                "Workspace",
                "True",
                "False",
                "BlockClosure",
                "Context",
            ],
        )
        self.assertEqual(
            mvp.KERNEL_CLASS_HEADERS_IN_DESCRIPTOR_ORDER[-1],
            mvp.KernelClassHeader("Context", 23, 23, 14, ("sender", "receiver", "detail", "alive")),
        )
        self.assertEqual(
            mvp.KERNEL_CLASS_HEADERS_BY_NAME["Class"],
            mvp.KernelClassHeader("Class", 0, 12, 8, ("superclass", "instanceKind", "methodStart", "methodCount")),
        )
        self.assertEqual(mvp.KERNEL_CLASS_HEADERS_BY_NAME["Form"], mvp.KernelClassHeader("Form", 10, 2, 7, ("bits",)))
        self.assertEqual(
            mvp.KERNEL_CLASS_HEADERS_BY_NAME["KernelInstaller"],
            mvp.KernelClassHeader("KernelInstaller", 17, 17, 9, ()),
        )
        self.assertEqual(
            mvp.KERNEL_CLASS_HEADERS_BY_NAME["Object"],
            mvp.KernelClassHeader("Object", 18, 18, 10, ()),
        )
        self.assertEqual(
            mvp.KERNEL_CLASS_HEADERS_BY_NAME["Workspace"],
            mvp.KernelClassHeader("Workspace", 19, 19, 11, ("currentViewKind", "currentTargetName", "currentSource", "lastSource")),
        )
        self.assertEqual(
            mvp.KERNEL_CLASS_HEADERS_BY_NAME["Context"],
            mvp.KernelClassHeader("Context", 23, 23, 14, ("sender", "receiver", "detail", "alive")),
        )
        self.assertEqual(
            mvp.KERNEL_CLASS_HEADERS_BY_NAME["TextLayout"],
            mvp.KernelClassHeader("TextLayout", 7, 8, None, ("left", "top", "scale", "lineSpacing")),
        )
        self.assertEqual(mvp.KERNEL_CLASS_NAME_TO_OBJECT_KIND["BitBlt"], mvp.SEED_OBJECT_BITBLT)
        self.assertEqual(mvp.KERNEL_CLASS_NAME_TO_OBJECT_KIND["CompiledMethod"], mvp.SEED_OBJECT_COMPILED_METHOD)
        self.assertEqual(mvp.KERNEL_CLASS_NAME_TO_OBJECT_KIND["KernelInstaller"], mvp.SEED_OBJECT_KERNEL_INSTALLER)
        self.assertEqual(mvp.OBJECT_KIND_DEFINITIONS[-1], ("RECORZ_MVP_OBJECT_CONTEXT", 24))
        self.assertEqual(mvp.CLASS_FIELD_METHOD_START, mvp.kernel_instance_variable_index("Class", "methodStart"))
        self.assertEqual(mvp.METHOD_FIELD_ENTRY, mvp.kernel_instance_variable_index("MethodDescriptor", "entry"))
        self.assertEqual(
            mvp.METHOD_ENTRY_FIELD_IMPLEMENTATION,
            mvp.kernel_instance_variable_index("MethodEntry", "implementation"),
        )
        self.assertEqual(
            mvp.SEED_ROOT_SPECS,
            mvp.build_root_specs_from_declarations(mvp.KERNEL_ROOT_DECLARATIONS_IN_ORDER),
        )
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
        self.assertEqual(
            mvp.KERNEL_ROOT_DECLARATIONS_BY_NAME["default_form"],
            mvp.KernelRootDeclaration("default_form", "DefaultForm", 0, "Form.rz"),
        )
        self.assertEqual(
            mvp.KERNEL_ROOT_DECLARATIONS_BY_NAME["transcript_metrics"],
            mvp.KernelRootDeclaration("transcript_metrics", "TranscriptMetrics", 5, "TextMetrics.rz"),
        )
        self.assertEqual(
            mvp.KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME["DefaultForm"],
            mvp.KernelBootObjectDeclaration(
                "DefaultForm",
                "before_glyphs",
                9,
                "Form",
                ((mvp.FIELD_SPEC_OBJECT_REF, "FramebufferBitmap"),),
                (),
                ("default_form",),
                "Form.rz",
            ),
        )
        self.assertEqual(
            mvp.KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME["TranscriptBehavior"],
            mvp.KernelBootObjectDeclaration(
                "TranscriptBehavior",
                "after_glyphs",
                1,
                "TextBehavior",
                ((mvp.FIELD_SPEC_GLYPH_REF, 32), (mvp.FIELD_SPEC_SMALL_INTEGER, 1)),
                (),
                ("transcript_behavior",),
                "TextBehavior.rz",
            ),
        )

    def test_materializes_named_seed_fields_in_source_slot_order(self) -> None:
        self.assertEqual(
            mvp.materialize_named_seed_fields(
                "MethodEntry",
                {
                    "implementation": (mvp.SEED_FIELD_OBJECT_INDEX, 17),
                    "executionId": (mvp.SEED_FIELD_SMALL_INTEGER, 3),
                },
            ),
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, 3),
                (mvp.SEED_FIELD_OBJECT_INDEX, 17),
            ],
        )
        self.assertEqual(
            mvp.materialize_named_seed_fields(
                "CompiledMethod",
                {
                    "word0": (mvp.SEED_FIELD_SMALL_INTEGER, 11),
                    "word2": (mvp.SEED_FIELD_SMALL_INTEGER, 33),
                },
            ),
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, 11),
                (mvp.SEED_FIELD_NIL, 0),
                (mvp.SEED_FIELD_SMALL_INTEGER, 33),
            ],
        )

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
                "KernelInstaller",
                "Object",
                "Workspace",
                "True",
                "False",
                "Context",
            ],
        )
        self.assertEqual(
            [header.class_name for header in mvp.KERNEL_SOURCE_CLASS_HEADERS],
            mvp.KERNEL_CLASS_BOOT_ORDER,
        )
        self.assertEqual(
            [header.source_boot_order for header in mvp.KERNEL_SOURCE_CLASS_HEADERS],
            list(range(len(mvp.KERNEL_SOURCE_CLASS_HEADERS))),
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
            mvp.METHOD_ENTRY_ORDER[-36:],
            [
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_FILE_OUT_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_FILE_OUT_PACKAGE_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_MEMORY_REPORT",
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_REMEMBER_OBJECT_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_OBJECT_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_SAVE_SNAPSHOT",
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_CONFIGURE_STARTUP_SELECTOR_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_CLEAR_STARTUP",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_FILE_IN",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_CONTENTS",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_SET_CONTENTS",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_EVALUATE",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_EVALUATE_CURRENT",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_FILE_IN_CURRENT",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASSES",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_PACKAGES",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_METHODS_FOR_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_PACKAGE_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_PROTOCOLS_FOR_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_PROTOCOL_OF_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_OBJECT_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_METHOD_OF_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_METHODS_FOR_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_PROTOCOLS_FOR_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_PROTOCOL_OF_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_METHOD_OF_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_FILE_OUT_CLASS_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_FILE_OUT_PACKAGE_NAMED",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_RERUN",
                "RECORZ_MVP_METHOD_ENTRY_WORKSPACE_REOPEN",
                "RECORZ_MVP_METHOD_ENTRY_CONTEXT_SENDER",
                "RECORZ_MVP_METHOD_ENTRY_CONTEXT_RECEIVER",
                "RECORZ_MVP_METHOD_ENTRY_CONTEXT_DETAIL",
                "RECORZ_MVP_METHOD_ENTRY_CONTEXT_ALIVE",
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
                "KernelInstaller": "KernelInstaller",
                "Workspace": "Workspace",
                "true": "True",
                "false": "False",
            },
        )
        self.assertEqual(
            mvp.SEED_ROOT_NAME_TO_BOOT_OBJECT_NAME,
            mvp.build_root_object_name_map_from_declarations(mvp.KERNEL_ROOT_DECLARATIONS_IN_ORDER),
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
        self.assertIs(mvp.BOOT_OBJECT_SPECS_BY_NAME["DefaultForm"], mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS[-5])
        self.assertIs(mvp.BOOT_OBJECT_SPECS_BY_NAME["KernelInstaller"], mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS[-4])
        self.assertIs(mvp.BOOT_OBJECT_SPECS_BY_NAME["Workspace"], mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS[-3])
        self.assertIs(mvp.BOOT_OBJECT_SPECS_BY_NAME["TranscriptBehavior"], mvp.BOOT_OBJECT_SPECS_AFTER_GLYPHS[-1])
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
            [family_spec.name for family_spec in mvp.BOOT_IMAGE_SPEC.fixed_boot_graph_spec.family_specs],
            [
                "before_glyphs",
                "glyph_bitmaps",
                "after_glyphs",
            ],
        )
        self.assertEqual(
            list(mvp.BOOT_OBJECT_FAMILY_SPECS_BY_NAME),
            [
                "before_glyphs",
                "glyph_bitmaps",
                "after_glyphs",
            ],
        )
        self.assertEqual(mvp.TRANSCRIPT_LAYOUT_FIELD_VALUES, mvp.boot_object_small_integer_field_values("TranscriptLayout"))
        self.assertEqual(mvp.TRANSCRIPT_STYLE_FIELD_VALUES, mvp.boot_object_small_integer_field_values("TranscriptStyle"))
        self.assertEqual(mvp.FRAMEBUFFER_BITMAP_FIELD_VALUES, mvp.boot_object_small_integer_field_values("FramebufferBitmap"))
        self.assertEqual(mvp.TRANSCRIPT_METRICS_FIELD_VALUES, mvp.boot_object_small_integer_field_values("TranscriptMetrics"))
        self.assertEqual(mvp.DEFAULT_FORM_BOOT_FIELD_SPECS, mvp.boot_object_field_specs("DefaultForm"))
        self.assertEqual(mvp.TRANSCRIPT_BEHAVIOR_BOOT_FIELD_SPECS, mvp.boot_object_field_specs("TranscriptBehavior"))
        self.assertEqual(mvp.GLYPH_FALLBACK_CODE, mvp.boot_object_first_glyph_field_value("TranscriptBehavior"))
        self.assertEqual(mvp.TRANSCRIPT_LAYOUT_FIELD_VALUES, (24, 24, 4, 2))
        self.assertEqual(mvp.TRANSCRIPT_STYLE_FIELD_VALUES, (0x001F2933, 0x00F7F3E8))
        self.assertEqual(mvp.FRAMEBUFFER_BITMAP_FIELD_VALUES, (1024, 768, mvp.BITMAP_STORAGE_FRAMEBUFFER, 0))
        self.assertEqual(mvp.TRANSCRIPT_METRICS_FIELD_VALUES, (6, 8))
        self.assertEqual(
            mvp.DEFAULT_FORM_BOOT_FIELD_SPECS,
            ((mvp.FIELD_SPEC_OBJECT_REF, "FramebufferBitmap"),),
        )
        self.assertEqual(
            mvp.TRANSCRIPT_BEHAVIOR_BOOT_FIELD_SPECS,
            (
                (mvp.FIELD_SPEC_GLYPH_REF, mvp.GLYPH_FALLBACK_CODE),
                (mvp.FIELD_SPEC_SMALL_INTEGER, 1),
            ),
        )
        self.assertEqual(
            mvp.build_small_integer_boot_field_specs((1, 2, 3)),
            (
                (mvp.FIELD_SPEC_SMALL_INTEGER, 1),
                (mvp.FIELD_SPEC_SMALL_INTEGER, 2),
                (mvp.FIELD_SPEC_SMALL_INTEGER, 3),
            ),
        )
        self.assertEqual(mvp.BOOT_OBJECT_SPEC_NAMES_IN_ORDER[0], "Transcript")
        self.assertEqual(mvp.BOOT_OBJECT_SPEC_NAMES_IN_ORDER[9], "DefaultForm")
        self.assertEqual(mvp.BOOT_OBJECT_SPEC_NAMES_IN_ORDER[-1], "TranscriptBehavior")
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
                "KernelInstaller",
                "Workspace",
                "True",
                "False",
            ],
        )
        self.assertEqual(
            tuple(mvp.BOOT_OBJECT_SPECS_BEFORE_GLYPHS),
            mvp.build_boot_object_specs_from_declarations(
                mvp.KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME,
                "before_glyphs",
            ),
        )
        self.assertEqual(
            [spec.name for spec in mvp.BOOT_OBJECT_SPECS_AFTER_GLYPHS],
            [
                "TranscriptMetrics",
                "TranscriptBehavior",
            ],
        )
        self.assertEqual(
            tuple(mvp.BOOT_OBJECT_SPECS_AFTER_GLYPHS),
            mvp.build_boot_object_specs_from_declarations(
                mvp.KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME,
                "after_glyphs",
            ),
        )
        self.assertEqual(
            mvp.materialize_boot_object_fields(
                mvp.BOOT_OBJECT_SPECS_BY_NAME["DefaultForm"].field_specs,
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
        self.assertIs(mvp.BOOT_IMAGE_SPEC.fixed_boot_graph_spec, mvp.FIXED_BOOT_GRAPH_SPEC)
        self.assertIs(mvp.BOOT_IMAGE_SPEC.ordering_spec, mvp.BOOT_IMAGE_ORDERING_SPEC)
        self.assertEqual(mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.boot_image_spec, mvp.BOOT_IMAGE_SPEC)
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.class_kind_order,
            mvp.BOOT_IMAGE_SPEC.ordering_spec.class_kind_order,
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.seed_layout_section_specs,
            tuple(mvp.SEED_LAYOUT_SECTION_SPECS),
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.selector_value_order,
            mvp.BOOT_IMAGE_SPEC.ordering_spec.selector_value_order,
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.compiled_method_entry_order,
            mvp.BOOT_IMAGE_SPEC.ordering_spec.compiled_method_entry_order,
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.method_entry_order,
            mvp.BOOT_IMAGE_SPEC.ordering_spec.method_entry_order,
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.initial_dynamic_seed_state_fields,
            mvp.INITIAL_DYNAMIC_SEED_STATE_FIELDS,
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.dynamic_seed_object_section_specs,
            tuple(mvp.DYNAMIC_SEED_OBJECT_SECTION_SPECS),
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.dynamic_seed_build_step_specs,
            tuple(mvp.DYNAMIC_SEED_BUILD_STEP_SPECS),
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.fixed_boot_object_count,
            mvp.BOOT_OBJECT_FIXED_COUNT,
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.glyph_bitmap_boot_specs,
            tuple(mvp.GLYPH_BITMAP_BOOT_SPECS),
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.global_name_to_boot_object_name,
            mvp.GLOBAL_NAME_TO_BOOT_OBJECT_NAME,
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.seed_root_name_to_boot_object_name,
            mvp.SEED_ROOT_NAME_TO_BOOT_OBJECT_NAME,
        )
        self.assertEqual(
            tuple(mvp.BOOT_OBJECT_FAMILY_SPECS),
            mvp.BOOT_IMAGE_SPEC.fixed_boot_graph_spec.family_specs,
        )
        self.assertIs(mvp.BOOT_OBJECT_FAMILY_SPECS_BY_NAME["before_glyphs"], mvp.BOOT_OBJECT_FAMILY_SPECS[0])
        self.assertIs(mvp.BOOT_OBJECT_FAMILY_SPECS_BY_NAME["glyph_bitmaps"], mvp.BOOT_OBJECT_FAMILY_SPECS[1])
        self.assertIs(mvp.BOOT_OBJECT_FAMILY_SPECS_BY_NAME["after_glyphs"], mvp.BOOT_OBJECT_FAMILY_SPECS[2])
        self.assertEqual(len(mvp.BOOT_OBJECT_SPECS_IN_ORDER), mvp.BOOT_OBJECT_FIXED_COUNT)
        self.assertEqual(len(mvp.BOOT_OBJECT_FAMILY_SPECS[0].object_specs), 14)
        self.assertTrue(mvp.BOOT_OBJECT_FAMILY_SPECS[1].collect_object_indices)
        self.assertEqual(len(mvp.BOOT_OBJECT_FAMILY_SPECS[1].object_specs), len(mvp.GLYPH_BITMAP_BOOT_SPECS))
        self.assertEqual(len(mvp.BOOT_OBJECT_FAMILY_SPECS[2].object_specs), 2)
        self.assertEqual(mvp.BOOT_OBJECT_FIXED_COUNT, 144)

    def test_builds_fixed_boot_seed_objects_from_declared_families(self) -> None:
        seed_objects, seed_object_indices_by_name, glyph_object_indices = mvp.build_fixed_boot_seed_objects()

        self.assertEqual(len(seed_objects), mvp.BOOT_OBJECT_FIXED_COUNT)
        self.assertEqual(seed_object_indices_by_name["Transcript"], 0)
        self.assertEqual(seed_object_indices_by_name["DefaultForm"], 9)
        self.assertEqual(seed_object_indices_by_name["KernelInstaller"], 10)
        self.assertEqual(seed_object_indices_by_name["Workspace"], 11)
        self.assertEqual(seed_object_indices_by_name["TranscriptBehavior"], 143)
        self.assertEqual(glyph_object_indices[0], 14)
        self.assertEqual(glyph_object_indices[-1], 141)
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
        self.assertEqual(
            mvp.KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION,
            mvp.KernelGlyphBitmapFamilyDeclaration(
                "GlyphBitmap",
                "glyph_bitmaps",
                "Bitmap",
                5,
                7,
                2,
                128,
                "Bitmap.rz",
            ),
        )
        self.assertEqual(mvp.GLYPH_BITMAP_NAME_PREFIX, "GlyphBitmap")
        self.assertEqual(mvp.GLYPH_BITMAP_WIDTH, 5)
        self.assertEqual(mvp.GLYPH_BITMAP_HEIGHT, 7)
        self.assertEqual(len(mvp.GLYPH_BITMAP_BOOT_SPECS), mvp.GLYPH_BITMAP_CODE_COUNT)
        self.assertEqual(
            mvp.GLYPH_BITMAP_BASE_FIELD_VALUES,
            (mvp.GLYPH_BITMAP_WIDTH, mvp.GLYPH_BITMAP_HEIGHT, mvp.BITMAP_STORAGE_GLYPH_MONO),
        )
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
                "KernelInstaller",
                "Object",
                "Workspace",
                "True",
                "False",
                "BlockClosure",
                "Context",
            ],
        )
        self.assertEqual(
            [header.class_name for header in mvp.KERNEL_CLASS_HEADERS_IN_DESCRIPTOR_ORDER],
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
            144,
            {mvp.SEED_OBJECT_TRANSCRIPT: 208},
            {mvp.SEED_OBJECT_TRANSCRIPT: 2},
        )
        self.assertEqual(class_indices[mvp.SEED_OBJECT_TRANSCRIPT], 145)
        self.assertEqual(class_indices[mvp.SEED_OBJECT_COMPILED_METHOD], 160)
        self.assertEqual(class_indices[mvp.KERNEL_CLASS_NAME_TO_OBJECT_KIND["Workspace"]], 163)
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
            mvp.BOOT_IMAGE_SPEC,
            mvp.BootImageSpec(
                fixed_boot_graph_spec=mvp.FIXED_BOOT_GRAPH_SPEC,
                ordering_spec=mvp.BOOT_IMAGE_ORDERING_SPEC,
                dynamic_seed_section_specs=tuple(mvp.DYNAMIC_SEED_SECTION_SPECS),
            ),
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_ORDERING_SPEC,
            mvp.BootImageOrderingSpec(
                class_kind_order=tuple(mvp.CLASS_DESCRIPTOR_KIND_ORDER),
                selector_value_order=tuple(mvp.SELECTOR_VALUE_ORDER),
                compiled_method_entry_order=tuple(mvp.COMPILED_METHOD_ENTRY_ORDER),
                method_entry_order=tuple(mvp.METHOD_ENTRY_ORDER),
            ),
        )
        self.assertEqual(
            mvp.DYNAMIC_SEED_SECTION_SPECS,
            [
                mvp.DynamicSeedSectionSpec(
                    "class_descriptors",
                    "class_kind_order",
                    mvp.build_class_seed_section,
                    4,
                    ("method_descriptors",),
                    (
                        "class_kind_order",
                        "class_class_index",
                        "class_indices",
                        "method_start_by_kind",
                        "method_count_by_kind",
                    ),
                ),
                mvp.DynamicSeedSectionSpec(
                    "selectors",
                    "selector_value_order",
                    mvp.build_selector_seed_section,
                    0,
                    required_state_fields=("seed_layout", "class_indices", "selector_value_order"),
                    state_update_fields=("selector_indices_by_value",),
                ),
                mvp.DynamicSeedSectionSpec(
                    "compiled_methods",
                    "compiled_method_entry_order",
                    mvp.build_compiled_method_seed_section,
                    1,
                    required_state_fields=("seed_layout", "class_indices", "compiled_method_entry_order"),
                    state_update_fields=("compiled_method_indices",),
                ),
                mvp.DynamicSeedSectionSpec(
                    "method_entries",
                    "method_entry_order",
                    mvp.build_method_entry_seed_section,
                    2,
                    ("compiled_methods",),
                    ("seed_layout", "class_indices", "compiled_method_indices", "method_entry_order"),
                    ("method_entry_indices",),
                ),
                mvp.DynamicSeedSectionSpec(
                    "method_descriptors",
                    "builtin_method_definitions",
                    mvp.build_method_descriptor_seed_section,
                    3,
                    ("selectors", "method_entries"),
                    (
                        "seed_layout",
                        "class_kind_order",
                        "class_indices",
                        "selector_indices_by_value",
                        "method_entry_indices",
                    ),
                    ("method_start_by_kind", "method_count_by_kind"),
                ),
            ],
        )
        self.assertEqual(
            [spec.build_order for spec in mvp.DYNAMIC_SEED_SECTION_SPECS],
            [4, 0, 1, 2, 3],
        )
        self.assertEqual(
            mvp.SEED_LAYOUT_SECTION_SPECS,
            [
                mvp.SeedLayoutSectionSpec("class_descriptors", "class_kind_order"),
                mvp.SeedLayoutSectionSpec("selectors", "selector_value_order"),
                mvp.SeedLayoutSectionSpec("compiled_methods", "compiled_method_entry_order"),
                mvp.SeedLayoutSectionSpec("method_entries", "method_entry_order"),
                mvp.SeedLayoutSectionSpec("method_descriptors", "builtin_method_definitions"),
            ],
        )
        self.assertEqual(
            mvp.DYNAMIC_SEED_OBJECT_SECTION_SPECS,
            [
                mvp.DynamicSeedObjectSectionSpec("class_descriptors"),
                mvp.DynamicSeedObjectSectionSpec("selectors"),
                mvp.DynamicSeedObjectSectionSpec("compiled_methods"),
                mvp.DynamicSeedObjectSectionSpec("method_entries"),
                mvp.DynamicSeedObjectSectionSpec("method_descriptors"),
            ],
        )
        self.assertEqual(
            mvp.DYNAMIC_SEED_BUILD_STEP_SPECS,
            [
                mvp.DynamicSeedBuildStepSpec(
                    "selectors",
                    mvp.build_selector_seed_section,
                    required_state_fields=("seed_layout", "class_indices", "selector_value_order"),
                    state_update_fields=("selector_indices_by_value",),
                ),
                mvp.DynamicSeedBuildStepSpec(
                    "compiled_methods",
                    mvp.build_compiled_method_seed_section,
                    required_state_fields=("seed_layout", "class_indices", "compiled_method_entry_order"),
                    state_update_fields=("compiled_method_indices",),
                ),
                mvp.DynamicSeedBuildStepSpec(
                    "method_entries",
                    mvp.build_method_entry_seed_section,
                    ("compiled_methods",),
                    ("seed_layout", "class_indices", "compiled_method_indices", "method_entry_order"),
                    ("method_entry_indices",),
                ),
                mvp.DynamicSeedBuildStepSpec(
                    "method_descriptors",
                    mvp.build_method_descriptor_seed_section,
                    ("selectors", "method_entries"),
                    (
                        "seed_layout",
                        "class_kind_order",
                        "class_indices",
                        "selector_indices_by_value",
                        "method_entry_indices",
                    ),
                    ("method_start_by_kind", "method_count_by_kind"),
                ),
                mvp.DynamicSeedBuildStepSpec(
                    "class_descriptors",
                    mvp.build_class_seed_section,
                    ("method_descriptors",),
                    (
                        "class_kind_order",
                        "class_class_index",
                        "class_indices",
                        "method_start_by_kind",
                        "method_count_by_kind",
                    ),
                ),
            ],
        )
        self.assertEqual(
            [spec.layout_section_name for spec in mvp.DYNAMIC_SEED_BUILD_STEP_SPECS],
            [
                "selectors",
                "compiled_methods",
                "method_entries",
                "method_descriptors",
                "class_descriptors",
            ],
        )
        self.assertEqual(
            [spec.required_layout_sections for spec in mvp.DYNAMIC_SEED_BUILD_STEP_SPECS],
            [
                (),
                (),
                ("compiled_methods",),
                ("selectors", "method_entries"),
                ("method_descriptors",),
            ],
        )
        self.assertEqual(
            [spec.required_state_fields for spec in mvp.DYNAMIC_SEED_BUILD_STEP_SPECS],
            [
                ("seed_layout", "class_indices", "selector_value_order"),
                ("seed_layout", "class_indices", "compiled_method_entry_order"),
                ("seed_layout", "class_indices", "compiled_method_indices", "method_entry_order"),
                (
                    "seed_layout",
                    "class_kind_order",
                    "class_indices",
                    "selector_indices_by_value",
                    "method_entry_indices",
                ),
                (
                    "class_kind_order",
                    "class_class_index",
                    "class_indices",
                    "method_start_by_kind",
                    "method_count_by_kind",
                ),
            ],
        )
        self.assertEqual(
            [spec.state_update_fields for spec in mvp.DYNAMIC_SEED_BUILD_STEP_SPECS],
            [
                ("selector_indices_by_value",),
                ("compiled_method_indices",),
                ("method_entry_indices",),
                ("method_start_by_kind", "method_count_by_kind"),
                (),
            ],
        )
        self.assertEqual(
            mvp.INITIAL_DYNAMIC_SEED_STATE_FIELDS,
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.initial_dynamic_seed_state_fields,
        )
        self.assertEqual(
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.initial_dynamic_seed_state_fields,
            (
                "seed_layout",
                "class_kind_order",
                "class_class_index",
                "class_indices",
                "selector_value_order",
                "compiled_method_entry_order",
                "method_entry_order",
            ),
        )
        self.assertEqual(
            [spec.builder.__name__ for spec in mvp.DYNAMIC_SEED_BUILD_STEP_SPECS],
            [
                "build_selector_seed_section",
                "build_compiled_method_seed_section",
                "build_method_entry_seed_section",
                "build_method_descriptor_seed_section",
                "build_class_seed_section",
            ],
        )
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
        self.assertIsNone(mvp.validate_dynamic_seed_build_step_specs())
        layout = mvp.build_seed_layout(140, mvp.CLASS_DESCRIPTOR_KIND_ORDER)
        self.assertEqual(layout["class_descriptors"], mvp.SeedLayoutSection(140, 24))
        self.assertEqual(
            layout["selectors"],
            mvp.SeedLayoutSection(140 + len(mvp.CLASS_DESCRIPTOR_KIND_ORDER), len(mvp.SELECTOR_VALUE_ORDER)),
        )
        self.assertEqual(
            layout["compiled_methods"],
            mvp.SeedLayoutSection(
                140 + len(mvp.CLASS_DESCRIPTOR_KIND_ORDER) + len(mvp.SELECTOR_VALUE_ORDER),
                len(mvp.COMPILED_METHOD_ENTRY_ORDER),
            ),
        )
        self.assertEqual(
            layout["method_entries"],
            mvp.SeedLayoutSection(
                140
                + len(mvp.CLASS_DESCRIPTOR_KIND_ORDER)
                + len(mvp.SELECTOR_VALUE_ORDER)
                + len(mvp.COMPILED_METHOD_ENTRY_ORDER),
                len(mvp.METHOD_ENTRY_ORDER),
            ),
        )
        self.assertEqual(
            layout["method_descriptors"],
            mvp.SeedLayoutSection(
                140
                + len(mvp.CLASS_DESCRIPTOR_KIND_ORDER)
                + len(mvp.SELECTOR_VALUE_ORDER)
                + len(mvp.COMPILED_METHOD_ENTRY_ORDER)
                + len(mvp.METHOD_ENTRY_ORDER),
                len(mvp.METHOD_ENTRY_ORDER),
            ),
        )

    def test_builds_dynamic_seed_sections_from_fixed_boot_graph(self) -> None:
        seed_objects, _seed_object_indices_by_name, _glyph_object_indices = mvp.build_fixed_boot_seed_objects()
        dynamic_sections = mvp.build_dynamic_seed_sections(seed_objects)

        self.assertEqual(dynamic_sections.seed_layout["class_descriptors"], mvp.SeedLayoutSection(144, 24))
        self.assertEqual(dynamic_sections.class_indices[mvp.SEED_OBJECT_TRANSCRIPT], 145)
        self.assertEqual(dynamic_sections.class_indices[mvp.SEED_OBJECT_COMPILED_METHOD], 160)
        self.assertEqual(dynamic_sections.class_indices[mvp.SEED_OBJECT_KERNEL_INSTALLER], 161)
        self.assertEqual(dynamic_sections.class_indices[mvp.KERNEL_CLASS_NAME_TO_OBJECT_KIND["Workspace"]], 163)
        self.assertEqual(seed_objects[0].class_index, 145)
        self.assertEqual(
            sorted(dynamic_sections.object_sections),
            [
                "class_descriptors",
                "compiled_methods",
                "method_descriptors",
                "method_entries",
                "selectors",
            ],
        )
        self.assertEqual(len(dynamic_sections.class_seed_objects), 24)
        self.assertEqual(len(dynamic_sections.selector_seed_objects), len(mvp.SELECTOR_VALUE_ORDER))
        self.assertEqual(len(dynamic_sections.compiled_method_seed_objects), 16)
        self.assertEqual(len(dynamic_sections.method_entry_seed_objects), len(mvp.METHOD_ENTRY_ORDER))
        self.assertEqual(len(dynamic_sections.method_seed_objects), len(mvp.METHOD_ENTRY_ORDER))
        self.assertEqual(
            dynamic_sections.class_seed_objects[0].fields,
            [
                (mvp.SEED_FIELD_NIL, 0),
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SEED_OBJECT_CLASS),
                (mvp.SEED_FIELD_OBJECT_INDEX, 324),
                (mvp.SEED_FIELD_SMALL_INTEGER, 2),
            ],
        )
        self.assertIsNone(
            mvp.validate_dynamic_seed_section_counts(dynamic_sections.seed_layout, dynamic_sections)
        )

    def test_builds_seed_bindings_and_encodes_seed_manifest(self) -> None:
        seed_objects, seed_object_indices_by_name, glyph_object_indices = mvp.build_fixed_boot_seed_objects()
        dynamic_sections = mvp.build_dynamic_seed_sections(seed_objects)
        for section_spec in mvp.DYNAMIC_SEED_OBJECT_SECTION_SPECS:
            seed_objects.extend(dynamic_sections.seed_objects_for_layout_section(section_spec.layout_section_name))

        bindings = mvp.build_seed_bindings(seed_object_indices_by_name)
        self.assertEqual(bindings.global_bindings[0], (mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRANSCRIPT"], 0))
        self.assertEqual(bindings.root_bindings[0], (mvp.SEED_ROOT_VALUES["RECORZ_MVP_SEED_ROOT_DEFAULT_FORM"], 9))

        manifest = mvp.encode_seed_manifest(seed_objects, bindings, glyph_object_indices)
        self.assertEqual(
            struct.unpack_from(mvp.SEED_HEADER_FORMAT, manifest, 0),
            (mvp.SEED_MAGIC, mvp.SEED_VERSION, len(seed_objects), 10, 6, 128, 0),
        )

    def test_declares_method_seed_orders_and_materializes_method_seed_objects(self) -> None:
        self.assertEqual(
            mvp.SELECTOR_VALUE_ORDER,
            list(range(1, 77)),
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
        class_indices = mvp.build_class_index_map(mvp.CLASS_DESCRIPTOR_KIND_ORDER, 141)
        selector_indices, selector_seed_objects = mvp.build_selector_seed_objects(
            162,
            class_indices[mvp.SEED_OBJECT_SELECTOR],
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.selector_value_order,
        )
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SHOW"]], 162)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_INSTANCE_KIND"]], 183)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_FILE_IN_CLASS_CHUNKS"]], 188)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_NEW"]], 189)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_CLASS_NAMED"]], 190)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_VALUE"]], 191)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SET_VALUE"]], 192)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SAVE_SNAPSHOT"]], 197)
        self.assertEqual(
            selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_CONFIGURE_STARTUP_SELECTOR_NAMED"]],
            198,
        )
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_CLEAR_STARTUP"]], 199)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_FILE_IN"]], 200)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_CLASS_NAMED"]], 201)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_CLASSES"]], 202)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_OBJECT_NAMED"]], 203)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_REOPEN"]], 204)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_EVALUATE"]], 205)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_RERUN"]], 206)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_CONTENTS"]], 207)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SET_CONTENTS"]], 208)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_EVALUATE_CURRENT"]], 209)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_FILE_IN_CURRENT"]], 210)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_METHODS_FOR_CLASS_NAMED"]], 211)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_METHOD_OF_CLASS_NAMED"]], 212)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_CLASS_METHODS_FOR_CLASS_NAMED"]], 213)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_CLASS_METHOD_OF_CLASS_NAMED"]], 214)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_FILE_OUT_CLASS_NAMED"]], 215)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_MEMORY_REPORT"]], 216)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_PROTOCOLS_FOR_CLASS_NAMED"]], 217)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_PROTOCOL_OF_CLASS_NAMED"]], 218)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_CLASS_PROTOCOLS_FOR_CLASS_NAMED"]], 219)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_CLASS_PROTOCOL_OF_CLASS_NAMED"]], 220)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_FILE_OUT_PACKAGE_NAMED"]], 221)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_PACKAGES"]], 222)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_BROWSE_PACKAGE_NAMED"]], 223)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SUM"]], 224)
        self.assertEqual(selector_indices[mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SET_LEFT_RIGHT"]], 225)
        self.assertEqual(
            selector_seed_objects[0].fields,
            [(mvp.SEED_FIELD_SMALL_INTEGER, mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SHOW"])],
        )
        compiled_method_indices, compiled_method_seed_objects = mvp.build_compiled_method_seed_objects(
            224,
            class_indices[mvp.SEED_OBJECT_COMPILED_METHOD],
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.compiled_method_entry_order,
        )
        self.assertEqual(compiled_method_indices["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"], 224)
        self.assertEqual(compiled_method_indices["RECORZ_MVP_METHOD_ENTRY_CLASS_INSTANCE_KIND"], 235)
        self.assertEqual(
            compiled_method_seed_objects[0].fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, instruction)
                for instruction in mvp.COMPILED_METHOD_PROGRAM_BY_ENTRY_NAME["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"]
            ],
        )
        method_entry_indices, method_entry_seed_objects = mvp.build_method_entry_seed_objects(
            236,
            class_indices[mvp.SEED_OBJECT_METHOD_ENTRY],
            compiled_method_indices,
            mvp.BOOT_IMAGE_SEED_BUILD_CONTEXT.method_entry_order,
        )
        self.assertEqual(
            method_entry_seed_objects[0].fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.METHOD_ENTRY_VALUES["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"]),
                (mvp.SEED_FIELD_OBJECT_INDEX, 224),
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
            296,
        )
        self.assertEqual(method_start_by_kind[mvp.SEED_OBJECT_CLASS], 296)
        self.assertEqual(method_count_by_kind[mvp.SEED_OBJECT_FORM], 6)
        self.assertEqual(
            method_seed_objects[0].fields,
            [
                (mvp.SEED_FIELD_OBJECT_INDEX, 183),
                (mvp.SEED_FIELD_SMALL_INTEGER, 0),
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SEED_OBJECT_CLASS),
                (mvp.SEED_FIELD_OBJECT_INDEX, 257),
            ],
        )

    def test_renders_generated_runtime_bindings_header(self) -> None:
        header = mvp.render_generated_runtime_bindings_header()

        self.assertIn("#ifndef RECORZ_QEMU_RISCV64_GENERATED_RUNTIME_BINDINGS_H", header)
        self.assertIn("#define RECORZ_MVP_PROGRAM_MAGIC_0 'R'", header)
        self.assertIn("#define RECORZ_MVP_IMAGE_SECTION_PROGRAM 1U", header)
        self.assertIn("#define RECORZ_MVP_IMAGE_ENTRY_SIZE 16U", header)
        self.assertIn("#define RECORZ_MVP_SEED_VERSION 16U", header)
        self.assertIn("#define RECORZ_MVP_SEED_INVALID_OBJECT_INDEX 65535U", header)
        self.assertIn("#define RECORZ_MVP_COMPILED_METHOD_MAX_INSTRUCTIONS 4U", header)
        self.assertIn("#define RECORZ_MVP_METHOD_UPDATE_MAGIC_0 'R'", header)
        self.assertIn("#define RECORZ_MVP_METHOD_UPDATE_VERSION 1U", header)
        self.assertIn("#define RECORZ_MVP_METHOD_UPDATE_HEADER_SIZE 16U", header)
        self.assertIn('#define RECORZ_MVP_METHOD_UPDATE_FW_CFG_NAME "opt/recorz-method-update"', header)
        self.assertIn("enum recorz_mvp_compiled_method_opcode {", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_PUSH_LITERAL = 2,", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_RETURN_RECEIVER = 13,", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_STORE_FIELD = 14,", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_PUSH_SELF = 15,", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_PUSH_SMALL_INTEGER = 16,", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_PUSH_STRING_LITERAL = 17,", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_JUMP = 18,", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_JUMP_IF_TRUE = 19,", header)
        self.assertIn("RECORZ_MVP_COMPILED_METHOD_OP_JUMP_IF_FALSE = 20,", header)
        self.assertIn("enum recorz_mvp_method_implementation {", header)
        self.assertIn("RECORZ_MVP_METHOD_IMPLEMENTATION_COMPILED = 7,", header)
        self.assertIn("#define RECORZ_MVP_TEXT_LAYOUT_FIELD_SCALE 2U", header)
        self.assertIn("#define RECORZ_MVP_TEXT_STYLE_FIELD_FOREGROUND_COLOR 0U", header)
        self.assertIn("#define RECORZ_MVP_SELECTOR_FIELD_VALUE 0U", header)
        self.assertIn("#define RECORZ_MVP_BITMAP_STORAGE_FRAMEBUFFER 1U", header)
        self.assertIn("enum recorz_mvp_opcode {", header)
        self.assertIn("RECORZ_MVP_OP_STORE_LEXICAL = 9,", header)
        self.assertIn("RECORZ_MVP_OP_PUSH_FIELD = 12,", header)
        self.assertIn("RECORZ_MVP_OP_STORE_FIELD = 14,", header)
        self.assertIn("RECORZ_MVP_OP_PUSH_SELF = 15,", header)
        self.assertIn("RECORZ_MVP_OP_PUSH_SMALL_INTEGER = 16,", header)
        self.assertIn("RECORZ_MVP_OP_PUSH_STRING_LITERAL = 17,", header)
        self.assertIn("RECORZ_MVP_OP_JUMP = 18,", header)
        self.assertIn("RECORZ_MVP_OP_JUMP_IF_TRUE = 19,", header)
        self.assertIn("RECORZ_MVP_OP_JUMP_IF_FALSE = 20,", header)
        self.assertIn("enum recorz_mvp_global {", header)
        self.assertIn("RECORZ_MVP_GLOBAL_BITMAP = 6,", header)
        self.assertIn("RECORZ_MVP_GLOBAL_KERNEL_INSTALLER = 7,", header)
        self.assertIn("RECORZ_MVP_GLOBAL_WORKSPACE = 8,", header)
        self.assertIn("RECORZ_MVP_GLOBAL_TRUE = 9,", header)
        self.assertIn("RECORZ_MVP_GLOBAL_FALSE = 10,", header)
        self.assertIn("enum recorz_mvp_selector {", header)
        self.assertIn("RECORZ_MVP_SELECTOR_INSTANCE_KIND = 22,", header)
        self.assertIn(
            "RECORZ_MVP_SELECTOR_INSTALL_COMPILED_METHOD_ON_CLASS_SELECTOR_ID_ARGUMENT_COUNT = 24,",
            header,
        )
        self.assertIn(
            "RECORZ_MVP_SELECTOR_INSTALL_METHOD_SOURCE_ON_CLASS = 25,",
            header,
        )
        self.assertIn(
            "RECORZ_MVP_SELECTOR_FILE_IN_CLASS_CHUNKS = 27,",
            header,
        )
        self.assertIn("RECORZ_MVP_SELECTOR_NEW = 28,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_CLASS_NAMED = 29,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_VALUE = 30,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_SET_VALUE = 31,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_FILE_IN = 39,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_CLASS_NAMED = 40,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_CLASSES = 41,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_OBJECT_NAMED = 42,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_REOPEN = 43,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_EVALUATE = 44,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_RERUN = 45,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_CONTENTS = 46,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_SET_CONTENTS = 47,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_EVALUATE_CURRENT = 48,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_FILE_IN_CURRENT = 49,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_METHODS_FOR_CLASS_NAMED = 50,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_METHOD_OF_CLASS_NAMED = 51,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_CLASS_METHODS_FOR_CLASS_NAMED = 52,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_CLASS_METHOD_OF_CLASS_NAMED = 53,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_FILE_OUT_CLASS_NAMED = 54,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_MEMORY_REPORT = 55,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_PROTOCOLS_FOR_CLASS_NAMED = 56,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_PROTOCOL_OF_CLASS_NAMED = 57,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_CLASS_PROTOCOLS_FOR_CLASS_NAMED = 58,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_CLASS_PROTOCOL_OF_CLASS_NAMED = 59,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_FILE_OUT_PACKAGE_NAMED = 60,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_PACKAGES = 61,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_BROWSE_PACKAGE_NAMED = 62,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_SUM = 63,", header)
        self.assertIn("RECORZ_MVP_SELECTOR_SET_LEFT_RIGHT = 64,", header)
        self.assertIn("enum recorz_mvp_literal_kind {", header)
        self.assertIn("RECORZ_MVP_LITERAL_SMALL_INTEGER = 2,", header)
        self.assertIn("enum recorz_mvp_object_kind {", header)
        self.assertIn("RECORZ_MVP_OBJECT_COMPILED_METHOD = 17,", header)
        self.assertIn("RECORZ_MVP_OBJECT_KERNEL_INSTALLER = 18,", header)
        self.assertIn("RECORZ_MVP_OBJECT_OBJECT = 19,", header)
        self.assertIn("RECORZ_MVP_OBJECT_WORKSPACE = 20,", header)
        self.assertIn("enum recorz_mvp_seed_field_kind {", header)
        self.assertIn("RECORZ_MVP_SEED_FIELD_OBJECT_INDEX = 2,", header)
        self.assertIn("enum recorz_mvp_seed_root {", header)
        self.assertIn("RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS = 6,", header)
        self.assertIn("enum recorz_mvp_method_entry {", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW = 1,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_CLASS_INSTANCE_KIND = 22,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_CLASS_NEW = 23,", header)
        self.assertIn(
            "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_INSTALL_COMPILED_METHOD_ON_CLASS_SELECTOR_ID_ARGUMENT_COUNT = 25,",
            header,
        )
        self.assertIn(
            "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_INSTALL_METHOD_SOURCE_ON_CLASS = 26,",
            header,
        )
        self.assertIn(
            "RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_FILE_IN_CLASS_CHUNKS = 28,",
            header,
        )
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_FILE_OUT_CLASS_NAMED = 29,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_FILE_OUT_PACKAGE_NAMED = 30,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_MEMORY_REPORT = 31,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_CLASS_NAMED = 32,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_REMEMBER_OBJECT_NAMED = 33,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_OBJECT_NAMED = 34,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_SAVE_SNAPSHOT = 35,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_CONFIGURE_STARTUP_SELECTOR_NAMED = 36,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_KERNEL_INSTALLER_CLEAR_STARTUP = 37,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_FILE_IN = 38,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_CONTENTS = 39,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_SET_CONTENTS = 40,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_EVALUATE = 41,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_EVALUATE_CURRENT = 42,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_FILE_IN_CURRENT = 43,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASSES = 44,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_PACKAGES = 45,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_METHODS_FOR_CLASS_NAMED = 46,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_PACKAGE_NAMED = 47,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_PROTOCOLS_FOR_CLASS_NAMED = 48,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_PROTOCOL_OF_CLASS_NAMED = 49,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_OBJECT_NAMED = 50,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_NAMED = 51,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_METHOD_OF_CLASS_NAMED = 52,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_METHODS_FOR_CLASS_NAMED = 53,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_PROTOCOLS_FOR_CLASS_NAMED = 54,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_PROTOCOL_OF_CLASS_NAMED = 55,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_BROWSE_CLASS_METHOD_OF_CLASS_NAMED = 56,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_FILE_OUT_CLASS_NAMED = 57,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_FILE_OUT_PACKAGE_NAMED = 58,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_RERUN = 59,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_WORKSPACE_REOPEN = 60,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_CONTEXT_SENDER = 61,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_CONTEXT_RECEIVER = 62,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_CONTEXT_DETAIL = 63,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_CONTEXT_ALIVE = 64,", header)
        self.assertIn("RECORZ_MVP_METHOD_ENTRY_COUNT = 65,", header)
        self.assertNotIn("RECORZ_MVP_GENERATED_METHOD_ENTRY_SPECS", header)
        self.assertIn("enum recorz_mvp_primitive_binding {", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_BITBLT_FILL_FORM_COLOR = 1,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_FORM_NEWLINE = 10,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_CLASS_NEW = 11,", header)
        self.assertIn(
            "RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_INSTALL_COMPILED_METHOD_ON_CLASS_SELECTOR_ID_ARGUMENT_COUNT = 13,",
            header,
        )
        self.assertIn(
            "RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_INSTALL_METHOD_SOURCE_ON_CLASS = 14,",
            header,
        )
        self.assertIn(
            "RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_FILE_IN_CLASS_CHUNKS = 16,",
            header,
        )
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_FILE_OUT_CLASS_NAMED = 17,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_FILE_OUT_PACKAGE_NAMED = 18,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_MEMORY_REPORT = 19,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_CLASS_NAMED = 20,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_REMEMBER_OBJECT_NAMED = 21,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_OBJECT_NAMED = 22,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_SAVE_SNAPSHOT = 23,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_CONFIGURE_STARTUP_SELECTOR_NAMED = 24,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_CLEAR_STARTUP = 25,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_FILE_IN = 26,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_CONTENTS = 27,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_SET_CONTENTS = 28,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_EVALUATE = 29,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_EVALUATE_CURRENT = 30,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_FILE_IN_CURRENT = 31,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASSES = 32,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_PACKAGES = 33,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_METHODS_FOR_CLASS_NAMED = 34,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_PACKAGE_NAMED = 35,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_PROTOCOLS_FOR_CLASS_NAMED = 36,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_PROTOCOL_OF_CLASS_NAMED = 37,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_OBJECT_NAMED = 38,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_NAMED = 39,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_METHOD_OF_CLASS_NAMED = 40,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_METHODS_FOR_CLASS_NAMED = 41,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_PROTOCOLS_FOR_CLASS_NAMED = 42,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_PROTOCOL_OF_CLASS_NAMED = 43,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_METHOD_OF_CLASS_NAMED = 44,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_FILE_OUT_CLASS_NAMED = 45,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_FILE_OUT_PACKAGE_NAMED = 46,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_RERUN = 47,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_WORKSPACE_REOPEN = 48,", header)
        self.assertIn("RECORZ_MVP_PRIMITIVE_COUNT = 49,", header)
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_MEMORY_REPORT] = "
            "execute_entry_kernel_installer_memory_report,",
            header,
        )
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
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_CLASS_NEW] = execute_entry_class_new,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_CLASS_NAMED] = execute_entry_kernel_installer_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_FILE_OUT_CLASS_NAMED] = "
            "execute_entry_kernel_installer_file_out_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_FILE_OUT_PACKAGE_NAMED] = "
            "execute_entry_kernel_installer_file_out_package_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_REMEMBER_OBJECT_NAMED] = execute_entry_kernel_installer_remember_object_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_OBJECT_NAMED] = execute_entry_kernel_installer_object_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_SAVE_SNAPSHOT] = execute_entry_kernel_installer_save_snapshot,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_CONFIGURE_STARTUP_SELECTOR_NAMED] = "
            "execute_entry_kernel_installer_configure_startup_selector_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_KERNEL_INSTALLER_CLEAR_STARTUP] = execute_entry_kernel_installer_clear_startup,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_FILE_IN] = execute_entry_workspace_file_in,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_CONTENTS] = execute_entry_workspace_contents,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_SET_CONTENTS] = execute_entry_workspace_set_contents,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_EVALUATE] = execute_entry_workspace_evaluate,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_EVALUATE_CURRENT] = execute_entry_workspace_evaluate_current,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_FILE_IN_CURRENT] = execute_entry_workspace_file_in_current,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASSES] = execute_entry_workspace_browse_classes,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_PACKAGES] = execute_entry_workspace_browse_packages,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_METHODS_FOR_CLASS_NAMED] = "
            "execute_entry_workspace_browse_methods_for_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_PACKAGE_NAMED] = "
            "execute_entry_workspace_browse_package_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_PROTOCOLS_FOR_CLASS_NAMED] = "
            "execute_entry_workspace_browse_protocols_for_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_PROTOCOL_OF_CLASS_NAMED] = "
            "execute_entry_workspace_browse_protocol_of_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_OBJECT_NAMED] = execute_entry_workspace_browse_object_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_NAMED] = execute_entry_workspace_browse_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_METHOD_OF_CLASS_NAMED] = "
            "execute_entry_workspace_browse_method_of_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_METHODS_FOR_CLASS_NAMED] = "
            "execute_entry_workspace_browse_class_methods_for_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_PROTOCOLS_FOR_CLASS_NAMED] = "
            "execute_entry_workspace_browse_class_protocols_for_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_PROTOCOL_OF_CLASS_NAMED] = "
            "execute_entry_workspace_browse_class_protocol_of_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_BROWSE_CLASS_METHOD_OF_CLASS_NAMED] = "
            "execute_entry_workspace_browse_class_method_of_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_FILE_OUT_CLASS_NAMED] = "
            "execute_entry_workspace_file_out_class_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_FILE_OUT_PACKAGE_NAMED] = "
            "execute_entry_workspace_file_out_package_named,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_RERUN] = execute_entry_workspace_rerun,",
            header,
        )
        self.assertIn(
            "[RECORZ_MVP_PRIMITIVE_WORKSPACE_REOPEN] = execute_entry_workspace_reopen,",
            header,
        )

    def test_builds_compiled_method_update_manifest(self) -> None:
        manifest = mvp.build_method_update_manifest("Transcript", "cr\n    ^self")
        header_size = struct.calcsize(mvp.METHOD_UPDATE_HEADER_FORMAT)
        magic, version, class_kind, selector, argument_count, instruction_count, reserved = struct.unpack(
            mvp.METHOD_UPDATE_HEADER_FORMAT,
            manifest[:header_size],
        )

        self.assertEqual(magic, mvp.METHOD_UPDATE_MAGIC)
        self.assertEqual(version, mvp.METHOD_UPDATE_VERSION)
        self.assertEqual(class_kind, mvp.SEED_OBJECT_TRANSCRIPT)
        self.assertEqual(selector, mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_CR"])
        self.assertEqual(argument_count, 0)
        self.assertEqual(instruction_count, 1)
        self.assertEqual(reserved, 0)
        self.assertEqual(
            struct.unpack("<I", manifest[header_size:header_size + 4]),
            (mvp.encode_compiled_method_instruction("return_receiver"),),
        )

    def test_rejects_primitive_method_update_manifest(self) -> None:
        with self.assertRaises(mvp.LoweringError):
            mvp.build_method_update_manifest("Form", "clear\n    <primitive: #formClear>")

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
        seed_layout = mvp.build_seed_layout(mvp.BOOT_OBJECT_FIXED_COUNT, mvp.CLASS_DESCRIPTOR_KIND_ORDER)
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
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (seed_layout["class_descriptors"].start_index * object_size),
        )
        first_selector_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (seed_layout["selectors"].start_index * object_size),
        )
        first_selector_fields = [
            struct.unpack_from(
                mvp.SEED_FIELD_FORMAT,
                manifest,
                struct.calcsize(mvp.SEED_HEADER_FORMAT)
                + (seed_layout["selectors"].start_index * object_size)
                + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
                + (field_index * struct.calcsize(mvp.SEED_FIELD_FORMAT)),
            )
            for field_index in range(1)
        ]
        first_compiled_method_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (seed_layout["compiled_methods"].start_index * object_size),
        )
        first_compiled_method_fields = [
            struct.unpack_from(
                mvp.SEED_FIELD_FORMAT,
                manifest,
                struct.calcsize(mvp.SEED_HEADER_FORMAT)
                + (seed_layout["compiled_methods"].start_index * object_size)
                + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
                + (field_index * struct.calcsize(mvp.SEED_FIELD_FORMAT)),
            )
            for field_index in range(4)
        ]
        first_method_entry_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (seed_layout["method_entries"].start_index * object_size),
        )
        first_method_entry_fields = [
            struct.unpack_from(
                mvp.SEED_FIELD_FORMAT,
                manifest,
                struct.calcsize(mvp.SEED_HEADER_FORMAT)
                + (seed_layout["method_entries"].start_index * object_size)
                + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
                + (field_index * struct.calcsize(mvp.SEED_FIELD_FORMAT)),
            )
            for field_index in range(2)
        ]
        first_method_header = struct.unpack_from(
            mvp.SEED_OBJECT_HEADER_FORMAT,
            manifest,
            struct.calcsize(mvp.SEED_HEADER_FORMAT) + (seed_layout["method_descriptors"].start_index * object_size),
        )
        first_method_fields = [
            struct.unpack_from(
                mvp.SEED_FIELD_FORMAT,
                manifest,
                struct.calcsize(mvp.SEED_HEADER_FORMAT)
                + (seed_layout["method_descriptors"].start_index * object_size)
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
        self.assertEqual(object_count, 388)
        self.assertEqual(global_binding_count, 10)
        self.assertEqual(root_binding_count, 6)
        self.assertEqual(glyph_code_count, len(mvp.GLYPH_BITMAP_BOOT_SPECS))
        self.assertEqual(reserved, 0)
        self.assertEqual(first_object_header, (mvp.SEED_OBJECT_TRANSCRIPT, 0, 145))
        self.assertEqual(class_class_header, (mvp.SEED_OBJECT_CLASS, 4, 144))
        self.assertEqual(first_selector_header, (mvp.SEED_OBJECT_SELECTOR, 1, 159))
        self.assertEqual(first_compiled_method_header, (mvp.SEED_OBJECT_COMPILED_METHOD, 4, 160))
        self.assertEqual(first_method_entry_header, (mvp.SEED_OBJECT_METHOD_ENTRY, 2, 158))
        self.assertEqual(first_method_header, (mvp.SEED_OBJECT_METHOD_DESCRIPTOR, 4, 157))
        self.assertEqual(
            first_selector_fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SHOW"]),
            ],
        )
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
        self.assertEqual(
            first_method_entry_fields,
            [
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.METHOD_ENTRY_VALUES["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"]),
                (mvp.SEED_FIELD_OBJECT_INDEX, 244),
            ],
        )
        self.assertEqual(
            first_method_fields,
            [
                (mvp.SEED_FIELD_OBJECT_INDEX, 189),
                (mvp.SEED_FIELD_SMALL_INTEGER, 0),
                (mvp.SEED_FIELD_SMALL_INTEGER, mvp.SEED_OBJECT_CLASS),
                (mvp.SEED_FIELD_OBJECT_INDEX, 281),
            ],
        )
        self.assertEqual(first_global_binding, (mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRANSCRIPT"], 0))
        self.assertEqual(first_root_binding, (mvp.SEED_ROOT_DEFAULT_FORM, 9))
        self.assertEqual(first_glyph_object_index, 14)

    def test_rejects_unsupported_globals(self) -> None:
        with self.assertRaises(mvp.LoweringError):
            mvp.build_program("Object new")

    def test_rejects_unsupported_compiler_features(self) -> None:
        with self.assertRaises(mvp.LoweringError):
            mvp.build_program("#unsupported")


if __name__ == "__main__":
    unittest.main()
