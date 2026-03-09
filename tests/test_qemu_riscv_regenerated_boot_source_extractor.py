from __future__ import annotations

import unittest

from tools.extract_qemu_riscv_regenerated_boot_source import extract_regenerated_boot_source


class QemuRiscvRegeneratedBootSourceExtractorTests(unittest.TestCase):
    def test_extract_regenerated_boot_source_accepts_crlf_serial_logs(self) -> None:
        log_text = (
            "booting\r\n"
            "recorz-regenerated-boot-source-begin 5\r\n"
            "recorz-regenerated-boot-source-data 68656c6c6f\r\n"
            "recorz-regenerated-boot-source-end\r\n"
            "rendered\r\n"
        )

        source_text = extract_regenerated_boot_source(log_text)

        self.assertEqual(source_text, "hello")

    def test_extract_regenerated_boot_source_accepts_prefixed_begin_marker(self) -> None:
        log_text = (
            "IMAGE READYrecorz-regenerated-boot-source-begin 9\n"
            "recorz-regenerated-boot-source-data 776f726b\n"
            "recorz-regenerated-boot-source-data 7370616365\n"
            "recorz-regenerated-boot-source-end\n"
        )

        source_text = extract_regenerated_boot_source(log_text)

        self.assertEqual(source_text, "workspace")


if __name__ == "__main__":
    unittest.main()
