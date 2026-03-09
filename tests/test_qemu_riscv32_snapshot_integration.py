from __future__ import annotations

import shutil
import subprocess
import sys
import unittest
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
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


SNAPSHOT_SAVE_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-snapshot-save-test"
SNAPSHOT_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-snapshot-reload-test"
SNAPSHOT_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "saved-live-image.bin"
SNAPSHOT_FILE_IN_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-snapshot-file-in-test"
SNAPSHOT_FILE_IN_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-snapshot-file-in-reload-test"
SNAPSHOT_FILE_IN_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "file-in-live-image.bin"
SNAPSHOT_WORKSPACE_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-startup-test"
SNAPSHOT_WORKSPACE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-startup-reload-test"
SNAPSHOT_WORKSPACE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-startup-live-image.bin"
SNAPSHOT_CLASS_SIDE_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-class-side-test"
SNAPSHOT_CLASS_SIDE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-class-side-reload-test"
SNAPSHOT_CLASS_SIDE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "class-side-live-image.bin"
SNAPSHOT_WORKSPACE_METHOD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-method-test"
SNAPSHOT_WORKSPACE_METHOD_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-method-reload-test"
SNAPSHOT_WORKSPACE_METHOD_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-method-live-image.bin"
SNAPSHOT_WORKSPACE_CLASS_SIDE_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-class-side-test"
SNAPSHOT_WORKSPACE_CLASS_SIDE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-class-side-reload-test"
SNAPSHOT_WORKSPACE_CLASS_SIDE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-class-side-live-image.bin"
SNAPSHOT_WORKSPACE_PROTOCOL_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-protocol-test"
SNAPSHOT_WORKSPACE_PROTOCOL_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-protocol-reload-test"
SNAPSHOT_WORKSPACE_PROTOCOL_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-protocol-live-image.bin"
SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-class-protocol-test"
SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-class-protocol-reload-test"
SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-class-protocol-live-image.bin"
SNAPSHOT_WORKSPACE_PACKAGE_LIST_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-package-list-test"
SNAPSHOT_WORKSPACE_PACKAGE_LIST_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-package-list-reload-test"
SNAPSHOT_WORKSPACE_PACKAGE_LIST_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-package-list-live-image.bin"
SNAPSHOT_WORKSPACE_PACKAGE_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-package-test"
SNAPSHOT_WORKSPACE_PACKAGE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-package-reload-test"
SNAPSHOT_WORKSPACE_PACKAGE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-package-live-image.bin"
SNAPSHOT_CLASS_FILE_OUT_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-class-file-out-test"
SNAPSHOT_CLASS_FILE_OUT_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-class-file-out-reload-test"
SNAPSHOT_CLASS_FILE_OUT_COLD_FILE_IN_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-class-file-out-cold-file-in-test"
SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "class-file-out-live-image.bin"
SNAPSHOT_CLASS_FILE_OUT_EXTRACTED_SOURCE_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "class-file-out-current-source.rz"
SNAPSHOT_PACKAGE_FILE_OUT_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-package-file-out-test"
SNAPSHOT_PACKAGE_FILE_OUT_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-package-file-out-reload-test"
SNAPSHOT_PACKAGE_FILE_OUT_COLD_FILE_IN_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-package-file-out-cold-file-in-test"
SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "package-file-out-live-image.bin"
SNAPSHOT_PACKAGE_FILE_OUT_EXTRACTED_SOURCE_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "package-file-out-current-source.rz"

