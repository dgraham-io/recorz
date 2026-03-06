#!/usr/bin/env python3
"""Lower a tiny compiled Recorz do-it into the MVP RISC-V demo assets."""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / "src") not in sys.path:
    sys.path.insert(0, str(ROOT / "src"))

from recorz.compiler import compile_do_it  # noqa: E402
from recorz.model import BindingRef, SendSite, SymbolAtom  # noqa: E402


OP_PUSH_GLOBAL = "RECORZ_MVP_OP_PUSH_GLOBAL"
OP_PUSH_LITERAL = "RECORZ_MVP_OP_PUSH_LITERAL"
OP_PUSH_LEXICAL = "RECORZ_MVP_OP_PUSH_LEXICAL"
OP_STORE_LEXICAL = "RECORZ_MVP_OP_STORE_LEXICAL"
OP_SEND = "RECORZ_MVP_OP_SEND"
OP_DUP = "RECORZ_MVP_OP_DUP"
OP_POP = "RECORZ_MVP_OP_POP"
OP_RETURN = "RECORZ_MVP_OP_RETURN"
OP_PUSH_NIL = "RECORZ_MVP_OP_PUSH_NIL"

LITERAL_STRING = "RECORZ_MVP_LITERAL_STRING"
LITERAL_SMALL_INTEGER = "RECORZ_MVP_LITERAL_SMALL_INTEGER"

GLOBAL_IDS = {
    "Transcript": "RECORZ_MVP_GLOBAL_TRANSCRIPT",
    "Display": "RECORZ_MVP_GLOBAL_DISPLAY",
    "BitBlt": "RECORZ_MVP_GLOBAL_BITBLT",
    "Glyphs": "RECORZ_MVP_GLOBAL_GLYPHS",
    "Form": "RECORZ_MVP_GLOBAL_FORM",
    "Bitmap": "RECORZ_MVP_GLOBAL_BITMAP",
}
SELECTOR_IDS = {
    "show:": "RECORZ_MVP_SELECTOR_SHOW",
    "cr": "RECORZ_MVP_SELECTOR_CR",
    "writeString:": "RECORZ_MVP_SELECTOR_WRITE_STRING",
    "newline": "RECORZ_MVP_SELECTOR_NEWLINE",
    "defaultForm": "RECORZ_MVP_SELECTOR_DEFAULT_FORM",
    "clear": "RECORZ_MVP_SELECTOR_CLEAR",
    "width": "RECORZ_MVP_SELECTOR_WIDTH",
    "height": "RECORZ_MVP_SELECTOR_HEIGHT",
    "bits": "RECORZ_MVP_SELECTOR_BITS",
    "+": "RECORZ_MVP_SELECTOR_ADD",
    "-": "RECORZ_MVP_SELECTOR_SUBTRACT",
    "*": "RECORZ_MVP_SELECTOR_MULTIPLY",
    "printString": "RECORZ_MVP_SELECTOR_PRINT_STRING",
    "fillForm:color:": "RECORZ_MVP_SELECTOR_FILL_FORM_COLOR",
    "at:": "RECORZ_MVP_SELECTOR_AT",
    "copyBitmap:toForm:x:y:scale:": "RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE",
    "copyBitmap:toForm:x:y:scale:color:": "RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE_COLOR",
    "copyBitmap:sourceX:sourceY:width:height:toForm:x:y:scale:color:": "RECORZ_MVP_SELECTOR_COPY_BITMAP_SOURCE_X_SOURCE_Y_WIDTH_HEIGHT_TO_FORM_X_Y_SCALE_COLOR",
    "monoWidth:height:": "RECORZ_MVP_SELECTOR_MONO_WIDTH_HEIGHT",
    "fromBits:": "RECORZ_MVP_SELECTOR_FROM_BITS",
}
OPCODE_VALUES = {
    OP_PUSH_GLOBAL: 1,
    OP_PUSH_LITERAL: 2,
    OP_SEND: 3,
    OP_DUP: 4,
    OP_POP: 5,
    OP_RETURN: 6,
    OP_PUSH_NIL: 7,
    OP_PUSH_LEXICAL: 8,
    OP_STORE_LEXICAL: 9,
}
GLOBAL_VALUES = {
    "RECORZ_MVP_GLOBAL_TRANSCRIPT": 1,
    "RECORZ_MVP_GLOBAL_DISPLAY": 2,
    "RECORZ_MVP_GLOBAL_BITBLT": 3,
    "RECORZ_MVP_GLOBAL_GLYPHS": 4,
    "RECORZ_MVP_GLOBAL_FORM": 5,
    "RECORZ_MVP_GLOBAL_BITMAP": 6,
}
SELECTOR_VALUES = {
    "RECORZ_MVP_SELECTOR_SHOW": 1,
    "RECORZ_MVP_SELECTOR_CR": 2,
    "RECORZ_MVP_SELECTOR_WRITE_STRING": 3,
    "RECORZ_MVP_SELECTOR_NEWLINE": 4,
    "RECORZ_MVP_SELECTOR_DEFAULT_FORM": 5,
    "RECORZ_MVP_SELECTOR_CLEAR": 6,
    "RECORZ_MVP_SELECTOR_WIDTH": 7,
    "RECORZ_MVP_SELECTOR_HEIGHT": 8,
    "RECORZ_MVP_SELECTOR_ADD": 9,
    "RECORZ_MVP_SELECTOR_SUBTRACT": 10,
    "RECORZ_MVP_SELECTOR_MULTIPLY": 11,
    "RECORZ_MVP_SELECTOR_PRINT_STRING": 12,
    "RECORZ_MVP_SELECTOR_BITS": 13,
    "RECORZ_MVP_SELECTOR_FILL_FORM_COLOR": 14,
    "RECORZ_MVP_SELECTOR_AT": 15,
    "RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE": 16,
    "RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE_COLOR": 17,
    "RECORZ_MVP_SELECTOR_MONO_WIDTH_HEIGHT": 18,
    "RECORZ_MVP_SELECTOR_FROM_BITS": 19,
    "RECORZ_MVP_SELECTOR_COPY_BITMAP_SOURCE_X_SOURCE_Y_WIDTH_HEIGHT_TO_FORM_X_Y_SCALE_COLOR": 20,
}
LITERAL_VALUES = {
    LITERAL_STRING: 1,
    LITERAL_SMALL_INTEGER: 2,
}

