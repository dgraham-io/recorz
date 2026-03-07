#!/usr/bin/env python3
"""Lower a tiny compiled Recorz do-it into the MVP RISC-V demo assets."""

from __future__ import annotations

import argparse
import json
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
KERNEL_MVP_ROOT = ROOT / "kernel" / "mvp"
KERNEL_MVP_MANIFEST_PATH = KERNEL_MVP_ROOT / "manifest.json"
if str(ROOT / "src") not in sys.path:
    sys.path.insert(0, str(ROOT / "src"))

from recorz.compiler import compile_do_it, compile_method  # noqa: E402
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
    "class": "RECORZ_MVP_SELECTOR_CLASS",
    "instanceKind": "RECORZ_MVP_SELECTOR_INSTANCE_KIND",
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
    "RECORZ_MVP_SELECTOR_CLASS": 21,
    "RECORZ_MVP_SELECTOR_INSTANCE_KIND": 22,
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
SEED_VERSION = 16
SEED_HEADER_FORMAT = "<4sHHHHHH"
SEED_BINDING_FORMAT = "<HH"
SEED_OBJECT_HEADER_FORMAT = "<BBH"
SEED_FIELD_FORMAT = "<Bi"
SEED_INVALID_OBJECT_INDEX = 0xFFFF
CLASS_FIELD_SUPERCLASS = 0
CLASS_FIELD_INSTANCE_KIND = 1
CLASS_FIELD_METHOD_START = 2
CLASS_FIELD_METHOD_COUNT = 3
METHOD_FIELD_SELECTOR = 0
METHOD_FIELD_ARGUMENT_COUNT = 1
METHOD_FIELD_PRIMITIVE_KIND = 2
METHOD_FIELD_ENTRY = 3
METHOD_ENTRY_FIELD_EXECUTION_ID = 0
METHOD_ENTRY_FIELD_IMPLEMENTATION = 1
ACCESSOR_METHOD_FIELD_FIELD_INDEX = 0
FIELD_SEND_METHOD_FIELD_FIELD_INDEX = 0
FIELD_SEND_METHOD_FIELD_SELECTOR = 1
ROOT_SEND_METHOD_FIELD_ROOT_ID = 0
ROOT_SEND_METHOD_FIELD_SELECTOR = 1
ROOT_SEND_METHOD_FIELD_RETURN_MODE = 2
ROOT_VALUE_METHOD_FIELD_ROOT_ID = 0
COMPILED_METHOD_MAX_INSTRUCTIONS = 4
INTERPRETED_METHOD_MAX_INSTRUCTIONS = 4

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
SEED_OBJECT_TEXT_METRICS = 11
SEED_OBJECT_TEXT_BEHAVIOR = 12
SEED_OBJECT_CLASS = 13
SEED_OBJECT_METHOD_DESCRIPTOR = 14
SEED_OBJECT_METHOD_ENTRY = 15
SEED_OBJECT_SELECTOR = 16
SEED_OBJECT_ACCESSOR_METHOD = 17
SEED_OBJECT_FIELD_SEND_METHOD = 18
SEED_OBJECT_ROOT_SEND_METHOD = 19
SEED_OBJECT_ROOT_VALUE_METHOD = 20
SEED_OBJECT_INTERPRETED_METHOD = 21
SEED_OBJECT_COMPILED_METHOD = 22

SEED_ROOT_DEFAULT_FORM = 1
SEED_ROOT_FRAMEBUFFER_BITMAP = 2
SEED_ROOT_TRANSCRIPT_BEHAVIOR = 3
SEED_ROOT_TRANSCRIPT_LAYOUT = 4
SEED_ROOT_TRANSCRIPT_STYLE = 5
SEED_ROOT_TRANSCRIPT_METRICS = 6

BITMAP_STORAGE_FRAMEBUFFER = 1
BITMAP_STORAGE_GLYPH_MONO = 2

KERNEL_CLASS_NAME_TO_OBJECT_KIND = {
    "Transcript": SEED_OBJECT_TRANSCRIPT,
    "Display": SEED_OBJECT_DISPLAY,
    "Form": SEED_OBJECT_FORM,
    "Bitmap": SEED_OBJECT_BITMAP,
    "BitBlt": SEED_OBJECT_BITBLT,
    "Glyphs": SEED_OBJECT_GLYPHS,
    "FormFactory": SEED_OBJECT_FORM_FACTORY,
    "BitmapFactory": SEED_OBJECT_BITMAP_FACTORY,
    "TextLayout": SEED_OBJECT_TEXT_LAYOUT,
    "TextStyle": SEED_OBJECT_TEXT_STYLE,
    "TextMetrics": SEED_OBJECT_TEXT_METRICS,
    "TextBehavior": SEED_OBJECT_TEXT_BEHAVIOR,
    "Class": SEED_OBJECT_CLASS,
}


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


@dataclass
class SeedObject:
    object_kind: int
    class_index: int
    fields: list[tuple[int, int]]


@dataclass(frozen=True)
class KernelMethodSource:
    class_name: str
    instance_variables: tuple[str, ...]
    relative_path: str
    selector: str
    source_text: str


