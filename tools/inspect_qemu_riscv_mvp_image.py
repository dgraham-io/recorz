#!/usr/bin/env python3
"""Inspect the QEMU RISC-V MVP boot image."""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools"))

import build_qemu_riscv_mvp_image as mvp  # noqa: E402


FEATURE_NAMES = {
    mvp.IMAGE_FEATURE_FNV1A32: "fnv1a32",
}
SECTION_NAMES = {
    mvp.IMAGE_SECTION_ENTRY: "entry",
    mvp.IMAGE_SECTION_PROGRAM: "program",
    mvp.IMAGE_SECTION_SEED: "seed",
}
ROOT_NAMES = {
    mvp.SEED_ROOT_DEFAULT_FORM: "default_form",
    mvp.SEED_ROOT_FRAMEBUFFER_BITMAP: "framebuffer_bitmap",
    mvp.SEED_ROOT_TRANSCRIPT_BEHAVIOR: "transcript_behavior",
    mvp.SEED_ROOT_TRANSCRIPT_LAYOUT: "transcript_layout",
    mvp.SEED_ROOT_TRANSCRIPT_STYLE: "transcript_style",
    mvp.SEED_ROOT_TRANSCRIPT_METRICS: "transcript_metrics",
}
PRIMITIVE_BINDING_BY_ENTRY_ID = {
    mvp.METHOD_ENTRY_VALUES[name]: binding_id
    for name, binding_id in mvp.PRIMITIVE_BINDING_BY_ENTRY_NAME.items()
}
COMPILED_METHOD_PROGRAM_BY_ENTRY_ID = {
    mvp.METHOD_ENTRY_VALUES[name]: list(program)
    for name, program in mvp.COMPILED_METHOD_PROGRAM_BY_ENTRY_NAME.items()
}


class ImageInspectionError(RuntimeError):
    """Raised when the boot image is structurally invalid."""


def _feature_names(feature_flags: int) -> list[str]:
    names: list[str] = []
    for bit, name in sorted(FEATURE_NAMES.items()):
        if feature_flags & bit:
            names.append(name)
    return names


def _section_name(kind: int) -> str:
    return SECTION_NAMES.get(kind, f"unknown:{kind}")


def inspect_program_manifest(blob: bytes) -> dict[str, object]:
    if len(blob) < struct.calcsize(mvp.PROGRAM_HEADER_FORMAT):
        raise ImageInspectionError("program manifest is truncated")
    magic, version, instruction_count, literal_count, lexical_count = struct.unpack_from(mvp.PROGRAM_HEADER_FORMAT, blob, 0)
    if magic != mvp.PROGRAM_MAGIC:
        raise ImageInspectionError("program manifest magic mismatch")
    return {
        "version": version,
        "instruction_count": instruction_count,
        "literal_count": literal_count,
        "lexical_count": lexical_count,
    }