PROGRAM_MAGIC = b"RCZP"
PROGRAM_VERSION = 1
PROGRAM_HEADER_FORMAT = "<4sHHHH"
PROGRAM_INSTRUCTION_FORMAT = "<BBH"
PROGRAM_LITERAL_HEADER_FORMAT = "<BBHi"

IMAGE_MAGIC = b"RCZI"
IMAGE_VERSION = 1
IMAGE_HEADER_FORMAT = "<4sHHII8s"
IMAGE_SECTION_FORMAT = "<HHII"
IMAGE_SECTION_PROGRAM = 1
IMAGE_SECTION_SEED = 2
IMAGE_SECTION_ENTRY = 3
IMAGE_FEATURE_FNV1A32 = 1
IMAGE_PROFILE = b"RV64MVP1"
IMAGE_ENTRY_MAGIC = b"RCZE"
IMAGE_ENTRY_VERSION = 1
IMAGE_ENTRY_FORMAT = "<4sHHHHHH"
IMAGE_ENTRY_KIND_DOIT = 1

SEED_MAGIC = b"RCZS"
SEED_VERSION = 3
SEED_HEADER_FORMAT = "<4sHHHHHH"
SEED_BINDING_FORMAT = "<HH"
SEED_OBJECT_HEADER_FORMAT = "<BBH"
SEED_FIELD_FORMAT = "<Bi"

SEED_FIELD_NIL = 0
SEED_FIELD_SMALL_INTEGER = 1
SEED_FIELD_OBJECT_INDEX = 2

SEED_OBJECT_TRANSCRIPT = 1
SEED_OBJECT_DISPLAY = 2
SEED_OBJECT_FORM = 3
SEED_OBJECT_BITMAP = 4
SEED_OBJECT_BITBLT = 5
SEED_OBJECT_GLYPHS = 6
SEED_OBJECT_FORM_FACTORY = 7
SEED_OBJECT_BITMAP_FACTORY = 8
SEED_OBJECT_TEXT_LAYOUT = 9
SEED_OBJECT_TEXT_STYLE = 10

SEED_ROOT_DEFAULT_FORM = 1
SEED_ROOT_FRAMEBUFFER_BITMAP = 2
SEED_ROOT_GLYPH_FALLBACK_BITMAP = 3
SEED_ROOT_TRANSCRIPT_LAYOUT = 4
SEED_ROOT_TRANSCRIPT_STYLE = 5

BITMAP_STORAGE_FRAMEBUFFER = 1
BITMAP_STORAGE_GLYPH_MONO = 2