METHOD_ENTRY_DEFINITIONS: list[tuple[str, int, str, int]] = [
    ("RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW", SEED_OBJECT_TRANSCRIPT, "show:", 1),
    ("RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_CR", SEED_OBJECT_TRANSCRIPT, "cr", 0),
    ("RECORZ_MVP_METHOD_ENTRY_DISPLAY_DEFAULT_FORM", SEED_OBJECT_DISPLAY, "defaultForm", 0),
    ("RECORZ_MVP_METHOD_ENTRY_DISPLAY_CLEAR", SEED_OBJECT_DISPLAY, "clear", 0),
    ("RECORZ_MVP_METHOD_ENTRY_DISPLAY_WRITE_STRING", SEED_OBJECT_DISPLAY, "writeString:", 1),
    ("RECORZ_MVP_METHOD_ENTRY_DISPLAY_NEWLINE", SEED_OBJECT_DISPLAY, "newline", 0),
    ("RECORZ_MVP_METHOD_ENTRY_BITBLT_FILL_FORM_COLOR", SEED_OBJECT_BITBLT, "fillForm:color:", 2),
    ("RECORZ_MVP_METHOD_ENTRY_BITBLT_COPY_BITMAP_TO_FORM_X_Y_SCALE", SEED_OBJECT_BITBLT, "copyBitmap:toForm:x:y:scale:", 5),
    (
        "RECORZ_MVP_METHOD_ENTRY_BITBLT_COPY_BITMAP_TO_FORM_X_Y_SCALE_COLOR",
        SEED_OBJECT_BITBLT,
        "copyBitmap:toForm:x:y:scale:color:",
        6,
    ),
    (
        "RECORZ_MVP_METHOD_ENTRY_BITBLT_COPY_BITMAP_SOURCE_X_SOURCE_Y_WIDTH_HEIGHT_TO_FORM_X_Y_SCALE_COLOR",
        SEED_OBJECT_BITBLT,
        "copyBitmap:sourceX:sourceY:width:height:toForm:x:y:scale:color:",
        10,
    ),
    ("RECORZ_MVP_METHOD_ENTRY_GLYPHS_AT", SEED_OBJECT_GLYPHS, "at:", 1),
    ("RECORZ_MVP_METHOD_ENTRY_FORM_FACTORY_FROM_BITS", SEED_OBJECT_FORM_FACTORY, "fromBits:", 1),
    ("RECORZ_MVP_METHOD_ENTRY_BITMAP_FACTORY_MONO_WIDTH_HEIGHT", SEED_OBJECT_BITMAP_FACTORY, "monoWidth:height:", 2),
    ("RECORZ_MVP_METHOD_ENTRY_BITMAP_WIDTH", SEED_OBJECT_BITMAP, "width", 0),
    ("RECORZ_MVP_METHOD_ENTRY_BITMAP_HEIGHT", SEED_OBJECT_BITMAP, "height", 0),
    ("RECORZ_MVP_METHOD_ENTRY_FORM_CLEAR", SEED_OBJECT_FORM, "clear", 0),
    ("RECORZ_MVP_METHOD_ENTRY_FORM_WRITE_STRING", SEED_OBJECT_FORM, "writeString:", 1),
    ("RECORZ_MVP_METHOD_ENTRY_FORM_NEWLINE", SEED_OBJECT_FORM, "newline", 0),
    ("RECORZ_MVP_METHOD_ENTRY_FORM_BITS", SEED_OBJECT_FORM, "bits", 0),
    ("RECORZ_MVP_METHOD_ENTRY_FORM_WIDTH", SEED_OBJECT_FORM, "width", 0),
    ("RECORZ_MVP_METHOD_ENTRY_FORM_HEIGHT", SEED_OBJECT_FORM, "height", 0),
    ("RECORZ_MVP_METHOD_ENTRY_CLASS_INSTANCE_KIND", SEED_OBJECT_CLASS, "instanceKind", 0),
]
METHOD_ENTRY_VALUES = {
    name: index + 1 for index, (name, _owner_kind, _selector, _argument_count) in enumerate(METHOD_ENTRY_DEFINITIONS)
}
METHOD_ENTRY_COUNT = len(METHOD_ENTRY_VALUES) + 1
METHOD_ENTRY_SPECS = {
    METHOD_ENTRY_VALUES[name]: (owner_kind, SELECTOR_VALUES[SELECTOR_IDS[selector]], argument_count)
    for name, owner_kind, selector, argument_count in METHOD_ENTRY_DEFINITIONS
}
ACCESSOR_METHOD_FIELD_BY_ENTRY_NAME: dict[str, int] = {}
FIELD_SEND_METHOD_SPEC_BY_ENTRY_NAME: dict[str, tuple[int, str]] = {}
METHOD_RETURN_RESULT = 1
METHOD_RETURN_RECEIVER = 2
ROOT_SEND_METHOD_SPEC_BY_ENTRY_NAME = {
}
ROOT_VALUE_METHOD_ROOT_BY_ENTRY_NAME: dict[str, int] = {}
COMPILED_METHOD_OP_PUSH_GLOBAL = 1
COMPILED_METHOD_OP_PUSH_ROOT = 2
COMPILED_METHOD_OP_PUSH_ARGUMENT = 3
COMPILED_METHOD_OP_PUSH_FIELD = 4
COMPILED_METHOD_OP_SEND = 5
COMPILED_METHOD_OP_RETURN_TOP = 6
COMPILED_METHOD_OP_RETURN_RECEIVER = 7
COMPILED_METHOD_OPCODE_VALUES = {
    "push_global": COMPILED_METHOD_OP_PUSH_GLOBAL,
    "push_root": COMPILED_METHOD_OP_PUSH_ROOT,
    "push_argument": COMPILED_METHOD_OP_PUSH_ARGUMENT,
    "push_field": COMPILED_METHOD_OP_PUSH_FIELD,
    "send": COMPILED_METHOD_OP_SEND,
    "return_top": COMPILED_METHOD_OP_RETURN_TOP,
    "return_receiver": COMPILED_METHOD_OP_RETURN_RECEIVER,
}
INTERPRETED_METHOD_OP_PUSH_ROOT = 1
INTERPRETED_METHOD_OP_PUSH_ARGUMENT = 2
INTERPRETED_METHOD_OP_SEND = 3
INTERPRETED_METHOD_OP_RETURN_TOP = 4
INTERPRETED_METHOD_OP_RETURN_RECEIVER = 5
INTERPRETED_METHOD_OPCODE_VALUES = {
    "push_root": INTERPRETED_METHOD_OP_PUSH_ROOT,
    "push_argument": INTERPRETED_METHOD_OP_PUSH_ARGUMENT,
    "send": INTERPRETED_METHOD_OP_SEND,
    "return_top": INTERPRETED_METHOD_OP_RETURN_TOP,
    "return_receiver": INTERPRETED_METHOD_OP_RETURN_RECEIVER,
}
INTERPRETED_METHOD_PROGRAM_BY_ENTRY_NAME = {
    "RECORZ_MVP_METHOD_ENTRY_DISPLAY_DEFAULT_FORM": [
        ("push_root", SEED_ROOT_DEFAULT_FORM, 0),
        ("return_top", 0, 0),
    ],
}
BUILTIN_METHODS_BY_KIND: dict[int, list[tuple[str, int, str]]] = {
    kind: [] for kind in range(SEED_OBJECT_TRANSCRIPT, SEED_OBJECT_COMPILED_METHOD + 1)
}
for entry_name, owner_kind, selector, argument_count in METHOD_ENTRY_DEFINITIONS:
    BUILTIN_METHODS_BY_KIND[owner_kind].append((selector, argument_count, entry_name))


