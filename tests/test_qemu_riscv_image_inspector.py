import struct
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools"))

import build_qemu_riscv_mvp_image as mvp  # noqa: E402
import inspect_qemu_riscv_mvp_image as inspector  # noqa: E402


class QemuRiscvImageInspectorTests(unittest.TestCase):
    def test_inspects_boot_image_summary(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        image = mvp.build_image_manifest(program)

        summary = inspector.inspect_image_bytes(image)

        self.assertEqual(summary["version"], mvp.IMAGE_VERSION)
        self.assertEqual(summary["profile"], "RV64MVP1")
        self.assertEqual(summary["feature_names"], ["fnv1a32"])
        self.assertEqual([section["name"] for section in summary["sections"]], ["program", "seed"])
        self.assertEqual(summary["program"]["instruction_count"], 7)
        self.assertEqual(summary["program"]["literal_count"], 1)
        self.assertEqual(summary["seed"]["object_count"], 136)
        self.assertEqual(summary["seed"]["glyph_code_count"], 128)

    def test_rejects_profile_mismatch(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        image = bytearray(mvp.build_image_manifest(program))
        header = bytearray(image[: struct.calcsize(mvp.IMAGE_HEADER_FORMAT)])
        header[-8:] = b"BADPROF!"
        image[: struct.calcsize(mvp.IMAGE_HEADER_FORMAT)] = header

        with self.assertRaises(inspector.ImageInspectionError):
            inspector.inspect_image_bytes(bytes(image))

    def test_rejects_checksum_mismatch(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        image = bytearray(mvp.build_image_manifest(program))
        image[-1] ^= 0x01

        with self.assertRaises(inspector.ImageInspectionError):
            inspector.inspect_image_bytes(bytes(image))


if __name__ == "__main__":
    unittest.main()
