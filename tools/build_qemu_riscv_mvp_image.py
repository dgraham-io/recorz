#!/usr/bin/env python3
"""Lower a tiny compiled Recorz do-it into the MVP RISC-V demo assets."""

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


ROOT = Path(__file__).resolve().parents[1]
KERNEL_MVP_ROOT = ROOT / "kernel" / "mvp"
KERNEL_SOURCE_BUNDLE_ENV = "RECORZ_MVP_KERNEL_SOURCE_BUNDLE"
RUNTIME_SPEC_PATH = ROOT / "platform" / "qemu-riscv64" / "runtime_spec.json"
if str(ROOT / "src") not in sys.path:
    sys.path.insert(0, str(ROOT / "src"))

from recorz.compiler import compile_do_it, compile_method  # noqa: E402
from recorz.model import BindingRef, CompiledBlock, SendSite, SymbolAtom  # noqa: E402


def load_runtime_spec() -> dict[str, object]:
    return json.loads(RUNTIME_SPEC_PATH.read_text(encoding="utf-8"))


def build_named_constant_maps(
    specs: list[tuple[str, str]],
    *,
    start: int = 1,
) -> tuple[dict[str, str], dict[str, int], list[tuple[str, int]]]:
    ids: dict[str, str] = {}
    values: dict[str, int] = {}
    definitions: list[tuple[str, int]] = []

    for index, (name, constant_name) in enumerate(specs, start=start):
        if name in ids:
            raise AssertionError(f"duplicate named constant spec {name!r}")
        if constant_name in values:
            raise AssertionError(f"duplicate constant name {constant_name!r}")
        ids[name] = constant_name
        values[constant_name] = index
        definitions.append((constant_name, index))
    return ids, values, definitions


def build_named_constant_maps_from_explicit_specs(
    specs: list[dict[str, object]],
) -> tuple[dict[str, str], dict[str, int], list[tuple[str, int]]]:
    ids: dict[str, str] = {}
    values: dict[str, int] = {}
    definitions: list[tuple[str, int]] = []

    for spec in specs:
        name = str(spec["name"])
        constant_name = str(spec["constant"])
        value = int(spec["value"])
        if name in ids:
            raise AssertionError(f"duplicate named constant spec {name!r}")
        if constant_name in values:
            raise AssertionError(f"duplicate constant name {constant_name!r}")
        if any(existing_value == value for existing_value in values.values()):
            raise AssertionError(f"duplicate constant value {value!r}")
        ids[name] = constant_name
        values[constant_name] = value
        definitions.append((constant_name, value))
    return ids, values, definitions


def build_constant_value_map_from_explicit_specs(specs: list[dict[str, object]]) -> dict[str, int]:
    values: dict[str, int] = {}
    for spec in specs:
        constant_name = str(spec["constant"])
        value = int(spec["value"])
        if constant_name in values:
            raise AssertionError(f"duplicate constant name {constant_name!r}")
        values[constant_name] = value
    return values


def build_constant_definitions_from_explicit_specs(specs: list[dict[str, object]]) -> list[tuple[str, int]]:
    return [
        (str(spec["constant"]), int(spec["value"]))
        for spec in specs
    ]


def build_named_value_map_from_explicit_specs(specs: list[dict[str, object]]) -> dict[str, int]:
    values: dict[str, int] = {}
    for spec in specs:
        name = str(spec["name"])
        value = int(spec["value"])
        if name in values:
            raise AssertionError(f"duplicate constant name {name!r}")
        values[name] = value
    return values


def constant_value(ids: dict[str, str], values: dict[str, int], name: str) -> int:
    return values[ids[name]]


RUNTIME_SPEC = load_runtime_spec()
PROGRAM_RUNTIME_SPEC = dict(RUNTIME_SPEC["program"])
IMAGE_RUNTIME_SPEC = dict(RUNTIME_SPEC["image"])
SEED_RUNTIME_SPEC = dict(RUNTIME_SPEC["seed"])
COMPILED_METHOD_RUNTIME_SPEC = dict(RUNTIME_SPEC["compiled_method"])
METHOD_UPDATE_RUNTIME_SPEC = dict(RUNTIME_SPEC["method_update"])
METHOD_IMPLEMENTATION_RUNTIME_SPEC = list(RUNTIME_SPEC["method_implementations"])

OPCODE_SPEC = list(RUNTIME_SPEC["opcodes"])
LITERAL_KIND_SPEC = list(RUNTIME_SPEC["literal_kinds"])
SEED_FIELD_KIND_SPEC = list(SEED_RUNTIME_SPEC["field_kinds"])
COMPILED_METHOD_OPCODE_SPEC = list(COMPILED_METHOD_RUNTIME_SPEC["opcodes"])

OPCODE_CONSTANT_NAMES = {str(spec["name"]): str(spec["constant"]) for spec in OPCODE_SPEC}
LITERAL_KIND_CONSTANT_NAMES = {str(spec["name"]): str(spec["constant"]) for spec in LITERAL_KIND_SPEC}

OP_PUSH_GLOBAL = OPCODE_CONSTANT_NAMES["push_global"]
OP_PUSH_LITERAL = OPCODE_CONSTANT_NAMES["push_literal"]
OP_PUSH_LEXICAL = OPCODE_CONSTANT_NAMES["push_lexical"]
OP_STORE_LEXICAL = OPCODE_CONSTANT_NAMES["store_lexical"]
OP_SEND = OPCODE_CONSTANT_NAMES["send"]
OP_DUP = OPCODE_CONSTANT_NAMES["dup"]
OP_POP = OPCODE_CONSTANT_NAMES["pop"]
OP_RETURN = OPCODE_CONSTANT_NAMES["return"]
OP_PUSH_NIL = OPCODE_CONSTANT_NAMES["push_nil"]
OP_PUSH_SELF = OPCODE_CONSTANT_NAMES["push_self"]
OP_PUSH_BLOCK_LITERAL = OPCODE_CONSTANT_NAMES["push_block_literal"]
OP_PUSH_THIS_CONTEXT = OPCODE_CONSTANT_NAMES["push_this_context"]

LITERAL_STRING = LITERAL_KIND_CONSTANT_NAMES["string"]
LITERAL_SMALL_INTEGER = LITERAL_KIND_CONSTANT_NAMES["small_integer"]

OPCODE_VALUES = build_constant_value_map_from_explicit_specs(OPCODE_SPEC)
OPCODE_DEFINITIONS = build_constant_definitions_from_explicit_specs(OPCODE_SPEC)
LITERAL_VALUES = build_constant_value_map_from_explicit_specs(LITERAL_KIND_SPEC)
LITERAL_KIND_DEFINITIONS = build_constant_definitions_from_explicit_specs(LITERAL_KIND_SPEC)

PROGRAM_MAGIC = str(PROGRAM_RUNTIME_SPEC["magic"]).encode("ascii")
PROGRAM_VERSION = int(PROGRAM_RUNTIME_SPEC["version"])
PROGRAM_HEADER_FORMAT = str(PROGRAM_RUNTIME_SPEC["header_format"])
PROGRAM_LEXICAL_LIMIT = int(PROGRAM_RUNTIME_SPEC["lexical_limit"])
PROGRAM_LEXICAL_NAME_LIMIT = int(PROGRAM_RUNTIME_SPEC["lexical_name_limit"])
PROGRAM_INSTRUCTION_FORMAT = str(PROGRAM_RUNTIME_SPEC["instruction_format"])
PROGRAM_LITERAL_HEADER_FORMAT = str(PROGRAM_RUNTIME_SPEC["literal_header_format"])

IMAGE_MAGIC = str(IMAGE_RUNTIME_SPEC["magic"]).encode("ascii")
IMAGE_VERSION = int(IMAGE_RUNTIME_SPEC["version"])
IMAGE_HEADER_FORMAT = str(IMAGE_RUNTIME_SPEC["header_format"])
IMAGE_SECTION_FORMAT = str(IMAGE_RUNTIME_SPEC["section_format"])
IMAGE_SECTION_PROGRAM = int(IMAGE_RUNTIME_SPEC["sections"]["program"])
IMAGE_SECTION_SEED = int(IMAGE_RUNTIME_SPEC["sections"]["seed"])
IMAGE_SECTION_ENTRY = int(IMAGE_RUNTIME_SPEC["sections"]["entry"])
IMAGE_FEATURE_FNV1A32 = int(IMAGE_RUNTIME_SPEC["features"]["fnv1a32"])
IMAGE_PROFILE = str(IMAGE_RUNTIME_SPEC["profile"]).encode("ascii")
IMAGE_ENTRY_MAGIC = str(IMAGE_RUNTIME_SPEC["entry"]["magic"]).encode("ascii")
IMAGE_ENTRY_VERSION = int(IMAGE_RUNTIME_SPEC["entry"]["version"])
IMAGE_ENTRY_FORMAT = str(IMAGE_RUNTIME_SPEC["entry"]["format"])
IMAGE_ENTRY_KIND_DOIT = int(IMAGE_RUNTIME_SPEC["entry"]["kinds"]["doit"])

SEED_MAGIC = str(SEED_RUNTIME_SPEC["magic"]).encode("ascii")
SEED_VERSION = int(SEED_RUNTIME_SPEC["version"])
SEED_HEADER_FORMAT = str(SEED_RUNTIME_SPEC["header_format"])
SEED_BINDING_FORMAT = str(SEED_RUNTIME_SPEC["binding_format"])
SEED_OBJECT_HEADER_FORMAT = str(SEED_RUNTIME_SPEC["object_header_format"])
SEED_FIELD_FORMAT = str(SEED_RUNTIME_SPEC["field_format"])
SEED_INVALID_OBJECT_INDEX = int(SEED_RUNTIME_SPEC["invalid_object_index"])
COMPILED_METHOD_MAX_INSTRUCTIONS = int(COMPILED_METHOD_RUNTIME_SPEC["max_instructions"])
METHOD_UPDATE_MAGIC = str(METHOD_UPDATE_RUNTIME_SPEC["magic"]).encode("ascii")
METHOD_UPDATE_VERSION = int(METHOD_UPDATE_RUNTIME_SPEC["version"])
METHOD_UPDATE_HEADER_FORMAT = str(METHOD_UPDATE_RUNTIME_SPEC["header_format"])
METHOD_UPDATE_FW_CFG_NAME = str(METHOD_UPDATE_RUNTIME_SPEC["fw_cfg_name"])

SEED_FIELD_KIND_IDS, SEED_FIELD_KIND_VALUES, SEED_FIELD_KIND_DEFINITIONS = (
    build_named_constant_maps_from_explicit_specs(SEED_FIELD_KIND_SPEC)
)
SEED_FIELD_KIND_SPECS = [
    (str(spec["name"]), str(spec["constant"]))
    for spec in SEED_FIELD_KIND_SPEC
]

SEED_FIELD_NIL = constant_value(SEED_FIELD_KIND_IDS, SEED_FIELD_KIND_VALUES, "nil")
SEED_FIELD_SMALL_INTEGER = constant_value(SEED_FIELD_KIND_IDS, SEED_FIELD_KIND_VALUES, "small_integer")
SEED_FIELD_OBJECT_INDEX = constant_value(SEED_FIELD_KIND_IDS, SEED_FIELD_KIND_VALUES, "object_index")

FIELD_SPEC_SMALL_INTEGER = "small_integer"
FIELD_SPEC_OBJECT_REF = "object_ref"
FIELD_SPEC_GLYPH_REF = "glyph_ref"


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
    lexical_names: list[str]


@dataclass
class SeedObject:
    object_kind: int
    class_index: int
    fields: list[tuple[int, int]]


@dataclass(frozen=True)
class KernelMethodSource:
    entry_name: str
    class_name: str
    instance_variables: tuple[str, ...]
    relative_path: str
    selector: str
    argument_count: int
    implementation_kind: str
    primitive_binding: str | None
    source_text: str


@dataclass(frozen=True)
class KernelBootObjectDeclaration:
    name: str
    family_name: str
    family_order: int
    class_name: str
    field_specs: tuple[tuple[str, object], ...]
    global_exports: tuple[str, ...]
    root_exports: tuple[str, ...]
    relative_path: str


@dataclass(frozen=True)
class KernelSelectorDeclaration:
    selector: str
    selector_order: int
    relative_path: str


@dataclass(frozen=True)
class KernelPrimitiveBindingOwner:
    binding_name: str
    entry_name: str
    class_name: str
    selector: str
    argument_count: int
    relative_path: str


@dataclass(frozen=True)
class BuilderOwnershipSummary:
    runtime_spec_path: str
    source_root: str
    generated_text_outputs: tuple[str, ...]
    generated_binary_outputs: tuple[str, ...]
    derived_metadata_surfaces: tuple[str, ...]


@dataclass(frozen=True)
class ImageManifestLayout:
    section_count: int
    feature_flags: int
    entry_offset: int
    program_offset: int
    seed_offset: int
    entry_length: int
    program_length: int
    seed_length: int


@dataclass(frozen=True)
class KernelRootDeclaration:
    root_name: str
    object_name: str
    root_order: int
    relative_path: str


@dataclass(frozen=True)
class KernelGlyphBitmapFamilyDeclaration:
    name_prefix: str
    family_name: str
    class_name: str
    width: int
    height: int
    storage_kind: int
    count: int
    relative_path: str


@dataclass(frozen=True)
class KernelClassHeader:
    class_name: str
    descriptor_order: int
    object_kind_order: int
    source_boot_order: int | None
    instance_variables: tuple[str, ...]


@dataclass(frozen=True)
class KernelSourceUnit:
    relative_path: str
    chunk_sources: tuple[str, ...]


@dataclass(frozen=True)
class BootObjectSpec:
    name: str
    object_kind_name: str
    field_specs: tuple[tuple[str, object], ...]
    global_exports: tuple[str, ...] = ()
    root_exports: tuple[str, ...] = ()


@dataclass(frozen=True)
class BootObjectFamilySpec:
    name: str
    object_specs: tuple[BootObjectSpec, ...]
    collect_object_indices: bool = False


@dataclass(frozen=True)
class FixedBootGraphSpec:
    family_specs: tuple[BootObjectFamilySpec, ...]


@dataclass(frozen=True)
class SeedLayoutSection:
    start_index: int
    count: int


@dataclass(frozen=True)
class SeedLayoutSectionSpec:
    name: str
    count_source: str


@dataclass(frozen=True)
class DynamicSeedObjectSectionSpec:
    layout_section_name: str


@dataclass(frozen=True)
class DynamicSeedBuildStepResult:
    seed_objects: list[SeedObject]
    state_updates: dict[str, object]


@dataclass(frozen=True)
class DynamicSeedBuildStepSpec:
    layout_section_name: str
    builder: Callable[[DynamicSeedBuildState], DynamicSeedBuildStepResult]
    required_layout_sections: tuple[str, ...] = ()
    required_state_fields: tuple[str, ...] = ()
    state_update_fields: tuple[str, ...] = ()


@dataclass(frozen=True)
class DynamicSeedSectionSpec:
    layout_section_name: str
    count_source: str
    builder: Callable[[DynamicSeedBuildState], DynamicSeedBuildStepResult]
    build_order: int
    required_layout_sections: tuple[str, ...] = ()
    required_state_fields: tuple[str, ...] = ()
    state_update_fields: tuple[str, ...] = ()


@dataclass(frozen=True)
class BootImageOrderingSpec:
    class_kind_order: tuple[int, ...]
    selector_value_order: tuple[int, ...]
    compiled_method_entry_order: tuple[str, ...]
    method_entry_order: tuple[str, ...]


@dataclass(frozen=True)
class BootImageSpec:
    fixed_boot_graph_spec: FixedBootGraphSpec
    ordering_spec: BootImageOrderingSpec
    dynamic_seed_section_specs: tuple[DynamicSeedSectionSpec, ...]


@dataclass(frozen=True)
class BootImageSeedBuildContext:
    boot_image_spec: BootImageSpec
    class_kind_order: tuple[int, ...]
    selector_value_order: tuple[int, ...]
    compiled_method_entry_order: tuple[str, ...]
    method_entry_order: tuple[str, ...]
    initial_dynamic_seed_state_fields: tuple[str, ...]
    fixed_boot_object_count: int
    glyph_bitmap_boot_specs: tuple[BootObjectSpec, ...]
    seed_layout_section_specs: tuple[SeedLayoutSectionSpec, ...]
    dynamic_seed_object_section_specs: tuple[DynamicSeedObjectSectionSpec, ...]
    dynamic_seed_build_step_specs: tuple[DynamicSeedBuildStepSpec, ...]
    global_name_to_boot_object_name: dict[str, str]
    seed_root_name_to_boot_object_name: dict[str, str]