def encode_interpreted_instruction(opcode_name: str, operand_a: int = 0, operand_b: int = 0) -> int:
    return (
        INTERPRETED_METHOD_OPCODE_VALUES[opcode_name]
        | ((operand_a & 0xFF) << 8)
        | ((operand_b & 0xFFFF) << 16)
    )


def encode_compiled_method_instruction(opcode_name: str, operand_a: int = 0, operand_b: int = 0) -> int:
    return (
        COMPILED_METHOD_OPCODE_VALUES[opcode_name]
        | ((operand_a & 0xFF) << 8)
        | ((operand_b & 0xFFFF) << 16)
    )


def split_kernel_method_chunks(source_text: str) -> list[str]:
    chunks: list[str] = []
    chunk_lines: list[str] = []

    for line in source_text.splitlines():
        if line.strip() == "!":
            chunk = "\n".join(chunk_lines).strip()
            if chunk:
                chunks.append(chunk)
            chunk_lines = []
            continue
        chunk_lines.append(line)
    trailing_chunk = "\n".join(chunk_lines).strip()
    if trailing_chunk:
        chunks.append(trailing_chunk)
    return chunks


def load_kernel_method_sources() -> dict[str, KernelMethodSource]:
    entry_definitions = {
        entry_name: (owner_kind, selector, argument_count)
        for entry_name, owner_kind, selector, argument_count in METHOD_ENTRY_DEFINITIONS
    }
    manifest_data = json.loads(KERNEL_MVP_MANIFEST_PATH.read_text(encoding="utf-8"))
    class_specs = manifest_data.get("classes")
    method_sources: dict[str, KernelMethodSource] = {}

    if not isinstance(class_specs, dict):
        raise LoweringError("kernel MVP manifest is missing a classes table")
    for class_name, class_spec in class_specs.items():
        expected_class_kind = KERNEL_CLASS_NAME_TO_OBJECT_KIND.get(class_name)
        if not isinstance(class_name, str) or not isinstance(class_spec, dict):
            raise LoweringError("kernel MVP manifest class entries are invalid")
        if expected_class_kind is None:
            raise LoweringError(f"kernel MVP manifest references unknown class {class_name}")
        instance_variables = class_spec.get("instance_variables")
        relative_path = class_spec.get("file")
        entry_names = class_spec.get("entries")
        if not isinstance(instance_variables, list) or not all(isinstance(name, str) for name in instance_variables):
            raise LoweringError(f"kernel MVP manifest instance variables are invalid for class {class_name}")
        if not isinstance(relative_path, str):
            raise LoweringError(f"kernel MVP manifest file path is invalid for class {class_name}")
        if not isinstance(entry_names, list) or not all(isinstance(entry_name, str) for entry_name in entry_names):
            raise LoweringError(f"kernel MVP manifest entries are invalid for class {class_name}")
        method_path = KERNEL_MVP_ROOT / relative_path
        if not method_path.is_file():
            raise LoweringError(f"kernel MVP method source is missing: {method_path}")
        chunk_sources = split_kernel_method_chunks(method_path.read_text(encoding="utf-8"))
        compiled_chunks: dict[tuple[str, int], str] = {}

        for chunk_source in chunk_sources:
            compiled = compile_method(chunk_source, class_name, list(instance_variables))
            signature = (compiled.selector, len(compiled.arg_names))

            if signature in compiled_chunks:
                raise LoweringError(
                    f"kernel MVP class file {relative_path} duplicates method {compiled.selector}/{len(compiled.arg_names)}"
                )
            compiled_chunks[signature] = chunk_source
        for entry_name in entry_names:
            owner_kind, selector, argument_count = entry_definitions.get(entry_name, (None, None, None))

            if owner_kind is None:
                raise LoweringError(f"kernel MVP manifest references unknown method entry {entry_name}")
            if owner_kind != expected_class_kind:
                raise LoweringError(
                    f"kernel MVP manifest class {class_name} includes method entry {entry_name} for a different owner kind"
                )
            if entry_name in method_sources:
                raise LoweringError(f"kernel MVP manifest duplicates method entry {entry_name}")
            chunk_source = compiled_chunks.get((selector, argument_count))
            if chunk_source is None:
                raise LoweringError(
                    f"kernel MVP class file {relative_path} is missing method {selector}/{argument_count} for entry {entry_name}"
                )
            method_sources[entry_name] = KernelMethodSource(
                class_name=class_name,
                instance_variables=tuple(instance_variables),
                relative_path=relative_path,
                selector=selector,
                source_text=chunk_source,
            )
        if len(compiled_chunks) != len(entry_names):
            raise LoweringError(
                f"kernel MVP class file {relative_path} contains method chunks that are not declared in the manifest"
            )
    return method_sources


