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
    mvp.IMAGE_SECTION_PROGRAM: "program",
    mvp.IMAGE_SECTION_SEED: "seed",
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
    if len(blob) < struct.calcsize(mvp.SEED_HEADER_FORMAT):
        raise ImageInspectionError("seed manifest is truncated")
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
        _reserved,
    ) = struct.unpack_from(mvp.SEED_HEADER_FORMAT, blob, 0)
    if magic != mvp.SEED_MAGIC:
        raise ImageInspectionError("seed manifest magic mismatch")
    return {
        "version": version,
        "object_count": object_count,
        "globals": {
            "nil": nil_global,
            "Transcript": transcript_global,
            "Display": display_global,
            "BitBlt": bitblt_global,
            "Glyphs": glyphs_global,
            "Form": form_global,
            "Bitmap": bitmap_global,
        },
        "default_form_index": default_form_index,
        "framebuffer_bitmap_index": framebuffer_bitmap_index,
        "glyph_bitmap_start_index": glyph_bitmap_start_index,
        "glyph_code_count": glyph_code_count,
        "glyph_fallback_offset": glyph_fallback_offset,
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
        if kind == mvp.IMAGE_SECTION_PROGRAM:
            program_summary = inspect_program_manifest(payload)
        elif kind == mvp.IMAGE_SECTION_SEED:
            seed_summary = inspect_seed_manifest(payload)
        sections.append(section_info)

    if program_summary is None or seed_summary is None:
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
    program = summary["program"]
    lines.append(
        "program: "
        f"instructions={program['instruction_count']} literals={program['literal_count']} lexicals={program['lexical_count']}"
    )
    seed = summary["seed"]
    lines.append(
        "seed: "
        f"objects={seed['object_count']} glyph_codes={seed['glyph_code_count']} default_form={seed['default_form_index']}"
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