@dataclass
class DynamicSeedBuildState:
    seed_layout: dict[str, SeedLayoutSection]
    class_kind_order: list[int]
    class_class_index: int
    class_indices: dict[int, int]
    selector_value_order: tuple[int, ...]
    compiled_method_entry_order: tuple[str, ...]
    method_entry_order: tuple[str, ...]
    selector_indices_by_value: dict[int, int]
    compiled_method_indices: dict[str, int]
    method_entry_indices: dict[str, int]
    method_start_by_kind: dict[int, int]
    method_count_by_kind: dict[int, int]


@dataclass
class DynamicSeedSections:
    seed_layout: dict[str, SeedLayoutSection]
    class_indices: dict[int, int]
    object_sections: dict[str, list[SeedObject]]

    def seed_objects_for_layout_section(self, layout_section_name: str) -> list[SeedObject]:
        return self.object_sections[layout_section_name]

    @property
    def class_seed_objects(self) -> list[SeedObject]:
        return self.seed_objects_for_layout_section("class_descriptors")

    @property
    def selector_seed_objects(self) -> list[SeedObject]:
        return self.seed_objects_for_layout_section("selectors")

    @property
    def compiled_method_seed_objects(self) -> list[SeedObject]:
        return self.seed_objects_for_layout_section("compiled_methods")

    @property
    def method_entry_seed_objects(self) -> list[SeedObject]:
        return self.seed_objects_for_layout_section("method_entries")

    @property
    def method_seed_objects(self) -> list[SeedObject]:
        return self.seed_objects_for_layout_section("method_descriptors")


@dataclass
class SeedBindings:
    global_bindings: list[tuple[int, int]]
    root_bindings: list[tuple[int, int]]


def build_small_integer_boot_field_specs(field_values: tuple[int, ...]) -> tuple[tuple[str, int], ...]:
    return tuple((FIELD_SPEC_SMALL_INTEGER, field_value) for field_value in field_values)


def build_boot_object_specs_from_declarations(
    boot_object_declarations_by_name: dict[str, KernelBootObjectDeclaration],
    family_name: str,
) -> tuple[BootObjectSpec, ...]:
    family_declarations = sorted(
        (
            declaration
            for declaration in boot_object_declarations_by_name.values()
            if declaration.family_name == family_name
        ),
        key=lambda declaration: declaration.family_order,
    )
    expected_family_orders = list(range(len(family_declarations)))
    actual_family_orders = [declaration.family_order for declaration in family_declarations]
    if actual_family_orders != expected_family_orders:
        raise LoweringError(
            f"kernel MVP boot family {family_name!r} must declare a contiguous order range starting at 0"
        )
    return tuple(
        BootObjectSpec(
            declaration.name,
            declaration.class_name,
            declaration.field_specs,
            declaration.global_exports,
            declaration.root_exports,
        )
        for declaration in family_declarations
    )


def build_fixed_boot_graph_spec(
    boot_object_declarations_by_name: dict[str, KernelBootObjectDeclaration],
    glyph_bitmap_family_declaration: KernelGlyphBitmapFamilyDeclaration,
) -> FixedBootGraphSpec:
    return FixedBootGraphSpec(
        (
            BootObjectFamilySpec(
                "before_glyphs",
                build_boot_object_specs_from_declarations(boot_object_declarations_by_name, "before_glyphs"),
            ),
            BootObjectFamilySpec(
                glyph_bitmap_family_declaration.family_name,
                tuple(build_glyph_bitmap_boot_specs(glyph_bitmap_family_declaration)),
                collect_object_indices=True,
            ),
            BootObjectFamilySpec(
                "after_glyphs",
                build_boot_object_specs_from_declarations(boot_object_declarations_by_name, "after_glyphs"),
            ),
        )
    )


def build_boot_object_family_spec_map(fixed_boot_graph_spec: FixedBootGraphSpec) -> dict[str, BootObjectFamilySpec]:
    family_spec_map: dict[str, BootObjectFamilySpec] = {}

    for family_spec in fixed_boot_graph_spec.family_specs:
        if family_spec.name in family_spec_map:
            raise AssertionError(f"duplicate boot object family spec name {family_spec.name!r}")
        family_spec_map[family_spec.name] = family_spec
    return family_spec_map


def flatten_boot_object_specs(fixed_boot_graph_spec: FixedBootGraphSpec) -> list[BootObjectSpec]:
    return [
        object_spec
        for family_spec in fixed_boot_graph_spec.family_specs
        for object_spec in family_spec.object_specs
    ]


def build_boot_object_spec_map(object_specs: list[BootObjectSpec]) -> dict[str, BootObjectSpec]:
    spec_map: dict[str, BootObjectSpec] = {}

    for object_spec in object_specs:
        if object_spec.name in spec_map:
            raise AssertionError(f"duplicate boot object spec name {object_spec.name!r}")
        spec_map[object_spec.name] = object_spec
    return spec_map


def build_boot_object_export_map(
    object_specs: list[BootObjectSpec],
    export_kind: str,
) -> dict[str, str]:
    export_map: dict[str, str] = {}

    for object_spec in object_specs:
        export_names = object_spec.global_exports if export_kind == "global" else object_spec.root_exports
        for export_name in export_names:
            if export_name in export_map:
                raise AssertionError(f"duplicate boot object export {export_name!r}")
            export_map[export_name] = object_spec.name

    return export_map
KERNEL_METHOD_IMPLEMENTATION_COMPILED = "compiled"
KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE = "primitive"
METHOD_IMPLEMENTATION_IDS, METHOD_IMPLEMENTATION_VALUES, METHOD_IMPLEMENTATION_DEFINITIONS = (
    build_named_constant_maps_from_explicit_specs(METHOD_IMPLEMENTATION_RUNTIME_SPEC)
)
KERNEL_CLASS_HEADER_PATTERN = re.compile(
    r"^RecorzKernelClass:\s*#(?P<class_name>[A-Za-z_]\w*)\s+descriptorOrder:\s*(?P<descriptor_order>\d+)"
    r"\s+objectKindOrder:\s*(?P<object_kind_order>\d+)"
    r"(?:\s+sourceBootOrder:\s*(?P<source_boot_order>\d+))?\s+instanceVariableNames:\s*'(?P<instance_variables>[^']*)'$"
)
KERNEL_BOOT_OBJECT_HEADER_PATTERN = re.compile(
    r"^RecorzKernelBootObject:\s*#(?P<name>[A-Za-z_]\w*)\s+family:\s*#(?P<family_name>[A-Za-z_]\w*)"
    r"\s+order:\s*(?P<family_order>\d+)\s+class:\s*#(?P<class_name>[A-Za-z_]\w*)$"
)
KERNEL_SELECTOR_PATTERN = re.compile(
    r"^RecorzKernelSelector:\s*#(?P<selector>\S+)\s+order:\s*(?P<selector_order>\d+)$"
)
KERNEL_ROOT_PATTERN = re.compile(
    r"^RecorzKernelRoot:\s*#(?P<root_name>[A-Za-z_]\w*)\s+object:\s*#(?P<object_name>[A-Za-z_]\w*)\s+order:\s*(?P<root_order>\d+)$"
)
KERNEL_GLYPH_BITMAP_FAMILY_PATTERN = re.compile(
    r"^RecorzKernelGlyphBitmapFamily:\s*#(?P<name_prefix>[A-Za-z_]\w*)\s+family:\s*#(?P<family_name>[A-Za-z_]\w*)"
    r"\s+class:\s*#(?P<class_name>[A-Za-z_]\w*)\s+width:\s*(?P<width>\d+)\s+height:\s*(?P<height>\d+)"
    r"\s+storageKind:\s*(?P<storage_kind>\d+)\s+count:\s*(?P<count>\d+)$"
)
KERNEL_BOOT_OBJECT_ATTRIBUTE_PATTERN = re.compile(
    r"^(?P<attribute_name>fields|globalExports|rootExports):\s*'(?P<attribute_value>[^']*)'$"
)
KERNEL_PRIMITIVE_DECLARATION_PATTERN = re.compile(r"^<primitive:\s*#(?P<binding>[A-Za-z_]\w*)>$")
COMPILED_METHOD_OPCODE_IDS, COMPILED_METHOD_OPCODE_VALUES, COMPILED_METHOD_OPCODE_DEFINITIONS = (
    build_named_constant_maps_from_explicit_specs(COMPILED_METHOD_OPCODE_SPEC)
)
COMPILED_METHOD_OP_PUSH_GLOBAL = constant_value(COMPILED_METHOD_OPCODE_IDS, COMPILED_METHOD_OPCODE_VALUES, "push_global")
COMPILED_METHOD_OP_PUSH_ROOT = constant_value(COMPILED_METHOD_OPCODE_IDS, COMPILED_METHOD_OPCODE_VALUES, "push_root")
COMPILED_METHOD_OP_PUSH_ARGUMENT = constant_value(
    COMPILED_METHOD_OPCODE_IDS,
    COMPILED_METHOD_OPCODE_VALUES,
    "push_argument",
)
COMPILED_METHOD_OP_PUSH_FIELD = constant_value(COMPILED_METHOD_OPCODE_IDS, COMPILED_METHOD_OPCODE_VALUES, "push_field")
COMPILED_METHOD_OP_SEND = constant_value(COMPILED_METHOD_OPCODE_IDS, COMPILED_METHOD_OPCODE_VALUES, "send")
COMPILED_METHOD_OP_RETURN_TOP = constant_value(COMPILED_METHOD_OPCODE_IDS, COMPILED_METHOD_OPCODE_VALUES, "return_top")
COMPILED_METHOD_OP_RETURN_RECEIVER = constant_value(
    COMPILED_METHOD_OPCODE_IDS,
    COMPILED_METHOD_OPCODE_VALUES,
    "return_receiver",
)
COMPILED_METHOD_OP_PUSH_THIS_CONTEXT = constant_value(
    COMPILED_METHOD_OPCODE_IDS,
    COMPILED_METHOD_OPCODE_VALUES,
    "push_this_context",
)


def encode_compiled_method_instruction(opcode_name: str, operand_a: int = 0, operand_b: int = 0) -> int:
    opcode = constant_value(COMPILED_METHOD_OPCODE_IDS, COMPILED_METHOD_OPCODE_VALUES, opcode_name)
    if opcode_name == "send":
        return opcode | ((operand_a & 0xFFFF) << 8) | ((operand_b & 0xFF) << 24)
    return opcode | ((operand_a & 0xFF) << 8) | ((operand_b & 0xFFFF) << 16)


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


def parse_kernel_class_header(header_source: str, relative_path: str) -> KernelClassHeader:
    normalized_header = " ".join(line.strip() for line in header_source.splitlines() if line.strip())
    match = KERNEL_CLASS_HEADER_PATTERN.fullmatch(normalized_header)

    if match is None:
        raise LoweringError(f"kernel MVP class file {relative_path} has an invalid class header")
    instance_variables = tuple(name for name in match.group("instance_variables").split() if name)
    return KernelClassHeader(
        class_name=match.group("class_name"),
        descriptor_order=int(match.group("descriptor_order")),
        object_kind_order=int(match.group("object_kind_order")),
        source_boot_order=int(match.group("source_boot_order")) if match.group("source_boot_order") is not None else None,
        instance_variables=instance_variables,
    )


def load_kernel_source_units_from_directory() -> list[KernelSourceUnit]:
    source_units: list[KernelSourceUnit] = []

    for method_path in sorted(KERNEL_MVP_ROOT.glob("*.rz")):
        chunk_sources = split_kernel_method_chunks(method_path.read_text(encoding="utf-8"))
        if not chunk_sources:
            raise LoweringError(f"kernel MVP class file {method_path.name} is empty")
        source_units.append(KernelSourceUnit(method_path.name, tuple(chunk_sources)))
    return source_units


def load_kernel_source_units_from_bundle(bundle_path: Path) -> list[KernelSourceUnit]:
    source_units: list[KernelSourceUnit] = []
    current_chunks: list[str] = []
    current_relative_path: str | None = None
    bundle_chunks = split_kernel_method_chunks(bundle_path.read_text(encoding="utf-8"))

    if not bundle_chunks:
        raise LoweringError(f"kernel source bundle {bundle_path} is empty")
    for chunk_source in bundle_chunks:
        stripped = chunk_source.lstrip()
        if stripped.startswith("RecorzKernelClass:"):
            if current_chunks:
                source_units.append(KernelSourceUnit(current_relative_path or bundle_path.name, tuple(current_chunks)))
            class_header = parse_kernel_class_header(chunk_source, bundle_path.name)
            current_chunks = [chunk_source]
            current_relative_path = f"{class_header.class_name}.rz"
            continue
        if not current_chunks:
            raise LoweringError(
                f"kernel source bundle {bundle_path} contains a non-class chunk before the first class header"
            )
        current_chunks.append(chunk_source)
    if current_chunks:
        source_units.append(KernelSourceUnit(current_relative_path or bundle_path.name, tuple(current_chunks)))
    if not source_units:
        raise LoweringError(f"kernel source bundle {bundle_path} did not declare any kernel classes")
    return source_units


def load_kernel_source_units() -> list[KernelSourceUnit]:
    bundle_path_text = os.environ.get(KERNEL_SOURCE_BUNDLE_ENV)

    if bundle_path_text:
        return load_kernel_source_units_from_bundle(Path(bundle_path_text))
    return load_kernel_source_units_from_directory()


def parse_kernel_boot_object_field_specs(field_source: str, relative_path: str) -> tuple[tuple[str, object], ...]:
    field_specs: list[tuple[str, object]] = []

    for token in field_source.split():
        if ":" not in token:
            raise LoweringError(
                f"kernel MVP boot object field {token!r} in {relative_path} must use kind:value syntax"
            )
        field_kind_name, field_value = token.split(":", 1)
        if field_kind_name == "small":
            field_specs.append((FIELD_SPEC_SMALL_INTEGER, int(field_value)))
            continue
        if field_kind_name == "object":
            field_specs.append((FIELD_SPEC_OBJECT_REF, field_value))
            continue
        if field_kind_name == "glyph":
            field_specs.append((FIELD_SPEC_GLYPH_REF, int(field_value)))
            continue
        raise LoweringError(
            f"kernel MVP boot object field kind {field_kind_name!r} in {relative_path} is unsupported"
        )

    return tuple(field_specs)


def parse_kernel_boot_object_chunk(chunk_source: str, relative_path: str) -> KernelBootObjectDeclaration:
    lines = [line.strip() for line in chunk_source.splitlines() if line.strip()]
    if not lines:
        raise LoweringError(f"kernel MVP boot object chunk in {relative_path} is empty")
    header_match = KERNEL_BOOT_OBJECT_HEADER_PATTERN.fullmatch(lines[0])
    if header_match is None:
        raise LoweringError(f"kernel MVP boot object chunk in {relative_path} has an invalid header")

    attributes: dict[str, str] = {}
    for line in lines[1:]:
        attribute_match = KERNEL_BOOT_OBJECT_ATTRIBUTE_PATTERN.fullmatch(line)
        if attribute_match is None:
            raise LoweringError(f"kernel MVP boot object chunk in {relative_path} has an invalid attribute line")
        attribute_name = attribute_match.group("attribute_name")
        if attribute_name in attributes:
            raise LoweringError(
                f"kernel MVP boot object chunk in {relative_path} repeats attribute {attribute_name!r}"
            )
        attributes[attribute_name] = attribute_match.group("attribute_value")

    return KernelBootObjectDeclaration(
        name=header_match.group("name"),
        family_name=re.sub(r"(?<!^)(?=[A-Z])", "_", header_match.group("family_name")).lower(),
        family_order=int(header_match.group("family_order")),
        class_name=header_match.group("class_name"),
        field_specs=parse_kernel_boot_object_field_specs(attributes.get("fields", ""), relative_path),
        global_exports=tuple(name for name in attributes.get("globalExports", "").split() if name),
        root_exports=tuple(name for name in attributes.get("rootExports", "").split() if name),
        relative_path=relative_path,
    )


def parse_kernel_selector_chunk(chunk_source: str, relative_path: str) -> KernelSelectorDeclaration:
    normalized_header = " ".join(line.strip() for line in chunk_source.splitlines() if line.strip())
    match = KERNEL_SELECTOR_PATTERN.fullmatch(normalized_header)
    if match is None:
        raise LoweringError(f"kernel MVP selector chunk in {relative_path} has an invalid header")
    return KernelSelectorDeclaration(
        selector=match.group("selector"),
        selector_order=int(match.group("selector_order")),
        relative_path=relative_path,
    )


