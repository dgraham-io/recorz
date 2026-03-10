#!/usr/bin/env python3
"""Inspect an RV32 live snapshot and extract image-visible workspace source."""

from __future__ import annotations

import argparse
import json
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools"))

import build_qemu_riscv_mvp_image as mvp  # noqa: E402


SNAPSHOT_MAGIC = b"RCZT"
SNAPSHOT_VERSION = 5
SNAPSHOT_HEADER_SIZE = 42
SNAPSHOT_VALUE_SIZE = 8
OBJECT_FIELD_LIMIT = 4
SNAPSHOT_OBJECT_SIZE = 4 + (OBJECT_FIELD_LIMIT * SNAPSHOT_VALUE_SIZE)
SNAPSHOT_DYNAMIC_CLASS_RECORD_SIZE = 8 + (2 * 64) + 128 + (OBJECT_FIELD_LIMIT * 64)
SNAPSHOT_PACKAGE_RECORD_SIZE = 64 + 128
SNAPSHOT_NAMED_OBJECT_RECORD_SIZE = 2 + 64
SNAPSHOT_LIVE_METHOD_SOURCE_RECORD_SIZE = 8 + 64
SNAPSHOT_LIVE_STRING_LITERAL_RECORD_SIZE = 8
MONO_BITMAP_MAX_HEIGHT = 64
GLYPH_BITMAP_COUNT = 128

VALUE_KIND_NAMES = {
    0: "nil",
    1: "object",
    2: "string",
    3: "small_integer",
}

WORKSPACE_FIELD_CURRENT_VIEW_KIND = 0
WORKSPACE_FIELD_CURRENT_TARGET_NAME = 1
WORKSPACE_FIELD_CURRENT_SOURCE = 2
WORKSPACE_FIELD_LAST_SOURCE = 3

MAX_GLOBAL_ID = max(mvp.GLOBAL_VALUES.values())
MAX_ROOT_ID = max(mvp.SEED_ROOT_VALUES.values())
WORKSPACE_GLOBAL_ID = mvp.GLOBAL_VALUES["RECORZ_MVP_GLOBAL_WORKSPACE"]


class SnapshotInspectionError(RuntimeError):
    """Raised when the snapshot is structurally invalid."""


@dataclass(frozen=True)
class SnapshotValue:
    kind: str
    value: int | str | None


@dataclass(frozen=True)
class SnapshotObject:
    kind: int
    field_count: int
    class_handle: int
    fields: tuple[SnapshotValue, ...]


@dataclass(frozen=True)
class SnapshotHeader:
    version: int
    object_count: int
    dynamic_class_count: int
    package_count: int
    named_object_count: int
    mono_bitmap_count: int
    next_dynamic_method_entry_execution_id: int
    string_byte_count: int
    cursor_x: int
    cursor_y: int
    startup_hook_receiver_handle: int
    startup_hook_selector_id: int
    live_method_source_count: int
    live_method_source_byte_count: int
    live_string_literal_count: int
    live_string_literal_byte_count: int
    total_size: int


@dataclass(frozen=True)
class ParsedSnapshot:
    header: SnapshotHeader
    objects: tuple[SnapshotObject, ...]
    global_handles: tuple[int, ...]


def _read_u16_le(blob: bytes, offset: int) -> int:
    return struct.unpack_from("<H", blob, offset)[0]


def _read_u32_le(blob: bytes, offset: int) -> int:
    return struct.unpack_from("<I", blob, offset)[0]


def _decode_snapshot_value(slot: bytes, string_section: bytes, object_count: int) -> SnapshotValue:
    kind = slot[0]
    aux = _read_u16_le(slot, 2)
    raw_u32 = _read_u32_le(slot, 4)

    if kind == 0:
        return SnapshotValue("nil", None)
    if kind == 1:
        if raw_u32 == 0 or raw_u32 > object_count:
            raise SnapshotInspectionError("snapshot object reference is out of range")
        return SnapshotValue("object", raw_u32)
    if kind == 2:
        if raw_u32 + aux >= len(string_section):
            raise SnapshotInspectionError("snapshot string value is out of range")
        if string_section[raw_u32 + aux] != 0:
            raise SnapshotInspectionError("snapshot string value is not terminated")
        return SnapshotValue("string", string_section[raw_u32 : raw_u32 + aux].decode("utf-8"))
    if kind == 3:
        return SnapshotValue("small_integer", struct.unpack_from("<i", slot, 4)[0])
    raise SnapshotInspectionError(f"unknown snapshot value kind {kind}")


