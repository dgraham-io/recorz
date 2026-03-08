from __future__ import annotations

import shutil
import subprocess
import unittest
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv64"
DEV_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-dev-loop-test"
DEV_SNAPSHOT_PATH = ROOT / "misc" / "qemu-riscv64-dev-loop-test" / "live.bin"
IMAGE_FIRST_UPDATE_PATH = ROOT / "examples" / "qemu_riscv_image_first_transcript_show_noop_update.rz"
TEXT_FOREGROUND = (31, 41, 51)


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
    "QEMU RISC-V dev-loop integration test requires qemu-system-riscv64 and riscv64-unknown-elf-gcc",
)
class QemuRiscvDevLoopIntegrationTests(unittest.TestCase):
    def run_target(self, target: str, *, file_in_payload: Path | None = None) -> str:
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={DEV_BUILD_DIR}",
            f"DEV_SNAPSHOT={DEV_SNAPSHOT_PATH}",
        ]
        if file_in_payload is not None:
            command.append(f"FILE_IN_PAYLOAD={file_in_payload}")
        command.append(target)
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(f"{target} failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        qemu_log_path = DEV_BUILD_DIR / "qemu.log"
        if qemu_log_path.exists():
            return qemu_log_path.read_text(encoding="utf-8")
        return result.stdout

    def render_dev_screenshot(self) -> tuple[str, int, int, bytes]:
        qemu_log = self.run_target("dev-screenshot")
        ppm_path = DEV_BUILD_DIR / "recorz-qemu-riscv64-mvp.ppm"
        width, height, data = _read_ppm(ppm_path)
        return qemu_log, width, height, data

    def test_dev_targets_boot_evolve_and_reboot_same_snapshot_without_switching_examples(self) -> None:
        init_log = self.run_target("dev-init")
        self.assertIn("recorz-snapshot-begin", init_log)
        self.assertTrue(DEV_SNAPSHOT_PATH.exists())

        baseline_log, baseline_width, baseline_height, baseline_data = self.render_dev_screenshot()
        self.assertEqual((baseline_width, baseline_height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", baseline_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", baseline_log)

        file_in_log = self.run_target("dev-file-in", file_in_payload=IMAGE_FIRST_UPDATE_PATH)
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", file_in_log)
        self.assertIn("recorz qemu-riscv64 mvp: applied external file-in", file_in_log)
        self.assertIn("recorz-snapshot-begin", file_in_log)

        evolved_log, width, height, evolved_data = self.render_dev_screenshot()
        self.assertEqual((width, height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", evolved_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", evolved_log)
        self.assertNotIn("panic:", evolved_log)

        baseline_line_1 = _region_histogram(baseline_data, baseline_width, 24, 24, 260, 56)
        baseline_line_2 = _region_histogram(baseline_data, baseline_width, 24, 58, 260, 90)
        evolved_line_1 = _region_histogram(evolved_data, width, 24, 24, 260, 56)
        evolved_line_2 = _region_histogram(evolved_data, width, 24, 58, 260, 90)

        self.assertGreater(baseline_line_1[TEXT_FOREGROUND], 300)
        self.assertLess(baseline_line_2[TEXT_FOREGROUND], 100)
        self.assertLess(evolved_line_1[TEXT_FOREGROUND], baseline_line_1[TEXT_FOREGROUND] - 250)
        self.assertLess(evolved_line_2[TEXT_FOREGROUND], 100)


if __name__ == "__main__":
    unittest.main()