def parse_kernel_root_chunk(chunk_source: str, relative_path: str) -> KernelRootDeclaration:
    normalized_header = " ".join(line.strip() for line in chunk_source.splitlines() if line.strip())
    match = KERNEL_ROOT_PATTERN.fullmatch(normalized_header)
    if match is None:
        raise LoweringError(f"kernel MVP root chunk in {relative_path} has an invalid header")
    return KernelRootDeclaration(
        root_name=match.group("root_name"),
        object_name=match.group("object_name"),
        root_order=int(match.group("root_order")),
        relative_path=relative_path,
    )


def parse_kernel_glyph_bitmap_family_chunk(
    chunk_source: str,
    relative_path: str,
) -> KernelGlyphBitmapFamilyDeclaration:
    normalized_header = " ".join(line.strip() for line in chunk_source.splitlines() if line.strip())
    match = KERNEL_GLYPH_BITMAP_FAMILY_PATTERN.fullmatch(normalized_header)
    if match is None:
        raise LoweringError(f"kernel MVP glyph bitmap family chunk in {relative_path} has an invalid header")
    return KernelGlyphBitmapFamilyDeclaration(
        name_prefix=match.group("name_prefix"),
        family_name=re.sub(r"(?<!^)(?=[A-Z])", "_", match.group("family_name")).lower(),
        class_name=match.group("class_name"),
        width=int(match.group("width")),
        height=int(match.group("height")),
        storage_kind=int(match.group("storage_kind")),
        count=int(match.group("count")),
        relative_path=relative_path,
    )


def load_kernel_class_headers() -> dict[str, KernelClassHeader]:
    class_headers_by_name: dict[str, KernelClassHeader] = {}

    for source_unit in KERNEL_SOURCE_UNITS:
        class_header = parse_kernel_class_header(source_unit.chunk_sources[0], source_unit.relative_path)
        if class_header.class_name in class_headers_by_name:
            raise LoweringError(f"kernel MVP class {class_header.class_name} is declared more than once")
        class_headers_by_name[class_header.class_name] = class_header

    descriptor_headers = sorted(
        class_headers_by_name.values(),
        key=lambda class_header: class_header.descriptor_order,
    )
    expected_descriptor_orders = list(range(len(descriptor_headers)))
    actual_descriptor_orders = [class_header.descriptor_order for class_header in descriptor_headers]
    if actual_descriptor_orders != expected_descriptor_orders:
        raise LoweringError(
            "kernel MVP class headers must declare a contiguous descriptorOrder range starting at 0"
        )
    return class_headers_by_name


def load_kernel_class_relative_paths() -> dict[str, str]:
    class_relative_paths: dict[str, str] = {}

    for source_unit in KERNEL_SOURCE_UNITS:
        class_header = parse_kernel_class_header(source_unit.chunk_sources[0], source_unit.relative_path)
        if class_header.class_name in class_relative_paths:
            raise LoweringError(f"kernel MVP class {class_header.class_name} is declared more than once")
        class_relative_paths[class_header.class_name] = source_unit.relative_path

    return class_relative_paths


def object_kind_constant_name_from_class_name(class_name: str) -> str:
    if class_name == "BitBlt":
        return "RECORZ_MVP_OBJECT_BITBLT"
    return f"RECORZ_MVP_OBJECT_{re.sub(r'(?<!^)(?=[A-Z])', '_', class_name).upper()}"


KERNEL_SOURCE_UNITS = load_kernel_source_units()
KERNEL_CLASS_HEADERS_BY_NAME = load_kernel_class_headers()
KERNEL_CLASS_RELATIVE_PATHS_BY_NAME = load_kernel_class_relative_paths()
KERNEL_CLASS_HEADERS_IN_DESCRIPTOR_ORDER = sorted(
    KERNEL_CLASS_HEADERS_BY_NAME.values(),
    key=lambda class_header: class_header.descriptor_order,
)
KERNEL_CLASS_HEADERS_IN_OBJECT_KIND_ORDER = sorted(
    KERNEL_CLASS_HEADERS_BY_NAME.values(),
    key=lambda class_header: class_header.object_kind_order,
)
KERNEL_SOURCE_CLASS_HEADERS = sorted(
    (
        class_header
        for class_header in KERNEL_CLASS_HEADERS_BY_NAME.values()
        if class_header.source_boot_order is not None
    ),
    key=lambda class_header: class_header.source_boot_order if class_header.source_boot_order is not None else -1,
)
expected_source_boot_orders = list(range(len(KERNEL_SOURCE_CLASS_HEADERS)))
actual_source_boot_orders = [class_header.source_boot_order for class_header in KERNEL_SOURCE_CLASS_HEADERS]
if actual_source_boot_orders != expected_source_boot_orders:
    raise LoweringError("kernel MVP source class headers must declare a contiguous sourceBootOrder range starting at 0")
expected_object_kind_orders = list(range(len(KERNEL_CLASS_HEADERS_IN_OBJECT_KIND_ORDER)))
actual_object_kind_orders = [class_header.object_kind_order for class_header in KERNEL_CLASS_HEADERS_IN_OBJECT_KIND_ORDER]
if actual_object_kind_orders != expected_object_kind_orders:
    raise LoweringError("kernel MVP class headers must declare a contiguous objectKindOrder range starting at 0")
CLASS_DESCRIPTOR_KIND_NAMES = [class_header.class_name for class_header in KERNEL_CLASS_HEADERS_IN_DESCRIPTOR_ORDER]
KERNEL_CLASS_BOOT_ORDER = [class_header.class_name for class_header in KERNEL_SOURCE_CLASS_HEADERS]
OBJECT_KIND_SPECS = [
    (class_header.class_name, object_kind_constant_name_from_class_name(class_header.class_name))
    for class_header in KERNEL_CLASS_HEADERS_IN_OBJECT_KIND_ORDER
]
OBJECT_KIND_IDS, OBJECT_KIND_VALUES, OBJECT_KIND_DEFINITIONS = build_named_constant_maps(OBJECT_KIND_SPECS)
MAX_SEED_OBJECT_KIND = max(OBJECT_KIND_VALUES.values())
KERNEL_CLASS_NAME_TO_OBJECT_KIND = {
    class_name: constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, class_name)
    for class_name in KERNEL_CLASS_HEADERS_BY_NAME
}
CLASS_DESCRIPTOR_KIND_ORDER = [
    KERNEL_CLASS_NAME_TO_OBJECT_KIND[kind_name]
    for kind_name in CLASS_DESCRIPTOR_KIND_NAMES
]
SEED_OBJECT_TRANSCRIPT = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "Transcript")
SEED_OBJECT_DISPLAY = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "Display")
SEED_OBJECT_FORM = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "Form")
SEED_OBJECT_BITMAP = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "Bitmap")
SEED_OBJECT_BITBLT = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "BitBlt")
SEED_OBJECT_GLYPHS = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "Glyphs")
SEED_OBJECT_FORM_FACTORY = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "FormFactory")
SEED_OBJECT_BITMAP_FACTORY = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "BitmapFactory")
SEED_OBJECT_TEXT_LAYOUT = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "TextLayout")
SEED_OBJECT_TEXT_STYLE = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "TextStyle")
SEED_OBJECT_TEXT_METRICS = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "TextMetrics")
SEED_OBJECT_TEXT_BEHAVIOR = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "TextBehavior")
SEED_OBJECT_CLASS = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "Class")
SEED_OBJECT_METHOD_DESCRIPTOR = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "MethodDescriptor")
SEED_OBJECT_METHOD_ENTRY = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "MethodEntry")
SEED_OBJECT_SELECTOR = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "Selector")
SEED_OBJECT_COMPILED_METHOD = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "CompiledMethod")
SEED_OBJECT_KERNEL_INSTALLER = constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, "KernelInstaller")


def kernel_instance_variable_index(class_name: str, field_name: str) -> int:
    class_header = KERNEL_CLASS_HEADERS_BY_NAME[class_name]
    try:
        return class_header.instance_variables.index(field_name)
    except ValueError as error:
        raise LoweringError(
            f"kernel MVP class #{class_name} does not declare instance variable {field_name!r}"
        ) from error


def materialize_named_seed_fields(
    class_name: str,
    field_values_by_name: dict[str, tuple[int, int]],
) -> list[tuple[int, int]]:
    class_header = KERNEL_CLASS_HEADERS_BY_NAME[class_name]
    instance_variables = class_header.instance_variables
    unknown_field_names = sorted(set(field_values_by_name) - set(instance_variables))
    if unknown_field_names:
        raise LoweringError(
            f"kernel MVP class #{class_name} does not declare instance variables "
            + ", ".join(repr(field_name) for field_name in unknown_field_names)
        )
    fields = [
        field_values_by_name.get(field_name, (SEED_FIELD_NIL, 0))
        for field_name in instance_variables
    ]
    while fields and fields[-1] == (SEED_FIELD_NIL, 0):
        fields.pop()
    return fields


CLASS_FIELD_SUPERCLASS = kernel_instance_variable_index("Class", "superclass")
CLASS_FIELD_INSTANCE_KIND = kernel_instance_variable_index("Class", "instanceKind")
CLASS_FIELD_METHOD_START = kernel_instance_variable_index("Class", "methodStart")
CLASS_FIELD_METHOD_COUNT = kernel_instance_variable_index("Class", "methodCount")
METHOD_FIELD_SELECTOR = kernel_instance_variable_index("MethodDescriptor", "selector")
METHOD_FIELD_ARGUMENT_COUNT = kernel_instance_variable_index("MethodDescriptor", "argumentCount")
METHOD_FIELD_PRIMITIVE_KIND = kernel_instance_variable_index("MethodDescriptor", "primitiveKind")
METHOD_FIELD_ENTRY = kernel_instance_variable_index("MethodDescriptor", "entry")
METHOD_ENTRY_FIELD_EXECUTION_ID = kernel_instance_variable_index("MethodEntry", "executionId")
METHOD_ENTRY_FIELD_IMPLEMENTATION = kernel_instance_variable_index("MethodEntry", "implementation")


def load_kernel_boot_object_declarations() -> dict[str, KernelBootObjectDeclaration]:
    boot_object_declarations_by_name: dict[str, KernelBootObjectDeclaration] = {}
    family_order_by_name: dict[str, set[int]] = {}

    for source_unit in KERNEL_SOURCE_UNITS:
        relative_path = source_unit.relative_path
        class_header = parse_kernel_class_header(source_unit.chunk_sources[0], relative_path)

        for chunk_source in source_unit.chunk_sources[1:]:
            if not chunk_source.lstrip().startswith("RecorzKernelBootObject:"):
                continue
            declaration = parse_kernel_boot_object_chunk(chunk_source, relative_path)
            if declaration.class_name != class_header.class_name:
                raise LoweringError(
                    f"kernel MVP boot object {declaration.name} in {relative_path} must declare class #{class_header.class_name}"
                )
            if declaration.name in boot_object_declarations_by_name:
                raise LoweringError(f"kernel MVP boot object {declaration.name} is declared more than once")
            if declaration.class_name not in KERNEL_CLASS_HEADERS_BY_NAME:
                raise LoweringError(
                    f"kernel MVP boot object {declaration.name} in {relative_path} declares unknown class {declaration.class_name}"
                )
            family_orders = family_order_by_name.setdefault(declaration.family_name, set())
            if declaration.family_order in family_orders:
                raise LoweringError(
                    f"kernel MVP boot family {declaration.family_name!r} repeats order {declaration.family_order}"
                )
            family_orders.add(declaration.family_order)
            boot_object_declarations_by_name[declaration.name] = declaration

    return boot_object_declarations_by_name


KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME = load_kernel_boot_object_declarations()


def boot_object_field_specs(object_name: str) -> tuple[tuple[str, object], ...]:
    return KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME[object_name].field_specs


def boot_object_small_integer_field_values(object_name: str) -> tuple[int, ...]:
    field_values: list[int] = []
    for field_kind, field_value in boot_object_field_specs(object_name):
        if field_kind != FIELD_SPEC_SMALL_INTEGER:
            raise LoweringError(
                f"kernel MVP boot object {object_name} does not declare only small-integer field values"
            )
        field_values.append(int(field_value))
    return tuple(field_values)


def boot_object_small_integer_fields_in_order(object_name: str) -> tuple[int, ...]:
    return tuple(
        int(field_value)
        for field_kind, field_value in boot_object_field_specs(object_name)
        if field_kind == FIELD_SPEC_SMALL_INTEGER
    )


def boot_object_first_glyph_field_value(object_name: str) -> int:
    for field_kind, field_value in boot_object_field_specs(object_name):
        if field_kind == FIELD_SPEC_GLYPH_REF:
            return int(field_value)
    raise LoweringError(f"kernel MVP boot object {object_name} does not declare a glyph field")


def load_kernel_selector_declarations() -> dict[str, KernelSelectorDeclaration]:
    selector_declarations_by_selector: dict[str, KernelSelectorDeclaration] = {}

    for source_unit in KERNEL_SOURCE_UNITS:
        relative_path = source_unit.relative_path
        class_header = parse_kernel_class_header(source_unit.chunk_sources[0], relative_path)

        for chunk_source in source_unit.chunk_sources[1:]:
            if not chunk_source.lstrip().startswith("RecorzKernelSelector:"):
                continue
            declaration = parse_kernel_selector_chunk(chunk_source, relative_path)
            if class_header.class_name != "Selector":
                raise LoweringError(
                    f"kernel MVP selector {declaration.selector!r} in {relative_path} must be declared in class #Selector"
                )
            if declaration.selector in selector_declarations_by_selector:
                raise LoweringError(
                    f"kernel MVP selector {declaration.selector!r} is declared more than once"
                )
            selector_declarations_by_selector[declaration.selector] = declaration

    selector_declarations_in_order = sorted(
        selector_declarations_by_selector.values(),
        key=lambda declaration: declaration.selector_order,
    )
    expected_selector_orders = list(range(len(selector_declarations_in_order)))
    actual_selector_orders = [declaration.selector_order for declaration in selector_declarations_in_order]
    if actual_selector_orders != expected_selector_orders:
        raise LoweringError(
            "kernel MVP selectors must declare a contiguous order range starting at 0"
        )
    return selector_declarations_by_selector


KERNEL_SELECTOR_DECLARATIONS_BY_SELECTOR = load_kernel_selector_declarations()
KERNEL_SELECTOR_DECLARATIONS_IN_ORDER = sorted(
    KERNEL_SELECTOR_DECLARATIONS_BY_SELECTOR.values(),
    key=lambda declaration: declaration.selector_order,
)


def load_kernel_root_declarations(
    boot_object_declarations_by_name: dict[str, KernelBootObjectDeclaration],
) -> dict[str, KernelRootDeclaration]:
    root_declarations_by_name: dict[str, KernelRootDeclaration] = {}

    for source_unit in KERNEL_SOURCE_UNITS:
        relative_path = source_unit.relative_path

        for chunk_source in source_unit.chunk_sources[1:]:
            if not chunk_source.lstrip().startswith("RecorzKernelRoot:"):
                continue
            declaration = parse_kernel_root_chunk(chunk_source, relative_path)
            if declaration.root_name in root_declarations_by_name:
                raise LoweringError(f"kernel MVP root {declaration.root_name!r} is declared more than once")
            boot_object = boot_object_declarations_by_name.get(declaration.object_name)
            if boot_object is None:
                raise LoweringError(
                    f"kernel MVP root {declaration.root_name!r} in {relative_path} references unknown boot object {declaration.object_name!r}"
                )
            if declaration.root_name not in boot_object.root_exports:
                raise LoweringError(
                    f"kernel MVP root {declaration.root_name!r} in {relative_path} must reference a boot object that exports it"
                )
            root_declarations_by_name[declaration.root_name] = declaration

    root_declarations_in_order = sorted(
        root_declarations_by_name.values(),
        key=lambda declaration: declaration.root_order,
    )
    expected_root_orders = list(range(len(root_declarations_in_order)))
    actual_root_orders = [declaration.root_order for declaration in root_declarations_in_order]
    if actual_root_orders != expected_root_orders:
        raise LoweringError(
            "kernel MVP roots must declare a contiguous order range starting at 0"
        )
    return root_declarations_by_name


KERNEL_ROOT_DECLARATIONS_BY_NAME = load_kernel_root_declarations(KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME)
KERNEL_ROOT_DECLARATIONS_IN_ORDER = sorted(
    KERNEL_ROOT_DECLARATIONS_BY_NAME.values(),
    key=lambda declaration: declaration.root_order,
)


