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
        objects.append(
            {
                "object_kind": object_kind,
                "field_count": field_count,
                "class_index": class_index,
                "fields": fields,
            }
        )
        offset += object_size

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
