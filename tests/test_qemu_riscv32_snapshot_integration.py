from __future__ import annotations

import json
import re
import select
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
TEXT_FOREGROUND = (31, 41, 51)
SNAPSHOT_BEGIN_RE = re.compile(r"recorz-snapshot-begin (\d+)")
SNAPSHOT_DATA_RE = re.compile(r"^recorz-snapshot-data ([0-9a-f]+)$", re.MULTILINE)
SNAPSHOT_END_RE = re.compile(r"^recorz-snapshot-end$", re.MULTILINE)
SNAPSHOT_SERIAL_TIMEOUT = 45.0


def _read_until(process: subprocess.Popen[str], marker: str, *, timeout: float) -> str:
    if process.stdout is None:
        raise AssertionError("QEMU process stdout is not available")

    output = ""
    deadline = time.monotonic() + timeout
    while marker not in output:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise AssertionError(f"timed out waiting for {marker!r}\ncurrent output:\n{output}")
        ready, _, _ = select.select([process.stdout], [], [], remaining)
        if not ready:
            continue
        chunk = process.stdout.read(1)
        if chunk == "":
            break
        output += chunk
    if marker not in output:
        raise AssertionError(f"did not observe {marker!r}\ncurrent output:\n{output}")
    return output


def _read_until_any(process: subprocess.Popen[str], markers: tuple[str, ...], *, timeout: float) -> tuple[str, str]:
    if process.stdout is None:
        raise AssertionError("QEMU process stdout is not available")

    output = ""
    deadline = time.monotonic() + timeout
    while True:
        for marker in markers:
            if marker in output:
                return output, marker
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise AssertionError(
                "timed out waiting for one of "
                f"{markers!r}\ncurrent output:\n{output}"
            )
        ready, _, _ = select.select([process.stdout], [], [], remaining)
        if not ready:
            continue
        chunk = process.stdout.read(1)
        if chunk == "":
            break
        output += chunk
    for marker in markers:
        if marker in output:
            return output, marker
    raise AssertionError(f"did not observe any of {markers!r}\ncurrent output:\n{output}")


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
SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_BUILD_DIR = ROOT / "misc" / "q32-ws-input-feedback-save"
SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_RELOAD_BUILD_DIR = ROOT / "misc" / "q32-ws-input-feedback-reload"
SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-input-monitor-feedback-live-image.bin"
SNAPSHOT_INTERACTIVE_DEV_LOOP_BUILD_DIR = ROOT / "misc" / "q32-devloop-save"
SNAPSHOT_INTERACTIVE_DEV_LOOP_RELOAD_BUILD_DIR = ROOT / "misc" / "q32-devloop-reload"
SNAPSHOT_INTERACTIVE_DEV_LOOP_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "interactive-dev-loop-live-image.bin"
SNAPSHOT_PACKAGE_DEV_LOOP_BUILD_DIR = ROOT / "misc" / "q32-package-devloop-save"
SNAPSHOT_PACKAGE_DEV_LOOP_RELOAD_BUILD_DIR = ROOT / "misc" / "q32-package-devloop-reload"
SNAPSHOT_PACKAGE_DEV_LOOP_CONTINUE_BUILD_DIR = ROOT / "misc" / "q32-package-devloop-continue"
SNAPSHOT_PACKAGE_DEV_LOOP_SECOND_RELOAD_BUILD_DIR = ROOT / "misc" / "q32-package-devloop-second-reload"
SNAPSHOT_PACKAGE_DEV_LOOP_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "package-dev-loop-live-image.bin"
SNAPSHOT_PACKAGE_DEV_LOOP_SECOND_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "package-dev-loop-second-live-image.bin"
SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-regenerated-boot-source-test"
SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-workspace-regenerated-boot-source-reload-test"
SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "workspace-regenerated-boot-source-live-image.bin"
SNAPSHOT_DISPLAY_CURSOR_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-display-cursor-snapshot-test"
SNAPSHOT_DISPLAY_CURSOR_RELOAD_BUILD_DIR = ROOT / "misc" / "qemu-riscv32-display-cursor-snapshot-reload-test"
SNAPSHOT_DISPLAY_CURSOR_OUTPUT_PATH = ROOT / "misc" / "qemu-riscv32-snapshots" / "display-cursor-live-image.bin"

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
SNAPSHOT_WORKSPACE_INPUT_MONITOR_EVALUATE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_evaluate_demo.rz"
SNAPSHOT_INTERACTIVE_DEV_LOOP_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_edit_current_package_demo.rz"
SNAPSHOT_PACKAGE_DEV_LOOP_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_package_dev_loop_demo.rz"
SNAPSHOT_WORKSPACE_REGENERATED_BOOT_SOURCE_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_workspace_regenerated_boot_source_save_demo.rz"
SNAPSHOT_DEVELOPMENT_HOME_BOOT_DEMO_PATH = ROOT / "examples" / "qemu_riscv_image_development_home_boot.rz"
SNAPSHOT_DISPLAY_CURSOR_SAVE_DEMO_PATH = ROOT / "examples" / "qemu_riscv_display_cursor_snapshot_save_demo.rz"
SNAPSHOT_DISPLAY_CURSOR_RELOAD_DEMO_PATH = ROOT / "examples" / "qemu_riscv_display_cursor_snapshot_reload_demo.rz"


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
                "-monitor",
                "none",
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
                output, _ = _read_until_any(
                    process,
                    (
                        "X ACCEPT  Y REVERT",
                        "T TEST  W SAVE",
                    ),
                    timeout=8.0,
                )
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write(serial_input)
                process.stdin.flush()
                process.stdin.close()
                process.stdin = None
                remaining_output, _ = process.communicate(timeout=SNAPSHOT_SERIAL_TIMEOUT)
                output += remaining_output
            except subprocess.TimeoutExpired:
                process.kill()
                remaining_output, _ = process.communicate(timeout=5.0)
                output += remaining_output
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

    def run_interactive_session(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        serial_input: str,
        ready_marker: str,
        snapshot_payload: Path | None = None,
    ) -> str:
        qemu_command = [
            "qemu-system-riscv32",
            "-machine",
            "virt",
            "-m",
            "32M",
            "-smp",
            "1",
        ]
        elf_path = self.build_elf(build_dir=build_dir, example_path=example_path)
        if snapshot_payload is not None:
            qemu_command.extend(
                [
                    "-fw_cfg",
                    f"name=opt/recorz-snapshot,file={snapshot_payload}",
                ]
            )
        qemu_command.extend(
            [
                "-kernel",
                str(elf_path),
                "-serial",
                "stdio",
                "-monitor",
                "none",
                "-display",
                "none",
                "-device",
                "ramfb",
            ]
        )
        process = subprocess.Popen(
            qemu_command,
            cwd=ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        try:
            try:
                output = _read_until(process, ready_marker, timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write(serial_input)
                process.stdin.flush()
                process.stdin.close()
                process.stdin = None
                remaining_output, _ = process.communicate(timeout=SNAPSHOT_SERIAL_TIMEOUT)
                output += remaining_output
            except subprocess.TimeoutExpired:
                process.kill()
                remaining_output, _ = process.communicate(timeout=5.0)
                output += remaining_output
        finally:
            if process.stdout is not None:
                process.stdout.close()
            if process.stdin is not None:
                process.stdin.close()
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

    def inspect_snapshot_summary(self, *, snapshot_path: Path) -> dict[str, object]:
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
        if not isinstance(inspected, dict):
            self.fail(f"snapshot inspection did not return a JSON object\nstdout:\n{result.stdout}")
        return inspected

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

    def test_snapshot_preserves_active_display_form_and_cursor_state(self) -> None:
        save_log = self.save_snapshot(
            build_dir=SNAPSHOT_DISPLAY_CURSOR_BUILD_DIR,
            example_path=SNAPSHOT_DISPLAY_CURSOR_SAVE_DEMO_PATH,
            snapshot_output=SNAPSHOT_DISPLAY_CURSOR_OUTPUT_PATH,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_DISPLAY_CURSOR_OUTPUT_PATH.exists())

        snapshot_summary = self.inspect_snapshot_summary(snapshot_path=SNAPSHOT_DISPLAY_CURSOR_OUTPUT_PATH)
        header = snapshot_summary.get("header")
        self.assertIsInstance(header, dict)
        assert isinstance(header, dict)
        self.assertEqual(header["active_cursor_visible"], 1)
        self.assertEqual(header["active_cursor_x"], 12)
        self.assertEqual(header["active_cursor_y"], 34)
        self.assertNotEqual(header["active_display_form_handle"], 11)
        self.assertNotEqual(header["active_cursor_handle"], 0)

        reload_log, width, height, _data = self.render_demo(
            build_dir=SNAPSHOT_DISPLAY_CURSOR_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_DISPLAY_CURSOR_RELOAD_DEMO_PATH,
            snapshot_payload=SNAPSHOT_DISPLAY_CURSOR_OUTPUT_PATH,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("DISPLAY RESTORED", reload_log)
        self.assertIn("CURSOR RESTORED", reload_log)
        self.assertIn("CURSOR VISIBLE", reload_log)
        self.assertIn("CURSOR X OK", reload_log)
        self.assertIn("CURSOR Y OK", reload_log)

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
            serial_input="\x0e\x06\x17",
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertIn("recorz qemu-riscv32 mvp: snapshot saved, shutting down", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_INPUT_MONITOR_OUTPUT_PATH.exists())

        snapshot_summary = self.inspect_snapshot_summary(
            snapshot_path=SNAPSHOT_WORKSPACE_INPUT_MONITOR_OUTPUT_PATH,
        )
        workspace_summary = snapshot_summary.get("workspace")
        self.assertIsInstance(workspace_summary, dict)
        assert isinstance(workspace_summary, dict)
        self.assertEqual(workspace_summary["current_view_kind"], 5)
        self.assertEqual(workspace_summary["current_target_name"], "Display>>newline")
        self.assertIsNone(workspace_summary["input_monitor_state"])
        self.assertEqual(snapshot_summary["workspace_tool"]["workspace_name"], "Workspace")
        self.assertEqual(snapshot_summary["workspace_tool"]["visible_left_column"], 0)
        self.assertIsNone(snapshot_summary["workspace_tool"]["status_text"])
        self.assertIsNone(snapshot_summary["workspace_tool"]["feedback_text"])
        self.assertTrue(snapshot_summary["workspace_session"]["workspace_matches_global"])
        self.assertEqual(snapshot_summary["workspace_session"]["workspace_handle"], workspace_summary["handle"])
        self.assertEqual(snapshot_summary["workspace_cursor"]["index"], 0)
        self.assertEqual(snapshot_summary["workspace_cursor"]["top_line"], 0)
        self.assertEqual(snapshot_summary["workspace_selection"]["start_line"], 0)
        self.assertEqual(snapshot_summary["workspace_selection"]["end_line"], 0)

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
        self.assertNotIn("panic:", reload_log)
        self.assertIn("Display>>newline", reload_log)
        self.assertIn("SOURCE EDITOR :: MODIFIED", reload_log)
        self.assertIn("| first second third fourth fifth six", reload_log)
        self.assertIn("fifth := 'FIVE'.", reload_log)

        source_region = _region_histogram(data, width, 24, 126, 980, 700)

        self.assertGreater(source_region[TEXT_FOREGROUND], 12000)

    def test_snapshot_preserves_plain_workspace_buffer_cursor_selection_and_viewport_state(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-plain-session-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = ROOT / "misc" / "q32-plain-ws-save"
            reload_build_dir = ROOT / "misc" / "q32-plain-ws-reload"
            snapshot_output = ROOT / "misc" / "qemu-riscv32-snapshots" / "plain-workspace-live-image.bin"
            example_path = temp_path / "plain_workspace_snapshot_demo.rz"
            buffer_lines = [f"line{index:02d}" for index in range(1, 13)]
            buffer_lines.append("ABCDEFGHIJKLMNOPQRSTUVWXYZ" * 4)
            buffer_text = "\n".join(buffer_lines)
            escaped_buffer_text = buffer_text.replace("'", "''")
            cursor_line = len(buffer_lines) - 1
            cursor_column = len(buffer_lines[-1])
            top_line = 5
            left_column = 24
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        f"Workspace setContents: '{escaped_buffer_text}'.",
                        f"Workspace cursor moveToIndex: {len(buffer_text)} line: {cursor_line} column: {cursor_column} topLine: {top_line}.",
                        f"Workspace selection collapseToLine: {cursor_line} column: {cursor_column}.",
                        f"Workspace setVisibleOriginTop: {top_line} left: {left_column}.",
                        "Workspace interactiveInputMonitor.",
                    ]
                ),
                encoding="utf-8",
            )

            save_log = self.save_snapshot_from_interactive_editor(
                build_dir=build_dir,
                example_path=example_path,
                snapshot_output=snapshot_output,
                serial_input="\x17",
            )
            self.assertIn("recorz-snapshot-begin", save_log)
            self.assertTrue(snapshot_output.exists())

            snapshot_summary = self.inspect_snapshot_summary(snapshot_path=snapshot_output)
            workspace_summary = snapshot_summary.get("workspace")
            workspace_tool = snapshot_summary.get("workspace_tool")
            workspace_session = snapshot_summary.get("workspace_session")
            workspace_cursor = snapshot_summary.get("workspace_cursor")
            workspace_selection = snapshot_summary.get("workspace_selection")
            self.assertIsInstance(workspace_summary, dict)
            self.assertIsInstance(workspace_tool, dict)
            self.assertIsInstance(workspace_session, dict)
            self.assertIsInstance(workspace_cursor, dict)
            self.assertIsInstance(workspace_selection, dict)
            assert isinstance(workspace_summary, dict)
            assert isinstance(workspace_tool, dict)
            assert isinstance(workspace_session, dict)
            assert isinstance(workspace_cursor, dict)
            assert isinstance(workspace_selection, dict)

            self.assertEqual(workspace_summary["current_view_kind"], 18)
            self.assertIsNone(workspace_summary["current_target_name"])
            self.assertIn("line12", str(workspace_summary["current_source"]))
            self.assertIn("ABCDEFGHIJKLMNOPQRSTUVWXYZ", str(workspace_summary["current_source"]))
            self.assertIsNone(workspace_summary["input_monitor_state"])

            self.assertEqual(workspace_tool["workspace_name"], "Workspace")
            self.assertGreaterEqual(workspace_tool["visible_left_column"], left_column)
            self.assertLess(workspace_tool["visible_left_column"], cursor_column)

            self.assertEqual(workspace_session["workspace_handle"], workspace_summary["handle"])
            self.assertTrue(workspace_session["workspace_matches_global"])
            self.assertEqual(workspace_session["escape"], 0)

            self.assertEqual(workspace_cursor["line"], cursor_line)
            self.assertEqual(workspace_cursor["column"], cursor_column)
            self.assertEqual(workspace_cursor["top_line"], top_line)
            self.assertEqual(workspace_selection["start_line"], cursor_line)
            self.assertEqual(workspace_selection["start_column"], cursor_column)
            self.assertEqual(workspace_selection["end_line"], cursor_line)
            self.assertEqual(workspace_selection["end_column"], cursor_column)

            reload_log, width, height, data = self.render_demo(
                build_dir=reload_build_dir,
                example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
                snapshot_payload=snapshot_output,
            )

            self.assertEqual((width, height), (1024, 768))
            self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
            self.assertNotIn("panic:", reload_log)
            self.assertGreater(
                _region_histogram(data, width, 24, 126, 980, 700)[TEXT_FOREGROUND],
                1800,
            )

    def test_snapshot_preserves_input_monitor_status_and_feedback_tail(self) -> None:
        save_log = self.save_snapshot_from_interactive_editor(
            build_dir=SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_INPUT_MONITOR_EVALUATE_DEMO_PATH,
            snapshot_output=SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_OUTPUT_PATH,
            serial_input="1 + 2\x10\x17",
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertTrue(SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_OUTPUT_PATH.exists())

        snapshot_summary = self.inspect_snapshot_summary(
            snapshot_path=SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_OUTPUT_PATH,
        )
        workspace_summary = snapshot_summary.get("workspace")
        self.assertIsInstance(workspace_summary, dict)
        assert isinstance(workspace_summary, dict)
        self.assertEqual(workspace_summary["current_view_kind"], 18)
        self.assertIsNone(workspace_summary["current_target_name"])
        self.assertEqual(workspace_summary["current_source"], "1 + 2")
        self.assertEqual(workspace_summary["last_source"], "RecorzKernelDoIt:\n1 + 2")
        self.assertIsNone(workspace_summary["input_monitor_state"])
        self.assertEqual(snapshot_summary["workspace_tool"]["status_text"], "PRINT COMPLETE")
        self.assertEqual(snapshot_summary["workspace_tool"]["feedback_text"], "3")
        self.assertTrue(snapshot_summary["workspace_session"]["workspace_matches_global"])
        self.assertEqual(snapshot_summary["workspace_cursor"]["index"], 5)
        self.assertEqual(snapshot_summary["workspace_cursor"]["column"], 5)
        self.assertEqual(snapshot_summary["workspace_selection"]["start_column"], 5)
        self.assertEqual(snapshot_summary["workspace_selection"]["end_column"], 5)

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_WORKSPACE_INPUT_MONITOR_FEEDBACK_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertNotIn("panic:", reload_log)
        self.assertIn("PRINT COMPLETE", reload_log)
        self.assertIn("1 + 2", reload_log)

        feedback_region = _region_histogram(data, width, 24, 652, 980, 744)
        self.assertGreater(feedback_region[TEXT_FOREGROUND], 250)

    def test_snapshot_proves_the_interactive_package_development_loop(self) -> None:
        elf_path = self.build_elf(
            build_dir=SNAPSHOT_INTERACTIVE_DEV_LOOP_BUILD_DIR,
            example_path=SNAPSHOT_INTERACTIVE_DEV_LOOP_DEMO_PATH,
        )
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
                "-monitor",
                "none",
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
            output = _read_until(process, "VIEW: INPUT", timeout=8.0)
            if process.stdin is None:
                self.fail("QEMU process stdin is not available")
            process.stdin.write("\x18")
            process.stdin.flush()
            output += _read_until(process, "STATUS: INSTALL COMPLETE", timeout=8.0)
            process.stdin.write("\x14")
            process.stdin.flush()
            output += _read_until(process, "SUMMARY Tests P=1 F=1 T=2", timeout=8.0)
            process.stdin.write("\x17")
            process.stdin.flush()
            process.stdin.close()
            process.stdin = None
            try:
                remaining_output, _ = process.communicate(timeout=SNAPSHOT_SERIAL_TIMEOUT)
            except subprocess.TimeoutExpired:
                process.kill()
                remaining_output, _ = process.communicate(timeout=5.0)
            output += remaining_output
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5.0)
            if process.stdout is not None:
                process.stdout.close()
            if process.stdin is not None:
                process.stdin.close()

        snapshot = self.extract_snapshot_bytes(output)
        if snapshot is None:
            self.fail(f"interactive development loop did not emit a snapshot\nstdout:\n{output}")
        SNAPSHOT_INTERACTIVE_DEV_LOOP_OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
        SNAPSHOT_INTERACTIVE_DEV_LOOP_OUTPUT_PATH.write_bytes(snapshot)
        (SNAPSHOT_INTERACTIVE_DEV_LOOP_BUILD_DIR / "qemu.log").write_text(output, encoding="utf-8")

        self.assertIn("STATUS: INSTALL COMPLETE", output)
        self.assertIn("SUMMARY Tests P=1 F=1 T=2", output)
        self.assertIn("recorz qemu-riscv32 mvp: snapshot saved, shutting down", output)
        self.assertTrue(SNAPSHOT_INTERACTIVE_DEV_LOOP_OUTPUT_PATH.exists())

        workspace_summary = self.inspect_workspace_snapshot(
            snapshot_path=SNAPSHOT_INTERACTIVE_DEV_LOOP_OUTPUT_PATH,
        )
        self.assertEqual(workspace_summary["current_view_kind"], 18)
        input_monitor_state = workspace_summary.get("input_monitor_state")
        self.assertIsInstance(input_monitor_state, dict)
        assert isinstance(input_monitor_state, dict)
        self.assertEqual(input_monitor_state["saved_target_name"], "Tests")
        self.assertEqual(input_monitor_state["status"], "TESTS COMPLETE")
        self.assertIn("RecorzKernelPackage: 'Tests'", str(workspace_summary["current_source"]))

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_INTERACTIVE_DEV_LOOP_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_INTERACTIVE_DEV_LOOP_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("VIEW: INPUT", reload_log)
        self.assertIn("STATUS: TESTS COMPLETE", reload_log)
        self.assertIn("TinySpec", reload_log)
        self.assertNotIn("panic:", reload_log)

        source_region = _region_histogram(data, width, 24, 126, 980, 700)
        self.assertGreater(source_region[TEXT_FOREGROUND], 1800)

    def test_snapshot_proves_the_package_home_dev_loop_can_continue_repeatedly(self) -> None:
        first_output = self.run_interactive_session(
            build_dir=SNAPSHOT_PACKAGE_DEV_LOOP_BUILD_DIR,
            example_path=SNAPSHOT_PACKAGE_DEV_LOOP_DEMO_PATH,
            ready_marker="VIEW: PACKAGES",
            serial_input="\x18\x14\x17",
        )

        first_snapshot = self.extract_snapshot_bytes(first_output)
        if first_snapshot is None:
            self.fail(f"package home dev loop did not emit an initial snapshot\nstdout:\n{first_output}")
        SNAPSHOT_PACKAGE_DEV_LOOP_OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
        SNAPSHOT_PACKAGE_DEV_LOOP_OUTPUT_PATH.write_bytes(first_snapshot)

        self.assertIn("VIEW: PACKAGES", first_output)
        self.assertIn("VIEW: INPUT", first_output)
        self.assertIn("SUMMARY Tests P=1 F=1 T=2", first_output)
        self.assertIn("TinySpec", first_output)
        self.assertIn("recorz qemu-riscv32 mvp: snapshot saved, shutting down", first_output)

        first_workspace_summary = self.inspect_workspace_snapshot(
            snapshot_path=SNAPSHOT_PACKAGE_DEV_LOOP_OUTPUT_PATH,
        )
        self.assertEqual(first_workspace_summary["current_view_kind"], 18)
        first_input_monitor_state = first_workspace_summary.get("input_monitor_state")
        self.assertIsInstance(first_input_monitor_state, dict)
        assert isinstance(first_input_monitor_state, dict)
        self.assertEqual(first_input_monitor_state["saved_target_name"], "Tests")
        self.assertEqual(first_input_monitor_state["status"], "TESTS COMPLETE")

        reload_log, width, height, data = self.render_demo(
            build_dir=SNAPSHOT_PACKAGE_DEV_LOOP_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_PACKAGE_DEV_LOOP_OUTPUT_PATH,
        )

        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
        self.assertIn("VIEW: INPUT", reload_log)
        self.assertIn("STATUS: TESTS COMPLETE", reload_log)
        self.assertIn("TinySpec", reload_log)
        self.assertNotIn("panic:", reload_log)
        self.assertGreater(
            _region_histogram(data, width, 24, 126, 980, 700)[TEXT_FOREGROUND],
            1800,
        )

        second_elf_path = self.build_elf(
            build_dir=SNAPSHOT_PACKAGE_DEV_LOOP_CONTINUE_BUILD_DIR,
            example_path=SNAPSHOT_DEVELOPMENT_HOME_BOOT_DEMO_PATH,
        )
        second_process = subprocess.Popen(
            [
                "qemu-system-riscv32",
                "-machine",
                "virt",
                "-m",
                "32M",
                "-smp",
                "1",
                "-fw_cfg",
                f"name=opt/recorz-snapshot,file={SNAPSHOT_PACKAGE_DEV_LOOP_OUTPUT_PATH}",
                "-kernel",
                str(second_elf_path),
                "-serial",
                "stdio",
                "-monitor",
                "none",
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
            second_output = _read_until(
                second_process,
                "VIEW: INPUT",
                timeout=8.0,
            )
            time.sleep(1.0)
            if second_process.stdin is None:
                self.fail("QEMU process stdin is not available")
            second_process.stdin.write("\x14")
            second_process.stdin.flush()
            second_output += _read_until(
                second_process,
                "SUMMARY Tests P=1 F=1 T=2",
                timeout=8.0,
            )
            second_process.stdin.write("\x17")
            second_process.stdin.flush()
            second_process.stdin.close()
            second_process.stdin = None
            remaining_output, _ = second_process.communicate(timeout=SNAPSHOT_SERIAL_TIMEOUT)
            second_output += remaining_output
        except subprocess.TimeoutExpired:
            second_process.kill()
            remaining_output, _ = second_process.communicate(timeout=5.0)
            second_output += remaining_output
        finally:
            if second_process.poll() is None:
                second_process.kill()
                second_process.wait(timeout=5.0)
            if second_process.stdout is not None:
                second_process.stdout.close()
            if second_process.stdin is not None:
                second_process.stdin.close()

        second_snapshot = self.extract_snapshot_bytes(second_output)
        if second_snapshot is None:
            self.fail(f"package home dev loop did not emit a continuation snapshot\nstdout:\n{second_output}")
        SNAPSHOT_PACKAGE_DEV_LOOP_SECOND_OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
        SNAPSHOT_PACKAGE_DEV_LOOP_SECOND_OUTPUT_PATH.write_bytes(second_snapshot)

        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", second_output)
        self.assertIn("SUMMARY Tests P=1 F=1 T=2", second_output)
        self.assertIn("recorz qemu-riscv32 mvp: snapshot saved, shutting down", second_output)

        second_workspace_summary = self.inspect_workspace_snapshot(
            snapshot_path=SNAPSHOT_PACKAGE_DEV_LOOP_SECOND_OUTPUT_PATH,
        )
        self.assertEqual(second_workspace_summary["current_view_kind"], 18)
        second_input_monitor_state = second_workspace_summary.get("input_monitor_state")
        self.assertIsInstance(second_input_monitor_state, dict)
        assert isinstance(second_input_monitor_state, dict)
        self.assertEqual(second_input_monitor_state["saved_target_name"], "Tests")
        self.assertEqual(second_input_monitor_state["status"], "TESTS COMPLETE")

        second_reload_log, second_width, second_height, second_data = self.render_demo(
            build_dir=SNAPSHOT_PACKAGE_DEV_LOOP_SECOND_RELOAD_BUILD_DIR,
            example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
            snapshot_payload=SNAPSHOT_PACKAGE_DEV_LOOP_SECOND_OUTPUT_PATH,
        )

        self.assertEqual((second_width, second_height), (1024, 768))
        self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", second_reload_log)
        self.assertIn("VIEW: INPUT", second_reload_log)
        self.assertIn("STATUS: TESTS COMPLETE", second_reload_log)
        self.assertIn("TinySpec", second_reload_log)
        self.assertNotIn("panic:", second_reload_log)
        self.assertGreater(
            _region_histogram(second_data, second_width, 24, 126, 980, 700)[TEXT_FOREGROUND],
            1800,
        )

    def test_snapshot_resume_can_return_from_a_source_editor_to_the_same_plain_workspace(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-return-resume-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "save-build"
            reload_build_dir = temp_path / "reload-build"
            first_snapshot_path = temp_path / "workspace-return-resume-first.bin"
            second_snapshot_path = temp_path / "workspace-return-resume-second.bin"
            example_path = temp_path / "workspace_return_resume_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: 'snapshot plain workspace'.",
                        "Workspace cursor moveToIndex: 8 line: 0 column: 8 topLine: 0.",
                        "Workspace selection setStartLine: 0 startColumn: 3 endLine: 0 endColumn: 8.",
                        "Workspace setVisibleOriginTop: 0 left: 5.",
                        "Workspace browseMethod: 'newline' ofClassNamed: 'Display'.",
                        "Workspace interactiveInputMonitor.",
                    ]
                ),
                encoding="utf-8",
            )

            first_output = self.save_snapshot_from_interactive_editor(
                build_dir=build_dir,
                example_path=example_path,
                snapshot_output=first_snapshot_path,
                serial_input="\x17",
            )
            self.assertIn("recorz-snapshot-begin", first_output)
            self.assertTrue(first_snapshot_path.exists())

            second_output = self.run_interactive_session(
                build_dir=reload_build_dir,
                example_path=SNAPSHOT_DEVELOPMENT_HOME_BOOT_DEMO_PATH,
                ready_marker="STATUS: METHOD SOURCE :: SOURCE EDITOR READY",
                snapshot_payload=first_snapshot_path,
                serial_input="\x0f\x17",
            )

            second_snapshot = self.extract_snapshot_bytes(second_output)
            if second_snapshot is None:
                self.fail(
                    "workspace return/resume flow did not emit a continuation snapshot\n"
                    f"stdout:\n{second_output}"
                )
            second_snapshot_path.write_bytes(second_snapshot)

            self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", second_output)
            self.assertIn("recorz qemu-riscv32 mvp: snapshot saved, shutting down", second_output)

            second_summary = self.inspect_snapshot_summary(snapshot_path=second_snapshot_path)
            workspace_summary = second_summary.get("workspace")
            workspace_tool = second_summary.get("workspace_tool")
            workspace_cursor = second_summary.get("workspace_cursor")
            workspace_selection = second_summary.get("workspace_selection")
            self.assertIsInstance(workspace_summary, dict)
            self.assertIsInstance(workspace_tool, dict)
            self.assertIsInstance(workspace_cursor, dict)
            self.assertIsInstance(workspace_selection, dict)
            assert isinstance(workspace_summary, dict)
            assert isinstance(workspace_tool, dict)
            assert isinstance(workspace_cursor, dict)
            assert isinstance(workspace_selection, dict)

            self.assertEqual(workspace_summary["current_view_kind"], 18)
            self.assertIsNone(workspace_summary["current_target_name"])
            self.assertEqual(workspace_summary["current_source"], "snapshot plain workspace")
            self.assertIsNone(workspace_summary["input_monitor_state"])

            self.assertEqual(workspace_tool["visible_left_column"], 5)
            self.assertEqual(workspace_cursor["column"], 8)
            self.assertEqual(workspace_selection["start_column"], 3)
            self.assertEqual(workspace_selection["end_column"], 8)

    def test_recovery_snapshot_after_regenerated_source_return_preserves_plain_workspace_state(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-regen-recovery-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "save-build"
            reload_build_dir = ROOT / "misc" / "q32-regen-recovery-reload"
            snapshot_path = temp_path / "workspace-regen-recovery.bin"
            example_path = temp_path / "workspace_regen_recovery_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: 'regen recovery workspace'.",
                        "Workspace cursor moveToIndex: 9 line: 0 column: 9 topLine: 0.",
                        "Workspace selection setStartLine: 0 startColumn: 2 endLine: 0 endColumn: 9.",
                        "Workspace setVisibleOriginTop: 0 left: 6.",
                        "Workspace interactiveInputMonitor.",
                    ]
                ),
                encoding="utf-8",
            )

            save_log = self.save_snapshot_from_interactive_editor(
                build_dir=build_dir,
                example_path=example_path,
                snapshot_output=snapshot_path,
                serial_input="\x07\x0f\x0b",
            )
            self.assertIn("recorz-snapshot-begin", save_log)
            self.assertTrue(snapshot_path.exists())

            snapshot_summary = self.inspect_snapshot_summary(snapshot_path=snapshot_path)
            workspace_summary = snapshot_summary.get("workspace")
            workspace_tool = snapshot_summary.get("workspace_tool")
            workspace_cursor = snapshot_summary.get("workspace_cursor")
            workspace_selection = snapshot_summary.get("workspace_selection")
            self.assertIsInstance(workspace_summary, dict)
            self.assertIsInstance(workspace_tool, dict)
            self.assertIsInstance(workspace_cursor, dict)
            self.assertIsInstance(workspace_selection, dict)
            assert isinstance(workspace_summary, dict)
            assert isinstance(workspace_tool, dict)
            assert isinstance(workspace_cursor, dict)
            assert isinstance(workspace_selection, dict)

            self.assertEqual(workspace_summary["current_view_kind"], 18)
            self.assertIsNone(workspace_summary["current_target_name"])
            self.assertEqual(workspace_summary["current_source"], "regen recovery workspace")
            self.assertIsNone(workspace_summary["input_monitor_state"])
            self.assertEqual(workspace_tool["visible_left_column"], 6)
            self.assertEqual(workspace_cursor["column"], 9)
            self.assertEqual(workspace_selection["start_column"], 9)
            self.assertEqual(workspace_selection["end_column"], 9)

            reload_log, width, height, data = self.render_demo(
                build_dir=reload_build_dir,
                example_path=SNAPSHOT_WORKSPACE_IDLE_DEMO_PATH,
                snapshot_payload=snapshot_path,
            )

            self.assertEqual((width, height), (1024, 768))
            self.assertIn("recorz qemu-riscv32 mvp: loaded snapshot", reload_log)
            self.assertIn("recovery workspace", reload_log)
            self.assertNotIn("panic:", reload_log)
            self.assertGreater(
                _region_histogram(data, width, 24, 126, 980, 700)[TEXT_FOREGROUND],
                1800,
            )

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