def load_kernel_glyph_bitmap_family_declaration() -> KernelGlyphBitmapFamilyDeclaration:
    glyph_family_declaration: KernelGlyphBitmapFamilyDeclaration | None = None

    for source_unit in KERNEL_SOURCE_UNITS:
        relative_path = source_unit.relative_path
        class_header = parse_kernel_class_header(source_unit.chunk_sources[0], relative_path)

        for chunk_source in source_unit.chunk_sources[1:]:
            if not chunk_source.lstrip().startswith("RecorzKernelGlyphBitmapFamily:"):
                continue
            declaration = parse_kernel_glyph_bitmap_family_chunk(chunk_source, relative_path)
            if declaration.class_name != class_header.class_name:
                raise LoweringError(
                    f"kernel MVP glyph bitmap family in {relative_path} must declare class #{class_header.class_name}"
                )
            if glyph_family_declaration is not None:
                raise LoweringError("kernel MVP glyph bitmap family is declared more than once")
            glyph_family_declaration = declaration

    if glyph_family_declaration is None:
        raise LoweringError("kernel MVP glyph bitmap family must be declared once in kernel source")
    return glyph_family_declaration


KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION = load_kernel_glyph_bitmap_family_declaration()
DEFAULT_FORM_BOOT_FIELD_SPECS = boot_object_field_specs("DefaultForm")
TRANSCRIPT_BEHAVIOR_BOOT_FIELD_SPECS = boot_object_field_specs("TranscriptBehavior")
TRANSCRIPT_LAYOUT_FIELD_SPECS = boot_object_field_specs("TranscriptLayout")
TRANSCRIPT_MARGINS_FIELD_VALUES = boot_object_small_integer_field_values("TranscriptMargins")
TRANSCRIPT_FLOW_FIELD_VALUES = boot_object_small_integer_field_values("TranscriptFlow")
TRANSCRIPT_STYLE_FIELD_VALUES = boot_object_small_integer_field_values("TranscriptStyle")
FRAMEBUFFER_BITMAP_FIELD_VALUES = boot_object_small_integer_field_values("FramebufferBitmap")
TRANSCRIPT_METRICS_FIELD_VALUES = boot_object_small_integer_fields_in_order("TranscriptMetrics")
BITMAP_STORAGE_FRAMEBUFFER = FRAMEBUFFER_BITMAP_FIELD_VALUES[2]
BITMAP_STORAGE_GLYPH_MONO = KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION.storage_kind
GLYPH_FALLBACK_CODE = boot_object_first_glyph_field_value("TranscriptBehavior")
GLYPH_BITMAP_NAME_PREFIX = KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION.name_prefix
GLYPH_BITMAP_WIDTH = KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION.width
GLYPH_BITMAP_HEIGHT = KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION.height
GLYPH_BITMAP_CODE_COUNT = KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION.count
GLYPH_BITMAP_BASE_FIELD_VALUES = (
    GLYPH_BITMAP_WIDTH,
    GLYPH_BITMAP_HEIGHT,
    KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION.storage_kind,
)


def build_glyph_bitmap_boot_specs(
    glyph_family_declaration: KernelGlyphBitmapFamilyDeclaration,
) -> list[BootObjectSpec]:
    return [
        BootObjectSpec(
            f"{glyph_family_declaration.name_prefix}{glyph_index}",
            glyph_family_declaration.class_name,
            build_small_integer_boot_field_specs(
                (glyph_family_declaration.width, glyph_family_declaration.height, glyph_family_declaration.storage_kind, glyph_index)
            ),
        )
        for glyph_index in range(glyph_family_declaration.count)
    ]


FIXED_BOOT_GRAPH_SPEC = build_fixed_boot_graph_spec(
    KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME,
    KERNEL_GLYPH_BITMAP_FAMILY_DECLARATION,
)


def upper_snake_name(name: str) -> str:
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()


def lower_snake_name(name: str) -> str:
    return upper_snake_name(name).lower()


def kernel_class_entry_stem(class_name: str) -> str:
    if class_name == "BitBlt":
        return "BITBLT"
    return upper_snake_name(class_name)


def kernel_object_kind_constant_name(class_name: str) -> str:
    return object_kind_constant_name_from_class_name(class_name)


def kernel_global_constant_name(global_name: str) -> str:
    return f"RECORZ_MVP_GLOBAL_{kernel_class_entry_stem(global_name)}"


def kernel_root_constant_name(root_name: str) -> str:
    return f"RECORZ_MVP_SEED_ROOT_{upper_snake_name(root_name)}"


def kernel_selector_constant_stem(selector: str) -> str:
    if selector == "value:":
        return "VALUE_ARG"
    if ":" in selector:
        keyword_parts = [part for part in selector.split(":") if part]
        return "_".join(upper_snake_name(part) for part in keyword_parts)
    if re.fullmatch(r"[A-Za-z_]\w*", selector):
        return upper_snake_name(selector)
    if selector == "+":
        return "ADD"
    if selector == "-":
        return "SUBTRACT"
    if selector == "*":
        return "MULTIPLY"
    if selector == "=":
        return "EQUAL"
    if selector == "<":
        return "LESS_THAN"
    if selector == ">":
        return "GREATER_THAN"
    raise LoweringError(f"kernel MVP selector {selector!r} cannot be converted into a constant stem")


def kernel_selector_constant_name(selector: str) -> str:
    return f"RECORZ_MVP_SELECTOR_{kernel_selector_constant_stem(selector)}"


def kernel_selector_entry_stem(selector: str) -> str:
    if selector in {"+", "-", "*"}:
        raise LoweringError(f"kernel MVP selector {selector!r} cannot be converted into a method entry name")
    try:
        return kernel_selector_constant_stem(selector)
    except LoweringError as error:
        raise LoweringError(f"kernel MVP selector {selector!r} cannot be converted into a method entry name") from error


def kernel_method_entry_name(class_name: str, selector: str) -> str:
    return f"RECORZ_MVP_METHOD_ENTRY_{kernel_class_entry_stem(class_name)}_{kernel_selector_entry_stem(selector)}"


def kernel_primitive_binding_constant_name(binding_name: str) -> str:
    return f"RECORZ_MVP_PRIMITIVE_{upper_snake_name(binding_name)}"


def kernel_primitive_handler_name(binding_name: str) -> str:
    return f"execute_entry_{lower_snake_name(binding_name)}"


def validate_kernel_primitive_binding_symbol_names(binding_names: list[str] | tuple[str, ...]) -> None:
    binding_name_by_constant_name: dict[str, str] = {}
    binding_name_by_handler_name: dict[str, str] = {}

    for binding_name in binding_names:
        constant_name = kernel_primitive_binding_constant_name(binding_name)
        prior_binding_name = binding_name_by_constant_name.get(constant_name)
        if prior_binding_name is not None and prior_binding_name != binding_name:
            raise LoweringError(
                f"primitive binding names {prior_binding_name!r} and {binding_name!r} map to the same constant {constant_name}"
            )
        binding_name_by_constant_name[constant_name] = binding_name
        handler_name = kernel_primitive_handler_name(binding_name)
        prior_handler_binding_name = binding_name_by_handler_name.get(handler_name)
        if prior_handler_binding_name is not None and prior_handler_binding_name != binding_name:
            raise LoweringError(
                f"primitive binding names {prior_handler_binding_name!r} and {binding_name!r} map to the same handler {handler_name}"
            )
        binding_name_by_handler_name[handler_name] = binding_name


def kernel_field_constant_name(class_name: str, field_name: str) -> str:
    return f"RECORZ_MVP_{kernel_class_entry_stem(class_name)}_FIELD_{upper_snake_name(field_name)}"


def build_kernel_field_definitions() -> list[tuple[str, int]]:
    return [
        (kernel_field_constant_name(class_header.class_name, field_name), field_index)
        for class_header in KERNEL_CLASS_HEADERS_IN_DESCRIPTOR_ORDER
        for field_index, field_name in enumerate(class_header.instance_variables)
    ]


def append_enum_definition(lines: list[str], enum_name: str, entries: list[tuple[str, int]]) -> None:
    lines.append(f"enum {enum_name} {{")
    for name, value in entries:
        lines.append(f"    {name} = {value},")
    lines.extend(
        [
            "};",
            "",
        ]
    )


def append_macro_definition(lines: list[str], name: str, value: str) -> None:
    lines.append(f"#define {name} {value}")


def append_magic_byte_definitions(lines: list[str], prefix: str, magic: bytes) -> None:
    for index, value in enumerate(magic):
        append_macro_definition(lines, f"{prefix}_{index}", repr(chr(value)))


def build_global_specs_from_boot_object_exports(
    fixed_boot_graph_spec: FixedBootGraphSpec,
) -> list[tuple[str, str]]:
    global_specs: list[tuple[str, str]] = []

    for object_spec in flatten_boot_object_specs(fixed_boot_graph_spec):
        for global_name in object_spec.global_exports:
            global_specs.append((global_name, kernel_global_constant_name(global_name)))
    return global_specs


def build_selector_specs_from_declarations(
    selector_declarations: list[KernelSelectorDeclaration] | tuple[KernelSelectorDeclaration, ...],
) -> list[tuple[str, str]]:
    return [
        (declaration.selector, kernel_selector_constant_name(declaration.selector))
        for declaration in selector_declarations
    ]


def build_root_object_name_map_from_declarations(
    root_declarations: list[KernelRootDeclaration] | tuple[KernelRootDeclaration, ...],
) -> dict[str, str]:
    return {
        declaration.root_name: declaration.object_name
        for declaration in root_declarations
    }


def build_root_specs_from_declarations(
    root_declarations: list[KernelRootDeclaration] | tuple[KernelRootDeclaration, ...],
) -> list[tuple[str, str]]:
    return [
        (declaration.root_name, kernel_root_constant_name(declaration.root_name))
        for declaration in root_declarations
    ]


GLOBAL_SPECS = build_global_specs_from_boot_object_exports(FIXED_BOOT_GRAPH_SPEC)
GLOBAL_IDS, GLOBAL_VALUES, GLOBAL_DEFINITIONS = build_named_constant_maps(GLOBAL_SPECS)
SEED_ROOT_SPECS = build_root_specs_from_declarations(KERNEL_ROOT_DECLARATIONS_IN_ORDER)
SEED_ROOT_IDS, SEED_ROOT_VALUES, SEED_ROOT_DEFINITIONS = build_named_constant_maps(SEED_ROOT_SPECS)
SEED_ROOT_DEFAULT_FORM = constant_value(SEED_ROOT_IDS, SEED_ROOT_VALUES, "default_form")
SEED_ROOT_FRAMEBUFFER_BITMAP = constant_value(SEED_ROOT_IDS, SEED_ROOT_VALUES, "framebuffer_bitmap")
SEED_ROOT_TRANSCRIPT_BEHAVIOR = constant_value(SEED_ROOT_IDS, SEED_ROOT_VALUES, "transcript_behavior")
SEED_ROOT_TRANSCRIPT_LAYOUT = constant_value(SEED_ROOT_IDS, SEED_ROOT_VALUES, "transcript_layout")
SEED_ROOT_TRANSCRIPT_STYLE = constant_value(SEED_ROOT_IDS, SEED_ROOT_VALUES, "transcript_style")
SEED_ROOT_TRANSCRIPT_METRICS = constant_value(SEED_ROOT_IDS, SEED_ROOT_VALUES, "transcript_metrics")
SEED_ROOT_TRANSCRIPT_FONT = constant_value(SEED_ROOT_IDS, SEED_ROOT_VALUES, "transcript_font")
SELECTOR_SPECS = build_selector_specs_from_declarations(KERNEL_SELECTOR_DECLARATIONS_IN_ORDER)
SELECTOR_IDS, SELECTOR_VALUES, SELECTOR_DEFINITIONS = build_named_constant_maps(SELECTOR_SPECS)


def parse_kernel_method_chunk(
    class_name: str,
    instance_variables: list[str],
    chunk_source: str,
) -> tuple[str, int, str, str | None]:
    lines = chunk_source.splitlines()
    if not lines:
        raise LoweringError(f"kernel MVP class {class_name} contains an empty method chunk")
    trimmed_body = [line.strip() for line in lines[1:] if line.strip()]
    if len(trimmed_body) == 1:
        primitive_match = KERNEL_PRIMITIVE_DECLARATION_PATTERN.fullmatch(trimmed_body[0])
        if primitive_match is not None:
            primitive_binding = primitive_match.group("binding")
            signature_probe = compile_method(f"{lines[0]}\n    ^self", class_name, list(instance_variables))
            return (
                signature_probe.selector,
                len(signature_probe.arg_names),
                KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE,
                primitive_binding,
            )
    if any(line.startswith("<primitive") for line in trimmed_body):
        raise LoweringError(f"kernel MVP primitive declaration is invalid in class {class_name}")
    compiled = compile_method(chunk_source, class_name, list(instance_variables))
    return (compiled.selector, len(compiled.arg_names), KERNEL_METHOD_IMPLEMENTATION_COMPILED, None)


def load_kernel_method_sources() -> dict[str, KernelMethodSource]:
    discovered_method_sources_by_class: dict[str, list[KernelMethodSource]] = {}
    seen_class_names: set[str] = set()
    discovered_entry_names: set[str] = set()
    method_source_units = (
        load_kernel_source_units_from_directory()
        if os.environ.get(KERNEL_SOURCE_BUNDLE_ENV)
        else KERNEL_SOURCE_UNITS
    )

    for source_unit in method_source_units:
        relative_path = source_unit.relative_path
        class_header = parse_kernel_class_header(source_unit.chunk_sources[0], relative_path)
        class_name = class_header.class_name
        instance_variables = list(class_header.instance_variables)
        expected_class_kind = KERNEL_CLASS_NAME_TO_OBJECT_KIND.get(class_name)
        if expected_class_kind is None:
            raise LoweringError(f"kernel MVP class file {relative_path} declares unknown class {class_name}")
        if class_name in seen_class_names:
            raise LoweringError(f"kernel MVP class {class_name} is declared more than once")
        seen_class_names.add(class_name)
        discovered_method_sources_by_class[class_name] = []
        chunk_signatures: set[tuple[str, int]] = set()

        for chunk_source in source_unit.chunk_sources[1:]:
            if chunk_source.lstrip().startswith("RecorzKernelBootObject:"):
                continue
            if chunk_source.lstrip().startswith("RecorzKernelRoot:"):
                continue
            if chunk_source.lstrip().startswith("RecorzKernelGlyphBitmapFamily:"):
                continue
            if chunk_source.lstrip().startswith("RecorzKernelSelector:"):
                continue
            selector, argument_count, implementation_kind, primitive_binding = parse_kernel_method_chunk(
                class_name,
                instance_variables,
                chunk_source,
            )
            signature = (selector, argument_count)
            if signature in chunk_signatures:
                raise LoweringError(
                    f"kernel MVP class file {relative_path} duplicates method {selector}/{argument_count}"
                )
            chunk_signatures.add(signature)
            entry_name = kernel_method_entry_name(class_name, selector)
            if entry_name in discovered_entry_names:
                raise LoweringError(
                    f"kernel MVP class file {relative_path} duplicates built-in entry {entry_name}"
                )
            discovered_entry_names.add(entry_name)
            discovered_method_sources_by_class[class_name].append(
                KernelMethodSource(
                    entry_name=entry_name,
                    class_name=class_name,
                    instance_variables=class_header.instance_variables,
                    relative_path=relative_path,
                    selector=selector,
                    argument_count=argument_count,
                    implementation_kind=implementation_kind,
                    primitive_binding=primitive_binding,
                    source_text=chunk_source,
                )
            )
    expected_class_names = {
        parse_kernel_class_header(source_unit.chunk_sources[0], source_unit.relative_path).class_name
        for source_unit in method_source_units
    }
    if seen_class_names != expected_class_names:
        missing_classes = sorted(expected_class_names - seen_class_names)
        extra_classes = sorted(seen_class_names - expected_class_names)
        details: list[str] = []
        if missing_classes:
            details.append(f"missing classes {', '.join(missing_classes)}")
        if extra_classes:
            details.append(f"unexpected classes {', '.join(extra_classes)}")
        raise LoweringError(
            "kernel MVP source tree does not match the expected boot class set"
            + (f": {'; '.join(details)}" if details else "")
        )
    ordered_method_sources: dict[str, KernelMethodSource] = {}
    for class_name in KERNEL_CLASS_BOOT_ORDER:
        for method_source in discovered_method_sources_by_class[class_name]:
            ordered_method_sources[method_source.entry_name] = method_source
    return ordered_method_sources


