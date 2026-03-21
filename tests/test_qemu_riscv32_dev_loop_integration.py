from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
DEV_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-dev-loop-test"
DEV_SNAPSHOT_PATH = ROOT / "misc" / "qemu-riscv32-dev-loop-test" / "live.bin"
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
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 dev-loop integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32DevLoopIntegrationTests(unittest.TestCase):
    def run_target(
        self,
        target: str,
        *,
        build_dir: Path = DEV_BUILD_DIR,
        snapshot_path: Path = DEV_SNAPSHOT_PATH,
        file_in_payload: Path | None = None,
        input_text: str | None = None,
        timeout: float = 120.0,
    ) -> str:
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"DEV_SNAPSHOT={snapshot_path}",
        ]
        if file_in_payload is not None:
            command.append(f"FILE_IN_PAYLOAD={file_in_payload}")
        command.append(target)
        try:
            result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, input=input_text, timeout=timeout)
        except subprocess.TimeoutExpired as exc:
            self.fail(
                f"{target} timed out after {timeout} seconds\n"
                f"stdout:\n{exc.stdout or ''}\n"
                f"stderr:\n{exc.stderr or ''}"
            )
        if result.returncode != 0:
            self.fail(f"{target} failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        qemu_log_path = build_dir / "qemu.log"
        combined_output = result.stdout
        if qemu_log_path.exists():
            combined_output += qemu_log_path.read_text(encoding="utf-8")
        return combined_output

    def render_dev_screenshot(
        self,
        *,
        build_dir: Path = DEV_BUILD_DIR,
        snapshot_path: Path = DEV_SNAPSHOT_PATH,
    ) -> tuple[str, int, int, bytes]:
        qemu_log = self.run_target("dev-screenshot", build_dir=build_dir, snapshot_path=snapshot_path)
        ppm_path = build_dir / "recorz-qemu-riscv32-mvp.ppm"
        width, height, data = _read_ppm(ppm_path)
        return qemu_log, width, height, data

    def test_dev_targets_boot_evolve_and_reboot_same_snapshot_without_switching_examples(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-dev-loop-evolve-", dir="/tmp") as temp_dir:
            build_dir = Path(temp_dir) / "build"
            snapshot_path = Path(temp_dir) / "dev" / "live.bin"

            init_log = self.run_target("dev-init", build_dir=build_dir, snapshot_path=snapshot_path)
            self.assertIn("recorz-snapshot-begin", init_log)
            self.assertTrue(snapshot_path.exists())

            baseline_log, baseline_width, baseline_height, baseline_data = self.render_dev_screenshot(
                build_dir=build_dir,
                snapshot_path=snapshot_path,
            )
            self.assertEqual((baseline_width, baseline_height), (1024, 768))
            self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", baseline_log)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", baseline_log)

            file_in_log = self.run_target(
                "dev-file-in",
                build_dir=build_dir,
                snapshot_path=snapshot_path,
                file_in_payload=IMAGE_FIRST_UPDATE_PATH,
            )
            self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", file_in_log)
            self.assertIn("recorz qemu-riscv32 mvp: applied external file-in", file_in_log)
            self.assertIn("recorz-snapshot-begin", file_in_log)

            evolved_log, width, height, evolved_data = self.render_dev_screenshot(
                build_dir=build_dir,
                snapshot_path=snapshot_path,
            )
            self.assertEqual((width, height), (1024, 768))
            self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", evolved_log)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", evolved_log)
            self.assertNotIn("panic:", evolved_log)

            baseline_line_1 = _region_histogram(baseline_data, baseline_width, 24, 24, 260, 56)
            baseline_line_2 = _region_histogram(baseline_data, baseline_width, 24, 58, 260, 90)
            evolved_line_1 = _region_histogram(evolved_data, width, 24, 24, 260, 56)
            evolved_line_2 = _region_histogram(evolved_data, width, 24, 58, 260, 90)

            self.assertGreater(baseline_line_1[TEXT_FOREGROUND], 200)
            self.assertLess(baseline_line_2[TEXT_FOREGROUND], 100)
            self.assertLess(evolved_line_1[TEXT_FOREGROUND], baseline_line_1[TEXT_FOREGROUND] - 180)
            self.assertLess(evolved_line_2[TEXT_FOREGROUND], 100)

    def test_dev_reset_creates_a_clean_snapshot_and_clears_stale_backup_state(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-dev-reset-clean-", dir="/tmp") as temp_dir:
            build_dir = Path(temp_dir) / "build"
            snapshot_path = Path(temp_dir) / "dev" / "live.bin"
            backup_path = snapshot_path.with_suffix(".bin.bak")

            backup_path.parent.mkdir(parents=True, exist_ok=True)
            backup_path.write_bytes(b"stale-backup")
            reset_log = self.run_target("dev-reset", build_dir=build_dir, snapshot_path=snapshot_path)

            self.assertIn("recorz-snapshot-begin", reset_log)
            self.assertTrue(snapshot_path.exists())
            self.assertFalse(backup_path.exists())

    def test_dev_restore_recovers_the_previous_snapshot_after_a_mutating_dev_file_in(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-dev-restore-", dir="/tmp") as temp_dir:
            build_dir = Path(temp_dir) / "build"
            snapshot_path = Path(temp_dir) / "dev" / "live.bin"
            backup_path = snapshot_path.with_suffix(".bin.bak")

            self.run_target("dev-reset", build_dir=build_dir, snapshot_path=snapshot_path)
            baseline_snapshot = snapshot_path.read_bytes()

            file_in_log = self.run_target(
                "dev-file-in",
                build_dir=build_dir,
                snapshot_path=snapshot_path,
                file_in_payload=IMAGE_FIRST_UPDATE_PATH,
            )
            self.assertIn("applied external file-in", file_in_log)
            self.assertTrue(snapshot_path.exists())
            self.assertTrue(backup_path.exists())
            self.assertEqual(backup_path.read_bytes(), baseline_snapshot)
            self.assertNotEqual(snapshot_path.read_bytes(), baseline_snapshot)

            restore_log = self.run_target("dev-restore", build_dir=build_dir, snapshot_path=snapshot_path)
            self.assertIn("restored", restore_log)
            self.assertEqual(snapshot_path.read_bytes(), baseline_snapshot)
            self.assertEqual(snapshot_path.read_bytes(), backup_path.read_bytes())

            screenshot_log, width, height, data = self.render_dev_screenshot(
                build_dir=build_dir,
                snapshot_path=snapshot_path,
            )
            self.assertEqual((width, height), (1024, 768))
            self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", screenshot_log)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", screenshot_log)
            self.assertNotIn("panic:", screenshot_log)
            self.assertGreater(_region_histogram(data, width, 24, 24, 260, 56)[TEXT_FOREGROUND], 200)


if __name__ == "__main__":
    unittest.main()