class LoweringError(RuntimeError):
    """Raised when the demo lowerer sees unsupported source."""


@dataclass(frozen=True)
class Literal:
    kind: str
    value: object


@dataclass(frozen=True)
class Instruction:
    opcode: str
    operand_a: str = "0"
    operand_b: int = 0


@dataclass
class Program:
    literals: list[Literal]
    instructions: list[Instruction]
    lexical_count: int


class Lowerer:
    def __init__(self, source: str) -> None:
        self.source = source
        self.literals: list[Literal] = []
        self.instructions: list[Instruction] = []
        self.lexical_map: dict[str, int] = {}

    def build(self) -> Program:
        compiled = compile_do_it(self.source, "Object", [])
        self.lexical_map = {name: index for index, name in enumerate(compiled.temp_names)}
        for instruction in compiled.instructions:
            self.lower_instruction(instruction.opcode, instruction.operand, compiled.literals)
        return Program(
            literals=self.literals,
            instructions=self.instructions,
            lexical_count=len(compiled.temp_names),
        )

    def lower_instruction(self, opcode: str, operand: object, compiled_literals: list[object]) -> None:
        if opcode == "push_binding":
            binding = self.require_binding(operand)
            if binding.kind == "global":
                global_id = GLOBAL_IDS.get(binding.name)
                if global_id is None:
                    raise LoweringError(f"Unsupported global {binding.name!r} in MVP target")
                self.instructions.append(Instruction(OP_PUSH_GLOBAL, global_id))
                return
            if binding.kind == "lexical":
                self.instructions.append(Instruction(OP_PUSH_LEXICAL, operand_b=self.lexical_index(binding.name)))
                return
            raise LoweringError("The MVP target does not support instance-variable binding access")

        if opcode == "store_binding":
            binding = self.require_binding(operand)
            if binding.kind != "lexical":
                raise LoweringError("The MVP target only supports lexical assignment")
            self.instructions.append(Instruction(OP_STORE_LEXICAL, operand_b=self.lexical_index(binding.name)))
            return

        if opcode == "push_literal":
            literal = compiled_literals[int(operand)]
            self.instructions.append(Instruction(OP_PUSH_LITERAL, operand_b=self.literal_index(literal)))
            return

        if opcode == "push_nil":
            self.instructions.append(Instruction(OP_PUSH_NIL))
            return

        if opcode == "duplicate":
            self.instructions.append(Instruction(OP_DUP))
            return

        if opcode == "pop":
            self.instructions.append(Instruction(OP_POP))
            return

        if opcode == "send":
            send_site = self.require_send_site(operand)
            if send_site.super_send:
                raise LoweringError("The MVP target does not support super sends")
            selector_id = SELECTOR_IDS.get(send_site.selector)
            if selector_id is None:
                raise LoweringError(
                    f"Unsupported selector {send_site.selector!r}; the MVP target only supports display access, bitmap/form factories, BitBlt fill/copy, text, arithmetic, and printString"
                )
            self.instructions.append(Instruction(OP_SEND, selector_id, send_site.argument_count))
            return

        if opcode == "return_local":
            self.instructions.append(Instruction(OP_RETURN))
            return

        raise LoweringError(f"Unsupported compiler opcode {opcode!r} in MVP target")

    def literal_index(self, literal: object) -> int:
        normalized = self.normalize_literal(literal)
        if normalized not in self.literals:
            self.literals.append(normalized)
        return self.literals.index(normalized)

    def normalize_literal(self, literal: object) -> Literal:
        if isinstance(literal, str):
            return Literal(LITERAL_STRING, literal)
        if isinstance(literal, int):
            return Literal(LITERAL_SMALL_INTEGER, literal)
        if isinstance(literal, SymbolAtom):
            raise LoweringError("The MVP target does not support symbol literals")
        raise LoweringError(f"Unsupported literal {literal!r} in MVP target")

    def lexical_index(self, name: str) -> int:
        if name not in self.lexical_map:
            raise LoweringError(f"Unknown lexical binding {name!r} in compiled do-it")
        return self.lexical_map[name]

    @staticmethod
    def require_binding(operand: object) -> BindingRef:
        if not isinstance(operand, BindingRef):
            raise LoweringError(f"Expected binding operand, found {operand!r}")
        return operand

    @staticmethod
    def require_send_site(operand: object) -> SendSite:
        if not isinstance(operand, SendSite):
            raise LoweringError(f"Expected send site operand, found {operand!r}")
        return operand


def build_program(source: str) -> Program:
    return Lowerer(source).build()


