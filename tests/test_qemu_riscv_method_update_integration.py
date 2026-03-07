from __future__ import annotations

import shutil
import subprocess
import unittest
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv64"
BUILD_DIR = ROOT / "misc" / "qemu-riscv64-method-update-test"
APPEND_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-append-method-test"
IN_IMAGE_INSTALL_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-in-image-install-test"
IN_IMAGE_SOURCE_INSTALL_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-in-image-source-install-test"
IN_IMAGE_CHUNK_FILE_IN_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-in-image-chunk-file-in-test"
IN_IMAGE_CLASS_FILE_IN_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-in-image-class-file-in-test"
IN_IMAGE_CLASS_CREATE_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-in-image-class-create-test"
IN_IMAGE_INSTANCE_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-in-image-instance-test"
PPM_PATH = BUILD_DIR / "recorz-qemu-riscv64-mvp.ppm"
QEMU_LOG_PATH = BUILD_DIR / "qemu.log"
UPDATE_TOOL_PATH = ROOT / "tools" / "build_qemu_riscv_method_update.py"
UPDATE_PAYLOAD_PATH = ROOT / "misc" / "qemu-riscv64-method-update-payloads" / "transcript_cr_noop_update.bin"
APPEND_UPDATE_PAYLOAD_PATH = ROOT / "misc" / "qemu-riscv64-method-update-payloads" / "display_cr_update.bin"
UPDATE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_method_update_demo.rz"
UPDATE_SOURCE_PATH = ROOT / "examples" / "qemu_riscv_transcript_cr_noop_update.rz"
APPEND_DEMO_PATH = ROOT / "examples" / "qemu_riscv_append_method_demo.rz"
APPEND_UPDATE_SOURCE_PATH = ROOT / "examples" / "qemu_riscv_display_cr_update.rz"
IN_IMAGE_INSTALL_DEMO_PATH = ROOT / "examples" / "qemu_riscv_in_image_install_demo.rz"
IN_IMAGE_SOURCE_INSTALL_DEMO_PATH = ROOT / "examples" / "qemu_riscv_in_image_source_install_demo.rz"
IN_IMAGE_CHUNK_FILE_IN_DEMO_PATH = ROOT / "examples" / "qemu_riscv_in_image_chunk_file_in_demo.rz"
IN_IMAGE_CLASS_FILE_IN_DEMO_PATH = ROOT / "examples" / "qemu_riscv_in_image_class_file_in_demo.rz"
IN_IMAGE_CLASS_CREATE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_in_image_class_create_demo.rz"
IN_IMAGE_INSTANCE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_in_image_instance_demo.rz"


def _read_ppm(path: Path) -> tuple[int, int, bytes]:
    with path.open("rb") as ppm_file:
        if ppm_file.readline().strip() != b"P6":
            raise AssertionError("expected a binary PPM screendump")
        line = ppm_file.readline()
        while line.startswith(b"#"):
            line = ppm_file.readline()
        width, height = map(int, line.split())
        if int(ppm_file.readline()) != 255:
            raise AssertionError("expected 8-bit PPM data")
        data = ppm_file.read()
    return width, height, data


def _pixel(data: bytes, width: int, x: int, y: int) -> tuple[int, int, int]:
    index = (y * width + x) * 3
    return tuple(data[index : index + 3])


def _region_histogram(data: bytes, width: int, x0: int, y0: int, x1: int, y1: int) -> Counter[tuple[int, int, int]]:
    histogram: Counter[tuple[int, int, int]] = Counter()
    for y in range(y0, y1):
        for x in range(x0, x1):
            histogram[_pixel(data, width, x, y)] += 1
    return histogram


