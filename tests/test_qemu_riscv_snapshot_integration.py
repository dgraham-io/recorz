from __future__ import annotations

import shutil
import subprocess
import unittest
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv64"
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


SNAPSHOT_SAVE_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-snapshot-save-test"
SNAPSHOT_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-snapshot-reload-test"
SNAPSHOT_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "saved-live-image.bin"
SNAPSHOT_CONTINUE_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-snapshot-continue-test"
SNAPSHOT_EVOLVED_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-snapshot-evolved-reload-test"
SNAPSHOT_CONTINUE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "continued-live-image.bin"
SNAPSHOT_FILE_IN_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-snapshot-file-in-test"
SNAPSHOT_FILE_IN_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-snapshot-file-in-reload-test"
SNAPSHOT_FILE_IN_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "file-in-live-image.bin"
SNAPSHOT_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_save_demo.rz"
SNAPSHOT_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_reload_demo.rz"
SNAPSHOT_CONTINUE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_continue_demo.rz"
SNAPSHOT_EVOLVED_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_evolved_reload_demo.rz"
SNAPSHOT_FILE_IN_BASE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_file_in_base_demo.rz"
SNAPSHOT_FILE_IN_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_file_in_save_demo.rz"
SNAPSHOT_FILE_IN_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_file_in_reload_demo.rz"
SNAPSHOT_FILE_IN_UPDATE_PATH = ROOT / "examples" / "qemu_riscv_snapshot_file_in_update.rz"
SNAPSHOT_STARTUP_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-startup-hook-test"
SNAPSHOT_STARTUP_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-startup-hook-reload-test"
SNAPSHOT_STARTUP_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "startup-hook-live-image.bin"
SNAPSHOT_STARTUP_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_startup_hook_save_demo.rz"
SNAPSHOT_STARTUP_IDLE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_startup_hook_idle_demo.rz"


@unittest.skipUnless(
    shutil.which("qemu-system-riscv64") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RISC-V snapshot integration test requires qemu-system-riscv64 and riscv64-unknown-elf-gcc",
)
class QemuRiscvSnapshotIntegrationTests(unittest.TestCase):
    def save_snapshot(self, *, build_dir: Path, example_path: Path, snapshot_output: Path) -> str:
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            f"SNAPSHOT_OUTPUT={snapshot_output}",
            "clean",
            "save-snapshot",
        ]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(f"QEMU save-snapshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        return (build_dir / "qemu.log").read_text(encoding="utf-8")

    def render_demo(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        snapshot_payload: Path,
    ) -> tuple[str, int, int, bytes]:
        ppm_path = build_dir / "recorz-qemu-riscv64-mvp.ppm"
        qemu_log_path = build_dir / "qemu.log"
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            f"SNAPSHOT_PAYLOAD={snapshot_payload}",
            "clean",
            "screenshot",
        ]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(f"QEMU screenshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        qemu_log = qemu_log_path.read_text(encoding="utf-8")
        width, height, data = _read_ppm(ppm_path)
        return qemu_log, width, height, data

    def continue_snapshot(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        snapshot_path: Path,
        file_in_payload: Path | None = None,
    ) -> str:
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            f"SNAPSHOT_PAYLOAD={snapshot_path}",
            "clean",
            "continue-snapshot",
        ]
        if file_in_payload is not None:
            command.insert(-2, f"FILE_IN_PAYLOAD={file_in_payload}")
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(
                f"QEMU continue-snapshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}"
            )
        return (build_dir / "qemu.log").read_text(encoding="utf-8")

    def test_live_snapshot_round_trips_dynamic_class_and_inherited_state(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_SAVE_BUILD_DIR,
            example_path=SNAPSHOT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(data, width, 24, 58, 220, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 120)
        self.assertGreater(line_1[TEXT_FOREGROUND], line_2[TEXT_FOREGROUND] + 120)

    def test_external_file_in_payload_can_evolve_a_saved_snapshot(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_SAVE_BUILD_DIR,
            example_path=SNAPSHOT_FILE_IN_BASE_DEMO_PATH,
            snapshot_output=SNAPSHOT_FILE_IN_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_FILE_IN_OUTPUT_PATH.exists())

        baseline_reload_log, baseline_width, baseline_height, baseline_data = self.render_demo(
            build_dir=SNAPSHOT_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_FILE_IN_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_FILE_IN_OUTPUT_PATH,
        )
        self.assertEqual((baseline_width, baseline_height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", baseline_reload_log)

        continue_log = self.continue_snapshot(
            build_dir=SNAPSHOT_FILE_IN_BUILD_DIR,
            example_path=SNAPSHOT_FILE_IN_SAVE_DEMO_PATH,
            snapshot_path=SNAPSHOT_FILE_IN_OUTPUT_PATH,
            file_in_payload=SNAPSHOT_FILE_IN_UPDATE_PATH,
        )
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", continue_log)
        self.assertIn("recorz qemu-riscv64 mvp: applied external file-in", continue_log)
        self.assertIn("recorz-snapshot-begin", continue_log)

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_FILE_IN_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_FILE_IN_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_FILE_IN_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        baseline_line_1 = _region_histogram(baseline_data, baseline_width, 24, 24, 260, 56)
        baseline_line_2 = _region_histogram(baseline_data, baseline_width, 24, 58, 260, 90)
        line_1 = _region_histogram(data, width, 24, 24, 260, 56)
        line_2 = _region_histogram(data, width, 24, 58, 260, 90)

        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], baseline_line_2[TEXT_FOREGROUND] + 180)
        self.assertGreater(line_2[TEXT_FOREGROUND], baseline_line_1[TEXT_FOREGROUND] - 80)

    def test_live_snapshot_supports_second_generation_evolution_without_rebuilding_cold_boot(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_SAVE_BUILD_DIR,
            example_path=SNAPSHOT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_CONTINUE_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_CONTINUE_OUTPUT_PATH.exists())

        continue_log = self.continue_snapshot(
            build_dir=SNAPSHOT_CONTINUE_BUILD_DIR,
            example_path=SNAPSHOT_CONTINUE_DEMO_PATH,
            snapshot_path=SNAPSHOT_CONTINUE_OUTPUT_PATH,
        )
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", continue_log)
        self.assertIn("recorz-snapshot-begin", continue_log)

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_EVOLVED_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_EVOLVED_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_CONTINUE_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 220, 56)
        line_2 = _region_histogram(data, width, 24, 58, 220, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 120)
        self.assertGreater(line_1[TEXT_FOREGROUND], line_2[TEXT_FOREGROUND] + 120)

    def test_snapshot_can_boot_into_persisted_startup_behavior_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_STARTUP_BUILD_DIR,
            example_path=SNAPSHOT_STARTUP_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_STARTUP_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_STARTUP_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_STARTUP_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_STARTUP_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_STARTUP_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (640, 480))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 260, 56)
        line_2 = _region_histogram(data, width, 24, 58, 260, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 320)
        self.assertLess(line_2[TEXT_FOREGROUND], 100)