def inspect_seed_manifest(blob: bytes) -> dict[str, object]:
    header_size = struct.calcsize(mvp.SEED_HEADER_FORMAT)
    object_size = struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT) + (4 * struct.calcsize(mvp.SEED_FIELD_FORMAT))
    object_header_size = struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
    field_size = struct.calcsize(mvp.SEED_FIELD_FORMAT)
    binding_size = struct.calcsize(mvp.SEED_BINDING_FORMAT)
    if len(blob) < header_size:
        raise ImageInspectionError("seed manifest is truncated")
    (
        magic,
        version,
        object_count,
        global_binding_count,
        root_binding_count,
        glyph_code_count,
        _reserved,
    ) = struct.unpack_from(mvp.SEED_HEADER_FORMAT, blob, 0)
    if magic != mvp.SEED_MAGIC:
        raise ImageInspectionError("seed manifest magic mismatch")
    expected_size = header_size + (object_count * object_size) + (global_binding_count * binding_size) + (
        root_binding_count * binding_size
    ) + (glyph_code_count * 2)
    if len(blob) != expected_size:
        raise ImageInspectionError("seed manifest size mismatch")

    offset = header_size
    objects: list[dict[str, object]] = []
    class_descriptor_count = 0
    class_link_count = 0
    method_descriptor_count = 0
    method_entry_object_count = 0
    selector_object_count = 0
    accessor_method_object_count = 0
    field_send_method_object_count = 0
    root_send_method_object_count = 0
    root_value_method_object_count = 0
    interpreted_method_object_count = 0
    compiled_method_object_count = 0
    accessor_method_kind = getattr(mvp, "SEED_OBJECT_ACCESSOR_METHOD", None)
    field_send_method_kind = getattr(mvp, "SEED_OBJECT_FIELD_SEND_METHOD", None)
    root_send_method_kind = getattr(mvp, "SEED_OBJECT_ROOT_SEND_METHOD", None)
    root_value_method_kind = getattr(mvp, "SEED_OBJECT_ROOT_VALUE_METHOD", None)
    interpreted_method_kind = getattr(mvp, "SEED_OBJECT_INTERPRETED_METHOD", None)
    selector_ids: set[int] = set()
    method_entry_ids: set[int] = set()
    for _ in range(object_count):
        object_kind, field_count, class_index = struct.unpack_from(mvp.SEED_OBJECT_HEADER_FORMAT, blob, offset)
        if field_count > 4:
            raise ImageInspectionError("seed manifest field count exceeds object field capacity")
        if class_index != mvp.SEED_INVALID_OBJECT_INDEX and class_index >= object_count:
            raise ImageInspectionError("seed manifest class index is out of range")
        fields: list[tuple[int, int]] = []
        field_offset = offset + object_header_size
        for _field_index in range(4):
            fields.append(struct.unpack_from(mvp.SEED_FIELD_FORMAT, blob, field_offset))
            field_offset += field_size
        if class_index != mvp.SEED_INVALID_OBJECT_INDEX:
            class_link_count += 1
        if object_kind == mvp.SEED_OBJECT_CLASS:
            class_descriptor_count += 1
        if object_kind == mvp.SEED_OBJECT_METHOD_DESCRIPTOR:
            method_descriptor_count += 1
        if object_kind == mvp.SEED_OBJECT_METHOD_ENTRY:
            method_entry_object_count += 1
        if object_kind == mvp.SEED_OBJECT_SELECTOR:
            if field_count <= 0:
                raise ImageInspectionError("seed manifest selector object is missing required fields")
            selector_id_field = fields[0]
            if (
                selector_id_field[0] != mvp.SEED_FIELD_SMALL_INTEGER
                or selector_id_field[1] < mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SHOW"]
                or selector_id_field[1] > mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_INSTANCE_KIND"]
            ):
                raise ImageInspectionError("seed manifest selector object id field is invalid")
            if int(selector_id_field[1]) in selector_ids:
                raise ImageInspectionError("seed manifest contains duplicate selector objects")
            selector_ids.add(int(selector_id_field[1]))
            selector_object_count += 1
        if accessor_method_kind is not None and object_kind == accessor_method_kind:
            accessor_method_object_count += 1
        if field_send_method_kind is not None and object_kind == field_send_method_kind:
            field_send_method_object_count += 1
        if root_send_method_kind is not None and object_kind == root_send_method_kind:
            root_send_method_object_count += 1
        if root_value_method_kind is not None and object_kind == root_value_method_kind:
            root_value_method_object_count += 1
        if interpreted_method_kind is not None and object_kind == interpreted_method_kind:
            interpreted_method_object_count += 1
        if object_kind == mvp.SEED_OBJECT_COMPILED_METHOD:
            compiled_method_object_count += 1
        objects.append(
            {
                "object_kind": object_kind,
                "field_count": field_count,
                "class_index": class_index,
                "fields": fields,
            }
        )
        offset += object_size

    declared_method_count = 0
    for object_summary in objects:
        class_index = int(object_summary["class_index"])
        if class_index == mvp.SEED_INVALID_OBJECT_INDEX:
            raise ImageInspectionError("seed manifest object is missing a class link")
        class_summary = objects[class_index]
        if class_summary["object_kind"] != mvp.SEED_OBJECT_CLASS:
            raise ImageInspectionError("seed manifest class link does not point at a class")
        metaclass_index = int(class_summary["class_index"])
        if metaclass_index == mvp.SEED_INVALID_OBJECT_INDEX:
            raise ImageInspectionError("seed manifest class is missing a metaclass link")
        if objects[metaclass_index]["object_kind"] != mvp.SEED_OBJECT_CLASS:
            raise ImageInspectionError("seed manifest metaclass link does not point at a class")
        if int(class_summary["field_count"]) <= mvp.CLASS_FIELD_INSTANCE_KIND:
            raise ImageInspectionError("seed manifest class descriptor is missing an instance kind")
        instance_kind_field = class_summary["fields"][mvp.CLASS_FIELD_INSTANCE_KIND]
        if instance_kind_field[0] != mvp.SEED_FIELD_SMALL_INTEGER:
            raise ImageInspectionError("seed manifest class instance kind field is invalid")
        if instance_kind_field[1] != object_summary["object_kind"]:
            raise ImageInspectionError("seed manifest class instance kind does not match object kind")
        if int(class_summary["field_count"]) > mvp.CLASS_FIELD_SUPERCLASS:
            superclass_field = class_summary["fields"][mvp.CLASS_FIELD_SUPERCLASS]
            if superclass_field[0] == mvp.SEED_FIELD_OBJECT_INDEX:
                superclass_index = superclass_field[1]
                if superclass_index < 0 or superclass_index >= object_count:
                    raise ImageInspectionError("seed manifest class superclass index is out of range")
                if objects[superclass_index]["object_kind"] != mvp.SEED_OBJECT_CLASS:
                    raise ImageInspectionError("seed manifest class superclass is not a class")
            elif superclass_field[0] != mvp.SEED_FIELD_NIL:
                raise ImageInspectionError("seed manifest class superclass field is invalid")
        if object_summary["object_kind"] == mvp.SEED_OBJECT_CLASS:
            class_instance_kind_field = object_summary["fields"][mvp.CLASS_FIELD_INSTANCE_KIND]
            if int(object_summary["field_count"]) <= mvp.CLASS_FIELD_METHOD_COUNT:
                raise ImageInspectionError("seed manifest class descriptor is missing method table fields")
            method_start_field = object_summary["fields"][mvp.CLASS_FIELD_METHOD_START]
            method_count_field = object_summary["fields"][mvp.CLASS_FIELD_METHOD_COUNT]
            if method_count_field[0] != mvp.SEED_FIELD_SMALL_INTEGER or method_count_field[1] < 0:
                raise ImageInspectionError("seed manifest class method count field is invalid")
            method_count = int(method_count_field[1])
            if method_count == 0:
                if method_start_field[0] != mvp.SEED_FIELD_NIL:
                    raise ImageInspectionError("seed manifest empty class method table has a start object")
                continue
            if method_start_field[0] != mvp.SEED_FIELD_OBJECT_INDEX:
                raise ImageInspectionError("seed manifest class method start is invalid")
            method_start = int(method_start_field[1])
            if method_start < 0 or method_start + method_count > object_count:
                raise ImageInspectionError("seed manifest class method range is out of range")
            for method_index in range(method_start, method_start + method_count):
                method_summary = objects[method_index]
                if method_summary["object_kind"] != mvp.SEED_OBJECT_METHOD_DESCRIPTOR:
                    raise ImageInspectionError("seed manifest class method range contains a non-method descriptor")
                if int(method_summary["field_count"]) <= mvp.METHOD_FIELD_ENTRY:
                    raise ImageInspectionError("seed manifest method descriptor is missing required fields")
                selector_field = method_summary["fields"][mvp.METHOD_FIELD_SELECTOR]
                argument_count_field = method_summary["fields"][mvp.METHOD_FIELD_ARGUMENT_COUNT]
                primitive_kind_field = method_summary["fields"][mvp.METHOD_FIELD_PRIMITIVE_KIND]
                entry_field = method_summary["fields"][mvp.METHOD_FIELD_ENTRY]
                if selector_field[0] != mvp.SEED_FIELD_OBJECT_INDEX:
                    raise ImageInspectionError("seed manifest method descriptor selector field is invalid")
                selector_index = int(selector_field[1])
                if selector_index < 0 or selector_index >= object_count:
                    raise ImageInspectionError("seed manifest method descriptor selector field is invalid")
                selector_summary = objects[selector_index]
                if selector_summary["object_kind"] != mvp.SEED_OBJECT_SELECTOR:
                    raise ImageInspectionError("seed manifest method descriptor selector field is invalid")
                if int(selector_summary["field_count"]) <= 0:
                    raise ImageInspectionError("seed manifest selector object is missing required fields")
                selector_id_field = selector_summary["fields"][0]
                if (
                    selector_id_field[0] != mvp.SEED_FIELD_SMALL_INTEGER
                    or selector_id_field[1] < mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_SHOW"]
                    or selector_id_field[1] > mvp.SELECTOR_VALUES["RECORZ_MVP_SELECTOR_INSTANCE_KIND"]
                ):
                    raise ImageInspectionError("seed manifest selector object id field is invalid")
                if argument_count_field[0] != mvp.SEED_FIELD_SMALL_INTEGER or argument_count_field[1] < 0:
                    raise ImageInspectionError("seed manifest method descriptor argument count field is invalid")
                if argument_count_field[1] > 10:
                    raise ImageInspectionError("seed manifest method descriptor argument count exceeds send capacity")
                if (
                    primitive_kind_field[0] != mvp.SEED_FIELD_SMALL_INTEGER
                    or primitive_kind_field[1] != class_instance_kind_field[1]
                ):
                    raise ImageInspectionError("seed manifest method descriptor primitive kind field is invalid")
                if entry_field[0] != mvp.SEED_FIELD_OBJECT_INDEX:
                    raise ImageInspectionError("seed manifest method descriptor entry field is invalid")
                entry_index = int(entry_field[1])
                if entry_index < 0 or entry_index >= object_count:
                    raise ImageInspectionError("seed manifest method descriptor entry field is invalid")
                entry_summary = objects[entry_index]
                if entry_summary["object_kind"] != mvp.SEED_OBJECT_METHOD_ENTRY:
                    raise ImageInspectionError("seed manifest method descriptor entry field is invalid")
                if int(entry_summary["field_count"]) <= mvp.METHOD_ENTRY_FIELD_IMPLEMENTATION:
                    raise ImageInspectionError("seed manifest method entry object is missing required fields")
                entry_id_field = entry_summary["fields"][mvp.METHOD_ENTRY_FIELD_EXECUTION_ID]
                implementation_field = entry_summary["fields"][mvp.METHOD_ENTRY_FIELD_IMPLEMENTATION]
                if (
                    entry_id_field[0] != mvp.SEED_FIELD_SMALL_INTEGER
                    or entry_id_field[1] <= 0
                    or entry_id_field[1] >= mvp.METHOD_ENTRY_COUNT
                ):
                    raise ImageInspectionError("seed manifest method entry execution id is invalid")
                entry_spec = mvp.METHOD_ENTRY_SPECS.get(entry_id_field[1])
                if entry_spec is None:
                    raise ImageInspectionError("seed manifest method entry execution id is invalid")
                if (
                    entry_spec[0] != class_instance_kind_field[1]
                    or entry_spec[1] != selector_id_field[1]
                    or entry_spec[2] != argument_count_field[1]
                ):
                    raise ImageInspectionError("seed manifest method descriptor entry metadata does not match selector")
                method_entry_ids.add(int(entry_id_field[1]))
                primitive_binding_expected = PRIMITIVE_BINDING_BY_ENTRY_ID.get(int(entry_id_field[1]))
                compiled_expected = COMPILED_METHOD_PROGRAM_BY_ENTRY_ID.get(int(entry_id_field[1]))
                if primitive_binding_expected is None and compiled_expected is None:
                    raise ImageInspectionError("seed manifest method entry does not match any known implementation")
                if primitive_binding_expected is not None:
                    if (
                        implementation_field[0] != mvp.SEED_FIELD_SMALL_INTEGER
                        or implementation_field[1] != primitive_binding_expected
                    ):
                        raise ImageInspectionError("primitive method entry binding id is invalid")
                    continue
                if compiled_expected is not None:
                    if implementation_field[0] != mvp.SEED_FIELD_OBJECT_INDEX:
                        raise ImageInspectionError("compiled method entry is missing an implementation object")
                    implementation_index = int(implementation_field[1])
                    if implementation_index < 0 or implementation_index >= object_count:
                        raise ImageInspectionError("compiled method entry implementation is out of range")
                    implementation_summary = objects[implementation_index]
                    if implementation_summary["object_kind"] != mvp.SEED_OBJECT_COMPILED_METHOD:
                        raise ImageInspectionError("compiled method entry implementation is invalid")
                    if int(implementation_summary["field_count"]) != len(compiled_expected):
                        raise ImageInspectionError("compiled method field count is invalid")
                    for field_index, expected_instruction in enumerate(compiled_expected):
                        field = implementation_summary["fields"][field_index]
                        if field[0] != mvp.SEED_FIELD_SMALL_INTEGER or field[1] != expected_instruction:
                            raise ImageInspectionError("compiled method instruction does not match entry")
                    continue
                raise AssertionError("unreachable method entry validation path")
            declared_method_count += method_count

    globals_summary: dict[str, int] = {}
    for _ in range(global_binding_count):
        binding_id, object_index = struct.unpack_from(mvp.SEED_BINDING_FORMAT, blob, offset)
        offset += binding_size
        if binding_id == 0:
            raise ImageInspectionError("seed manifest global binding id is invalid")
        if object_index >= object_count:
            raise ImageInspectionError("seed manifest global binding object index is out of range")
        if binding_id == mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_TRANSCRIPT"]:
            globals_summary["Transcript"] = object_index
        elif binding_id == mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_DISPLAY"]:
            globals_summary["Display"] = object_index
        elif binding_id == mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_BITBLT"]:
            globals_summary["BitBlt"] = object_index
        elif binding_id == mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_GLYPHS"]:
            globals_summary["Glyphs"] = object_index
        elif binding_id == mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_FORM"]:
            globals_summary["Form"] = object_index
        elif binding_id == mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_BITMAP"]:
            globals_summary["Bitmap"] = object_index
        else:
            raise ImageInspectionError("seed manifest global binding id is unknown")

    roots_summary: dict[str, int] = {}
    for _ in range(root_binding_count):
        binding_id, object_index = struct.unpack_from(mvp.SEED_BINDING_FORMAT, blob, offset)
        offset += binding_size
        if object_index >= object_count:
            raise ImageInspectionError("seed manifest root binding object index is out of range")
        root_name = ROOT_NAMES.get(binding_id)
        if root_name is None:
            raise ImageInspectionError("seed manifest root binding id is unknown")
        roots_summary[root_name] = object_index

    return {
        "version": version,
        "object_count": object_count,
        "class_descriptor_count": class_descriptor_count,
        "class_link_count": class_link_count,
        "method_descriptor_count": method_descriptor_count,
        "method_entry_object_count": method_entry_object_count,
        "selector_object_count": selector_object_count,
        "accessor_method_object_count": accessor_method_object_count,
        "field_send_method_object_count": field_send_method_object_count,
        "root_send_method_object_count": root_send_method_object_count,
        "root_value_method_object_count": root_value_method_object_count,
        "interpreted_method_object_count": interpreted_method_object_count,
        "compiled_method_object_count": compiled_method_object_count,
        "declared_method_count": declared_method_count,
        "method_entry_count": len(method_entry_ids),
        "global_binding_count": global_binding_count,
        "root_binding_count": root_binding_count,
        "globals": globals_summary,
        "roots": roots_summary,
        "glyph_code_count": glyph_code_count,
    }


