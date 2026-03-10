from __future__ import annotations

import json
import re
import shutil
import subprocess
import sys
import unittest
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
TEXT_FOREGROUND = (31, 41, 51)
SNAPSHOT_BEGIN_RE = re.compile(r"recorz-snapshot-begin (\d+)")
SNAPSHOT_DATA_RE = re.compile(r"^recorz-snapshot-data ([0-9a-f]+)$", re.MULTILINE)
SNAPSHOT_END_RE = re.compile(r"^recorz-snapshot-end$", re.MULTILINE)


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
SNAPSHOT_KERNEL_CLASS_FILE_OUT_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-kernel-class-file-out-test"
SNAPSHOT_KERNEL_CLASS_FILE_OUT_COLD_FILE_IN_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-kernel-class-file-out-cold-file-in-test"
SNAPSHOT_KERNEL_CLASS_FILE_OUT_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "kernel-class-file-out-live-image.bin"
SNAPSHOT_KERNEL_CLASS_FILE_OUT_EXTRACTED_SOURCE_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "kernel-class-file-out-current-source.rz"
SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-save-and-reopen-test"
SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-save-and-reopen-reload-test"
SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-save-and-reopen-live-image.bin"
SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-save-and-rerun-test"
SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-save-and-rerun-reload-test"
SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-save-and-rerun-live-image.bin"
SNAPSHOT_WORKSPACE_RECOVERY_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-recovery-test"
SNAPSHOT_WORKSPACE_RECOVERY_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-recovery-reload-test"
SNAPSHOT_WORKSPACE_RECOVERY_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-recovery-live-image.bin"
SNAPSHOT_WORKSPACE_INPUT_MONITOR_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-input-monitor-snapshot-test"
SNAPSHOT_WORKSPACE_INPUT_MONITOR_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-input-monitor-snapshot-reload-test"
SNAPSHOT_WORKSPACE_INPUT_MONITOR_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-input-monitor-live-image.bin"
SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-regenerated-boot-source-test"
SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-regenerated-boot-source-reload-test"
SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-regenerated-boot-source-live-image.bin"

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
SNAPSHOT_KERNEL_CLASS_FILE_OUT_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_kernel_class_file_out_snapshot_save_demo.rz"
SNAPSHOT_KERNEL_CLASS_FILE_OUT_COLD_FILE_IN_DEMO_PATH = ROOT / "examples" / "qemu_riscv_kernel_class_file_out_cold_file_in_demo.rz"
SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_save_and_reopen_demo.rz"
SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_save_and_rerun_demo.rz"
SNAPSHOT_WORKSPACE_SAVE_RECOVERY_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_save_recovery_snapshot_demo.rz"
SNAPSHOT_WORKSPACE_INPUT_MONITOR_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_save_and_reopen_demo.rz"
SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_regenerated_boot_source_save_demo.rz"


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 snapshot integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32SnapshotIntegrationTests(unittest.TestCase):
    def build_elf(self, *, build_dir: Path, example_path: Path) -> Path:
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            "clean",
            "all",
        ]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(f"QEMU RV32 build failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        return build_dir / "recorz-qemu-riscv32-mvp.elf"

    def extract_snapshot_bytes(self, log_text: str) -> bytes | None:
        normalized_log = log_text.replace("\r\n", "\n").replace("\r", "\n")
        begin_match = SNAPSHOT_BEGIN_RE.search(normalized_log)
        end_match = SNAPSHOT_END_RE.search(normalized_log)

        if begin_match is None or end_match is None or end_match.start() < begin_match.end():
            return None
        expected_size = int(begin_match.group(1))
        payload_region = normalized_log[begin_match.end() : end_match.start()]
        payload_hex = "".join(match.group(1) for match in SNAPSHOT_DATA_RE.finditer(payload_region))
        snapshot = bytes.fromhex(payload_hex)
        if len(snapshot) != expected_size:
            self.fail(
                f"snapshot size mismatch: expected {expected_size} bytes, extracted {len(snapshot)} bytes"
            )
        return snapshot

    def save_snapshot_from_interactive_editor(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        snapshot_output: Path,
        serial_input: str,
    ) -> str:
        elf_path = self.build_elf(build_dir=build_dir, example_path=example_path)
        process = subprocess.Popen(
            [
                "qemu-system-riscv32",
                "-machine",
                "virt",
                "-m",
                "32M",
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
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        try:
            try:
                output, _ = process.communicate(input=serial_input, timeout=20.0)
            except subprocess.TimeoutExpired:
                process.kill()
                output, _ = process.communicate(timeout=5.0)
        finally:
            if process.stdout is not None:
                process.stdout.close()
            if process.stdin is not None:
                process.stdin.close()

        snapshot = self.extract_snapshot_bytes(output)
        if snapshot is None:
            self.fail(f"interactive editor run did not emit a snapshot\nstdout:\n{output}")
        snapshot_output.parent.mkdir(parents=True, exist_ok=True)
        snapshot_output.write_bytes(snapshot)
        (build_dir / "qemu.log").write_text(output, encoding="utf-8")
        return output

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

    def inspect_workspace_snapshot(self, *, snapshot_path: Path) -> dict[str, object]:
        result = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools" / "inspect_qemu_riscv_snapshot.py"),
                str(snapshot_path),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            self.fail(
                "snapshot inspection failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        inspected = json.loads(result.stdout)
        workspace = inspected.get("workspace")
        if not isinstance(workspace, dict):
            self.fail(f"snapshot is missing the workspace summary\nstdout:\n{result.stdout}")
        return workspace

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

    def test_snapshot_can_reopen_workspace_browser_state_via_workspace_save_and_reopen(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_SAVE_AND_REOPEN_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

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

    def test_snapshot_can_reopen_workspace_input_monitor_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot_from_interactive_editor(
            build_dir=SNAPSHOT_WORKSPACE_INPUT_MONITOR_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_INPUT_MONITOR_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_INPUT_MONITOR_OUTPUT_PATH,
            serial_input=("\x0e" * 6) + ("\x06" * 4) + "\x04",
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_INPUT_MONITOR_OUTPUT_PATH.exists())

        workspace_summary = self.inspect_workspace_snapshot(
            snapshot_path=SNAPSHOT_WORKSPACE_INPUT_MONITOR_OUTPUT_PATH,
        )
        self.assertEqual(workspace_summary["current_view_kind"], 18)
        self.assertRegex(
            str(workspace_summary["current_target_name"]),
            r"CURSOR:[1-9][0-9]*;TOP:0;VIEW:5;TARGET:Display>>newline",
        )

        extracted_source = self.extract_workspace_current_source(
            snapshot_path=SNAPSHOT_WORKSPACE_INPUT_MONITOR_OUTPUT_PATH,
            output_path=ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-input-monitor-current-source.rz",
        )
        self.assertIn("twentieth := 'TWENTY'.", extracted_source)
        self.assertIn("Transcript show: twentieth.", extracted_source)

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_INPUT_MONITOR_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_INPUT_MONITOR_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)
        self.assertIn("VIEW: INPUT", reload_log)
        self.assertRegex(reload_log, r"LINE: [2-9]")
        self.assertRegex(reload_log, r"COL: [2-9]")
        self.assertIn("TOP: 1", reload_log)
        self.assertIn("newline    | first second third", reload_log)
        self.assertIn("seventh := 'SEVEN'.", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 420, 56)
        line_2 = _region_histogram(data, width, 24, 58, 420, 90)
        source_region = _region_histogram(data, width, 24, 126, 980, 700)

        self.assertGreater(line_1[TEXT_FOREGROUND], 240)
        self.assertGreater(line_2[TEXT_FOREGROUND], 160)
        self.assertGreater(source_region[TEXT_FOREGROUND], 2000)

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

    def test_snapshot_can_reopen_workspace_regenerated_boot_source_browser_state_without_demo_specific_program(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 360, 56)
        line_2 = _region_histogram(data, width, 24, 58, 360, 90)
        source_region = _region_histogram(data, width, 24, 92, 900, 260)

        self.assertGreater(line_1[TEXT_FOREGROUND], 500)
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

    def test_snapshot_extracted_seeded_kernel_class_source_cold_files_into_a_fresh_image(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_KERNEL_CLASS_FILE_OUT_BUILD_DIR,
            example_path=SNAPSHOT_KERNEL_CLASS_FILE_OUT_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_KERNEL_CLASS_FILE_OUT_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_KERNEL_CLASS_FILE_OUT_OUTPUT_PATH.exists())

        extracted_source = self.extract_workspace_current_source(
            snapshot_path=SNAPSHOT_KERNEL_CLASS_FILE_OUT_OUTPUT_PATH,
            output_path=SNAPSHOT_KERNEL_CLASS_FILE_OUT_EXTRACTED_SOURCE_PATH,
        )
        self.assertIn("RecorzKernelClass: #Workspace", extracted_source)
        self.assertIn(
            "instanceVariableNames: 'currentViewKind currentTargetName currentSource lastSource'",
            extracted_source,
        )
        self.assertIn("contents", extracted_source)
        self.assertIn("<primitive: #workspaceContents>", extracted_source)

        reload_log, width, height, _data = self.render_cold_file_in_demo(
            build_dir=SNAPSHOT_KERNEL_CLASS_FILE_OUT_COLD_FILE_IN_BUILD_DIR,
            example_path=SNAPSHOT_KERNEL_CLASS_FILE_OUT_COLD_FILE_IN_DEMO_PATH,
            file_in_payload=SNAPSHOT_KERNEL_CLASS_FILE_OUT_EXTRACTED_SOURCE_PATH,
        )
        flat_reload_log = reload_log.replace("\r", "").replace("\n", "")
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: applied external file-in", reload_log)
        self.assertIn("RecorzKernelClass: #Workspace", flat_reload_log)
        self.assertIn("currentViewKind currentTargetName currentSource lastSource", flat_reload_log)
        self.assertIn("ROUNDTRIP", flat_reload_log)
        self.assertIn("KERNEL", flat_reload_log)
        self.assertNotIn("panic:", reload_log)

    def test_workspace_save_and_rerun_replays_last_source_after_snapshot_resume(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_OUTPUT_PATH.exists())

        reload_log, width, height, _data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_SAVE_AND_RERUN_OUTPUT_PATH,
        )
        flat_reload_log = reload_log.replace("\r", "").replace("\n", "")
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertIn("RERUN", flat_reload_log)
        self.assertNotIn("panic:", reload_log)

    def test_workspace_can_save_a_recovery_snapshot_without_a_startup_hook(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_WORKSPACE_RECOVERY_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_SAVE_RECOVERY_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_RECOVERY_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_RECOVERY_OUTPUT_PATH.exists())

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_RECOVERY_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_RECOVERY_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv32 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)

        line_1 = _region_histogram(data, width, 24, 24, 260, 56)
        line_2 = _region_histogram(data, width, 24, 58, 260, 90)
        self.assertGreater(line_1[TEXT_FOREGROUND], 300)
        self.assertLess(line_2[TEXT_FOREGROUND], 120)

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