def compile_kernel_method_program(class_name: str, instance_variables: list[str], source: str) -> list[int]:
    compiled = compile_method(source, class_name, instance_variables)
    arg_indices = {name: index for index, name in enumerate(compiled.arg_names)}
    field_indices = {name: index for index, name in enumerate(instance_variables)}
    instructions = list(compiled.instructions)
    return_receiver = False
    lowered: list[int] = []

    if compiled.literals:
        raise LoweringError(f"Kernel method {class_name}>>{compiled.selector} uses unsupported literals")
    if compiled.temp_names:
        raise LoweringError(f"Kernel method {class_name}>>{compiled.selector} uses unsupported temporaries")
    if len(instructions) >= 3 and instructions[-3].opcode == "pop" and instructions[-2].opcode == "push_self" and instructions[-1].opcode == "return_local":
        instructions = instructions[:-3]
        return_receiver = True
    elif len(instructions) >= 2 and instructions[-2].opcode == "push_self" and instructions[-1].opcode == "return_local":
        instructions = instructions[:-2]
        return_receiver = True
    elif instructions and instructions[-1].opcode == "return_local":
        instructions = instructions[:-1]
    else:
        raise LoweringError(f"Kernel method {class_name}>>{compiled.selector} has an unsupported return shape")
    instruction_index = 0
    while instruction_index < len(instructions):
        instruction = instructions[instruction_index]

        if instruction.opcode == "push_binding":
            binding = instruction.operand
            if not isinstance(binding, BindingRef):
                raise LoweringError(
                    f"Kernel method {class_name}>>{compiled.selector} expected a binding operand, found {binding!r}"
                )
            if (
                binding.kind == "global"
                and binding.name == "Display"
                and instruction_index + 1 < len(instructions)
                and instructions[instruction_index + 1].opcode == "send"
            ):
                send_site = instructions[instruction_index + 1].operand
                if (
                    isinstance(send_site, SendSite)
                    and not send_site.super_send
                    and send_site.selector == "defaultForm"
                    and send_site.argument_count == 0
                ):
                    lowered.append(encode_compiled_method_instruction("push_root", SEED_ROOT_DEFAULT_FORM))
                    instruction_index += 2
                    continue
            if binding.kind == "global":
                global_id = GLOBAL_IDS.get(binding.name)
                if global_id is None:
                    raise LoweringError(f"Kernel method {class_name}>>{compiled.selector} uses unsupported global {binding.name!r}")
                lowered.append(encode_compiled_method_instruction("push_global", GLOBAL_VALUES[global_id]))
                instruction_index += 1
                continue
            if binding.kind == "lexical":
                if binding.name not in arg_indices:
                    raise LoweringError(
                        f"Kernel method {class_name}>>{compiled.selector} uses unsupported lexical {binding.name!r}"
                    )
                lowered.append(encode_compiled_method_instruction("push_argument", arg_indices[binding.name]))
                instruction_index += 1
                continue
            if binding.kind == "instance":
                if binding.name not in field_indices:
                    raise LoweringError(
                        f"Kernel method {class_name}>>{compiled.selector} uses unknown instance variable {binding.name!r}"
                    )
                lowered.append(encode_compiled_method_instruction("push_field", field_indices[binding.name]))
                instruction_index += 1
                continue
            raise LoweringError(f"Kernel method {class_name}>>{compiled.selector} uses unknown binding kind {binding.kind!r}")
        if instruction.opcode == "send":
            send_site = instruction.operand
            if not isinstance(send_site, SendSite):
                raise LoweringError(
                    f"Kernel method {class_name}>>{compiled.selector} expected a send operand, found {send_site!r}"
                )
            if send_site.super_send:
                raise LoweringError(f"Kernel method {class_name}>>{compiled.selector} uses unsupported super send")
            selector_id = SELECTOR_IDS.get(send_site.selector)
            if selector_id is None:
                raise LoweringError(
                    f"Kernel method {class_name}>>{compiled.selector} uses unsupported selector {send_site.selector!r}"
                )
            lowered.append(
                encode_compiled_method_instruction("send", SELECTOR_VALUES[selector_id], send_site.argument_count)
            )
            instruction_index += 1
            continue
        raise LoweringError(
            f"Kernel method {class_name}>>{compiled.selector} uses unsupported compiler opcode {instruction.opcode!r}"
        )
    # unreachable
    lowered.append(
        encode_compiled_method_instruction("return_receiver" if return_receiver else "return_top")
    )
    if not lowered or len(lowered) > COMPILED_METHOD_MAX_INSTRUCTIONS:
        raise LoweringError(
            f"Kernel method {class_name}>>{compiled.selector} lowers to {len(lowered)} instructions; MVP compiled methods support at most {COMPILED_METHOD_MAX_INSTRUCTIONS}"
        )
    return lowered