def instruction_operand_a_value(instruction: Instruction) -> int:
    if instruction.operand_a == "0":
        return 0
    if instruction.opcode == OP_PUSH_GLOBAL:
        return GLOBAL_VALUES[instruction.operand_a]
    if instruction.opcode == OP_SEND:
        return SELECTOR_VALUES[instruction.operand_a]
    raise AssertionError(f"unexpected operand_a for {instruction.opcode}: {instruction.operand_a}")


def build_program_manifest(program: Program) -> bytes:
    manifest = bytearray(
        struct.pack(
            PROGRAM_HEADER_FORMAT,
            PROGRAM_MAGIC,
            PROGRAM_VERSION,
            len(program.instructions),
            len(program.literals),
            program.lexical_count,
        )
    )
    for instruction in program.instructions:
        manifest.extend(
            struct.pack(
                PROGRAM_INSTRUCTION_FORMAT,
                OPCODE_VALUES[instruction.opcode],
                instruction_operand_a_value(instruction),
                instruction.operand_b,
            )
        )
    for literal in program.literals:
        if literal.kind == LITERAL_STRING:
            encoded = str(literal.value).encode("utf-8")
            manifest.extend(struct.pack(PROGRAM_LITERAL_HEADER_FORMAT, LITERAL_VALUES[literal.kind], 0, 0, len(encoded)))
            manifest.extend(encoded)
            manifest.append(0)
            continue
        if literal.kind == LITERAL_SMALL_INTEGER:
            manifest.extend(struct.pack(PROGRAM_LITERAL_HEADER_FORMAT, LITERAL_VALUES[literal.kind], 0, 0, int(literal.value)))
            continue
        raise AssertionError(f"unknown literal kind {literal.kind}")
    return bytes(manifest)


