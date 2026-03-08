from __future__ import annotations

import unittest

from tools.extract_qemu_riscv_snapshot import extract_snapshot_bytes


class QemuRiscvSnapshotExtractorTests(unittest.TestCase):
    def test_extract_snapshot_bytes_accepts_crlf_serial_logs(self) -> None:
        log_text = (
            "booting\r\n"
            "recorz-snapshot-begin 4\r\n"
            "recorz-snapshot-data 00010203\r\n"
            "recorz-snapshot-end\r\n"
            "rendered\r\n"
        )

        snapshot = extract_snapshot_bytes(log_text)

        self.assertEqual(snapshot, bytes([0x00, 0x01, 0x02, 0x03]))

    def test_extract_snapshot_bytes_accepts_prefixed_begin_marker(self) -> None:
        log_text = (
            "IMAGE READYrecorz-snapshot-begin 4\n"
            "recorz-snapshot-data deadbeef\n"
            "recorz-snapshot-end\n"
        )

        snapshot = extract_snapshot_bytes(log_text)

        self.assertEqual(snapshot, bytes.fromhex("deadbeef"))


if __name__ == "__main__":
    unittest.main()