def inspect_entry_manifest(blob: bytes) -> dict[str, object]:
    if len(blob) < struct.calcsize(mvp.IMAGE_ENTRY_FORMAT):
        raise ImageInspectionError("entry manifest is truncated")
    magic, version, kind, flags, program_section_kind, argument_count, reserved = struct.unpack_from(
        mvp.IMAGE_ENTRY_FORMAT,
        blob,
        0,
    )
    if magic != mvp.IMAGE_ENTRY_MAGIC:
        raise ImageInspectionError("entry manifest magic mismatch")
    if kind != mvp.IMAGE_ENTRY_KIND_DOIT:
        raise ImageInspectionError("entry manifest kind is unsupported")
    if flags != 0:
        raise ImageInspectionError("entry manifest flags are unsupported")
    if program_section_kind != mvp.IMAGE_SECTION_PROGRAM:
        raise ImageInspectionError("entry manifest program section reference is invalid")
    if argument_count != 0:
        raise ImageInspectionError("entry manifest argument count is unsupported")
    if reserved != 0:
        raise ImageInspectionError("entry manifest reserved field is nonzero")
    return {
        "version": version,
        "kind": "doit",
        "flags": flags,
        "program_section": _section_name(program_section_kind),
        "argument_count": argument_count,
    }


def inspect_image_bytes(blob: bytes) -> dict[str, object]:
    header_size = struct.calcsize(mvp.IMAGE_HEADER_FORMAT)
    section_size = struct.calcsize(mvp.IMAGE_SECTION_FORMAT)
    if len(blob) < header_size:
        raise ImageInspectionError("boot image is truncated")

    magic, version, section_count, feature_flags, checksum, profile = struct.unpack_from(mvp.IMAGE_HEADER_FORMAT, blob, 0)
    if magic != mvp.IMAGE_MAGIC:
        raise ImageInspectionError("boot image magic mismatch")
    if profile != mvp.IMAGE_PROFILE:
        raise ImageInspectionError("boot image profile mismatch")
    if len(blob) < header_size + (section_count * section_size):
        raise ImageInspectionError("boot image section table is truncated")

    computed_checksum = mvp.fnv1a32(blob[header_size:])
    if computed_checksum != checksum:
        raise ImageInspectionError("boot image checksum mismatch")

    sections: list[dict[str, object]] = []
    entry_summary: dict[str, object] | None = None
    program_summary: dict[str, object] | None = None
    seed_summary: dict[str, object] | None = None
    offset = header_size
    for _ in range(section_count):
        kind, _reserved, section_offset, section_length = struct.unpack_from(mvp.IMAGE_SECTION_FORMAT, blob, offset)
        offset += section_size
        if section_offset + section_length > len(blob):
            raise ImageInspectionError("boot image section is out of bounds")
        payload = blob[section_offset : section_offset + section_length]
        section_info = {
            "kind": kind,
            "name": _section_name(kind),
            "offset": section_offset,
            "size": section_length,
        }
        if kind == mvp.IMAGE_SECTION_ENTRY:
            entry_summary = inspect_entry_manifest(payload)
        elif kind == mvp.IMAGE_SECTION_PROGRAM:
            program_summary = inspect_program_manifest(payload)
        elif kind == mvp.IMAGE_SECTION_SEED:
            seed_summary = inspect_seed_manifest(payload)
        sections.append(section_info)

    if entry_summary is None or program_summary is None or seed_summary is None:
        raise ImageInspectionError("boot image is missing required sections")

    return {
        "version": version,
        "section_count": section_count,
        "feature_flags": feature_flags,
        "feature_names": _feature_names(feature_flags),
        "checksum": checksum,
        "computed_checksum": computed_checksum,
        "profile": profile.decode("ascii"),
        "sections": sections,
        "entry": entry_summary,
        "program": program_summary,
        "seed": seed_summary,
    }