@unittest.skipUnless(
    shutil.which("qemu-system-riscv64") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RISC-V method update integration test requires qemu-system-riscv64 and riscv64-unknown-elf-gcc",
)
class QemuRiscvMethodUpdateIntegrationTests(unittest.TestCase):
    def build_update_payload(self, class_name: str, source_path: Path, output_path: Path) -> None:
        result = subprocess.run(
            [
                "python3",
                str(UPDATE_TOOL_PATH),
                class_name,
                str(source_path),
                str(output_path),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            self.fail(f"method update build failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")

    def render_demo(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        update_payload: Path | None,
    ) -> tuple[str, int, int, bytes]:
        ppm_path = build_dir / "recorz-qemu-riscv64-mvp.ppm"
        qemu_log_path = build_dir / "qemu.log"
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
        ]
        if update_payload is not None:
            command.append(f"UPDATE_PAYLOAD={update_payload}")
        command.extend(["clean", "screenshot"])
        result = subprocess.run(
            command,
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            self.fail(f"QEMU screenshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        qemu_log = qemu_log_path.read_text(encoding="utf-8")
        width, height, data = _read_ppm(ppm_path)
        return qemu_log, width, height, data

    def capture_timeout_output(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        update_payload: Path | None,
    ) -> str:
        elf_path = build_dir / "recorz-qemu-riscv64-mvp.elf"
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
        ]
        if update_payload is not None:
            command.append(f"UPDATE_PAYLOAD={update_payload}")
        command.extend(["clean", "all"])
        build = subprocess.run(
            command,
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if build.returncode != 0:
            self.fail(f"QEMU build failed\nstdout:\n{build.stdout}\nstderr:\n{build.stderr}")
        try:
            subprocess.run(
                [
                    "qemu-system-riscv64",
                    "-machine",
                    "virt",
                    "-m",
                    "128M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
                timeout=5,
                check=False,
            )
            self.fail("QEMU run unexpectedly exited without timing out")
        except subprocess.TimeoutExpired as exc:
            parts: list[str] = []
            for chunk in (exc.stdout, exc.stderr):
                if chunk is None:
                    continue
                if isinstance(chunk, bytes):
                    parts.append(chunk.decode("utf-8", errors="replace"))
                else:
                    parts.append(chunk)
            return "".join(parts)

    def test_live_method_update_changes_framebuffer_output(self) -> None:
        foreground = (72, 96, 32)
        background = (242, 242, 242)

        self.build_update_payload("Transcript", UPDATE_SOURCE_PATH, UPDATE_PAYLOAD_PATH)
        baseline_log, width, height, baseline_data = self.render_demo(
            build_dir=BUILD_DIR,
            example_path=UPDATE_DEMO_PATH,
            update_payload=None,
        )
        updated_log, updated_width, updated_height, updated_data = self.render_demo(
            build_dir=BUILD_DIR,
            example_path=UPDATE_DEMO_PATH,
            update_payload=UPDATE_PAYLOAD_PATH,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertEqual((updated_width, updated_height), (640, 480))
        self.assertNotIn("applied method update", baseline_log)
        self.assertIn("recorz qemu-riscv64 mvp: applied method update", updated_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", updated_log)

        baseline_line_1 = _region_histogram(baseline_data, width, 24, 24, 220, 56)
        baseline_line_2 = _region_histogram(baseline_data, width, 24, 58, 220, 90)
        updated_line_1 = _region_histogram(updated_data, updated_width, 24, 24, 220, 56)
        updated_line_2 = _region_histogram(updated_data, updated_width, 24, 58, 220, 90)

        self.assertGreater(baseline_line_1[foreground], 300)
        self.assertGreater(baseline_line_2[foreground], 300)
        self.assertGreater(updated_line_1[foreground], baseline_line_1[foreground] + 500)
        self.assertEqual(updated_line_2[foreground], 0)
        self.assertGreater(updated_line_2[background], baseline_line_2[background] + 500)

    def test_appended_method_update_adds_missing_selector(self) -> None:
        foreground = (72, 96, 32)

        baseline_output = self.capture_timeout_output(
            build_dir=APPEND_BUILD_DIR,
            example_path=APPEND_DEMO_PATH,
            update_payload=None,
        )
        self.assertIn("panic: selector is not understood by receiver class", baseline_output)
        self.assertIn("vm: send selector=cr args=0", baseline_output)

        self.build_update_payload("Display", APPEND_UPDATE_SOURCE_PATH, APPEND_UPDATE_PAYLOAD_PATH)
        updated_log, width, height, updated_data = self.render_demo(
            build_dir=APPEND_BUILD_DIR,
            example_path=APPEND_DEMO_PATH,
            update_payload=APPEND_UPDATE_PAYLOAD_PATH,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: applied method update", updated_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", updated_log)

        line_1 = _region_histogram(updated_data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(updated_data, width, 24, 58, 220, 90)
        self.assertGreater(line_1[foreground], 300)
        self.assertGreater(line_2[foreground], 300)

    def test_in_image_installer_adds_missing_selector_without_update_payload(self) -> None:
        foreground = (72, 96, 32)

        baseline_output = self.capture_timeout_output(
            build_dir=APPEND_BUILD_DIR,
            example_path=APPEND_DEMO_PATH,
            update_payload=None,
        )
        self.assertIn("panic: selector is not understood by receiver class", baseline_output)
        self.assertIn("vm: send selector=cr args=0", baseline_output)

        updated_log, width, height, updated_data = self.render_demo(
            build_dir=IN_IMAGE_INSTALL_BUILD_DIR,
            example_path=IN_IMAGE_INSTALL_DEMO_PATH,
            update_payload=None,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertNotIn("applied method update", updated_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", updated_log)

        line_1 = _region_histogram(updated_data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(updated_data, width, 24, 58, 220, 90)
        self.assertGreater(line_1[foreground], 300)
        self.assertGreater(line_2[foreground], 300)

    def test_in_image_source_installer_adds_missing_selector_without_update_payload(self) -> None:
        foreground = (72, 96, 32)

        baseline_output = self.capture_timeout_output(
            build_dir=APPEND_BUILD_DIR,
            example_path=APPEND_DEMO_PATH,
            update_payload=None,
        )
        self.assertIn("panic: selector is not understood by receiver class", baseline_output)
        self.assertIn("vm: send selector=cr args=0", baseline_output)

        updated_log, width, height, updated_data = self.render_demo(
            build_dir=IN_IMAGE_SOURCE_INSTALL_BUILD_DIR,
            example_path=IN_IMAGE_SOURCE_INSTALL_DEMO_PATH,
            update_payload=None,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertNotIn("applied method update", updated_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", updated_log)

        line_1 = _region_histogram(updated_data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(updated_data, width, 24, 58, 220, 90)
        self.assertGreater(line_1[foreground], 300)
        self.assertGreater(line_2[foreground], 300)

    def test_in_image_chunk_file_in_updates_multiple_methods_without_update_payload(self) -> None:
        foreground = (72, 96, 32)

        baseline_output = self.capture_timeout_output(
            build_dir=APPEND_BUILD_DIR,
            example_path=APPEND_DEMO_PATH,
            update_payload=None,
        )
        self.assertIn("panic: selector is not understood by receiver class", baseline_output)
        self.assertIn("vm: send selector=cr args=0", baseline_output)

        updated_log, width, height, updated_data = self.render_demo(
            build_dir=IN_IMAGE_CHUNK_FILE_IN_BUILD_DIR,
            example_path=IN_IMAGE_CHUNK_FILE_IN_DEMO_PATH,
            update_payload=None,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertNotIn("applied method update", updated_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", updated_log)

        line_1 = _region_histogram(updated_data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(updated_data, width, 24, 58, 220, 90)
        line_3 = _region_histogram(updated_data, width, 24, 92, 220, 124)
        self.assertGreater(line_1[foreground], 300)
        self.assertGreater(line_2[foreground], 300)
        self.assertGreater(line_3[foreground], 300)

    def test_in_image_class_file_in_resolves_existing_class_without_update_payload(self) -> None:
        foreground = (72, 96, 32)

        baseline_output = self.capture_timeout_output(
            build_dir=APPEND_BUILD_DIR,
            example_path=APPEND_DEMO_PATH,
            update_payload=None,
        )
        self.assertIn("panic: selector is not understood by receiver class", baseline_output)
        self.assertIn("vm: send selector=cr args=0", baseline_output)

        updated_log, width, height, updated_data = self.render_demo(
            build_dir=IN_IMAGE_CLASS_FILE_IN_BUILD_DIR,
            example_path=IN_IMAGE_CLASS_FILE_IN_DEMO_PATH,
            update_payload=None,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertNotIn("applied method update", updated_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", updated_log)

        line_1 = _region_histogram(updated_data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(updated_data, width, 24, 58, 220, 90)
        line_3 = _region_histogram(updated_data, width, 24, 92, 220, 124)
        self.assertGreater(line_1[foreground], 300)
        self.assertGreater(line_2[foreground], 300)
        self.assertGreater(line_3[foreground], 300)

    def test_in_image_class_file_in_creates_missing_class_without_update_payload(self) -> None:
        foreground = (72, 96, 32)

        updated_log, width, height, updated_data = self.render_demo(
            build_dir=IN_IMAGE_CLASS_CREATE_BUILD_DIR,
            example_path=IN_IMAGE_CLASS_CREATE_DEMO_PATH,
            update_payload=None,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: created class Helper", updated_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", updated_log)

        line_1 = _region_histogram(updated_data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(updated_data, width, 24, 58, 220, 90)
        self.assertGreater(line_1[foreground], 300)
        self.assertGreater(line_2[foreground], 300)

    def test_in_image_class_instances_can_send_installed_methods_without_update_payload(self) -> None:
        foreground = (72, 96, 32)

        updated_log, width, height, updated_data = self.render_demo(
            build_dir=IN_IMAGE_INSTANCE_BUILD_DIR,
            example_path=IN_IMAGE_INSTANCE_DEMO_PATH,
            update_payload=None,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: created class Helper", updated_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", updated_log)

        line_1 = _region_histogram(updated_data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(updated_data, width, 24, 58, 220, 90)
        line_3 = _region_histogram(updated_data, width, 24, 92, 220, 124)
        self.assertGreater(line_1[foreground], 300)
        self.assertGreater(line_2[foreground], 300)
        self.assertGreater(line_3[foreground], 300)


if __name__ == "__main__":
    unittest.main()