def kernel_chunk_is_runtime_file_in_chunk(chunk_source: str) -> bool:
    stripped = chunk_source.lstrip()

    return not (
        stripped.startswith("RecorzKernelBootObject:") or
        stripped.startswith("RecorzKernelRoot:") or
        stripped.startswith("RecorzKernelGlyphBitmapFamily:") or
        stripped.startswith("RecorzKernelSelector:")
    )


def load_kernel_canonical_class_sources() -> dict[str, str]:
    canonical_sources_by_name: dict[str, str] = {}

    for source_unit in KERNEL_SOURCE_UNITS:
        relative_path = source_unit.relative_path
        class_header = parse_kernel_class_header(source_unit.chunk_sources[0], relative_path)
        canonical_chunks = [
            chunk_source
            for chunk_source in source_unit.chunk_sources
            if kernel_chunk_is_runtime_file_in_chunk(chunk_source)
        ]
        if not canonical_chunks or not canonical_chunks[0].lstrip().startswith("RecorzKernelClass:"):
            raise LoweringError(f"kernel MVP class file {relative_path} is missing a canonical class header chunk")
        canonical_sources_by_name[class_header.class_name] = "\n!\n".join(canonical_chunks) + "\n!\n"

    expected_class_names = set(KERNEL_CLASS_HEADERS_BY_NAME)
    if set(canonical_sources_by_name) != expected_class_names:
        missing_classes = sorted(expected_class_names - set(canonical_sources_by_name))
        raise LoweringError(
            "kernel MVP canonical source registry is missing classes: "
            + ", ".join(missing_classes)
        )
    return canonical_sources_by_name


def load_kernel_builder_class_sources() -> dict[str, str]:
    builder_sources_by_name: dict[str, str] = {}

    for source_unit in KERNEL_SOURCE_UNITS:
        relative_path = source_unit.relative_path
        class_header = parse_kernel_class_header(source_unit.chunk_sources[0], relative_path)
        builder_sources_by_name[class_header.class_name] = "\n!\n".join(source_unit.chunk_sources) + "\n!\n"

    expected_class_names = set(KERNEL_CLASS_HEADERS_BY_NAME)
    if set(builder_sources_by_name) != expected_class_names:
        missing_classes = sorted(expected_class_names - set(builder_sources_by_name))
        raise LoweringError(
            "kernel MVP builder source registry is missing classes: "
            + ", ".join(missing_classes)
        )
    return builder_sources_by_name


def compile_kernel_method_program(class_name: str, instance_variables: list[str], source: str) -> list[int]:
    compiled = compile_method(source, class_name, instance_variables)
    arg_indices = {name: index for index, name in enumerate(compiled.arg_names)}
    field_indices = {name: index for index, name in enumerate(instance_variables)}
    instructions = list(compiled.instructions)
    return_receiver = False
    lowered: list[int] = []

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

        if instruction.opcode == "push_literal":
            literal_index = instruction.operand
            if not isinstance(literal_index, int) or literal_index < 0 or literal_index >= len(compiled.literals):
                raise LoweringError(
                    f"Kernel method {class_name}>>{compiled.selector} uses an invalid literal index {literal_index!r}"
                )
            literal = compiled.literals[literal_index]
            if isinstance(literal, int):
                if literal < -32768 or literal > 32767:
                    raise LoweringError(
                        f"Kernel method {class_name}>>{compiled.selector} uses an out-of-range small integer literal {literal}"
                    )
                lowered.append(
                    encode_compiled_method_instruction("push_small_integer", 0, literal & 0xFFFF)
                )
                instruction_index += 1
                continue
            raise LoweringError(
                f"Kernel method {class_name}>>{compiled.selector} uses unsupported literal {literal!r}"
            )
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
                if (
                    isinstance(send_site, SendSite)
                    and not send_site.super_send
                    and send_site.selector == "font"
                    and send_site.argument_count == 0
                ):
                    lowered.append(encode_compiled_method_instruction("push_root", SEED_ROOT_TRANSCRIPT_FONT))
                    instruction_index += 2
                    continue
                if (
                    isinstance(send_site, SendSite)
                    and not send_site.super_send
                    and send_site.selector == "style"
                    and send_site.argument_count == 0
                ):
                    lowered.append(encode_compiled_method_instruction("push_root", SEED_ROOT_TRANSCRIPT_STYLE))
                    instruction_index += 2
                    continue
                if (
                    isinstance(send_site, SendSite)
                    and not send_site.super_send
                    and send_site.selector == "layout"
                    and send_site.argument_count == 0
                ):
                    lowered.append(encode_compiled_method_instruction("push_root", SEED_ROOT_TRANSCRIPT_LAYOUT))
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
        if instruction.opcode == "push_this_context":
            lowered.append(encode_compiled_method_instruction("push_this_context"))
            instruction_index += 1
            continue
        if instruction.opcode == "duplicate":
            lowered.append(encode_compiled_method_instruction("dup"))
            instruction_index += 1
            continue
        if instruction.opcode == "pop":
            lowered.append(encode_compiled_method_instruction("pop"))
            instruction_index += 1
            continue
        if instruction.opcode == "store_binding":
            binding = instruction.operand
            if not isinstance(binding, BindingRef):
                raise LoweringError(
                    f"Kernel method {class_name}>>{compiled.selector} expected a binding operand, found {binding!r}"
                )
            if binding.kind != "instance":
                raise LoweringError(
                    f"Kernel method {class_name}>>{compiled.selector} uses unsupported store binding kind {binding.kind!r}"
                )
            if binding.name not in field_indices:
                raise LoweringError(
                    f"Kernel method {class_name}>>{compiled.selector} uses unknown instance variable {binding.name!r}"
                )
            lowered.append(encode_compiled_method_instruction("store_field", field_indices[binding.name]))
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


def build_method_update_manifest(class_name: str, source: str) -> bytes:
    class_header = KERNEL_CLASS_HEADERS_BY_NAME.get(class_name)
    if class_header is None:
        raise LoweringError(f"kernel MVP method update references unknown class {class_name!r}")

    selector, argument_count, implementation_kind, _primitive_binding = parse_kernel_method_chunk(
        class_name,
        list(class_header.instance_variables),
        source,
    )
    if implementation_kind != KERNEL_METHOD_IMPLEMENTATION_COMPILED:
        raise LoweringError("kernel MVP method update must lower to a compiled method")
    if selector not in KERNEL_SELECTOR_DECLARATIONS_BY_SELECTOR:
        raise LoweringError(f"kernel MVP method update references undeclared selector {selector!r}")

    instructions = compile_kernel_method_program(class_name, list(class_header.instance_variables), source)
    manifest = bytearray(
        struct.pack(
            METHOD_UPDATE_HEADER_FORMAT,
            METHOD_UPDATE_MAGIC,
            METHOD_UPDATE_VERSION,
            KERNEL_CLASS_NAME_TO_OBJECT_KIND[class_name],
            SELECTOR_VALUES[SELECTOR_IDS[selector]],
            argument_count,
            len(instructions),
            0,
        )
    )
    for instruction in instructions:
        manifest.extend(struct.pack("<I", instruction))
    return bytes(manifest)


KERNEL_METHOD_SOURCE_BY_ENTRY_NAME = load_kernel_method_sources()
KERNEL_CANONICAL_CLASS_SOURCES_BY_NAME = load_kernel_canonical_class_sources()
KERNEL_BUILDER_CLASS_SOURCES_BY_NAME = load_kernel_builder_class_sources()


def validate_kernel_method_selectors(
    method_sources_by_entry_name: dict[str, KernelMethodSource],
    selector_declarations_by_selector: dict[str, KernelSelectorDeclaration],
) -> None:
    undeclared_selectors = sorted(
        {
            source.selector
            for source in method_sources_by_entry_name.values()
            if source.selector not in selector_declarations_by_selector
        }
    )
    if undeclared_selectors:
        raise LoweringError(
            "kernel MVP methods use undeclared selectors: "
            + ", ".join(repr(selector) for selector in undeclared_selectors)
        )


validate_kernel_method_selectors(
    KERNEL_METHOD_SOURCE_BY_ENTRY_NAME,
    KERNEL_SELECTOR_DECLARATIONS_BY_SELECTOR,
)
METHOD_ENTRY_ORDER = list(KERNEL_METHOD_SOURCE_BY_ENTRY_NAME)
METHOD_ENTRY_DEFINITIONS: list[tuple[str, int, str, int]] = [
    (
        entry_name,
        KERNEL_CLASS_NAME_TO_OBJECT_KIND[source.class_name],
        source.selector,
        source.argument_count,
    )
    for entry_name, source in KERNEL_METHOD_SOURCE_BY_ENTRY_NAME.items()
]
METHOD_ENTRY_VALUES = {
    name: index + 1 for index, (name, _owner_kind, _selector, _argument_count) in enumerate(METHOD_ENTRY_DEFINITIONS)
}
METHOD_ENTRY_COUNT = len(METHOD_ENTRY_VALUES) + 1
METHOD_ENTRY_SPECS = {
    METHOD_ENTRY_VALUES[name]: (owner_kind, SELECTOR_VALUES[SELECTOR_IDS[selector]], argument_count)
    for name, owner_kind, selector, argument_count in METHOD_ENTRY_DEFINITIONS
}
BUILTIN_METHODS_BY_KIND = {
    kind: [] for kind in range(SEED_OBJECT_TRANSCRIPT, MAX_SEED_OBJECT_KIND + 1)
}
for entry_name, owner_kind, selector, argument_count in METHOD_ENTRY_DEFINITIONS:
    BUILTIN_METHODS_BY_KIND[owner_kind].append((selector, argument_count, entry_name))

COMPILED_METHOD_PROGRAM_BY_ENTRY_NAME = {
    entry_name: compile_kernel_method_program(source.class_name, list(source.instance_variables), source.source_text)
    for entry_name, source in KERNEL_METHOD_SOURCE_BY_ENTRY_NAME.items()
    if source.implementation_kind == KERNEL_METHOD_IMPLEMENTATION_COMPILED
}
PRIMITIVE_BINDING_VALUES: dict[str, int] = {}
PRIMITIVE_BINDING_BY_ENTRY_NAME: dict[str, int] = {}
for entry_name, source in KERNEL_METHOD_SOURCE_BY_ENTRY_NAME.items():
    if source.implementation_kind != KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE:
        continue
    assert source.primitive_binding is not None
    if source.primitive_binding not in PRIMITIVE_BINDING_VALUES:
        PRIMITIVE_BINDING_VALUES[source.primitive_binding] = len(PRIMITIVE_BINDING_VALUES) + 1
    PRIMITIVE_BINDING_BY_ENTRY_NAME[entry_name] = PRIMITIVE_BINDING_VALUES[source.primitive_binding]
validate_kernel_primitive_binding_symbol_names(tuple(PRIMITIVE_BINDING_VALUES))
PRIMITIVE_BINDING_OWNERS_IN_ENTRY_ORDER = tuple(
    KernelPrimitiveBindingOwner(
        binding_name=source.primitive_binding if source.primitive_binding is not None else "",
        entry_name=entry_name,
        class_name=source.class_name,
        selector=source.selector,
        argument_count=source.argument_count,
        relative_path=source.relative_path,
    )
    for entry_name, source in KERNEL_METHOD_SOURCE_BY_ENTRY_NAME.items()
    if source.implementation_kind == KERNEL_METHOD_IMPLEMENTATION_PRIMITIVE
)
SELECTOR_VALUE_ORDER = [value for _constant_name, value in SELECTOR_DEFINITIONS]
COMPILED_METHOD_ENTRY_ORDER = [
    entry_name for entry_name in METHOD_ENTRY_ORDER if entry_name in COMPILED_METHOD_PROGRAM_BY_ENTRY_NAME
]


def append_generated_seed_class_source_registry(lines: list[str]) -> None:
    lines.extend(
        [
            "struct recorz_mvp_seed_class_source_record {",
            "    const char *class_name;",
            "    const char *relative_path;",
            "    const char *canonical_source;",
            "    const char *builder_source;",
            "    uint8_t descriptor_order;",
            "    uint8_t object_kind_order;",
            "    int16_t source_boot_order;",
            "    uint8_t instance_variable_count;",
            "    const char *instance_variable_names[4];",
            "};",
            "",
            "static const struct recorz_mvp_seed_class_source_record recorz_mvp_generated_seed_class_sources[] = {",
            "    {0, 0, 0, 0, 0, 0, -1, 0, {0, 0, 0, 0}},",
        ]
    )
    for class_header in KERNEL_CLASS_HEADERS_IN_OBJECT_KIND_ORDER:
        instance_variable_literals = [json.dumps(name) for name in class_header.instance_variables]
        while len(instance_variable_literals) < 4:
            instance_variable_literals.append("0")
        source_boot_order = -1 if class_header.source_boot_order is None else class_header.source_boot_order
        lines.append(
            "    {"
            + ", ".join(
                [
                    json.dumps(class_header.class_name),
                    json.dumps(KERNEL_CLASS_RELATIVE_PATHS_BY_NAME[class_header.class_name]),
                    json.dumps(KERNEL_CANONICAL_CLASS_SOURCES_BY_NAME[class_header.class_name]),
                    json.dumps(KERNEL_BUILDER_CLASS_SOURCES_BY_NAME[class_header.class_name]),
                    f"{class_header.descriptor_order}",
                    f"{class_header.object_kind_order}",
                    f"{source_boot_order}",
                    f"{len(class_header.instance_variables)}",
                    "{" + ", ".join(instance_variable_literals) + "}",
                ]
            )
            + "},"
        )
    lines.extend(
        [
            "};",
            "",
        ]
    )


def append_generated_selector_registry(lines: list[str]) -> None:
    lines.extend(
        [
            "struct recorz_mvp_selector_record {",
            "    const char *selector;",
            "    const char *constant_name;",
            "    const char *relative_path;",
            "    uint16_t value;",
            "    uint16_t declaration_order;",
            "};",
            "",
            "static const struct recorz_mvp_selector_record recorz_mvp_generated_selectors[] = {",
            "    {0, 0, 0, 0, 0},",
        ]
    )
    for declaration in KERNEL_SELECTOR_DECLARATIONS_IN_ORDER:
        constant_name = SELECTOR_IDS[declaration.selector]
        lines.append(
            "    {"
            + ", ".join(
                [
                    json.dumps(declaration.selector),
                    json.dumps(constant_name),
                    json.dumps(declaration.relative_path),
                    f"{SELECTOR_VALUES[constant_name]}",
                    f"{declaration.selector_order}",
                ]
            )
            + "},"
        )
    lines.extend(
        [
            "};",
            "",
        ]
    )


def append_generated_primitive_binding_registry(lines: list[str]) -> None:
    lines.extend(
        [
            "struct recorz_mvp_primitive_binding_record {",
            "    const char *binding_name;",
            "    const char *constant_name;",
            "    const char *handler_name;",
            "    uint16_t value;",
            "};",
            "",
            "static const struct recorz_mvp_primitive_binding_record recorz_mvp_generated_primitive_bindings[] = {",
            "    {0, 0, 0, 0},",
        ]
    )
    for binding_name, binding_id in sorted(PRIMITIVE_BINDING_VALUES.items(), key=lambda item: item[1]):
        lines.append(
            "    {"
            + ", ".join(
                [
                    json.dumps(binding_name),
                    json.dumps(kernel_primitive_binding_constant_name(binding_name)),
                    json.dumps(kernel_primitive_handler_name(binding_name)),
                    f"{binding_id}",
                ]
            )
            + "},"
        )
    lines.extend(
        [
            "};",
            "",
            "struct recorz_mvp_primitive_binding_owner_record {",
            "    uint16_t primitive_binding_value;",
            "    const char *entry_name;",
            "    const char *class_name;",
            "    const char *selector;",
            "    uint8_t argument_count;",
            "    const char *relative_path;",
            "};",
            "",
            "static const struct recorz_mvp_primitive_binding_owner_record "
            "recorz_mvp_generated_primitive_binding_owners[] = {",
            "    {0, 0, 0, 0, 0, 0},",
        ]
    )
    for owner in PRIMITIVE_BINDING_OWNERS_IN_ENTRY_ORDER:
        lines.append(
            "    {"
            + ", ".join(
                [
                    f"{PRIMITIVE_BINDING_VALUES[owner.binding_name]}",
                    json.dumps(owner.entry_name),
                    json.dumps(owner.class_name),
                    json.dumps(owner.selector),
                    f"{owner.argument_count}",
                    json.dumps(owner.relative_path),
                ]
            )
            + "},"
        )
    lines.extend(
        [
            "};",
            "",
        ]
    )