SNAPSHOT_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_save_demo.rz"
SNAPSHOT_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_reload_demo.rz"
SNAPSHOT_FILE_IN_BASE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_file_in_base_demo.rz"
SNAPSHOT_FILE_IN_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_file_in_save_demo.rz"
SNAPSHOT_FILE_IN_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_snapshot_file_in_reload_demo.rz"
SNAPSHOT_FILE_IN_UPDATE_PATH = ROOT / "examples" / "qemu_riscv_snapshot_file_in_update.rz"
SNAPSHOT_WORKSPACE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_startup_save_demo.rz"
SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_startup_idle_demo.rz"
SNAPSHOT_CLASS_SIDE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_side_snapshot_save_demo.rz"
SNAPSHOT_CLASS_SIDE_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_side_snapshot_reload_demo.rz"
SNAPSHOT_WORKSPACE_METHOD_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_method_browser_save_demo.rz"
SNAPSHOT_WORKSPACE_CLASS_SIDE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_class_side_browser_save_demo.rz"
SNAPSHOT_WORKSPACE_PROTOCOL_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_protocol_browser_save_demo.rz"
SNAPSHOT_WORKSPACE_CLASS_PROTOCOL_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_class_protocol_browser_save_demo.rz"
SNAPSHOT_WORKSPACE_PACKAGE_LIST_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_package_list_save_demo.rz"
SNAPSHOT_WORKSPACE_PACKAGE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_package_browser_save_demo.rz"
SNAPSHOT_CLASS_FILE_OUT_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_file_out_snapshot_save_demo.rz"
SNAPSHOT_CLASS_FILE_OUT_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_file_out_snapshot_reload_demo.rz"
SNAPSHOT_CLASS_FILE_OUT_COLD_FILE_IN_DEMO_PATH = ROOT / "examples" / "qemu_riscv_class_file_out_cold_file_in_demo.rz"
SNAPSHOT_PACKAGE_FILE_OUT_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_package_file_out_snapshot_save_demo.rz"
SNAPSHOT_PACKAGE_FILE_OUT_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_package_file_out_snapshot_reload_demo.rz"
SNAPSHOT_PACKAGE_FILE_OUT_COLD_FILE_IN_DEMO_PATH = ROOT / "examples" / "qemu_riscv_package_file_out_cold_file_in_demo.rz"


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 snapshot integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32SnapshotIntegrationTests(unittest.TestCase):
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
            self.fail(f"QEMU RV32 save-snapshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        return (build_dir / "qemu.log").read_text(encoding="utf-8")

    def render_demo(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        snapshot_payload: Path,
    ) -> tuple[str, int, int, bytes]:
        ppm_path = build_dir / "recorz-qemu-riscv32-mvp.ppm"
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
            self.fail(f"QEMU RV32 screenshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        qemu_log = qemu_log_path.read_text(encoding="utf-8")
        width, height, data = _read_ppm(ppm_path)
        return qemu_log, width, height, data

    def extract_workspace_current_source(self, *, snapshot_path: Path, output_path: Path) -> str:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        if output_path.exists():
            output_path.unlink()
        result = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools" / "inspect_qemu_riscv_snapshot.py"),
                str(snapshot_path),
                "--extract-workspace-current-source",
                str(output_path),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            self.fail(
                "workspace source extraction failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        return output_path.read_text(encoding="utf-8")

    def render_cold_file_in_demo(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        file_in_payload: Path,
    ) -> tuple[str, int, int, bytes]:
        ppm_path = build_dir / "recorz-qemu-riscv32-mvp.ppm"
        qemu_log_path = build_dir / "qemu.log"
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            f"FILE_IN_PAYLOAD={file_in_payload}",
            "clean",
            "screenshot",
        ]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(
                f"QEMU RV32 cold file-in screenshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}"
            )
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
                f"QEMU RV32 continue-snapshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}"
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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)

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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", baseline_reload_log)

        continue_log = self.continue_snapshot(
            build_dir=SNAPSHOT_FILE_IN_BUILD_DIR,
            example_path=SNAPSHOT_FILE_IN_SAVE_DEMO_PATH,
            snapshot_path=SNAPSHOT_FILE_IN_OUTPUT_PATH,
            file_in_payload=SNAPSHOT_FILE_IN_UPDATE_PATH,
        )
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", continue_log)
        self.assertIn("recorz qemu-riscv32 mvp: applied external file-in", continue_log)
        self.assertIn("recorz-snapshot-begin", continue_log)

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_FILE_IN_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_FILE_IN_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_FILE_IN_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        baseline_line_1 = _region_histogram(baseline_data, baseline_width, 24, 24, 260, 56)
        baseline_line_2 = _region_histogram(baseline_data, baseline_width, 24, 58, 260, 90)
        line_1 = _region_histogram(data, width, 24, 24, 260, 56)
        line_2 = _region_histogram(data, width, 24, 58, 260, 90)

        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], baseline_line_2[TEXT_FOREGROUND] + 180)
        self.assertGreater(line_2[TEXT_FOREGROUND], baseline_line_1[TEXT_FOREGROUND] - 80)

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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 360, 56)
        line_2 = _region_histogram(data, width, 24, 58, 360, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 450)
        self.assertGreater(line_2[TEXT_FOREGROUND], 300)

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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 260, 56)
        line_2 = _region_histogram(data, width, 24, 58, 260, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 300)

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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 420, 90)
        line_3 = _region_histogram(data, width, 24, 92, 520, 124)
        source_region = _region_histogram(data, width, 24, 126, 900, 360)

        self.assertGreater(line_1[TEXT_FOREGROUND], 240)
        self.assertGreater(line_2[TEXT_FOREGROUND], 120)
        self.assertGreater(line_3[TEXT_FOREGROUND], 80)
        self.assertGreater(source_region[TEXT_FOREGROUND], 1200)

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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 520, 56)
        line_2 = _region_histogram(data, width, 24, 58, 520, 90)
        line_3 = _region_histogram(data, width, 24, 92, 520, 124)
        source_region = _region_histogram(data, width, 24, 126, 900, 360)

        self.assertGreater(line_1[TEXT_FOREGROUND], 240)
        self.assertGreater(line_2[TEXT_FOREGROUND], 120)
        self.assertGreater(line_3[TEXT_FOREGROUND], 80)
        self.assertGreater(source_region[TEXT_FOREGROUND], 1200)

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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)

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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 360, 56)
        line_2 = _region_histogram(data, width, 24, 58, 360, 90)
        source_region = _region_histogram(data, width, 24, 92, 900, 260)

        self.assertGreater(line_1[TEXT_FOREGROUND], 800)
        self.assertGreater(line_2[TEXT_FOREGROUND], 500)
        self.assertGreater(source_region[TEXT_FOREGROUND], 2500)

    def test_snapshot_preserves_workspace_package_file_out_buffer_for_refile_in(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_PACKAGE_FILE_OUT_BUILD_DIR,
            example_path=SNAPSHOT_PACKAGE_FILE_OUT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_PACKAGE_FILE_OUT_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_PACKAGE_FILE_OUT_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 260, 56)
        line_2 = _region_histogram(data, width, 24, 58, 260, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertGreater(line_2[TEXT_FOREGROUND], 300)

    def test_snapshot_can_reopen_workspace_package_source_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_PACKAGE_FILE_OUT_BUILD_DIR,
            example_path=SNAPSHOT_PACKAGE_FILE_OUT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_PACKAGE_FILE_OUT_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 360, 56)
        line_2 = _region_histogram(data, width, 24, 58, 360, 90)
        source_region = _region_histogram(data, width, 24, 92, 900, 260)

        self.assertGreater(line_1[TEXT_FOREGROUND], 700)
        self.assertGreater(line_2[TEXT_FOREGROUND], 500)
        self.assertGreater(source_region[TEXT_FOREGROUND], 2500)

    def test_snapshot_extracted_class_source_cold_files_into_a_fresh_image(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_CLASS_FILE_OUT_BUILD_DIR,
            example_path=SNAPSHOT_CLASS_FILE_OUT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH.exists())

        extracted_source = self.extract_workspace_current_source(
            snapshot_path=SNAPSHOT_CLASS_FILE_OUT_OUTPUT_PATH,
            output_path=SNAPSHOT_CLASS_FILE_OUT_EXTRACTED_SOURCE_PATH,
        )
        self.assertIn("RecorzKernelClass: #Exported", extracted_source)
        self.assertIn("Snapshot export demo", extracted_source)
        self.assertIn("setValue: text", extracted_source)

        reload_log, width, height, _data = self.render_cold_file_in_demo(
            build_dir=SNAPSHOT_CLASS_FILE_OUT_COLD_FILE_IN_BUILD_DIR,
            example_path=SNAPSHOT_CLASS_FILE_OUT_COLD_FILE_IN_DEMO_PATH,
            file_in_payload=SNAPSHOT_CLASS_FILE_OUT_EXTRACTED_SOURCE_PATH,
        )
        flat_reload_log = reload_log.replace("\r", "").replace("\n", "")
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: applied external file-in", reload_log)
        self.assertIn("RecorzKernelClass: #Exported", flat_reload_log)
        self.assertIn("Snapshot export demo", flat_reload_log)
        self.assertIn("ROUNDTRIP", flat_reload_log)
        self.assertIn("COLD", flat_reload_log)
        self.assertNotIn("panic:", reload_log)

    def test_snapshot_extracted_package_source_cold_files_into_a_fresh_image(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_PACKAGE_FILE_OUT_BUILD_DIR,
            example_path=SNAPSHOT_PACKAGE_FILE_OUT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH.exists())

        extracted_source = self.extract_workspace_current_source(
            snapshot_path=SNAPSHOT_PACKAGE_FILE_OUT_OUTPUT_PATH,
            output_path=SNAPSHOT_PACKAGE_FILE_OUT_EXTRACTED_SOURCE_PATH,
        )
        self.assertIn("RecorzKernelPackage: 'Tools'", extracted_source)
        self.assertIn("Shared utilities", extracted_source)
        self.assertIn("RecorzKernelClass: #PackageEcho", extracted_source)

        reload_log, width, height, _data = self.render_cold_file_in_demo(
            build_dir=SNAPSHOT_PACKAGE_FILE_OUT_COLD_FILE_IN_BUILD_DIR,
            example_path=SNAPSHOT_PACKAGE_FILE_OUT_COLD_FILE_IN_DEMO_PATH,
            file_in_payload=SNAPSHOT_PACKAGE_FILE_OUT_EXTRACTED_SOURCE_PATH,
        )
        flat_reload_log = reload_log.replace("\r", "").replace("\n", "")
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: applied external file-in", reload_log)
        self.assertIn("RecorzKernelPackage: 'Tools'", flat_reload_log)
        self.assertIn("Shared utilities", flat_reload_log)
        self.assertIn("ROUNDTRIP", flat_reload_log)
        self.assertIn("TOOLS", flat_reload_log)
        self.assertNotIn("panic:", reload_log)

    def test_snapshot_can_reopen_workspace_package_list_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_PACKAGE_LIST_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_PACKAGE_LIST_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_PACKAGE_LIST_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_PACKAGE_LIST_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_PACKAGE_LIST_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_PACKAGE_LIST_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 520, 56)
        line_2 = _region_histogram(data, width, 24, 58, 520, 90)
        line_3 = _region_histogram(data, width, 24, 92, 520, 124)
        line_4 = _region_histogram(data, width, 24, 126, 520, 158)

        self.assertGreater(line_1[TEXT_FOREGROUND], 700)
        self.assertGreater(line_2[TEXT_FOREGROUND], 700)
        self.assertGreater(line_3[TEXT_FOREGROUND], 300)
        self.assertGreater(line_4[TEXT_FOREGROUND], 300)

    def test_snapshot_can_reopen_workspace_package_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_PACKAGE_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_PACKAGE_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_PACKAGE_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_PACKAGE_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_PACKAGE_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_PACKAGE_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 520, 56)
        line_2 = _region_histogram(data, width, 24, 58, 520, 90)
        line_3 = _region_histogram(data, width, 24, 92, 520, 124)
        line_4 = _region_histogram(data, width, 24, 126, 520, 158)

        self.assertGreater(line_1[TEXT_FOREGROUND], 700)
        self.assertGreater(line_2[TEXT_FOREGROUND], 500)
        self.assertGreater(line_3[TEXT_FOREGROUND], 300)
        self.assertGreater(line_4[TEXT_FOREGROUND], 300)

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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
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
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
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


if __name__ == "__main__":
    unittest.main()
