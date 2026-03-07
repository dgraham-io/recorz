from __future__ import annotations

import shutil
import subprocess
import unittest
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv64"
BUILD_DIR = ROOT / "misc" / "qemu-riscv64-method-update-test"
PPM_PATH = BUILD_DIR / "recorz-qemu-riscv64-mvp.ppm"
QEMU_LOG_PATH = BUILD_DIR / "qemu.log"
UPDATE_TOOL_PATH = ROOT / "tools" / "build_qemu_riscv_method_update.py"
UPDATE_PAYLOAD_PATH = ROOT / "misc" / "qemu-riscv64-method-update-payloads" / "transcript_cr_noop_update.bin"
UPDATE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_method_update_demo.rz"
UPDATE_SOURCE_PATH = ROOT / "examples" / "qemu_riscv_transcript_cr_noop_update.rz"


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
    def build_update_payload(self) -> None:
        result = subprocess.run(
            [
                "python3",
                str(UPDATE_TOOL_PATH),
                "Transcript",
                str(UPDATE_SOURCE_PATH),
                str(UPDATE_PAYLOAD_PATH),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            self.fail(f"method update build failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")

    def render_demo(self, *, update_payload: Path | None) -> tuple[str, int, int, bytes]:
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={BUILD_DIR}",
            f"EXAMPLE={UPDATE_DEMO_PATH}",
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
        qemu_log = QEMU_LOG_PATH.read_text(encoding="utf-8")
        width, height, data = _read_ppm(PPM_PATH)
        return qemu_log, width, height, data

    def test_live_method_update_changes_framebuffer_output(self) -> None:
        foreground = (72, 96, 32)
        background = (242, 242, 242)

        self.build_update_payload()
        baseline_log, width, height, baseline_data = self.render_demo(update_payload=None)
        updated_log, updated_width, updated_height, updated_data = self.render_demo(update_payload=UPDATE_PAYLOAD_PATH)

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


if __name__ == "__main__":
    unittest.main()