def render_summary(summary: dict[str, object]) -> str:
    lines = [
        "RCZI boot image",
        f"version: {summary['version']}",
        f"profile: {summary['profile']}",
        f"features: {', '.join(summary['feature_names']) or '(none)'}",
        f"checksum: 0x{int(summary['checksum']):08x}",
        f"sections: {summary['section_count']}",
    ]
    for section in summary["sections"]:
        lines.append(f"  - {section['name']} offset={section['offset']} size={section['size']}")
    entry = summary["entry"]
    lines.append(
        "entry: "
        f"kind={entry['kind']} args={entry['argument_count']} target={entry['program_section']}"
    )
    program = summary["program"]
    lines.append(
        "program: "
        f"instructions={program['instruction_count']} literals={program['literal_count']} lexicals={program['lexical_count']}"
    )
    seed = summary["seed"]
    lines.append(
        "seed: "
        f"objects={seed['object_count']} classes={seed['class_descriptor_count']} class_links={seed['class_link_count']} "
        f"methods={seed['declared_method_count']} method_descriptors={seed['method_descriptor_count']} "
        f"selector_objects={seed['selector_object_count']} "
        f"accessor_method_objects={seed['accessor_method_object_count']} "
        f"field_send_method_objects={seed['field_send_method_object_count']} "
        f"root_send_method_objects={seed['root_send_method_object_count']} "
        f"root_value_method_objects={seed['root_value_method_object_count']} "
        f"interpreted_method_objects={seed['interpreted_method_object_count']} "
        f"compiled_method_objects={seed['compiled_method_object_count']} "
        f"method_entry_objects={seed['method_entry_object_count']} "
        f"method_entries={seed['method_entry_count']} "
        f"globals={seed['global_binding_count']} roots={seed['root_binding_count']} "
        f"glyph_codes={seed['glyph_code_count']} default_form={seed['roots']['default_form']}"
    )
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path, help="Path to an RCZI boot image")
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON")
    args = parser.parse_args(argv)

    summary = inspect_image_bytes(args.image.read_bytes())
    if args.json:
        print(json.dumps(summary, indent=2, sort_keys=True))
    else:
        print(render_summary(summary))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