def build_seed_manifest() -> bytes:
    seed_objects: list[tuple[int, list[tuple[int, int]]]] = [
        (SEED_OBJECT_TRANSCRIPT, []),
        (SEED_OBJECT_DISPLAY, []),
        (SEED_OBJECT_BITBLT, []),
        (SEED_OBJECT_GLYPHS, []),
        (SEED_OBJECT_FORM_FACTORY, []),
        (SEED_OBJECT_BITMAP_FACTORY, []),
        (
            SEED_OBJECT_TEXT_LAYOUT,
            [
                (SEED_FIELD_SMALL_INTEGER, 24),
                (SEED_FIELD_SMALL_INTEGER, 24),
                (SEED_FIELD_SMALL_INTEGER, 4),
                (SEED_FIELD_SMALL_INTEGER, 2),
            ],
        ),
        (
            SEED_OBJECT_TEXT_STYLE,
            [
                (SEED_FIELD_SMALL_INTEGER, 0x00486020),
                (SEED_FIELD_SMALL_INTEGER, 0x00F2F2F2),
            ],
        ),
        (
            SEED_OBJECT_BITMAP,
            [
                (SEED_FIELD_SMALL_INTEGER, 640),
                (SEED_FIELD_SMALL_INTEGER, 480),
                (SEED_FIELD_SMALL_INTEGER, BITMAP_STORAGE_FRAMEBUFFER),
                (SEED_FIELD_SMALL_INTEGER, 0),
            ],
        ),
        (
            SEED_OBJECT_FORM,
            [
                (SEED_FIELD_OBJECT_INDEX, 8),
            ],
        ),
    ]
    for glyph_index in range(128):
        seed_objects.append(
            (
                SEED_OBJECT_BITMAP,
                [
                    (SEED_FIELD_SMALL_INTEGER, 5),
                    (SEED_FIELD_SMALL_INTEGER, 7),
                    (SEED_FIELD_SMALL_INTEGER, BITMAP_STORAGE_GLYPH_MONO),
                    (SEED_FIELD_SMALL_INTEGER, glyph_index),
                ],
            )
        )

    global_bindings = [
        (GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRANSCRIPT"], 0),
        (GLOBAL_VALUES["RECORZ_MVP_GLOBAL_DISPLAY"], 1),
        (GLOBAL_VALUES["RECORZ_MVP_GLOBAL_BITBLT"], 2),
        (GLOBAL_VALUES["RECORZ_MVP_GLOBAL_GLYPHS"], 3),
        (GLOBAL_VALUES["RECORZ_MVP_GLOBAL_FORM"], 4),
        (GLOBAL_VALUES["RECORZ_MVP_GLOBAL_BITMAP"], 5),
    ]
    root_bindings = [
        (SEED_ROOT_DEFAULT_FORM, 9),
        (SEED_ROOT_FRAMEBUFFER_BITMAP, 8),
        (SEED_ROOT_GLYPH_FALLBACK_BITMAP, 10 + 32),
        (SEED_ROOT_TRANSCRIPT_LAYOUT, 6),
        (SEED_ROOT_TRANSCRIPT_STYLE, 7),
    ]
    glyph_object_indices = [10 + glyph_index for glyph_index in range(128)]

    manifest = bytearray(
        struct.pack(
            SEED_HEADER_FORMAT,
            SEED_MAGIC,
            SEED_VERSION,
            len(seed_objects),
            len(global_bindings),
            len(root_bindings),
            128,
            0,
        )
    )
    for object_kind, fields in seed_objects:
        manifest.extend(struct.pack(SEED_OBJECT_HEADER_FORMAT, object_kind, len(fields), 0))
        for field_kind, field_value in fields + [(SEED_FIELD_NIL, 0)] * (4 - len(fields)):
            manifest.extend(struct.pack(SEED_FIELD_FORMAT, field_kind, field_value))
    for binding_id, object_index in global_bindings:
        manifest.extend(struct.pack(SEED_BINDING_FORMAT, binding_id, object_index))
    for binding_id, object_index in root_bindings:
        manifest.extend(struct.pack(SEED_BINDING_FORMAT, binding_id, object_index))
    for object_index in glyph_object_indices:
        manifest.extend(struct.pack("<H", object_index))
    return bytes(manifest)


def build_entry_manifest() -> bytes:
    return struct.pack(
        IMAGE_ENTRY_FORMAT,
        IMAGE_ENTRY_MAGIC,
        IMAGE_ENTRY_VERSION,
        IMAGE_ENTRY_KIND_DOIT,
        0,
        IMAGE_SECTION_PROGRAM,
        0,
        0,
    )


def fnv1a32(data: bytes, *, seed: int = 0x811C9DC5) -> int:
    value = seed
    for byte in data:
        value ^= byte
        value = (value * 0x01000193) & 0xFFFFFFFF
    return value


def build_image_manifest(program: Program) -> bytes:
    entry_manifest = build_entry_manifest()
    program_manifest = build_program_manifest(program)
    seed_manifest = build_seed_manifest()
    section_count = 3
    header_size = struct.calcsize(IMAGE_HEADER_FORMAT) + (section_count * struct.calcsize(IMAGE_SECTION_FORMAT))
    entry_offset = header_size
    program_offset = entry_offset + len(entry_manifest)
    seed_offset = program_offset + len(program_manifest)
    feature_flags = IMAGE_FEATURE_FNV1A32

    manifest = bytearray(
        struct.pack(
            IMAGE_HEADER_FORMAT,
            IMAGE_MAGIC,
            IMAGE_VERSION,
            section_count,
            feature_flags,
            0,
            IMAGE_PROFILE,
        )
    )
    manifest.extend(
        struct.pack(
            IMAGE_SECTION_FORMAT,
            IMAGE_SECTION_ENTRY,
            0,
            entry_offset,
            len(entry_manifest),
        )
    )
    manifest.extend(
        struct.pack(
            IMAGE_SECTION_FORMAT,
            IMAGE_SECTION_PROGRAM,
            0,
            program_offset,
            len(program_manifest),
        )
    )
    manifest.extend(
        struct.pack(
            IMAGE_SECTION_FORMAT,
            IMAGE_SECTION_SEED,
            0,
            seed_offset,
            len(seed_manifest),
        )
    )
    manifest.extend(entry_manifest)
    manifest.extend(program_manifest)
    manifest.extend(seed_manifest)
    checksum = fnv1a32(manifest[struct.calcsize(IMAGE_HEADER_FORMAT) :])
    manifest[: struct.calcsize(IMAGE_HEADER_FORMAT)] = struct.pack(
        IMAGE_HEADER_FORMAT,
        IMAGE_MAGIC,
        IMAGE_VERSION,
        section_count,
        feature_flags,
        checksum,
        IMAGE_PROFILE,
    )
    return bytes(manifest)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Recorz source file to lower")
    parser.add_argument("image_output", type=Path, help="Generated boot image path")
    args = parser.parse_args(argv)

    source_text = args.source.read_text()
    program = build_program(source_text)
    args.image_output.parent.mkdir(parents=True, exist_ok=True)
    args.image_output.write_bytes(build_image_manifest(program))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