def parse_snapshot(blob: bytes) -> ParsedSnapshot:
    if len(blob) < SNAPSHOT_HEADER_SIZE:
        raise SnapshotInspectionError("snapshot is too small")
    if blob[:4] != SNAPSHOT_MAGIC:
        raise SnapshotInspectionError("snapshot magic mismatch")

    header = SnapshotHeader(
        version=_read_u16_le(blob, 4),
        object_count=_read_u16_le(blob, 6),
        dynamic_class_count=_read_u16_le(blob, 8),
        package_count=_read_u16_le(blob, 10),
        named_object_count=_read_u16_le(blob, 12),
        mono_bitmap_count=_read_u16_le(blob, 14),
        next_dynamic_method_entry_execution_id=_read_u16_le(blob, 16),
        string_byte_count=_read_u32_le(blob, 18),
        cursor_x=_read_u16_le(blob, 22),
        cursor_y=_read_u16_le(blob, 24),
        startup_hook_receiver_handle=_read_u16_le(blob, 26),
        startup_hook_selector_id=_read_u16_le(blob, 28),
        live_method_source_count=_read_u16_le(blob, 30),
        live_method_source_byte_count=_read_u16_le(blob, 32),
        live_string_literal_count=_read_u16_le(blob, 34),
        live_string_literal_byte_count=_read_u16_le(blob, 36),
        total_size=_read_u32_le(blob, 38),
    )
    if header.version != SNAPSHOT_VERSION:
        raise SnapshotInspectionError("snapshot version mismatch")
    if header.object_count == 0:
        raise SnapshotInspectionError("snapshot object count is zero")
    if header.total_size != len(blob):
        raise SnapshotInspectionError("snapshot size mismatch")

    string_section_offset = (
        SNAPSHOT_HEADER_SIZE
        + (header.object_count * SNAPSHOT_OBJECT_SIZE)
        + (MAX_GLOBAL_ID * 2)
        + (MAX_ROOT_ID * 2)
        + (GLYPH_BITMAP_COUNT * 2)
        + (header.dynamic_class_count * SNAPSHOT_DYNAMIC_CLASS_RECORD_SIZE)
        + (header.package_count * SNAPSHOT_PACKAGE_RECORD_SIZE)
        + (header.named_object_count * SNAPSHOT_NAMED_OBJECT_RECORD_SIZE)
        + (header.live_method_source_count * SNAPSHOT_LIVE_METHOD_SOURCE_RECORD_SIZE)
        + header.live_method_source_byte_count
        + (header.live_string_literal_count * SNAPSHOT_LIVE_STRING_LITERAL_RECORD_SIZE)
        + header.live_string_literal_byte_count
        + (header.mono_bitmap_count * MONO_BITMAP_MAX_HEIGHT * 4)
    )
    if string_section_offset + header.string_byte_count != len(blob):
        raise SnapshotInspectionError("snapshot string section size mismatch")
    string_section = blob[string_section_offset:]

    objects: list[SnapshotObject] = []
    offset = SNAPSHOT_HEADER_SIZE
    for _handle in range(1, header.object_count + 1):
        kind = blob[offset]
        field_count = blob[offset + 1]
        class_handle = _read_u16_le(blob, offset + 2)
        fields: list[SnapshotValue] = []
        field_offset = offset + 4
        for _field_index in range(OBJECT_FIELD_LIMIT):
            fields.append(
                _decode_snapshot_value(
                    blob[field_offset : field_offset + SNAPSHOT_VALUE_SIZE],
                    string_section,
                    header.object_count,
                )
            )
            field_offset += SNAPSHOT_VALUE_SIZE
        objects.append(SnapshotObject(kind, field_count, class_handle, tuple(fields)))
        offset += SNAPSHOT_OBJECT_SIZE

    global_handles = tuple(
        _read_u16_le(blob, offset + (global_index * 2))
        for global_index in range(MAX_GLOBAL_ID)
    )

    return ParsedSnapshot(header=header, objects=tuple(objects), global_handles=global_handles)


def _hex_value(ch: str) -> int:
    if "0" <= ch <= "9":
        return ord(ch) - ord("0")
    if "a" <= ch <= "f":
        return 10 + (ord(ch) - ord("a"))
    if "A" <= ch <= "F":
        return 10 + (ord(ch) - ord("A"))
    raise SnapshotInspectionError(f"invalid input-monitor state escape {ch!r}")


def _decode_input_monitor_state_text(cursor: str, start: int) -> tuple[str, int]:
    chars: list[str] = []
    index = start

    while index < len(cursor) and cursor[index] != ";":
        ch = cursor[index]
        if ch == "%":
            if index + 2 >= len(cursor):
                raise SnapshotInspectionError("truncated input-monitor state escape")
            value = (_hex_value(cursor[index + 1]) << 4) | _hex_value(cursor[index + 2])
            chars.append(chr(value))
            index += 3
            continue
        chars.append(ch)
        index += 1
    return ("".join(chars), index)