def render_generated_runtime_bindings_header() -> str:
    lines = [
        "/* Auto-generated from kernel MVP method and primitive declarations. */",
        "#ifndef RECORZ_QEMU_RISCV64_GENERATED_RUNTIME_BINDINGS_H",
        "#define RECORZ_QEMU_RISCV64_GENERATED_RUNTIME_BINDINGS_H",
        "",
    ]
    append_magic_byte_definitions(lines, "RECORZ_MVP_PROGRAM_MAGIC", PROGRAM_MAGIC)
    append_macro_definition(lines, "RECORZ_MVP_PROGRAM_VERSION", f"{PROGRAM_VERSION}U")
    append_macro_definition(lines, "RECORZ_MVP_PROGRAM_HEADER_SIZE", f"{struct.calcsize(PROGRAM_HEADER_FORMAT)}U")
    append_macro_definition(
        lines,
        "RECORZ_MVP_PROGRAM_INSTRUCTION_SIZE",
        f"{struct.calcsize(PROGRAM_INSTRUCTION_FORMAT)}U",
    )
    append_macro_definition(
        lines,
        "RECORZ_MVP_PROGRAM_LITERAL_HEADER_SIZE",
        f"{struct.calcsize(PROGRAM_LITERAL_HEADER_FORMAT)}U",
    )
    lines.append("")
    append_magic_byte_definitions(lines, "RECORZ_MVP_IMAGE_MAGIC", IMAGE_MAGIC)
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_VERSION", f"{IMAGE_VERSION}U")
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_HEADER_SIZE", f"{struct.calcsize(IMAGE_HEADER_FORMAT)}U")
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_SECTION_SIZE", f"{struct.calcsize(IMAGE_SECTION_FORMAT)}U")
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_SECTION_PROGRAM", f"{IMAGE_SECTION_PROGRAM}U")
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_SECTION_SEED", f"{IMAGE_SECTION_SEED}U")
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_SECTION_ENTRY", f"{IMAGE_SECTION_ENTRY}U")
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_FEATURE_FNV1A32", f"{IMAGE_FEATURE_FNV1A32}U")
    append_magic_byte_definitions(lines, "RECORZ_MVP_IMAGE_PROFILE", IMAGE_PROFILE)
    append_magic_byte_definitions(lines, "RECORZ_MVP_IMAGE_ENTRY_MAGIC", IMAGE_ENTRY_MAGIC)
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_ENTRY_VERSION", f"{IMAGE_ENTRY_VERSION}U")
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_ENTRY_SIZE", f"{struct.calcsize(IMAGE_ENTRY_FORMAT)}U")
    append_macro_definition(lines, "RECORZ_MVP_IMAGE_ENTRY_KIND_DOIT", f"{IMAGE_ENTRY_KIND_DOIT}U")
    lines.append("")
    append_magic_byte_definitions(lines, "RECORZ_MVP_SEED_MAGIC", SEED_MAGIC)
    append_macro_definition(lines, "RECORZ_MVP_SEED_VERSION", f"{SEED_VERSION}U")
    append_macro_definition(lines, "RECORZ_MVP_SEED_HEADER_SIZE", f"{struct.calcsize(SEED_HEADER_FORMAT)}U")
    append_macro_definition(
        lines,
        "RECORZ_MVP_SEED_OBJECT_SIZE",
        f"{struct.calcsize(SEED_OBJECT_HEADER_FORMAT) + (4 * struct.calcsize(SEED_FIELD_FORMAT))}U",
    )
    append_macro_definition(lines, "RECORZ_MVP_SEED_BINDING_SIZE", f"{struct.calcsize(SEED_BINDING_FORMAT)}U")
    append_macro_definition(lines, "RECORZ_MVP_SEED_INVALID_OBJECT_INDEX", f"{SEED_INVALID_OBJECT_INDEX}U")
    append_macro_definition(lines, "RECORZ_MVP_COMPILED_METHOD_MAX_INSTRUCTIONS", f"{COMPILED_METHOD_MAX_INSTRUCTIONS}U")
    append_magic_byte_definitions(lines, "RECORZ_MVP_METHOD_UPDATE_MAGIC", METHOD_UPDATE_MAGIC)
    append_macro_definition(lines, "RECORZ_MVP_METHOD_UPDATE_VERSION", f"{METHOD_UPDATE_VERSION}U")
    append_macro_definition(lines, "RECORZ_MVP_METHOD_UPDATE_HEADER_SIZE", f"{struct.calcsize(METHOD_UPDATE_HEADER_FORMAT)}U")
    append_macro_definition(lines, "RECORZ_MVP_METHOD_UPDATE_FW_CFG_NAME", f"\"{METHOD_UPDATE_FW_CFG_NAME}\"")
    lines.append("")
    append_enum_definition(lines, "recorz_mvp_compiled_method_opcode", COMPILED_METHOD_OPCODE_DEFINITIONS)
    append_enum_definition(lines, "recorz_mvp_method_implementation", METHOD_IMPLEMENTATION_DEFINITIONS)
    for field_constant_name, field_index in build_kernel_field_definitions():
        append_macro_definition(lines, field_constant_name, f"{field_index}U")
    append_macro_definition(lines, "RECORZ_MVP_BITMAP_STORAGE_FRAMEBUFFER", f"{BITMAP_STORAGE_FRAMEBUFFER}U")
    append_macro_definition(lines, "RECORZ_MVP_BITMAP_STORAGE_GLYPH_MONO", f"{BITMAP_STORAGE_GLYPH_MONO}U")
    lines.append("")
    append_enum_definition(lines, "recorz_mvp_opcode", OPCODE_DEFINITIONS)
    append_enum_definition(lines, "recorz_mvp_global", GLOBAL_DEFINITIONS)
    append_enum_definition(lines, "recorz_mvp_selector", SELECTOR_DEFINITIONS)
    append_enum_definition(lines, "recorz_mvp_literal_kind", LITERAL_KIND_DEFINITIONS)
    append_enum_definition(lines, "recorz_mvp_object_kind", OBJECT_KIND_DEFINITIONS)
    append_enum_definition(lines, "recorz_mvp_seed_field_kind", SEED_FIELD_KIND_DEFINITIONS)
    append_enum_definition(lines, "recorz_mvp_seed_root", SEED_ROOT_DEFINITIONS)
    append_macro_definition(
        lines,
        "RECORZ_MVP_GENERATED_SEED_CLASS_SOURCE_RECORD_COUNT",
        f"{len(KERNEL_CLASS_HEADERS_IN_OBJECT_KIND_ORDER) + 1}U",
    )
    append_generated_seed_class_source_registry(lines)
    append_macro_definition(
        lines,
        "RECORZ_MVP_GENERATED_SELECTOR_RECORD_COUNT",
        f"{len(KERNEL_SELECTOR_DECLARATIONS_IN_ORDER) + 1}U",
    )
    append_generated_selector_registry(lines)
    lines.append("enum recorz_mvp_method_entry {")
    for entry_name in METHOD_ENTRY_ORDER:
        lines.append(f"    {entry_name} = {METHOD_ENTRY_VALUES[entry_name]},")
    lines.append(f"    RECORZ_MVP_METHOD_ENTRY_COUNT = {len(METHOD_ENTRY_ORDER) + 1},")
    lines.extend(
        [
            "};",
            "",
            "enum recorz_mvp_primitive_binding {",
        ]
    )
    for binding_name, binding_id in sorted(PRIMITIVE_BINDING_VALUES.items(), key=lambda item: item[1]):
        lines.append(f"    {kernel_primitive_binding_constant_name(binding_name)} = {binding_id},")
    lines.append(f"    RECORZ_MVP_PRIMITIVE_COUNT = {len(PRIMITIVE_BINDING_VALUES) + 1},")
    lines.extend(
        [
            "};",
            "",
            f"#define RECORZ_MVP_GENERATED_PRIMITIVE_BINDING_RECORD_COUNT {len(PRIMITIVE_BINDING_VALUES) + 1}U",
            (
                "#define RECORZ_MVP_GENERATED_PRIMITIVE_BINDING_OWNER_RECORD_COUNT "
                f"{len(PRIMITIVE_BINDING_OWNERS_IN_ENTRY_ORDER) + 1}U"
            ),
            "",
        ]
    )
    append_generated_primitive_binding_registry(lines)
    lines.extend(
        [
            "#define RECORZ_MVP_GENERATED_PRIMITIVE_BINDING_HANDLERS \\",
        ]
    )
    primitive_bindings = sorted(PRIMITIVE_BINDING_VALUES.items(), key=lambda item: item[1])
    for index, (binding_name, _binding_id) in enumerate(primitive_bindings):
        line = (
            f"    [{kernel_primitive_binding_constant_name(binding_name)}] = "
            f"{kernel_primitive_handler_name(binding_name)},"
        )
        if index != len(primitive_bindings) - 1:
            line += " \\"
        lines.append(line)
    lines.extend(
        [
            "",
            "#endif",
            "",
        ]
    )
    return "\n".join(lines)


def render_generated_primitive_bindings_header() -> str:
    # Compatibility shim for the prior helper name.
    return render_generated_runtime_bindings_header()


def write_output_bytes_if_changed(path: Path, data: bytes) -> bool:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() == data:
        return False
    temporary_output = path.with_name(f".{path.name}.{os.getpid()}.tmp")
    try:
        temporary_output.write_bytes(data)
        os.replace(temporary_output, path)
    finally:
        if temporary_output.exists():
            temporary_output.unlink()
    return True


def write_output_text_if_changed(path: Path, text: str, *, encoding: str = "utf-8") -> bool:
    return write_output_bytes_if_changed(path, text.encode(encoding))


def write_generated_runtime_bindings_header(output_path: Path) -> bool:
    return write_output_text_if_changed(output_path, render_generated_runtime_bindings_header(), encoding="utf-8")


