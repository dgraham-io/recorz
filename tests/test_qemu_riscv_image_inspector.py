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
    @staticmethod
    def _rewrite_checksum(image: bytearray) -> None:
        header_size = struct.calcsize(mvp.IMAGE_HEADER_FORMAT)
        magic, version, section_count, feature_flags, _checksum, profile = struct.unpack_from(mvp.IMAGE_HEADER_FORMAT, image, 0)
        checksum = mvp.fnv1a32(bytes(image[header_size:]))
        image[:header_size] = struct.pack(
            mvp.IMAGE_HEADER_FORMAT,
            magic,
            version,
            section_count,
            feature_flags,
            checksum,
            profile,
        )

    def test_inspects_boot_image_summary(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        image = mvp.build_image_manifest(program)

        summary = inspector.inspect_image_bytes(image)

        self.assertEqual(summary["version"], mvp.IMAGE_VERSION)
        self.assertEqual(summary["profile"], "RV64MVP1")
        self.assertEqual(summary["feature_names"], ["fnv1a32"])
        self.assertEqual([section["name"] for section in summary["sections"]], ["entry", "program", "seed"])
        self.assertEqual(summary["entry"]["kind"], "doit")
        self.assertEqual(summary["entry"]["argument_count"], 0)
        self.assertEqual(summary["entry"]["program_section"], "program")
        self.assertEqual(summary["program"]["instruction_count"], 7)
        self.assertEqual(summary["program"]["literal_count"], 1)
        self.assertEqual(summary["seed"]["object_count"], 217)
        self.assertEqual(summary["seed"]["class_descriptor_count"], 16)
        self.assertEqual(summary["seed"]["class_link_count"], 217)
        self.assertEqual(summary["seed"]["method_descriptor_count"], 22)
        self.assertEqual(summary["seed"]["selector_object_count"], 17)
        self.assertEqual(summary["seed"]["method_entry_object_count"], 22)
        self.assertEqual(summary["seed"]["declared_method_count"], 22)
        self.assertEqual(summary["seed"]["method_entry_count"], 22)
        self.assertEqual(summary["seed"]["global_binding_count"], 6)
        self.assertEqual(summary["seed"]["root_binding_count"], 6)
        self.assertEqual(summary["seed"]["globals"]["Transcript"], 0)
        self.assertEqual(summary["seed"]["roots"]["default_form"], 9)
        self.assertEqual(summary["seed"]["roots"]["transcript_behavior"], 139)
        self.assertEqual(summary["seed"]["roots"]["transcript_layout"], 6)
        self.assertEqual(summary["seed"]["roots"]["transcript_style"], 7)
        self.assertEqual(summary["seed"]["roots"]["transcript_metrics"], 138)
        self.assertEqual(summary["seed"]["glyph_code_count"], 128)

    def test_rejects_profile_mismatch(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        image = bytearray(mvp.build_image_manifest(program))
        header = bytearray(image[: struct.calcsize(mvp.IMAGE_HEADER_FORMAT)])
        header[-8:] = b"BADPROF!"
        image[: struct.calcsize(mvp.IMAGE_HEADER_FORMAT)] = header

        with self.assertRaises(inspector.ImageInspectionError):
            inspector.inspect_image_bytes(bytes(image))

    def test_rejects_entry_kind_mismatch(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        image = bytearray(mvp.build_image_manifest(program))
        header_size = struct.calcsize(mvp.IMAGE_HEADER_FORMAT)
        section_size = struct.calcsize(mvp.IMAGE_SECTION_FORMAT)
        entry_offset = struct.unpack_from(mvp.IMAGE_SECTION_FORMAT, image, header_size)[2]

        image[entry_offset + 6] = 0x02
        image[entry_offset + 7] = 0x00
        self._rewrite_checksum(image)

        with self.assertRaises(inspector.ImageInspectionError):
            inspector.inspect_image_bytes(bytes(image))

    def test_rejects_checksum_mismatch(self) -> None:
        program = mvp.build_program("Transcript show: 'HELLO'; cr")
        image = bytearray(mvp.build_image_manifest(program))
        image[-1] ^= 0x01

        with self.assertRaises(inspector.ImageInspectionError):
            inspector.inspect_image_bytes(bytes(image))

    def test_rejects_method_descriptor_primitive_kind_mismatch(self) -> None:
        seed = bytearray(mvp.build_seed_manifest())
        header_size = struct.calcsize(mvp.SEED_HEADER_FORMAT)
        object_size = struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT) + (4 * struct.calcsize(mvp.SEED_FIELD_FORMAT))
        method_offset = header_size + (195 * object_size) + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)
        primitive_kind_field_offset = method_offset + (2 * struct.calcsize(mvp.SEED_FIELD_FORMAT))

        seed[primitive_kind_field_offset + 1 : primitive_kind_field_offset + 5] = struct.pack("<i", mvp.SEED_OBJECT_DISPLAY)

        with self.assertRaises(inspector.ImageInspectionError):
            inspector.inspect_seed_manifest(bytes(seed))

    def test_rejects_method_descriptor_entry_mismatch(self) -> None:
        seed = bytearray(mvp.build_seed_manifest())
        header_size = struct.calcsize(mvp.SEED_HEADER_FORMAT)
        object_size = struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT) + (4 * struct.calcsize(mvp.SEED_FIELD_FORMAT))
        class_instance_kind_entry_object_index = 173 + (
            mvp.METHOD_ENTRY_VALUES["RECORZ_MVP_METHOD_ENTRY_CLASS_INSTANCE_KIND"] - 1
        )
        entry_offset = header_size + (
            class_instance_kind_entry_object_index * object_size
        ) + struct.calcsize(mvp.SEED_OBJECT_HEADER_FORMAT)

        seed[entry_offset + 1 : entry_offset + 5] = struct.pack(
            "<i",
            mvp.METHOD_ENTRY_VALUES["RECORZ_MVP_METHOD_ENTRY_TRANSCRIPT_SHOW"],
        )

        with self.assertRaises(inspector.ImageInspectionError):
            inspector.inspect_seed_manifest(bytes(seed))


if __name__ == "__main__":
    unittest.main()