def _parse_workspace_input_monitor_state(text: str) -> dict[str, object]:
    if not text.startswith("CURSOR:"):
        raise SnapshotInspectionError("input-monitor state is missing the CURSOR header")
    index = len("CURSOR:")
    while index < len(text) and text[index].isdigit():
        index += 1
    cursor_index = int(text[len("CURSOR:") : index])
    if not text[index:].startswith(";TOP:"):
        raise SnapshotInspectionError("input-monitor state is missing the TOP header")
    index += len(";TOP:")
    top_start = index
    while index < len(text) and text[index].isdigit():
        index += 1
    top_line = int(text[top_start:index])
    if not text[index:].startswith(";VIEW:"):
        raise SnapshotInspectionError("input-monitor state is missing the VIEW header")
    index += len(";VIEW:")
    view_start = index
    while index < len(text) and text[index].isdigit():
        index += 1
    saved_view_kind = int(text[view_start:index])
    saved_target_name = ""
    status = ""
    feedback = ""
    if text[index:].startswith(";TARGET:"):
        index += len(";TARGET:")
        saved_target_name, index = _decode_input_monitor_state_text(text, index)
    if text[index:].startswith(";STATUS:"):
        index += len(";STATUS:")
        status, index = _decode_input_monitor_state_text(text, index)
    if text[index:].startswith(";FEEDBACK:"):
        index += len(";FEEDBACK:")
        feedback, index = _decode_input_monitor_state_text(text, index)
    if index != len(text):
        raise SnapshotInspectionError("input-monitor state contains unexpected trailing text")
    return {
        "cursor_index": cursor_index,
        "top_line": top_line,
        "saved_view_kind": saved_view_kind,
        "saved_target_name": saved_target_name,
        "status": status,
        "feedback": feedback,
    }


def _workspace_summary(snapshot: ParsedSnapshot) -> dict[str, object] | None:
    workspace_handle = snapshot.global_handles[WORKSPACE_GLOBAL_ID - 1]
    if workspace_handle == 0:
        return None
    if workspace_handle > len(snapshot.objects):
        raise SnapshotInspectionError("workspace handle is out of range")

    workspace_object = snapshot.objects[workspace_handle - 1]
    fields = workspace_object.fields
    input_monitor_state = None
    current_view_kind = fields[WORKSPACE_FIELD_CURRENT_VIEW_KIND].value
    current_target_name = fields[WORKSPACE_FIELD_CURRENT_TARGET_NAME].value

    if isinstance(current_view_kind, int) and current_view_kind == 18 and isinstance(current_target_name, str):
        input_monitor_state = _parse_workspace_input_monitor_state(current_target_name)
    return {
        "handle": workspace_handle,
        "kind": workspace_object.kind,
        "field_count": workspace_object.field_count,
        "class_handle": workspace_object.class_handle,
        "current_view_kind": current_view_kind,
        "current_target_name": current_target_name,
        "current_source": fields[WORKSPACE_FIELD_CURRENT_SOURCE].value,
        "last_source": fields[WORKSPACE_FIELD_LAST_SOURCE].value,
        "input_monitor_state": input_monitor_state,
    }


def inspect_snapshot(blob: bytes) -> dict[str, object]:
    snapshot = parse_snapshot(blob)
    return {
        "header": {
            "version": snapshot.header.version,
            "object_count": snapshot.header.object_count,
            "dynamic_class_count": snapshot.header.dynamic_class_count,
            "package_count": snapshot.header.package_count,
            "named_object_count": snapshot.header.named_object_count,
            "mono_bitmap_count": snapshot.header.mono_bitmap_count,
            "next_dynamic_method_entry_execution_id": snapshot.header.next_dynamic_method_entry_execution_id,
            "string_byte_count": snapshot.header.string_byte_count,
            "cursor_x": snapshot.header.cursor_x,
            "cursor_y": snapshot.header.cursor_y,
            "startup_hook_receiver_handle": snapshot.header.startup_hook_receiver_handle,
            "startup_hook_selector_id": snapshot.header.startup_hook_selector_id,
            "live_method_source_count": snapshot.header.live_method_source_count,
            "live_method_source_byte_count": snapshot.header.live_method_source_byte_count,
            "live_string_literal_count": snapshot.header.live_string_literal_count,
            "live_string_literal_byte_count": snapshot.header.live_string_literal_byte_count,
            "total_size": snapshot.header.total_size,
        },
        "workspace": _workspace_summary(snapshot),
    }


def extract_workspace_current_source(blob: bytes) -> str | None:
    snapshot = parse_snapshot(blob)
    workspace = _workspace_summary(snapshot)
    if workspace is None:
        return None
    current_source = workspace["current_source"]
    if current_source is None:
        return None
    if not isinstance(current_source, str):
        raise SnapshotInspectionError("workspace current source is not a string")
    return current_source


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot_path", type=Path, help="Path to a saved Recorz snapshot blob")
    parser.add_argument(
        "--extract-workspace-current-source",
        type=Path,
        help="Write the current Workspace source buffer to this path",
    )
    args = parser.parse_args()

    blob = args.snapshot_path.read_bytes()
    inspection = inspect_snapshot(blob)

    if args.extract_workspace_current_source is not None:
        current_source = extract_workspace_current_source(blob)
        if current_source is None:
            raise SystemExit("snapshot does not contain a Workspace current source")
        args.extract_workspace_current_source.write_text(current_source, encoding="utf-8")

    print(json.dumps(inspection, indent=2))


if __name__ == "__main__":
    main()