class Lowerer:
    def __init__(self, source: str) -> None:
        self.source = source
        self.literals: list[Literal] = []
        self.instructions: list[Instruction] = []
        self.lexical_map: dict[str, int] = {}

    def build(self) -> Program:
        compiled = compile_do_it(self.source, "Object", [])
        if len(compiled.temp_names) > PROGRAM_LEXICAL_LIMIT:
            raise LoweringError("MVP program lexical count exceeds runtime capacity")
        self.lexical_map = {name: index for index, name in enumerate(compiled.temp_names)}
        for name in compiled.temp_names:
            encoded_name = name.encode("ascii")
            if len(encoded_name) >= PROGRAM_LEXICAL_NAME_LIMIT:
                raise LoweringError("MVP program lexical name exceeds runtime capacity")
        for instruction in compiled.instructions:
            self.lower_instruction(instruction.opcode, instruction.operand, compiled.literals)
        return Program(
            literals=self.literals,
            instructions=self.instructions,
            lexical_count=len(compiled.temp_names),
            lexical_names=list(compiled.temp_names),
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

        if opcode == "make_block":
            compiled_block = compiled_literals[int(operand)]
            if not isinstance(compiled_block, CompiledBlock):
                raise LoweringError(f"Expected a compiled block literal, found {compiled_block!r}")
            if not compiled_block.source.strip():
                raise LoweringError("The MVP target requires block literals to preserve their source")
            self.instructions.append(
                Instruction(OP_PUSH_BLOCK_LITERAL, operand_b=self.literal_index(compiled_block.source))
            )
            return

        if opcode == "push_nil":
            self.instructions.append(Instruction(OP_PUSH_NIL))
            return

        if opcode == "push_self":
            self.instructions.append(Instruction(OP_PUSH_SELF))
            return

        if opcode == "push_true":
            self.instructions.append(Instruction(OP_PUSH_GLOBAL, GLOBAL_IDS["true"]))
            return

        if opcode == "push_false":
            self.instructions.append(Instruction(OP_PUSH_GLOBAL, GLOBAL_IDS["false"]))
            return

        if opcode == "push_this_context":
            self.instructions.append(Instruction(OP_PUSH_THIS_CONTEXT))
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


def build_boot_program(source: str) -> Program:
    program = build_program(source)
    source_literal = Literal(LITERAL_STRING, source)
    literals = list(program.literals)

    if source_literal not in literals:
        literals.append(source_literal)
    source_literal_index = literals.index(source_literal)
    prelude = [
        Instruction(OP_PUSH_SELF),
        Instruction(OP_PUSH_LITERAL, operand_b=source_literal_index),
        Instruction(OP_SEND, "RECORZ_MVP_SELECTOR_SEED_BOOT_CONTENTS", 1),
        Instruction(OP_POP),
    ]
    return Program(
        literals=literals,
        instructions=prelude + list(program.instructions),
        lexical_count=program.lexical_count,
        lexical_names=list(program.lexical_names),
    )


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
    for name in program.lexical_names:
        encoded_name = name.encode("ascii")
        if len(encoded_name) >= PROGRAM_LEXICAL_NAME_LIMIT:
            raise LoweringError("MVP program lexical name exceeds runtime capacity")
        manifest.extend(encoded_name)
        manifest.append(0)
    return bytes(manifest)


def build_named_object_bindings(
    names: list[str],
    ids: dict[str, str],
    values: dict[str, int],
    object_names_by_name: dict[str, str],
    object_indices_by_name: dict[str, int],
) -> list[tuple[int, int]]:
    bindings: list[tuple[int, int]] = []

    for name in names:
        object_name = object_names_by_name[name]
        bindings.append((constant_value(ids, values, name), object_indices_by_name[object_name]))
    return bindings


def materialize_boot_object_fields(
    field_specs: tuple[tuple[str, object], ...],
    object_indices_by_name: dict[str, int],
    glyph_object_indices: list[int],
) -> list[tuple[int, int]]:
    fields: list[tuple[int, int]] = []

    for field_kind, field_value in field_specs:
        if field_kind == FIELD_SPEC_SMALL_INTEGER:
            fields.append((SEED_FIELD_SMALL_INTEGER, int(field_value)))
            continue
        if field_kind == FIELD_SPEC_OBJECT_REF:
            fields.append((SEED_FIELD_OBJECT_INDEX, object_indices_by_name[str(field_value)]))
            continue
        if field_kind == FIELD_SPEC_GLYPH_REF:
            fields.append((SEED_FIELD_OBJECT_INDEX, glyph_object_indices[int(field_value)]))
            continue
        raise AssertionError(f"unknown boot object field spec kind {field_kind!r}")
    return fields


def build_class_seed_objects(
    class_kind_order: list[int],
    class_class_index: int,
    method_start_by_kind: dict[int, int],
    method_count_by_kind: dict[int, int],
) -> tuple[dict[int, int], list[SeedObject]]:
    class_indices = build_class_index_map(class_kind_order, class_class_index)
    class_seed_objects = [
        SeedObject(
            SEED_OBJECT_CLASS,
            class_class_index,
            materialize_named_seed_fields(
                "Class",
                {
                    "superclass": (SEED_FIELD_NIL, 0),
                    "instanceKind": (SEED_FIELD_SMALL_INTEGER, class_kind),
                    "methodStart": (
                        SEED_FIELD_OBJECT_INDEX if class_kind in method_start_by_kind else SEED_FIELD_NIL,
                        method_start_by_kind.get(class_kind, 0),
                    ),
                    "methodCount": (SEED_FIELD_SMALL_INTEGER, method_count_by_kind.get(class_kind, 0)),
                },
            ),
        )
        for class_kind in class_kind_order
    ]
    return class_indices, class_seed_objects


def build_class_index_map(class_kind_order: list[int], class_class_index: int) -> dict[int, int]:
    class_indices = {
        class_kind: class_class_index + offset
        for offset, class_kind in enumerate(class_kind_order[1:], start=1)
    }
    return class_indices


def build_seed_layout(
    base_object_index: int,
    class_kind_order: list[int] | tuple[int, ...],
    selector_value_order: list[int] | tuple[int, ...] | None = None,
    compiled_method_entry_order: list[str] | tuple[str, ...] | None = None,
    method_entry_order: list[str] | tuple[str, ...] | None = None,
    seed_layout_section_specs: list[SeedLayoutSectionSpec] | tuple[SeedLayoutSectionSpec, ...] | None = None,
) -> dict[str, SeedLayoutSection]:
    if selector_value_order is None:
        selector_value_order = SELECTOR_VALUE_ORDER
    if compiled_method_entry_order is None:
        compiled_method_entry_order = COMPILED_METHOD_ENTRY_ORDER
    if method_entry_order is None:
        method_entry_order = METHOD_ENTRY_ORDER
    if seed_layout_section_specs is None:
        seed_layout_section_specs = SEED_LAYOUT_SECTION_SPECS
    section_count_sources = {
        "class_kind_order": len(class_kind_order),
        "selector_value_order": len(selector_value_order),
        "compiled_method_entry_order": len(compiled_method_entry_order),
        "method_entry_order": len(method_entry_order),
        "builtin_method_definitions": sum(len(BUILTIN_METHODS_BY_KIND.get(class_kind, [])) for class_kind in class_kind_order),
    }
    layout: dict[str, SeedLayoutSection] = {}
    next_index = base_object_index

    for section_spec in seed_layout_section_specs:
        count = section_count_sources[section_spec.count_source]
        layout[section_spec.name] = SeedLayoutSection(next_index, count)
        next_index += count

    return layout


def validate_dynamic_seed_section_counts(
    seed_layout: dict[str, SeedLayoutSection],
    dynamic_sections: DynamicSeedSections,
    dynamic_seed_object_section_specs: tuple[DynamicSeedObjectSectionSpec, ...] | None = None,
) -> None:
    if dynamic_seed_object_section_specs is None:
        dynamic_seed_object_section_specs = DYNAMIC_SEED_OBJECT_SECTION_SPECS
    for section_spec in dynamic_seed_object_section_specs:
        seed_objects = dynamic_sections.seed_objects_for_layout_section(section_spec.layout_section_name)
        if len(seed_objects) != seed_layout[section_spec.layout_section_name].count:
            raise AssertionError(
                f"{section_spec.layout_section_name} seed object count does not match declared seed layout"
            )


def build_selector_seed_section(
    build_state: DynamicSeedBuildState,
) -> DynamicSeedBuildStepResult:
    selector_indices_by_value, selector_seed_objects = build_selector_seed_objects(
        build_state.seed_layout["selectors"].start_index,
        build_state.class_indices[SEED_OBJECT_SELECTOR],
        build_state.selector_value_order,
    )
    return DynamicSeedBuildStepResult(
        selector_seed_objects,
        {
            "selector_indices_by_value": selector_indices_by_value,
        },
    )


def build_compiled_method_seed_section(
    build_state: DynamicSeedBuildState,
) -> DynamicSeedBuildStepResult:
    compiled_method_indices, compiled_method_seed_objects = build_compiled_method_seed_objects(
        build_state.seed_layout["compiled_methods"].start_index,
        build_state.class_indices[SEED_OBJECT_COMPILED_METHOD],
        build_state.compiled_method_entry_order,
    )
    return DynamicSeedBuildStepResult(
        compiled_method_seed_objects,
        {
            "compiled_method_indices": compiled_method_indices,
        },
    )


def build_method_entry_seed_section(
    build_state: DynamicSeedBuildState,
) -> DynamicSeedBuildStepResult:
    method_entry_indices, method_entry_seed_objects = build_method_entry_seed_objects(
        build_state.seed_layout["method_entries"].start_index,
        build_state.class_indices[SEED_OBJECT_METHOD_ENTRY],
        build_state.compiled_method_indices,
        build_state.method_entry_order,
    )
    return DynamicSeedBuildStepResult(
        method_entry_seed_objects,
        {
            "method_entry_indices": method_entry_indices,
        },
    )


def build_method_descriptor_seed_section(
    build_state: DynamicSeedBuildState,
) -> DynamicSeedBuildStepResult:
    method_start_by_kind, method_count_by_kind, method_seed_objects = (
        build_method_descriptor_seed_objects(
            build_state.class_kind_order,
            build_state.class_indices[SEED_OBJECT_METHOD_DESCRIPTOR],
            build_state.selector_indices_by_value,
            build_state.method_entry_indices,
            build_state.seed_layout["method_descriptors"].start_index,
        )
    )
    return DynamicSeedBuildStepResult(
        method_seed_objects,
        {
            "method_start_by_kind": method_start_by_kind,
            "method_count_by_kind": method_count_by_kind,
        },
    )


def build_class_seed_section(
    build_state: DynamicSeedBuildState,
) -> DynamicSeedBuildStepResult:
    class_indices, class_seed_objects = build_class_seed_objects(
        build_state.class_kind_order,
        build_state.class_class_index,
        build_state.method_start_by_kind,
        build_state.method_count_by_kind,
    )
    if class_indices != build_state.class_indices:
        raise AssertionError("class seed section rebuilt unexpected class indices")
    return DynamicSeedBuildStepResult(class_seed_objects, {})


DYNAMIC_SEED_SECTION_SPECS = [
    DynamicSeedSectionSpec(
        "class_descriptors",
        "class_kind_order",
        build_class_seed_section,
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
    DynamicSeedSectionSpec(
        "selectors",
        "selector_value_order",
        build_selector_seed_section,
        0,
        required_state_fields=("seed_layout", "class_indices", "selector_value_order"),
        state_update_fields=("selector_indices_by_value",),
    ),
    DynamicSeedSectionSpec(
        "compiled_methods",
        "compiled_method_entry_order",
        build_compiled_method_seed_section,
        1,
        required_state_fields=("seed_layout", "class_indices", "compiled_method_entry_order"),
        state_update_fields=("compiled_method_indices",),
    ),
    DynamicSeedSectionSpec(
        "method_entries",
        "method_entry_order",
        build_method_entry_seed_section,
        2,
        ("compiled_methods",),
        ("seed_layout", "class_indices", "compiled_method_indices", "method_entry_order"),
        ("method_entry_indices",),
    ),
    DynamicSeedSectionSpec(
        "method_descriptors",
        "builtin_method_definitions",
        build_method_descriptor_seed_section,
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
]
def build_initial_dynamic_seed_state_fields(
    boot_image_spec: BootImageSpec,
) -> tuple[str, ...]:
    ordering_field_names = tuple(BootImageOrderingSpec.__annotations__)
    return (
        "seed_layout",
        ordering_field_names[0],
        "class_class_index",
        "class_indices",
        *ordering_field_names[1:],
    )


def build_seed_layout_section_specs(
    dynamic_seed_section_specs: tuple[DynamicSeedSectionSpec, ...],
) -> list[SeedLayoutSectionSpec]:
    return [
        SeedLayoutSectionSpec(section_spec.layout_section_name, section_spec.count_source)
        for section_spec in dynamic_seed_section_specs
    ]


def build_dynamic_seed_object_section_specs(
    dynamic_seed_section_specs: tuple[DynamicSeedSectionSpec, ...],
) -> list[DynamicSeedObjectSectionSpec]:
    return [
        DynamicSeedObjectSectionSpec(section_spec.layout_section_name)
        for section_spec in dynamic_seed_section_specs
    ]


def build_dynamic_seed_build_step_specs(
    dynamic_seed_section_specs: tuple[DynamicSeedSectionSpec, ...],
) -> list[DynamicSeedBuildStepSpec]:
    build_orders = [section_spec.build_order for section_spec in dynamic_seed_section_specs]
    if len(set(build_orders)) != len(build_orders):
        raise AssertionError("dynamic seed section specs declare duplicate build orders")
    ordered_section_specs = sorted(dynamic_seed_section_specs, key=lambda section_spec: section_spec.build_order)
    return [
        DynamicSeedBuildStepSpec(
            section_spec.layout_section_name,
            section_spec.builder,
            section_spec.required_layout_sections,
            section_spec.required_state_fields,
            section_spec.state_update_fields,
        )
        for section_spec in ordered_section_specs
    ]


def build_boot_image_spec(
    fixed_boot_graph_spec: FixedBootGraphSpec,
    ordering_spec: BootImageOrderingSpec,
    dynamic_seed_section_specs: list[DynamicSeedSectionSpec],
) -> BootImageSpec:
    return BootImageSpec(
        fixed_boot_graph_spec=fixed_boot_graph_spec,
        ordering_spec=ordering_spec,
        dynamic_seed_section_specs=tuple(dynamic_seed_section_specs),
    )


def build_boot_image_seed_build_context(
    boot_image_spec: BootImageSpec,
) -> BootImageSeedBuildContext:
    boot_object_specs_in_order = flatten_boot_object_specs(boot_image_spec.fixed_boot_graph_spec)
    glyph_bitmap_boot_specs = build_boot_object_family_spec_map(boot_image_spec.fixed_boot_graph_spec)["glyph_bitmaps"].object_specs
    seed_layout_section_specs = build_seed_layout_section_specs(boot_image_spec.dynamic_seed_section_specs)
    dynamic_seed_object_section_specs = build_dynamic_seed_object_section_specs(boot_image_spec.dynamic_seed_section_specs)
    dynamic_seed_build_step_specs = build_dynamic_seed_build_step_specs(boot_image_spec.dynamic_seed_section_specs)
    return BootImageSeedBuildContext(
        boot_image_spec=boot_image_spec,
        class_kind_order=boot_image_spec.ordering_spec.class_kind_order,
        selector_value_order=boot_image_spec.ordering_spec.selector_value_order,
        compiled_method_entry_order=boot_image_spec.ordering_spec.compiled_method_entry_order,
        method_entry_order=boot_image_spec.ordering_spec.method_entry_order,
        initial_dynamic_seed_state_fields=build_initial_dynamic_seed_state_fields(boot_image_spec),
        fixed_boot_object_count=len(boot_object_specs_in_order),
        glyph_bitmap_boot_specs=glyph_bitmap_boot_specs,
        seed_layout_section_specs=tuple(seed_layout_section_specs),
        dynamic_seed_object_section_specs=tuple(dynamic_seed_object_section_specs),
        dynamic_seed_build_step_specs=tuple(dynamic_seed_build_step_specs),
        global_name_to_boot_object_name=build_boot_object_export_map(boot_object_specs_in_order, "global"),
        seed_root_name_to_boot_object_name=build_root_object_name_map_from_declarations(
            KERNEL_ROOT_DECLARATIONS_IN_ORDER
        ),
    )


BOOT_IMAGE_ORDERING_SPEC = BootImageOrderingSpec(
    class_kind_order=tuple(CLASS_DESCRIPTOR_KIND_ORDER),
    selector_value_order=tuple(SELECTOR_VALUE_ORDER),
    compiled_method_entry_order=tuple(COMPILED_METHOD_ENTRY_ORDER),
    method_entry_order=tuple(METHOD_ENTRY_ORDER),
)
BOOT_IMAGE_SPEC = build_boot_image_spec(FIXED_BOOT_GRAPH_SPEC, BOOT_IMAGE_ORDERING_SPEC, DYNAMIC_SEED_SECTION_SPECS)
BOOT_OBJECT_FAMILY_SPECS = list(BOOT_IMAGE_SPEC.fixed_boot_graph_spec.family_specs)
BOOT_OBJECT_FAMILY_SPECS_BY_NAME = build_boot_object_family_spec_map(BOOT_IMAGE_SPEC.fixed_boot_graph_spec)
BOOT_OBJECT_FAMILY_NAMES = [family_spec.name for family_spec in BOOT_IMAGE_SPEC.fixed_boot_graph_spec.family_specs]
BOOT_OBJECT_SPECS_BEFORE_GLYPHS = BOOT_OBJECT_FAMILY_SPECS_BY_NAME["before_glyphs"].object_specs
GLYPH_BITMAP_BOOT_SPECS = BOOT_OBJECT_FAMILY_SPECS_BY_NAME["glyph_bitmaps"].object_specs
BOOT_OBJECT_SPECS_AFTER_GLYPHS = BOOT_OBJECT_FAMILY_SPECS_BY_NAME["after_glyphs"].object_specs
BOOT_OBJECT_SPECS_IN_ORDER = flatten_boot_object_specs(BOOT_IMAGE_SPEC.fixed_boot_graph_spec)
BOOT_OBJECT_SPEC_NAMES_IN_ORDER = [object_spec.name for object_spec in BOOT_OBJECT_SPECS_IN_ORDER]
BOOT_OBJECT_SPECS_BY_NAME = build_boot_object_spec_map(BOOT_OBJECT_SPECS_IN_ORDER)
BOOT_OBJECT_FIXED_COUNT = len(BOOT_OBJECT_SPECS_IN_ORDER)
GLOBAL_NAME_TO_BOOT_OBJECT_NAME = build_boot_object_export_map(BOOT_OBJECT_SPECS_IN_ORDER, "global")
SEED_ROOT_NAME_TO_BOOT_OBJECT_NAME = build_root_object_name_map_from_declarations(
    KERNEL_ROOT_DECLARATIONS_IN_ORDER
)
SEED_LAYOUT_SECTION_SPECS = build_seed_layout_section_specs(BOOT_IMAGE_SPEC.dynamic_seed_section_specs)
SEED_LAYOUT_SECTION_NAMES = [section_spec.name for section_spec in SEED_LAYOUT_SECTION_SPECS]
DYNAMIC_SEED_OBJECT_SECTION_SPECS = build_dynamic_seed_object_section_specs(BOOT_IMAGE_SPEC.dynamic_seed_section_specs)
DYNAMIC_SEED_BUILD_STEP_SPECS = build_dynamic_seed_build_step_specs(BOOT_IMAGE_SPEC.dynamic_seed_section_specs)
BOOT_IMAGE_SEED_BUILD_CONTEXT = build_boot_image_seed_build_context(
    BOOT_IMAGE_SPEC,
)
INITIAL_DYNAMIC_SEED_STATE_FIELDS = BOOT_IMAGE_SEED_BUILD_CONTEXT.initial_dynamic_seed_state_fields


def build_selector_seed_objects(
    selector_start_index: int,
    selector_class_index: int,
    selector_value_order: list[int] | tuple[int, ...] = SELECTOR_VALUE_ORDER,
) -> tuple[dict[int, int], list[SeedObject]]:
    selector_indices_by_value: dict[int, int] = {}
    selector_seed_objects: list[SeedObject] = []

    for selector_value in selector_value_order:
        selector_indices_by_value[selector_value] = selector_start_index + len(selector_seed_objects)
        selector_seed_objects.append(
            SeedObject(
                SEED_OBJECT_SELECTOR,
                selector_class_index,
                materialize_named_seed_fields(
                    "Selector",
                    {
                        "value": (SEED_FIELD_SMALL_INTEGER, selector_value),
                    },
                ),
            )
        )

    return selector_indices_by_value, selector_seed_objects


def build_compiled_method_seed_objects(
    compiled_method_start_index: int,
    compiled_method_class_index: int,
    compiled_method_entry_order: list[str] | tuple[str, ...] = COMPILED_METHOD_ENTRY_ORDER,
) -> tuple[dict[str, int], list[SeedObject]]:
    compiled_method_indices: dict[str, int] = {}
    compiled_method_seed_objects: list[SeedObject] = []

    for entry_name in compiled_method_entry_order:
        program = COMPILED_METHOD_PROGRAM_BY_ENTRY_NAME[entry_name]
        compiled_method_indices[entry_name] = compiled_method_start_index + len(compiled_method_seed_objects)
        compiled_method_seed_objects.append(
            SeedObject(
                SEED_OBJECT_COMPILED_METHOD,
                compiled_method_class_index,
                materialize_named_seed_fields(
                    "CompiledMethod",
                    {
                        f"word{index}": (SEED_FIELD_SMALL_INTEGER, instruction)
                        for index, instruction in enumerate(program)
                    },
                ),
            )
        )

    return compiled_method_indices, compiled_method_seed_objects


def build_method_entry_seed_objects(
    method_entry_start_index: int,
    method_entry_class_index: int,
    compiled_method_indices: dict[str, int],
    method_entry_order: list[str] | tuple[str, ...] = METHOD_ENTRY_ORDER,
) -> tuple[dict[str, int], list[SeedObject]]:
    method_entry_indices: dict[str, int] = {}
    method_entry_seed_objects: list[SeedObject] = []

    for entry_name in method_entry_order:
        implementation_field_kind = SEED_FIELD_NIL
        implementation_field_value = 0

        if entry_name in compiled_method_indices:
            implementation_field_kind = SEED_FIELD_OBJECT_INDEX
            implementation_field_value = compiled_method_indices[entry_name]
        elif entry_name in PRIMITIVE_BINDING_BY_ENTRY_NAME:
            implementation_field_kind = SEED_FIELD_SMALL_INTEGER
            implementation_field_value = PRIMITIVE_BINDING_BY_ENTRY_NAME[entry_name]
        method_entry_indices[entry_name] = method_entry_start_index + len(method_entry_seed_objects)
        method_entry_seed_objects.append(
            SeedObject(
                SEED_OBJECT_METHOD_ENTRY,
                method_entry_class_index,
                materialize_named_seed_fields(
                    "MethodEntry",
                    {
                        "executionId": (SEED_FIELD_SMALL_INTEGER, METHOD_ENTRY_VALUES[entry_name]),
                        "implementation": (implementation_field_kind, implementation_field_value),
                    },
                ),
            )
        )

    return method_entry_indices, method_entry_seed_objects


def build_method_descriptor_seed_objects(
    class_kind_order: list[int],
    method_descriptor_class_index: int,
    selector_indices_by_value: dict[int, int],
    method_entry_indices: dict[str, int],
    method_start_index: int,
) -> tuple[dict[int, int], dict[int, int], list[SeedObject]]:
    method_start_by_kind: dict[int, int] = {}
    method_count_by_kind: dict[int, int] = {}
    method_seed_objects: list[SeedObject] = []

    for class_kind in class_kind_order:
        method_definitions = BUILTIN_METHODS_BY_KIND.get(class_kind, [])
        method_count_by_kind[class_kind] = len(method_definitions)
        if method_definitions:
            method_start_by_kind[class_kind] = method_start_index + len(method_seed_objects)
        for selector, argument_count, entry_name in method_definitions:
            method_seed_objects.append(
                SeedObject(
                    SEED_OBJECT_METHOD_DESCRIPTOR,
                    method_descriptor_class_index,
                    materialize_named_seed_fields(
                        "MethodDescriptor",
                        {
                            "selector": (
                                SEED_FIELD_OBJECT_INDEX,
                                selector_indices_by_value[SELECTOR_VALUES[SELECTOR_IDS[selector]]],
                            ),
                            "argumentCount": (SEED_FIELD_SMALL_INTEGER, argument_count),
                            "primitiveKind": (SEED_FIELD_SMALL_INTEGER, class_kind),
                            "entry": (SEED_FIELD_OBJECT_INDEX, method_entry_indices[entry_name]),
                        },
                    ),
                )
            )

    return method_start_by_kind, method_count_by_kind, method_seed_objects


def add_named_seed_object(
    seed_objects: list[SeedObject],
    seed_object_indices_by_name: dict[str, int],
    name: str,
    object_kind: int,
    fields: list[tuple[int, int]],
) -> int:
    object_index = len(seed_objects)

    if name in seed_object_indices_by_name:
        raise AssertionError(f"duplicate seed object name {name!r}")
    seed_object_indices_by_name[name] = object_index
    seed_objects.append(SeedObject(object_kind, SEED_INVALID_OBJECT_INDEX, fields))
    return object_index


def build_fixed_boot_seed_objects(
    build_context: BootImageSeedBuildContext = BOOT_IMAGE_SEED_BUILD_CONTEXT,
) -> tuple[list[SeedObject], dict[str, int], list[int]]:
    seed_objects: list[SeedObject] = []
    seed_object_indices_by_name: dict[str, int] = {}
    glyph_object_indices: list[int] = []

    for family_spec in build_context.boot_image_spec.fixed_boot_graph_spec.family_specs:
        for spec in family_spec.object_specs:
            object_index = add_named_seed_object(
                seed_objects,
                seed_object_indices_by_name,
                spec.name,
                constant_value(OBJECT_KIND_IDS, OBJECT_KIND_VALUES, spec.object_kind_name),
                materialize_boot_object_fields(spec.field_specs, seed_object_indices_by_name, glyph_object_indices),
            )
            if family_spec.collect_object_indices:
                glyph_object_indices.append(object_index)

    if len(seed_objects) != build_context.fixed_boot_object_count:
        raise AssertionError("boot object count does not match declared boot object families")
    if len(glyph_object_indices) != len(build_context.glyph_bitmap_boot_specs):
        raise AssertionError("glyph object indices do not match declared glyph bitmap specs")

    return seed_objects, seed_object_indices_by_name, glyph_object_indices


def validate_dynamic_seed_build_step_specs(
    build_context: BootImageSeedBuildContext = BOOT_IMAGE_SEED_BUILD_CONTEXT,
) -> None:
    declared_section_names = {
        section_spec.layout_section_name
        for section_spec in build_context.dynamic_seed_object_section_specs
    }
    produced_section_names: set[str] = set()
    produced_state_field_names: set[str] = set()
    valid_state_field_names = set(DynamicSeedBuildState.__annotations__)
    available_state_field_names = set(build_context.initial_dynamic_seed_state_fields)

    for build_step_spec in build_context.dynamic_seed_build_step_specs:
        if build_step_spec.layout_section_name not in declared_section_names:
            raise AssertionError(
                f"dynamic seed build step declares unknown section {build_step_spec.layout_section_name!r}"
            )
        if build_step_spec.layout_section_name in produced_section_names:
            raise AssertionError(
                f"dynamic seed build steps declare duplicate section {build_step_spec.layout_section_name!r}"
            )
        missing_required_sections = [
            section_name
            for section_name in build_step_spec.required_layout_sections
            if section_name not in produced_section_names
        ]
        if missing_required_sections:
            raise AssertionError(
                f"dynamic seed build step {build_step_spec.layout_section_name!r} requires undeclared prior sections "
                + ", ".join(repr(section_name) for section_name in missing_required_sections)
            )
        invalid_required_state_fields = [
            field_name
            for field_name in build_step_spec.required_state_fields
            if field_name not in valid_state_field_names
        ]
        if invalid_required_state_fields:
            raise AssertionError(
                f"dynamic seed build step {build_step_spec.layout_section_name!r} declares unknown required state fields "
                + ", ".join(repr(field_name) for field_name in invalid_required_state_fields)
            )
        unavailable_required_state_fields = [
            field_name
            for field_name in build_step_spec.required_state_fields
            if field_name not in available_state_field_names
        ]
        if unavailable_required_state_fields:
            raise AssertionError(
                f"dynamic seed build step {build_step_spec.layout_section_name!r} requires unavailable prior state fields "
                + ", ".join(repr(field_name) for field_name in unavailable_required_state_fields)
            )
        invalid_state_fields = [
            field_name
            for field_name in build_step_spec.state_update_fields
            if field_name not in valid_state_field_names
        ]
        if invalid_state_fields:
            raise AssertionError(
                f"dynamic seed build step {build_step_spec.layout_section_name!r} declares unknown state updates "
                + ", ".join(repr(field_name) for field_name in invalid_state_fields)
            )
        duplicate_state_fields = [
            field_name
            for field_name in build_step_spec.state_update_fields
            if field_name in produced_state_field_names
        ]
        if duplicate_state_fields:
            raise AssertionError(
                f"dynamic seed build step {build_step_spec.layout_section_name!r} redeclares state updates "
                + ", ".join(repr(field_name) for field_name in duplicate_state_fields)
            )
        produced_section_names.add(build_step_spec.layout_section_name)
        produced_state_field_names.update(build_step_spec.state_update_fields)
        available_state_field_names.update(build_step_spec.state_update_fields)

    if produced_section_names != declared_section_names:
        missing_section_names = sorted(declared_section_names - produced_section_names)
        extra_section_names = sorted(produced_section_names - declared_section_names)
        details: list[str] = []
        if missing_section_names:
            details.append("missing " + ", ".join(repr(section_name) for section_name in missing_section_names))
        if extra_section_names:
            details.append("extra " + ", ".join(repr(section_name) for section_name in extra_section_names))
        raise AssertionError("dynamic seed build step coverage does not match declared object sections: " + "; ".join(details))


def apply_dynamic_seed_build_step_result(
    build_state: DynamicSeedBuildState,
    build_step_spec: DynamicSeedBuildStepSpec,
    build_step_result: DynamicSeedBuildStepResult,
) -> list[SeedObject]:
    state_update_fields = tuple(build_step_result.state_updates)

    if state_update_fields != build_step_spec.state_update_fields:
        raise AssertionError(
            f"dynamic seed build step {build_step_spec.layout_section_name!r} returned unexpected state updates "
            f"{state_update_fields!r}; expected {build_step_spec.state_update_fields!r}"
        )
    for field_name in build_step_spec.state_update_fields:
        setattr(build_state, field_name, build_step_result.state_updates[field_name])
    return build_step_result.seed_objects


def build_dynamic_seed_sections(
    seed_objects: list[SeedObject],
    build_context: BootImageSeedBuildContext = BOOT_IMAGE_SEED_BUILD_CONTEXT,
) -> DynamicSeedSections:
    validate_dynamic_seed_build_step_specs(build_context)
    seed_layout = build_seed_layout(
        build_context.fixed_boot_object_count,
        build_context.class_kind_order,
        selector_value_order=build_context.selector_value_order,
        compiled_method_entry_order=build_context.compiled_method_entry_order,
        method_entry_order=build_context.method_entry_order,
        seed_layout_section_specs=build_context.seed_layout_section_specs,
    )
    class_class_index = seed_layout["class_descriptors"].start_index
    class_kind_order = list(build_context.class_kind_order)
    class_indices = build_class_index_map(class_kind_order, class_class_index)
    build_state = DynamicSeedBuildState(
        seed_layout=seed_layout,
        class_kind_order=class_kind_order,
        class_class_index=class_class_index,
        class_indices=class_indices,
        selector_value_order=build_context.selector_value_order,
        compiled_method_entry_order=build_context.compiled_method_entry_order,
        method_entry_order=build_context.method_entry_order,
        selector_indices_by_value={},
        compiled_method_indices={},
        method_entry_indices={},
        method_start_by_kind={},
        method_count_by_kind={},
    )
    dynamic_section_results: dict[str, list[SeedObject]] = {}
    built_layout_sections: set[str] = set()
    available_state_fields = set(build_context.initial_dynamic_seed_state_fields)

    for seed_object in seed_objects:
        seed_object.class_index = class_indices[seed_object.object_kind]

    for build_step_spec in build_context.dynamic_seed_build_step_specs:
        missing_required_sections = [
            section_name
            for section_name in build_step_spec.required_layout_sections
            if section_name not in built_layout_sections
        ]
        if missing_required_sections:
            raise AssertionError(
                f"dynamic seed build step {build_step_spec.layout_section_name!r} is running before required sections "
                + ", ".join(repr(section_name) for section_name in missing_required_sections)
            )
        missing_required_state_fields = [
            field_name
            for field_name in build_step_spec.required_state_fields
            if field_name not in available_state_fields
        ]
        if missing_required_state_fields:
            raise AssertionError(
                f"dynamic seed build step {build_step_spec.layout_section_name!r} is running before required state fields "
                + ", ".join(repr(field_name) for field_name in missing_required_state_fields)
            )
        dynamic_section_results[build_step_spec.layout_section_name] = apply_dynamic_seed_build_step_result(
            build_state,
            build_step_spec,
            build_step_spec.builder(build_state),
        )
        built_layout_sections.add(build_step_spec.layout_section_name)
        available_state_fields.update(build_step_spec.state_update_fields)
    dynamic_sections = DynamicSeedSections(
        seed_layout=seed_layout,
        class_indices=build_state.class_indices,
        object_sections=dynamic_section_results,
    )
    validate_dynamic_seed_section_counts(seed_layout, dynamic_sections)
    return dynamic_sections


def build_seed_bindings(
    seed_object_indices_by_name: dict[str, int],
    build_context: BootImageSeedBuildContext = BOOT_IMAGE_SEED_BUILD_CONTEXT,
) -> SeedBindings:
    return SeedBindings(
        global_bindings=build_named_object_bindings(
            [name for name, _constant_name in GLOBAL_SPECS],
            GLOBAL_IDS,
            GLOBAL_VALUES,
            build_context.global_name_to_boot_object_name,
            seed_object_indices_by_name,
        ),
        root_bindings=build_named_object_bindings(
            [name for name, _constant_name in SEED_ROOT_SPECS],
            SEED_ROOT_IDS,
            SEED_ROOT_VALUES,
            build_context.seed_root_name_to_boot_object_name,
            seed_object_indices_by_name,
        ),
    )


def encode_seed_manifest(
    seed_objects: list[SeedObject],
    bindings: SeedBindings,
    glyph_object_indices: list[int],
    build_context: BootImageSeedBuildContext = BOOT_IMAGE_SEED_BUILD_CONTEXT,
) -> bytes:
    manifest = bytearray(
        struct.pack(
            SEED_HEADER_FORMAT,
            SEED_MAGIC,
            SEED_VERSION,
            len(seed_objects),
            len(bindings.global_bindings),
            len(bindings.root_bindings),
            len(build_context.glyph_bitmap_boot_specs),
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
    for binding_id, object_index in bindings.global_bindings:
        manifest.extend(struct.pack(SEED_BINDING_FORMAT, binding_id, object_index))
    for binding_id, object_index in bindings.root_bindings:
        manifest.extend(struct.pack(SEED_BINDING_FORMAT, binding_id, object_index))
    for object_index in glyph_object_indices:
        manifest.extend(struct.pack("<H", object_index))
    return bytes(manifest)


def build_seed_manifest() -> bytes:
    build_context = BOOT_IMAGE_SEED_BUILD_CONTEXT
    seed_objects, seed_object_indices_by_name, glyph_object_indices = build_fixed_boot_seed_objects(build_context)
    dynamic_sections = build_dynamic_seed_sections(seed_objects, build_context)

    for section_spec in build_context.dynamic_seed_object_section_specs:
        seed_objects.extend(dynamic_sections.seed_objects_for_layout_section(section_spec.layout_section_name))
    bindings = build_seed_bindings(seed_object_indices_by_name, build_context)
    return encode_seed_manifest(seed_objects, bindings, glyph_object_indices, build_context)


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


def describe_builder_ownership() -> BuilderOwnershipSummary:
    return BuilderOwnershipSummary(
        runtime_spec_path=str(RUNTIME_SPEC_PATH),
        source_root=str(KERNEL_MVP_ROOT),
        generated_text_outputs=(
            "generated runtime bindings header",
        ),
        generated_binary_outputs=(
            "image manifest",
            "demo image",
        ),
        derived_metadata_surfaces=(
            "class headers",
            "selector declarations",
            "root declarations",
            "object kinds",
            "method entries",
            "primitive bindings",
            "seed layout",
        ),
    )


def describe_image_manifest(program: Program) -> ImageManifestLayout:
    entry_length = len(build_entry_manifest())
    program_length = len(build_program_manifest(program))
    seed_length = len(build_seed_manifest())
    section_count = 3
    header_size = struct.calcsize(IMAGE_HEADER_FORMAT) + (section_count * struct.calcsize(IMAGE_SECTION_FORMAT))
    entry_offset = header_size
    program_offset = entry_offset + entry_length
    seed_offset = program_offset + program_length
    return ImageManifestLayout(
        section_count=section_count,
        feature_flags=IMAGE_FEATURE_FNV1A32,
        entry_offset=entry_offset,
        program_offset=program_offset,
        seed_offset=seed_offset,
        entry_length=entry_length,
        program_length=program_length,
        seed_length=seed_length,
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
    manifest_layout = describe_image_manifest(program)

    manifest = bytearray(
        struct.pack(
            IMAGE_HEADER_FORMAT,
            IMAGE_MAGIC,
            IMAGE_VERSION,
            manifest_layout.section_count,
            manifest_layout.feature_flags,
            0,
            IMAGE_PROFILE,
        )
    )
    manifest.extend(
        struct.pack(
            IMAGE_SECTION_FORMAT,
            IMAGE_SECTION_ENTRY,
            0,
            manifest_layout.entry_offset,
            manifest_layout.entry_length,
        )
    )
    manifest.extend(
        struct.pack(
            IMAGE_SECTION_FORMAT,
            IMAGE_SECTION_PROGRAM,
            0,
            manifest_layout.program_offset,
            manifest_layout.program_length,
        )
    )
    manifest.extend(
        struct.pack(
            IMAGE_SECTION_FORMAT,
            IMAGE_SECTION_SEED,
            0,
            manifest_layout.seed_offset,
            manifest_layout.seed_length,
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
        manifest_layout.section_count,
        manifest_layout.feature_flags,
        checksum,
        IMAGE_PROFILE,
    )
    return bytes(manifest)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Recorz source file to lower")
    parser.add_argument("image_output", type=Path, help="Generated boot image path")
    args = parser.parse_args(argv)

    source_text = args.source.read_text(encoding="utf-8")
    program = build_boot_program(source_text)
    write_output_bytes_if_changed(args.image_output, build_image_manifest(program))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
