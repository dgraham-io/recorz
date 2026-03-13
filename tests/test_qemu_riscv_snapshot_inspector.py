from __future__ import annotations

import struct
import unittest

from tools.inspect_qemu_riscv_snapshot import (
    MAX_GLOBAL_ID,
    MAX_ROOT_ID,
    METHOD_SOURCE_NAME_LIMIT,
    SNAPSHOT_HEADER_SIZE,
    SNAPSHOT_MAGIC,
    SNAPSHOT_NAMED_OBJECT_RECORD_SIZE,
    SNAPSHOT_OBJECT_SIZE,
    SNAPSHOT_VERSION,
    WORKSPACE_CURSOR_GLOBAL_ID,
    WORKSPACE_GLOBAL_ID,
    WORKSPACE_SELECTION_GLOBAL_ID,
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


def _encode_object_reference(handle: int) -> bytes:
    return bytes([1, 0, 0, 0]) + struct.pack("<I", handle)


def _encode_field(field: object, string_section: bytearray) -> bytes:
    if field is None:
        return _encode_nil()
    if isinstance(field, int):
        return _encode_small_integer(field)
    if isinstance(field, str):
        offset = len(string_section)
        string_section.extend(field.encode("utf-8"))
        string_section.append(0)
        return _encode_string(offset, field)
    if isinstance(field, tuple) and len(field) == 2 and field[0] == "object":
        return _encode_object_reference(int(field[1]))
    raise AssertionError(f"unsupported snapshot field {field!r}")


def _build_snapshot(
    *,
    objects: list[tuple[int, int, tuple[object, ...]]],
    global_handles: dict[int, int] | None = None,
    named_objects: list[tuple[int, str]] | None = None,
) -> bytes:
    string_section = bytearray()
    object_section = bytearray()
    global_handles = {} if global_handles is None else global_handles
    named_objects = [] if named_objects is None else named_objects

    for kind, class_handle, fields in objects:
        if len(fields) > 4:
            raise AssertionError("snapshot test helper only supports up to 4 object fields")
        object_record = bytearray(SNAPSHOT_OBJECT_SIZE)
        object_record[0] = kind
        object_record[1] = len(fields)
        struct.pack_into("<H", object_record, 2, class_handle)
        for field_index, field in enumerate(fields):
            start = 4 + (field_index * 8)
            object_record[start : start + 8] = _encode_field(field, string_section)
        object_section.extend(object_record)

    global_section = bytearray(MAX_GLOBAL_ID * 2)
    for global_id, handle in global_handles.items():
        struct.pack_into("<H", global_section, (global_id - 1) * 2, handle)

    root_section = bytearray(MAX_ROOT_ID * 2)
    glyph_section = bytearray(128 * 2)
    named_object_section = bytearray()
    for handle, name in named_objects:
        encoded_name = name.encode("utf-8")
        if len(encoded_name) >= METHOD_SOURCE_NAME_LIMIT:
            raise AssertionError("named object name exceeds snapshot capacity")
        record = bytearray(SNAPSHOT_NAMED_OBJECT_RECORD_SIZE)
        struct.pack_into("<H", record, 0, handle)
        record[2 : 2 + len(encoded_name)] = encoded_name
        named_object_section.extend(record)

    total_size = (
        SNAPSHOT_HEADER_SIZE
        + len(object_section)
        + len(global_section)
        + len(root_section)
        + len(glyph_section)
        + len(named_object_section)
        + len(string_section)
    )
    header = bytearray(SNAPSHOT_HEADER_SIZE)
    header[0:4] = SNAPSHOT_MAGIC
    struct.pack_into("<H", header, 4, SNAPSHOT_VERSION)
    struct.pack_into("<H", header, 6, len(objects))
    struct.pack_into("<H", header, 12, len(named_objects))
    struct.pack_into("<I", header, 18, len(string_section))
    struct.pack_into("<I", header, 48, total_size)

    return bytes(
        header
        + object_section
        + global_section
        + root_section
        + glyph_section
        + named_object_section
        + string_section
    )


def _build_workspace_snapshot(*, view_kind: int, target_name: str, current_source: str, last_source: str) -> bytes:
    return _build_snapshot(
        objects=[
            (
                42,
                1,
                (view_kind, target_name, current_source, last_source),
            )
        ],
        global_handles={WORKSPACE_GLOBAL_ID: 1},
    )


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
        self.assertEqual(inspection["header"]["active_display_form_handle"], 0)
        self.assertEqual(inspection["header"]["active_cursor_handle"], 0)
        self.assertEqual(inspection["header"]["active_cursor_visible"], 0)
        self.assertEqual(inspection["workspace"]["handle"], 1)
        self.assertEqual(inspection["workspace"]["current_view_kind"], 13)
        self.assertEqual(inspection["workspace"]["current_target_name"], "Tools")
        self.assertIn("RecorzKernelPackage", inspection["workspace"]["current_source"])
        self.assertIn("browsePackageNamed:", inspection["workspace"]["last_source"])
        self.assertIsNone(inspection["workspace"]["input_monitor_state"])

    def test_inspect_snapshot_decodes_input_monitor_state(self) -> None:
        snapshot = _build_workspace_snapshot(
            view_kind=18,
            target_name=(
                "CURSOR:5;TOP:1;VIEW:5;TARGET:Display>>newline;"
                "STATUS:PRINT COMPLETE;FEEDBACK:OUT> 3%0AOUT> READY"
            ),
            current_source="1 + 2",
            last_source="1 + 2",
        )

        inspection = inspect_snapshot(snapshot)

        self.assertEqual(inspection["workspace"]["current_view_kind"], 18)
        input_monitor_state = inspection["workspace"]["input_monitor_state"]
        assert isinstance(input_monitor_state, dict)
        self.assertEqual(input_monitor_state["cursor_index"], 5)
        self.assertEqual(input_monitor_state["top_line"], 1)
        self.assertEqual(input_monitor_state["saved_view_kind"], 5)
        self.assertEqual(input_monitor_state["saved_target_name"], "Display>>newline")
        self.assertEqual(input_monitor_state["status"], "PRINT COMPLETE")
        self.assertEqual(input_monitor_state["feedback"], "OUT> 3\nOUT> READY")

    def test_inspect_snapshot_reports_workspace_tool_session_cursor_and_selection_state(self) -> None:
        snapshot = _build_snapshot(
            objects=[
                (
                    42,
                    1,
                    (
                        18,
                        "CURSOR:123;TOP:7;VIEW:0;STATUS:WORKSPACE READY",
                        "first\nsecond\nthird",
                        "first\nsecond\nthird",
                    ),
                ),
                (35, 1, ("WORKSPACE READY", "OUT> READY", "Workspace", 17)),
                (36, 1, (("object", 1), 0, 0, 0)),
                (30, 1, (123, 23, 88, 7)),
                (31, 1, (23, 88, 23, 88)),
            ],
            global_handles={
                WORKSPACE_GLOBAL_ID: 1,
                WORKSPACE_CURSOR_GLOBAL_ID: 4,
                WORKSPACE_SELECTION_GLOBAL_ID: 5,
            },
            named_objects=[
                (2, "BootWorkspaceTool"),
                (3, "BootWorkspaceSession"),
            ],
        )

        inspection = inspect_snapshot(snapshot)

        workspace_tool = inspection["workspace_tool"]
        assert isinstance(workspace_tool, dict)
        self.assertEqual(workspace_tool["workspace_name"], "Workspace")
        self.assertEqual(workspace_tool["status_text"], "WORKSPACE READY")
        self.assertEqual(workspace_tool["feedback_text"], "OUT> READY")
        self.assertEqual(workspace_tool["visible_left_column"], 17)

        workspace_session = inspection["workspace_session"]
        assert isinstance(workspace_session, dict)
        self.assertEqual(workspace_session["workspace_handle"], 1)
        self.assertTrue(workspace_session["workspace_matches_global"])
        self.assertEqual(workspace_session["selected"], 0)
        self.assertEqual(workspace_session["list_top"], 0)
        self.assertEqual(workspace_session["escape"], 0)

        workspace_cursor = inspection["workspace_cursor"]
        assert isinstance(workspace_cursor, dict)
        self.assertEqual(workspace_cursor["index"], 123)
        self.assertEqual(workspace_cursor["line"], 23)
        self.assertEqual(workspace_cursor["column"], 88)
        self.assertEqual(workspace_cursor["top_line"], 7)

        workspace_selection = inspection["workspace_selection"]
        assert isinstance(workspace_selection, dict)
        self.assertEqual(workspace_selection["start_line"], 23)
        self.assertEqual(workspace_selection["start_column"], 88)
        self.assertEqual(workspace_selection["end_line"], 23)
        self.assertEqual(workspace_selection["end_column"], 88)

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