COMPILED_METHOD_SOURCE_BY_ENTRY_NAME = load_kernel_method_sources()
COMPILED_METHOD_PROGRAM_BY_ENTRY_NAME = {
    entry_name: compile_kernel_method_program(source.class_name, list(source.instance_variables), source.source_text)
    for entry_name, source in COMPILED_METHOD_SOURCE_BY_ENTRY_NAME.items()
}


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
    seed_objects: list[SeedObject] = [
        SeedObject(SEED_OBJECT_TRANSCRIPT, SEED_INVALID_OBJECT_INDEX, []),
        SeedObject(SEED_OBJECT_DISPLAY, SEED_INVALID_OBJECT_INDEX, []),
        SeedObject(SEED_OBJECT_BITBLT, SEED_INVALID_OBJECT_INDEX, []),
        SeedObject(SEED_OBJECT_GLYPHS, SEED_INVALID_OBJECT_INDEX, []),
        SeedObject(SEED_OBJECT_FORM_FACTORY, SEED_INVALID_OBJECT_INDEX, []),
        SeedObject(SEED_OBJECT_BITMAP_FACTORY, SEED_INVALID_OBJECT_INDEX, []),
        SeedObject(
            SEED_OBJECT_TEXT_LAYOUT,
            SEED_INVALID_OBJECT_INDEX,
            [
                (SEED_FIELD_SMALL_INTEGER, 24),
                (SEED_FIELD_SMALL_INTEGER, 24),
                (SEED_FIELD_SMALL_INTEGER, 4),
                (SEED_FIELD_SMALL_INTEGER, 2),
            ],
        ),
        SeedObject(
            SEED_OBJECT_TEXT_STYLE,
            SEED_INVALID_OBJECT_INDEX,
            [
                (SEED_FIELD_SMALL_INTEGER, 0x00486020),
                (SEED_FIELD_SMALL_INTEGER, 0x00F2F2F2),
            ],
        ),
        SeedObject(
            SEED_OBJECT_BITMAP,
            SEED_INVALID_OBJECT_INDEX,
            [
                (SEED_FIELD_SMALL_INTEGER, 640),
                (SEED_FIELD_SMALL_INTEGER, 480),
                (SEED_FIELD_SMALL_INTEGER, BITMAP_STORAGE_FRAMEBUFFER),
                (SEED_FIELD_SMALL_INTEGER, 0),
            ],
        ),
        SeedObject(
            SEED_OBJECT_FORM,
            SEED_INVALID_OBJECT_INDEX,
            [
                (SEED_FIELD_OBJECT_INDEX, 8),
            ],
        ),
    ]
    for glyph_index in range(128):
        seed_objects.append(
            SeedObject(
                SEED_OBJECT_BITMAP,
                SEED_INVALID_OBJECT_INDEX,
                [
                    (SEED_FIELD_SMALL_INTEGER, 5),
                    (SEED_FIELD_SMALL_INTEGER, 7),
                    (SEED_FIELD_SMALL_INTEGER, BITMAP_STORAGE_GLYPH_MONO),
                    (SEED_FIELD_SMALL_INTEGER, glyph_index),
                ],
            )
        )
    seed_objects.append(
        SeedObject(
            SEED_OBJECT_TEXT_METRICS,
            SEED_INVALID_OBJECT_INDEX,
            [
                (SEED_FIELD_SMALL_INTEGER, 6),
                (SEED_FIELD_SMALL_INTEGER, 8),
            ],
        )
    )
    text_metrics_index = len(seed_objects) - 1
    seed_objects.append(
        SeedObject(
            SEED_OBJECT_TEXT_BEHAVIOR,
            SEED_INVALID_OBJECT_INDEX,
            [
                (SEED_FIELD_OBJECT_INDEX, 10 + 32),
                (SEED_FIELD_SMALL_INTEGER, 1),
            ],
        )
    )
    text_behavior_index = len(seed_objects) - 1

    class_class_index = len(seed_objects)
    class_kinds_in_order = [
        SEED_OBJECT_CLASS,
        SEED_OBJECT_TRANSCRIPT,
        SEED_OBJECT_DISPLAY,
        SEED_OBJECT_BITBLT,
        SEED_OBJECT_GLYPHS,
        SEED_OBJECT_FORM_FACTORY,
        SEED_OBJECT_BITMAP_FACTORY,
        SEED_OBJECT_TEXT_LAYOUT,
        SEED_OBJECT_TEXT_STYLE,
        SEED_OBJECT_BITMAP,
        SEED_OBJECT_FORM,
        SEED_OBJECT_TEXT_METRICS,
        SEED_OBJECT_TEXT_BEHAVIOR,
        SEED_OBJECT_METHOD_DESCRIPTOR,
        SEED_OBJECT_METHOD_ENTRY,
        SEED_OBJECT_SELECTOR,
        SEED_OBJECT_ACCESSOR_METHOD,
        SEED_OBJECT_FIELD_SEND_METHOD,
        SEED_OBJECT_ROOT_SEND_METHOD,
        SEED_OBJECT_ROOT_VALUE_METHOD,
        SEED_OBJECT_INTERPRETED_METHOD,
        SEED_OBJECT_COMPILED_METHOD,
    ]
    class_indices = {
        SEED_OBJECT_TRANSCRIPT: class_class_index + 1,
        SEED_OBJECT_DISPLAY: class_class_index + 2,
        SEED_OBJECT_BITBLT: class_class_index + 3,
        SEED_OBJECT_GLYPHS: class_class_index + 4,
        SEED_OBJECT_FORM_FACTORY: class_class_index + 5,
        SEED_OBJECT_BITMAP_FACTORY: class_class_index + 6,
        SEED_OBJECT_TEXT_LAYOUT: class_class_index + 7,
        SEED_OBJECT_TEXT_STYLE: class_class_index + 8,
        SEED_OBJECT_BITMAP: class_class_index + 9,
        SEED_OBJECT_FORM: class_class_index + 10,
        SEED_OBJECT_TEXT_METRICS: class_class_index + 11,
        SEED_OBJECT_TEXT_BEHAVIOR: class_class_index + 12,
        SEED_OBJECT_METHOD_DESCRIPTOR: class_class_index + 13,
        SEED_OBJECT_METHOD_ENTRY: class_class_index + 14,
        SEED_OBJECT_SELECTOR: class_class_index + 15,
        SEED_OBJECT_ACCESSOR_METHOD: class_class_index + 16,
        SEED_OBJECT_FIELD_SEND_METHOD: class_class_index + 17,
        SEED_OBJECT_ROOT_SEND_METHOD: class_class_index + 18,
        SEED_OBJECT_ROOT_VALUE_METHOD: class_class_index + 19,
        SEED_OBJECT_INTERPRETED_METHOD: class_class_index + 20,
        SEED_OBJECT_COMPILED_METHOD: class_class_index + 21,
    }

    for seed_object in seed_objects:
        seed_object.class_index = class_indices[seed_object.object_kind]

    selector_start_index = class_class_index + len(class_kinds_in_order)
    method_start_by_kind: dict[int, int] = {}
    method_count_by_kind: dict[int, int] = {}
    selector_indices_by_value: dict[int, int] = {}
    selector_seed_objects: list[SeedObject] = []
    accessor_method_indices: dict[str, int] = {}
    accessor_method_seed_objects: list[SeedObject] = []
    field_send_method_indices: dict[str, int] = {}
    field_send_method_seed_objects: list[SeedObject] = []
    root_send_method_indices: dict[str, int] = {}
    root_send_method_seed_objects: list[SeedObject] = []
    root_value_method_indices: dict[str, int] = {}
    root_value_method_seed_objects: list[SeedObject] = []
    interpreted_method_indices: dict[str, int] = {}
    interpreted_method_seed_objects: list[SeedObject] = []
    compiled_method_indices: dict[str, int] = {}
    compiled_method_seed_objects: list[SeedObject] = []
    method_entry_indices: dict[str, int] = {}
    method_entry_seed_objects: list[SeedObject] = []
    method_seed_objects: list[SeedObject] = []

    for selector_value in sorted({METHOD_ENTRY_SPECS[METHOD_ENTRY_VALUES[name]][1] for name, *_rest in METHOD_ENTRY_DEFINITIONS}):
        selector_indices_by_value[selector_value] = selector_start_index + len(selector_seed_objects)
        selector_seed_objects.append(
            SeedObject(
                SEED_OBJECT_SELECTOR,
                class_indices[SEED_OBJECT_SELECTOR],
                [
                    (SEED_FIELD_SMALL_INTEGER, selector_value),
                ],
            )
        )

    accessor_method_start_index = selector_start_index + len(selector_seed_objects)
    for entry_name, _owner_kind, _selector, _argument_count in METHOD_ENTRY_DEFINITIONS:
        field_index = ACCESSOR_METHOD_FIELD_BY_ENTRY_NAME.get(entry_name)
        if field_index is None:
            continue
        accessor_method_indices[entry_name] = accessor_method_start_index + len(accessor_method_seed_objects)
        accessor_method_seed_objects.append(
            SeedObject(
                SEED_OBJECT_ACCESSOR_METHOD,
                class_indices[SEED_OBJECT_ACCESSOR_METHOD],
                [
                    (SEED_FIELD_SMALL_INTEGER, field_index),
                ],
            )
        )

    field_send_method_start_index = accessor_method_start_index + len(accessor_method_seed_objects)
    for entry_name, method_spec in FIELD_SEND_METHOD_SPEC_BY_ENTRY_NAME.items():
        field_index, selector = method_spec
        field_send_method_indices[entry_name] = field_send_method_start_index + len(field_send_method_seed_objects)
        field_send_method_seed_objects.append(
            SeedObject(
                SEED_OBJECT_FIELD_SEND_METHOD,
                class_indices[SEED_OBJECT_FIELD_SEND_METHOD],
                [
                    (SEED_FIELD_SMALL_INTEGER, field_index),
                    (SEED_FIELD_OBJECT_INDEX, selector_indices_by_value[SELECTOR_VALUES[SELECTOR_IDS[selector]]]),
                ],
            )
        )

    root_send_method_start_index = field_send_method_start_index + len(field_send_method_seed_objects)
    for entry_name, method_spec in ROOT_SEND_METHOD_SPEC_BY_ENTRY_NAME.items():
        root_id, selector, return_mode = method_spec
        root_send_method_indices[entry_name] = root_send_method_start_index + len(root_send_method_seed_objects)
        root_send_method_seed_objects.append(
            SeedObject(
                SEED_OBJECT_ROOT_SEND_METHOD,
                class_indices[SEED_OBJECT_ROOT_SEND_METHOD],
                [
                    (SEED_FIELD_SMALL_INTEGER, root_id),
                    (SEED_FIELD_OBJECT_INDEX, selector_indices_by_value[SELECTOR_VALUES[SELECTOR_IDS[selector]]]),
                    (SEED_FIELD_SMALL_INTEGER, return_mode),
                ],
            )
        )

    root_value_method_start_index = root_send_method_start_index + len(root_send_method_seed_objects)
    for entry_name, root_id in ROOT_VALUE_METHOD_ROOT_BY_ENTRY_NAME.items():
        root_value_method_indices[entry_name] = root_value_method_start_index + len(root_value_method_seed_objects)
        root_value_method_seed_objects.append(
            SeedObject(
                SEED_OBJECT_ROOT_VALUE_METHOD,
                class_indices[SEED_OBJECT_ROOT_VALUE_METHOD],
                [
                    (SEED_FIELD_SMALL_INTEGER, root_id),
                ],
            )
        )

    interpreted_method_start_index = root_value_method_start_index + len(root_value_method_seed_objects)
    for entry_name, program in INTERPRETED_METHOD_PROGRAM_BY_ENTRY_NAME.items():
        encoded_fields: list[tuple[int, int]] = []

        for opcode_name, operand_a, operand_b in program:
            if opcode_name == "send":
                encoded_fields.append(
                    (
                        SEED_FIELD_SMALL_INTEGER,
                        encode_interpreted_instruction(
                            opcode_name,
                            SELECTOR_VALUES[SELECTOR_IDS[str(operand_a)]],
                            int(operand_b),
                        ),
                    )
                )
            else:
                encoded_fields.append(
                    (
                        SEED_FIELD_SMALL_INTEGER,
                        encode_interpreted_instruction(opcode_name, int(operand_a), int(operand_b)),
                    )
                )
        interpreted_method_indices[entry_name] = interpreted_method_start_index + len(interpreted_method_seed_objects)
        interpreted_method_seed_objects.append(
            SeedObject(
                SEED_OBJECT_INTERPRETED_METHOD,
                class_indices[SEED_OBJECT_INTERPRETED_METHOD],
                encoded_fields,
            )
        )

    compiled_method_start_index = interpreted_method_start_index + len(interpreted_method_seed_objects)
    for entry_name, program in COMPILED_METHOD_PROGRAM_BY_ENTRY_NAME.items():
        encoded_fields = [
            (SEED_FIELD_SMALL_INTEGER, instruction)
            for instruction in program
        ]
        compiled_method_indices[entry_name] = compiled_method_start_index + len(compiled_method_seed_objects)
        compiled_method_seed_objects.append(
            SeedObject(
                SEED_OBJECT_COMPILED_METHOD,
                class_indices[SEED_OBJECT_COMPILED_METHOD],
                encoded_fields,
            )
        )

    method_entry_start_index = compiled_method_start_index + len(compiled_method_seed_objects)
    for entry_name, _owner_kind, _selector, _argument_count in METHOD_ENTRY_DEFINITIONS:
        implementation_kind = SEED_FIELD_NIL
        implementation_index = 0

        if entry_name in accessor_method_indices:
            implementation_kind = SEED_FIELD_OBJECT_INDEX
            implementation_index = accessor_method_indices[entry_name]
        elif entry_name in field_send_method_indices:
            implementation_kind = SEED_FIELD_OBJECT_INDEX
            implementation_index = field_send_method_indices[entry_name]
        elif entry_name in root_send_method_indices:
            implementation_kind = SEED_FIELD_OBJECT_INDEX
            implementation_index = root_send_method_indices[entry_name]
        elif entry_name in root_value_method_indices:
            implementation_kind = SEED_FIELD_OBJECT_INDEX
            implementation_index = root_value_method_indices[entry_name]
        elif entry_name in compiled_method_indices:
            implementation_kind = SEED_FIELD_OBJECT_INDEX
            implementation_index = compiled_method_indices[entry_name]
        elif entry_name in interpreted_method_indices:
            implementation_kind = SEED_FIELD_OBJECT_INDEX
            implementation_index = interpreted_method_indices[entry_name]
        method_entry_indices[entry_name] = method_entry_start_index + len(method_entry_seed_objects)
        method_entry_seed_objects.append(
            SeedObject(
                SEED_OBJECT_METHOD_ENTRY,
                class_indices[SEED_OBJECT_METHOD_ENTRY],
                [
                    (SEED_FIELD_SMALL_INTEGER, METHOD_ENTRY_VALUES[entry_name]),
                    (implementation_kind, implementation_index),
                ],
            )
        )

    method_start_index = method_entry_start_index + len(method_entry_seed_objects)

    for class_kind in class_kinds_in_order:
        method_definitions = BUILTIN_METHODS_BY_KIND.get(class_kind, [])
        method_count_by_kind[class_kind] = len(method_definitions)
        if method_definitions:
            method_start_by_kind[class_kind] = method_start_index + len(method_seed_objects)
        for selector, argument_count, entry_name in method_definitions:
            method_seed_objects.append(
                SeedObject(
                    SEED_OBJECT_METHOD_DESCRIPTOR,
                    class_indices[SEED_OBJECT_METHOD_DESCRIPTOR],
                    [
                        (SEED_FIELD_OBJECT_INDEX, selector_indices_by_value[SELECTOR_VALUES[SELECTOR_IDS[selector]]]),
                        (SEED_FIELD_SMALL_INTEGER, argument_count),
                        (SEED_FIELD_SMALL_INTEGER, class_kind),
                        (SEED_FIELD_OBJECT_INDEX, method_entry_indices[entry_name]),
                    ],
                )
            )

    class_seed_objects = [
        SeedObject(
            SEED_OBJECT_CLASS,
            class_class_index,
            [
                (SEED_FIELD_NIL, 0),
                (SEED_FIELD_SMALL_INTEGER, class_kind),
                (
                    SEED_FIELD_OBJECT_INDEX if class_kind in method_start_by_kind else SEED_FIELD_NIL,
                    method_start_by_kind.get(class_kind, 0),
                ),
                (SEED_FIELD_SMALL_INTEGER, method_count_by_kind.get(class_kind, 0)),
            ],
        )
        for class_kind in class_kinds_in_order
    ]

    seed_objects.extend(class_seed_objects)
    seed_objects.extend(selector_seed_objects)
    seed_objects.extend(accessor_method_seed_objects)
    seed_objects.extend(field_send_method_seed_objects)
    seed_objects.extend(root_send_method_seed_objects)
    seed_objects.extend(root_value_method_seed_objects)
    seed_objects.extend(interpreted_method_seed_objects)
    seed_objects.extend(compiled_method_seed_objects)
    seed_objects.extend(method_entry_seed_objects)
    seed_objects.extend(method_seed_objects)

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
        (SEED_ROOT_TRANSCRIPT_BEHAVIOR, text_behavior_index),
        (SEED_ROOT_TRANSCRIPT_LAYOUT, 6),
        (SEED_ROOT_TRANSCRIPT_STYLE, 7),
        (SEED_ROOT_TRANSCRIPT_METRICS, text_metrics_index),
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
    for seed_object in seed_objects:
        manifest.extend(
            struct.pack(
                SEED_OBJECT_HEADER_FORMAT,
                seed_object.object_kind,
                len(seed_object.fields),
                seed_object.class_index,
            )
        )
        for field_kind, field_value in seed_object.fields + [(SEED_FIELD_NIL, 0)] * (4 - len(seed_object.fields)):
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
