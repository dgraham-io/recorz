from __future__ import annotations

import struct
import unittest

from tools.inspect_qemu_riscv_snapshot import (
    MAX_GLOBAL_ID,
    MAX_ROOT_ID,
    SNAPSHOT_HEADER_SIZE,
    SNAPSHOT_MAGIC,
    SNAPSHOT_OBJECT_SIZE,
    SNAPSHOT_VERSION,
    WORKSPACE_GLOBAL_ID,
    extract_workspace_current_source,
    inspect_snapshot,
)


def _encode_nil() -> bytes:
    return bytes(8)


def _encode_small_integer(value: int) -> bytes:
    return bytes([3, 0, 0, 0]) + struct.pack("<i", value)


def _encode_string(offset: int, text: str) -> bytes:
    encoded = text.encode("utf-8")
    return bytes([2, 0]) + struct.pack("<H", len(encoded)) + struct.pack("<I", offset)


def _build_workspace_snapshot(*, view_kind: int, target_name: str, current_source: str, last_source: str) -> bytes:
    string_section = bytearray()
    offsets: dict[str, int] = {}
    for label, text in (
        ("target_name", target_name),
        ("current_source", current_source),
        ("last_source", last_source),
    ):
        offsets[label] = len(string_section)
        string_section.extend(text.encode("utf-8"))
        string_section.append(0)

    object_record = bytearray(SNAPSHOT_OBJECT_SIZE)
    object_record[0] = 42
    object_record[1] = 4
    struct.pack_into("<H", object_record, 2, 1)
    object_record[4:12] = _encode_small_integer(view_kind)
    object_record[12:20] = _encode_string(offsets["target_name"], target_name)
    object_record[20:28] = _encode_string(offsets["current_source"], current_source)
    object_record[28:36] = _encode_string(offsets["last_source"], last_source)

    global_section = bytearray(MAX_GLOBAL_ID * 2)
    struct.pack_into("<H", global_section, (WORKSPACE_GLOBAL_ID - 1) * 2, 1)

    root_section = bytearray(MAX_ROOT_ID * 2)
    glyph_section = bytearray(128 * 2)

    total_size = (
        SNAPSHOT_HEADER_SIZE
        + len(object_record)
        + len(global_section)
        + len(root_section)
        + len(glyph_section)
        + len(string_section)
    )
    header = bytearray(SNAPSHOT_HEADER_SIZE)
    header[0:4] = SNAPSHOT_MAGIC
    struct.pack_into("<H", header, 4, SNAPSHOT_VERSION)
    struct.pack_into("<H", header, 6, 1)
    struct.pack_into("<I", header, 18, len(string_section))
    struct.pack_into("<I", header, 38, total_size)

    return bytes(header + object_record + global_section + root_section + glyph_section + string_section)


class QemuRiscvSnapshotInspectorTests(unittest.TestCase):
    def test_inspect_snapshot_reports_workspace_state(self) -> None:
        snapshot = _build_workspace_snapshot(
            view_kind=13,
            target_name="Tools",
            current_source="RecorzKernelPackage: 'Tools' comment: 'Utilities'",
            last_source="Workspace browsePackageNamed: 'Tools'.",
        )

        inspection = inspect_snapshot(snapshot)

        self.assertEqual(inspection["header"]["version"], SNAPSHOT_VERSION)
        self.assertEqual(inspection["header"]["object_count"], 1)
        self.assertEqual(inspection["workspace"]["handle"], 1)
        self.assertEqual(inspection["workspace"]["current_view_kind"], 13)
        self.assertEqual(inspection["workspace"]["current_target_name"], "Tools")
        self.assertIn("RecorzKernelPackage", inspection["workspace"]["current_source"])
        self.assertIn("browsePackageNamed:", inspection["workspace"]["last_source"])

    def test_extract_workspace_current_source_returns_image_visible_source(self) -> None:
        source_text = "RecorzKernelClass: #Exported superclass: #Object"
        snapshot = _build_workspace_snapshot(
            view_kind=8,
            target_name="Exported",
            current_source=source_text,
            last_source="Workspace fileOutClassNamed: 'Exported'.",
        )

        extracted = extract_workspace_current_source(snapshot)

        self.assertEqual(extracted, source_text)


if __name__ == "__main__":
    unittest.main()
