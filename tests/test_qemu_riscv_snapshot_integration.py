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
SNAPSHOT_WORKSPACE_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-startup-test"
SNAPSHOT_WORKSPACE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-startup-reload-test"
SNAPSHOT_WORKSPACE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-startup-live-image.bin"
SNAPSHOT_WORKSPACE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_startup_save_demo.rz"
SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_startup_idle_demo.rz"
SNAPSHOT_WORKSPACE_CLASS_LIST_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-class-list-test"
SNAPSHOT_WORKSPACE_CLASS_LIST_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-class-list-reload-test"
SNAPSHOT_WORKSPACE_CLASS_LIST_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-class-list-live-image.bin"
SNAPSHOT_WORKSPACE_CLASS_LIST_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_class_list_save_demo.rz"
SNAPSHOT_WORKSPACE_OBJECT_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-object-test"
SNAPSHOT_WORKSPACE_OBJECT_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-object-reload-test"
SNAPSHOT_WORKSPACE_OBJECT_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-object-live-image.bin"
SNAPSHOT_WORKSPACE_OBJECT_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_object_browser_save_demo.rz"
SNAPSHOT_WORKSPACE_SESSION_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-session-test"
SNAPSHOT_WORKSPACE_SESSION_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-session-reload-test"
SNAPSHOT_WORKSPACE_SESSION_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-session-live-image.bin"
SNAPSHOT_WORKSPACE_SESSION_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_session_save_demo.rz"
SNAPSHOT_WORKSPACE_BUFFER_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-buffer-test"
SNAPSHOT_WORKSPACE_BUFFER_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-buffer-reload-test"
SNAPSHOT_WORKSPACE_BUFFER_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-buffer-live-image.bin"
SNAPSHOT_WORKSPACE_BUFFER_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_buffer_save_demo.rz"
SNAPSHOT_WORKSPACE_METHOD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-method-test"
SNAPSHOT_WORKSPACE_METHOD_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-method-reload-test"
SNAPSHOT_WORKSPACE_METHOD_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-method-live-image.bin"
SNAPSHOT_WORKSPACE_METHOD_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_method_browser_save_demo.rz"
SNAPSHOT_CLASS_SIDE_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-class-side-test"
SNAPSHOT_CLASS_SIDE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-class-side-reload-test"
SNAPSHOT_CLASS_SIDE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "class-side-live-image.bin"
SNAPSHOT_CLASS_SIDE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_side_snapshot_save_demo.rz"
SNAPSHOT_CLASS_SIDE_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_side_snapshot_reload_demo.rz"
SNAPSHOT_WORKSPACE_CLASS_SIDE_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-class-side-test"
SNAPSHOT_WORKSPACE_CLASS_SIDE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-class-side-reload-test"
SNAPSHOT_WORKSPACE_CLASS_SIDE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-class-side-live-image.bin"
SNAPSHOT_WORKSPACE_CLASS_SIDE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_class_side_browser_save_demo.rz"
SNAPSHOT_WORKSPACE_PROTOCOL_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-protocol-test"
SNAPSHOT_WORKSPACE_PROTOCOL_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-protocol-reload-test"
SNAPSHOT_WORKSPACE_PROTOCOL_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-protocol-live-image.bin"
SNAPSHOT_WORKSPACE_PROTOCOL_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_protocol_browser_save_demo.rz"
SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-class-protocol-test"
SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-workspace-class-protocol-reload-test"
SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "workspace-class-protocol-live-image.bin"
SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_class_protocol_browser_save_demo.rz"
SNAPSHOT_CLASS_FILE_OUT_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-class-file-out-test"
SNAPSHOT_CLASS_FILE_OUT_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv64-class-file-out-reload-test"
SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv64-snapshots" / "class-file-out-live-image.bin"
SNAPSHOT_CLASS_FILE_OUT_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_file_out_snapshot_save_demo.rz"
SNAPSHOT_CLASS_FILE_OUT_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_file_out_snapshot_reload_demo.rz"


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

        self.assertEqual((width, height), (1024, 768))
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
        self.assertEqual((baseline_width, baseline_height), (1024, 768))
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

        self.assertEqual((width, height), (1024, 768))
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

        self.assertEqual((width, height), (1024, 768))
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

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 260, 56)
        line_2 = _region_histogram(data, width, 24, 58, 260, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 320)
        self.assertLess(line_2[TEXT_FOREGROUND], 100)

    def test_snapshot_can_reopen_workspace_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 420, 90)
        line_3 = _region_histogram(data, width, 24, 92, 420, 124)
        line_4 = _region_histogram(data, width, 24, 126, 320, 158)
        line_5 = _region_histogram(data, width, 24, 160, 360, 192)
        line_6 = _region_histogram(data, width, 24, 194, 320, 226)

        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 420)
        self.assertGreater(line_3[TEXT_FOREGROUND], 180)
        self.assertGreater(line_4[TEXT_FOREGROUND], 120)
        self.assertGreater(line_5[TEXT_FOREGROUND], 180)
        self.assertLess(line_6[TEXT_FOREGROUND], 80)

    def test_snapshot_can_reopen_workspace_class_list_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_CLASS_LIST_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_CLASS_LIST_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_CLASS_LIST_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_CLASS_LIST_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_CLASS_LIST_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_CLASS_LIST_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 420, 90)
        line_3 = _region_histogram(data, width, 24, 92, 320, 124)
        line_4 = _region_histogram(data, width, 24, 126, 320, 158)
        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 220)
        self.assertGreater(line_3[TEXT_FOREGROUND], 160)
        self.assertGreater(line_4[TEXT_FOREGROUND], 140)

    def test_snapshot_can_reopen_workspace_object_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_OBJECT_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_OBJECT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_OBJECT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_OBJECT_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_OBJECT_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_OBJECT_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 500, 90)
        line_3 = _region_histogram(data, width, 24, 92, 420, 124)
        line_4 = _region_histogram(data, width, 24, 126, 320, 158)
        line_5 = _region_histogram(data, width, 24, 160, 500, 192)
        line_6 = _region_histogram(data, width, 24, 194, 320, 226)

        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 420)
        self.assertGreater(line_3[TEXT_FOREGROUND], 320)
        self.assertGreater(line_4[TEXT_FOREGROUND], 120)
        self.assertGreater(line_5[TEXT_FOREGROUND], 280)
        self.assertLess(line_6[TEXT_FOREGROUND], 80)

    def test_snapshot_can_rerun_workspace_source_session_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_SESSION_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_SESSION_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_SESSION_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_SESSION_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_SESSION_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_SESSION_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 420, 90)
        line_3 = _region_histogram(data, width, 24, 92, 420, 124)
        line_4 = _region_histogram(data, width, 24, 126, 320, 158)
        line_5 = _region_histogram(data, width, 24, 160, 360, 192)
        line_6 = _region_histogram(data, width, 24, 194, 420, 226)
        line_7 = _region_histogram(data, width, 24, 228, 420, 260)

        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 420)
        self.assertGreater(line_3[TEXT_FOREGROUND], 180)
        self.assertGreater(line_4[TEXT_FOREGROUND], 120)
        self.assertGreater(line_5[TEXT_FOREGROUND], 180)
        self.assertGreater(line_6[TEXT_FOREGROUND], 900)
        self.assertLess(line_7[TEXT_FOREGROUND], 80)

    def test_snapshot_can_evaluate_persisted_workspace_buffer_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_BUFFER_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_BUFFER_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_BUFFER_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_BUFFER_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_BUFFER_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_BUFFER_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 420, 90)
        line_3 = _region_histogram(data, width, 24, 92, 420, 124)
        line_4 = _region_histogram(data, width, 24, 126, 320, 158)
        line_5 = _region_histogram(data, width, 24, 160, 360, 192)
        line_6 = _region_histogram(data, width, 24, 194, 420, 226)
        line_7 = _region_histogram(data, width, 24, 228, 420, 260)

        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 420)
        self.assertGreater(line_3[TEXT_FOREGROUND], 180)
        self.assertGreater(line_4[TEXT_FOREGROUND], 120)
        self.assertGreater(line_5[TEXT_FOREGROUND], 180)
        self.assertGreater(line_6[TEXT_FOREGROUND], 900)
        self.assertLess(line_7[TEXT_FOREGROUND], 80)

    def test_snapshot_can_reopen_workspace_method_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_METHOD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_METHOD_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_METHOD_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_METHOD_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_METHOD_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_METHOD_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 420, 90)
        line_3 = _region_histogram(data, width, 24, 92, 520, 124)
        source_region = _region_histogram(data, width, 24, 126, 900, 360)

        self.assertGreater(line_1[TEXT_FOREGROUND], 240)
        self.assertGreater(line_2[TEXT_FOREGROUND], 120)
        self.assertGreater(line_3[TEXT_FOREGROUND], 80)
        self.assertGreater(source_region[TEXT_FOREGROUND], 2200)

    def test_snapshot_preserves_live_class_side_method_lookup(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_CLASS_SIDE_BUILD_DIR,
            example_path=SNAPSHOT_CLASS_SIDE_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_CLASS_SIDE_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_CLASS_SIDE_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_CLASS_SIDE_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_CLASS_SIDE_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_CLASS_SIDE_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 420, 90)
        line_3 = _region_histogram(data, width, 24, 92, 420, 124)

        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 900)
        self.assertLess(line_3[TEXT_FOREGROUND], 80)

    def test_snapshot_preserves_workspace_class_file_out_buffer_for_refile_in(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_CLASS_FILE_OUT_BUILD_DIR,
            example_path=SNAPSHOT_CLASS_FILE_OUT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_CLASS_FILE_OUT_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_CLASS_FILE_OUT_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 260, 56)
        line_2 = _region_histogram(data, width, 24, 58, 260, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 300)

    def test_snapshot_can_reopen_workspace_class_source_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_CLASS_FILE_OUT_BUILD_DIR,
            example_path=SNAPSHOT_CLASS_FILE_OUT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_CLASS_FILE_OUT_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 360, 56)
        line_2 = _region_histogram(data, width, 24, 58, 360, 90)
        source_region = _region_histogram(data, width, 24, 92, 900, 260)

        self.assertGreater(line_1[TEXT_FOREGROUND], 800)
        self.assertGreater(line_2[TEXT_FOREGROUND], 500)
        self.assertGreater(source_region[TEXT_FOREGROUND], 2500)

    def test_snapshot_can_reopen_workspace_class_side_method_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_CLASS_SIDE_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_CLASS_SIDE_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_CLASS_SIDE_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_CLASS_SIDE_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_CLASS_SIDE_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_CLASS_SIDE_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 520, 56)
        line_2 = _region_histogram(data, width, 24, 58, 520, 90)
        line_3 = _region_histogram(data, width, 24, 92, 520, 124)
        source_region = _region_histogram(data, width, 24, 126, 900, 360)

        self.assertGreater(line_1[TEXT_FOREGROUND], 900)
        self.assertGreater(line_2[TEXT_FOREGROUND], 900)
        self.assertGreater(line_3[TEXT_FOREGROUND], 900)
        self.assertGreater(source_region[TEXT_FOREGROUND], 2500)

    def test_snapshot_can_reopen_workspace_protocol_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_PROTOCOL_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_PROTOCOL_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_PROTOCOL_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_PROTOCOL_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_PROTOCOL_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_PROTOCOL_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 520, 56)
        line_2 = _region_histogram(data, width, 24, 58, 520, 90)
        line_3 = _region_histogram(data, width, 24, 92, 520, 124)
        line_4 = _region_histogram(data, width, 24, 126, 520, 158)
        line_5 = _region_histogram(data, width, 24, 160, 520, 192)

        self.assertGreater(line_1[TEXT_FOREGROUND], 700)
        self.assertGreater(line_2[TEXT_FOREGROUND], 500)
        self.assertGreater(line_3[TEXT_FOREGROUND], 700)
        self.assertGreater(line_4[TEXT_FOREGROUND], 500)
        self.assertGreater(line_5[TEXT_FOREGROUND], 300)

    def test_snapshot_can_reopen_workspace_class_protocol_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 520, 56)
        line_2 = _region_histogram(data, width, 24, 58, 520, 90)
        line_3 = _region_histogram(data, width, 24, 92, 520, 124)
        line_4 = _region_histogram(data, width, 24, 126, 520, 158)
        line_5 = _region_histogram(data, width, 24, 160, 520, 192)

        self.assertGreater(line_1[TEXT_FOREGROUND], 700)
        self.assertGreater(line_2[TEXT_FOREGROUND], 500)
        self.assertGreater(line_3[TEXT_FOREGROUND], 500)
        self.assertGreater(line_4[TEXT_FOREGROUND], 500)
        self.assertGreater(line_5[TEXT_FOREGROUND], 300)
