from __future__ import annotations

import os
import shutil
import select
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
DEFAULT_EXAMPLE = ROOT / "examples" / "qemu_riscv_fb_demo.rz"
MEMORY_REPORT_EXAMPLE = ROOT / "examples" / "qemu_riscv_memory_report_demo.rz"
MULTISTATEMENT_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_multistatement_source_demo.rz"
TEMPS_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_temps_method_demo.rz"
PACKAGE_COMMENT_ROUNDTRIP_EXAMPLE = ROOT / "examples" / "qemu_riscv_package_comment_roundtrip_demo.rz"
BINARY_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_binary_method_demo.rz"
CONDITIONAL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_conditional_method_demo.rz"
TOP_LEVEL_BLOCK_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_top_level_block_demo.rz"
TOP_LEVEL_BLOCK_CAPTURE_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_top_level_block_capture_demo.rz"
TOP_LEVEL_BLOCK_RETURN_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_top_level_block_return_demo.rz"
TOP_LEVEL_SELF_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_top_level_self_demo.rz"
TOP_LEVEL_SOURCE_BUFFER_EXAMPLE = ROOT / "examples" / "qemu_riscv_top_level_source_buffer_demo.rz"
WORKSPACE_BLOCK_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_block_demo.rz"
WORKSPACE_BLOCK_ARGUMENT_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_block_argument_demo.rz"
WORKSPACE_BLOCK_CAPTURE_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_block_capture_demo.rz"
WORKSPACE_BLOCK_RETURN_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_block_return_demo.rz"
WORKSPACE_SELF_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_self_demo.rz"
WORKSPACE_TEMPS_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_temps_demo.rz"
WORKSPACE_ACCEPT_CURRENT_PACKAGE_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_accept_current_package_demo.rz"
WORKSPACE_RECOVER_LAST_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_recover_last_source_demo.rz"
REGENERATED_KERNEL_SOURCE_BROWSER_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_regenerated_kernel_source_browser_demo.rz"
WORKSPACE_INPUT_MONITOR_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_demo.rz"
WORKSPACE_INPUT_MONITOR_EVALUATE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_evaluate_demo.rz"
WORKSPACE_INPUT_MONITOR_ACCEPT_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_accept_demo.rz"
WORKSPACE_INPUT_MONITOR_BROWSER_BRIDGE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_browser_bridge_demo.rz"
WORKSPACE_INPUT_MONITOR_PACKAGE_BRIDGE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_package_bridge_demo.rz"
WORKSPACE_EDIT_METHOD_ENTRY_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_edit_method_entry_demo.rz"
WORKSPACE_EDIT_PACKAGE_ENTRY_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_edit_package_entry_demo.rz"
TEST_RUNNER_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_test_runner_demo.rz"
METHOD_BLOCK_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_method_block_demo.rz"
METHOD_BLOCK_CAPTURE_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_method_block_capture_demo.rz"
METHOD_BLOCK_RETURN_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_method_block_return_demo.rz"
METHOD_BLOCK_ARGUMENT_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_method_block_argument_demo.rz"
THIS_CONTEXT_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_this_context_demo.rz"
COMPILED_CONTEXT_BRIDGE_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_compiled_context_bridge_demo.rz"
PAREN_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_parenthesized_method_demo.rz"
MULTIKEYWORD_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_multikeyword_method_demo.rz"
EXPRESSION_KEYWORD_SEND_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_expression_keyword_send_demo.rz"
NIL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_nil_method_demo.rz"
SELF_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_self_method_demo.rz"
SMALL_INTEGER_LITERAL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_small_integer_literal_demo.rz"
STRING_LITERAL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_string_literal_demo.rz"
BOOLEAN_LITERAL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_boolean_literal_demo.rz"
WORKSPACE_ACCEPT_CURRENT_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_accept_current_demo.rz"
COMPILED_METHOD_CONTEXT_UPDATE_DEMO = ROOT / "examples" / "qemu_riscv_compiled_method_context_update_demo.rz"
COMPILED_METHOD_CONTEXT_UPDATE_SOURCE = ROOT / "examples" / "qemu_riscv_object_detail_sender_context_update.rz"
UPDATE_TOOL = ROOT / "tools" / "build_qemu_riscv_method_update.py"


def _build_elf(build_dir: Path, example_path: Path = DEFAULT_EXAMPLE, *, profile: str = "dev") -> Path:
    result = subprocess.run(
        [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"RV32_PROFILE={profile}",
            f"EXAMPLE={example_path}",
            "clean",
            "all",
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise AssertionError(
            "RV32 QEMU build failed\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return build_dir / "recorz-qemu-riscv32-mvp.elf"


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


def _read_file_until(path: Path, marker: str, *, timeout: float) -> str:
    deadline = time.monotonic() + timeout
    output = ""
    while marker not in output:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise AssertionError(f"timed out waiting for {marker!r}\ncurrent output:\n{output}")
        if path.exists():
            output = path.read_text(encoding="utf-8", errors="ignore")
            if marker in output:
                break
        time.sleep(min(0.1, remaining))
    return output


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 serial integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32SerialIntegrationTests(unittest.TestCase):
    @unittest.skipUnless(
        sys.platform == "darwin" or os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"),
        "QEMU keyboard-input integration test requires a graphical display backend",
    )
    def test_workspace_interactive_input_monitor_accepts_virtio_keyboard_input(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-kbd-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_EXAMPLE)
            log_path = build_dir / "qemu-keyboard.log"
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
                    f"file:{log_path}",
                    "-monitor",
                    "stdio",
                    "-device",
                    "ramfb",
                    "-device",
                    "virtio-keyboard-device",
                ],
                cwd=ROOT,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                _read_file_until(log_path, "VIEW: INPUT", timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("sendkey shift-a\n")
                process.stdin.write("sendkey shift-s\n")
                process.stdin.write("sendkey shift-d\n")
                process.stdin.write("sendkey shift-f\n")
                process.stdin.flush()
                output = _read_file_until(log_path, "COL: 5", timeout=8.0).replace("\r", "")
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            self.assertIn("TOP: 1ASDF", output)
            self.assertNotIn("panic:", output)

    def test_default_demo_boots_and_prints_transcript_over_serial(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-serial-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: booting", output)
            self.assertIn("HELLO FROM RECORZ", output)
            self.assertIn("SIZE: 1024 x 768", output)
            self.assertIn("RAMFB ONLINE.", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_workspace_can_recover_last_evaluated_source_into_the_current_buffer(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-recover-last-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_RECOVER_LAST_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("SAFE", output)
            self.assertIn("Transcript show: 'SAFE'.", output)
            self.assertNotIn("panic:", output)

    def test_workspace_can_browse_regenerated_kernel_source_inside_the_image(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-regenerated-kernel-source-browser-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, REGENERATED_KERNEL_SOURCE_BROWSER_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=10.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("RecorzKernelClass: #Workspace", output)
            self.assertIn("RecorzKernelClass: #Selector", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_supports_cursor_editing(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_EXAMPLE)
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
                process.stdin.write("ab\x02C\x06D\x04")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("VIEW: INPUT", output)
            self.assertIn("MOVE: ARROWS CTRL-B/F/P/N", output)
            self.assertIn("LINE: 1", output)
            self.assertIn("TOP: 1", output)
            self.assertIn("EXIT: CTRL-", output)
            self.assertIn("BUFFER=aCbD", output.replace("\n", ""))
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_supports_vertical_cursor_movement(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-vertical-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_EXAMPLE)
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
                process.stdin.write("A\nBC\nD\x10X\x0eY\x04")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("LINE: 3", output)
            self.assertIn("TOP: 1", output)
            self.assertIn("BUFFER=A\nBXC\nDY", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_scrolls_multiline_buffers(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-scroll-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_EXAMPLE)
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
                process.stdin.write(("A\n" * 40) + "\x04")
                process.stdin.flush()
                time.sleep(1.5)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("LINE: 41", output)
            self.assertRegex(output, r"TOP: ([2-9]|[1-9][0-9]+)")
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_evaluate_current_buffer(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-run-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_EVALUATE_EXAMPLE)
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
                process.stdin.write("Transcript show: (1 + 2) printString.\x12!\x04")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("RUN: CTRL-R", output)
            self.assertIn("STATUS: RUN OK", output)
            self.assertIn("OUT> 3", output)
            self.assertIn(
                "BUFFER=Transcript show: (1 + 2) printString.!",
                output.replace("\n", ""),
            )
            self.assertIn("RERUN=3", output.replace("\n", ""))
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_accept_current_buffer(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-accept-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_ACCEPT_EXAMPLE)
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
                process.stdin.write("\x18\x04")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("ACCEPT: CTRL-X", output)
            self.assertIn("ACC", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_save_and_reopen_from_the_editor(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-save-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_EDIT_METHOD_ENTRY_EXAMPLE)
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
                process.stdin.write("\x17")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("SAVE: CTRL-W", output)
            self.assertIn("recorz-snapshot-begin", output)
            self.assertIn("recorz-snapshot-end", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_return_to_its_browser_context(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-browse-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_BROWSER_BRIDGE_EXAMPLE)
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
                process.stdin.write("\x0f")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("BROWSE: CTRL-O", output)
            self.assertGreaterEqual(output.count("CLASS: DISPLAY"), 2)
            self.assertGreaterEqual(output.count("METHOD: NEWLINE"), 2)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_closes_back_to_method_browser_context(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-close-method-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_BROWSER_BRIDGE_EXAMPLE)
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
                process.stdin.write("\x04")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("DONE: CTRL-D", output)
            self.assertGreaterEqual(output.count("CLASS: DISPLAY"), 2)
            self.assertGreaterEqual(output.count("METHOD: NEWLINE"), 2)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_closes_back_to_package_source_context(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-close-package-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_PACKAGE_BRIDGE_EXAMPLE)
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
                process.stdin.write("\x04")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("DONE: CTRL-D", output)
            self.assertGreaterEqual(output.count("PACKAGE: TOOLS"), 2)
            self.assertGreaterEqual(output.count("VIEW: SOURCE"), 2)
            self.assertNotIn("panic:", output)

    def test_workspace_edit_method_entry_opens_the_interactive_editor_from_a_method_browser(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-edit-method-entry-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_EDIT_METHOD_ENTRY_EXAMPLE)
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
                process.stdin.write("\x04")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("DONE: CTRL-D", output)
            self.assertGreaterEqual(output.count("VIEW: INPUT"), 1)
            self.assertGreaterEqual(output.count("CLASS: DISPLAY"), 1)
            self.assertGreaterEqual(output.count("METHOD: NEWLINE"), 1)
            self.assertNotIn("panic:", output)

    def test_workspace_edit_package_entry_opens_the_interactive_editor_from_package_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-edit-package-entry-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_EDIT_PACKAGE_ENTRY_EXAMPLE)
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
                process.stdin.write("\x04")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += process.stdout.read() or ""
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("DONE: CTRL-D", output)
            self.assertGreaterEqual(output.count("VIEW: INPUT"), 1)
            self.assertGreaterEqual(output.count("PACKAGE: TOOLS"), 1)
            self.assertGreaterEqual(output.count("VIEW: SOURCE"), 1)
            self.assertNotIn("panic:", output)

    def test_memory_report_demo_prints_usage_within_budget(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-memory-report-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, MEMORY_REPORT_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("MEMORY", output)
            self.assertIn("PROFILE DEV", output)

            expected_limits = {
                "HEAP": 768,
                "DCLS": 24,
                "NOBJ": 24,
                "MSRC": 128,
                "MSRP": 32768,
                "RSTR": 8192,
                "SSTR": 16384,
                "SNAP": 65536,
                "MONO": 16,
            }
            for label, expected_limit in expected_limits.items():
                match = re.search(rf"{label} (\d+)/(\d+)", output)
                self.assertIsNotNone(match, f"missing {label} line in output:\n{output}")
                used = int(match.group(1))
                limit = int(match.group(2))
                self.assertEqual(limit, expected_limit, label)
                self.assertLess(used, limit, label)

    def test_runtime_string_pool_compacts_dead_workspace_strings(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-runtime-strings-") as temp_dir:
            temp_path = Path(temp_dir)
            example_path = temp_path / "runtime_string_compaction.rz"
            class_source = (
                "RecorzKernelClass: #ReplayLongRuntimeStringReuse superclass: #Object instanceVariableNames: ''\n!\n"
                "value\n    ^self\n!\n"
                "setValue: extremelyVerboseArgumentNameForSourceInflationPurposes\n    ^self\n!\n"
                "RecorzKernelClassSide: #ReplayLongRuntimeStringReuse\n!\n"
                "detail\n    ^self"
            )
            escaped_class_source = class_source.replace("'", "''")
            repeated_updates = "\n".join(
                "Workspace setContents: (KernelInstaller fileOutClassNamed: 'ReplayLongRuntimeStringReuse')."
                for _ in range(70)
            )
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        f"KernelInstaller fileInClassChunks: '{escaped_class_source}'.",
                        repeated_updates,
                        "Transcript show: Workspace contents.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(temp_path / "build", example_path)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("RecorzKernelClass:", output)
            self.assertIn("ReplayLongRuntime", output)
            self.assertIn("setValue:", output)
            self.assertNotIn("panic: runtime string pool overflow", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_multistatement_methods_and_unary_expression_chains(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-multistatement-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, MULTISTATEMENT_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class Reporter", output)
            self.assertIn("1024", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_temporaries_and_local_assignment(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-temps-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, TEMPS_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class TempReporter", output)
            self.assertIn("TEMP", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_binary_method_expressions(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-binary-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, BINARY_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class BinaryReporter", output)
            self.assertIn("BINARY", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_comparisons_and_conditional_control_flow(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-conditional-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, CONDITIONAL_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class ConditionalReporter", output)
            self.assertIn("COND", output)
            self.assertIn("LT", output)
            self.assertIn("GE", output)
            self.assertIn("EQ", output)
            self.assertIn("IFFALSE", output)
            self.assertIn("GT", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_workspace_supports_zero_argument_block_literals_and_value(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-block-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_BLOCK_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("BLOCK", output)
            self.assertIn("VALUE", output)
            self.assertIn("NEST", output)
            self.assertIn("NIL", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_top_level_programs_support_block_literals_and_boolean_conditionals(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-top-level-block-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, TOP_LEVEL_BLOCK_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("TBLOCK", output)
            self.assertIn("7", output)
            self.assertIn("11", output)
            self.assertIn("TCOND", output)
            self.assertIn("FCOND", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_top_level_blocks_capture_and_mutate_top_level_temporaries(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-top-level-block-capture-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, TOP_LEVEL_BLOCK_CAPTURE_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("TBCAP", output)
            self.assertIn("7", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_top_level_block_non_local_return_exits_the_program_do_it(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-top-level-block-return-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, TOP_LEVEL_BLOCK_RETURN_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("TBRET", output)
            self.assertNotIn("LATE", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_top_level_programs_use_workspace_as_the_receiver(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-top-level-self-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, TOP_LEVEL_SELF_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("TOP SELF", output)
            self.assertIn("SELFWORK", output)
            self.assertIn("CTXWORK", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_top_level_programs_seed_workspace_contents_with_their_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-top-level-source-buffer-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, TOP_LEVEL_SOURCE_BUFFER_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("topLevelSeedMarker := 0.", output)
            self.assertIn("Transcript show: self contents.", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_workspace_supports_single_argument_block_literals_and_value_colon(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-block-argument-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_BLOCK_ARGUMENT_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("7", output)
            self.assertIn("11", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_workspace_blocks_capture_and_mutate_workspace_temporaries(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-block-capture-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_BLOCK_CAPTURE_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("WBCAP", output)
            self.assertIn("7", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_workspace_blocks_support_non_local_return_to_the_enclosing_do_it(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-block-return-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_BLOCK_RETURN_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("WBRET", output)
            self.assertIn("INNER", output)
            self.assertIn("AFTER", output)
            self.assertNotIn("LATE", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_workspace_accept_current_installs_and_keeps_method_browser_context(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-accept-current-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_ACCEPT_CURRENT_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertNotIn("panic:", output)
            self.assertIn("ACC", output)
            self.assertIn("METHOD: NEWLINE", output)
            self.assertIn("NEWLINE", output)

    def test_workspace_evaluation_uses_workspace_as_the_receiver(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-self-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_SELF_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("WSELF", output)
            self.assertIn("WCTX", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_workspace_accept_current_supports_package_source_browser_updates(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-accept-package-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_ACCEPT_CURRENT_PACKAGE_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertNotIn("panic:", output)
            self.assertIn("PACKAGE: TOOLS", output)
            self.assertIn("VIEW: SOURCE", output)
            self.assertIn("COMMENT: UPDATED UTILITIES", output)
            self.assertIn("PACKAGEBETA", output)

    def test_in_image_test_runner_runs_class_and_package_tests(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-test-runner-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, TEST_RUNNER_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertNotIn("panic:", output)
            self.assertIn("CLASS", output)
            self.assertIn("PASS TinySpec>>testPass", output)
            self.assertIn("FAIL TinySpec>>testFail", output)
            self.assertIn("SUMMARY TinySpec P=1 F=1 T=2", output)
            self.assertIn("COUNTS 1 1 2", output)
            self.assertIn("LAST TinySpec>>testFail", output)
            self.assertIn("PACKAGE", output)
            self.assertIn("PASS SecondSpec>>testPass", output)
            self.assertIn("SUMMARY Tests P=2 F=1 T=3", output)
            self.assertIn("COUNTS 2 1 3", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_workspace_evaluation_supports_temporaries_and_assignment(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-temps-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_TEMPS_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("WTEMP", output)
            self.assertIn("7", output)
            self.assertIn("11", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_methods_support_zero_argument_block_literals(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-method-block-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, METHOD_BLOCK_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class MethodBlockReporter", output)
            self.assertIn("MBLOCK", output)
            self.assertIn("METHOD", output)
            self.assertIn("CAPTURED", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_methods_support_lexical_capture_in_blocks(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-method-block-capture-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, METHOD_BLOCK_CAPTURE_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class MethodBlockCaptureReporter", output)
            self.assertIn("MCAP", output)
            self.assertIn("3", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_methods_support_non_local_block_return(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-method-block-return-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, METHOD_BLOCK_RETURN_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class MethodBlockReturnReporter", output)
            self.assertIn("MRET", output)
            self.assertIn("EARLY", output)
            self.assertNotIn("LATE", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_methods_support_single_argument_blocks(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-method-block-argument-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, METHOD_BLOCK_ARGUMENT_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class MethodBlockArgumentReporter", output)
            self.assertIn("MARG", output)
            self.assertIn("7", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_methods_expose_this_context_sender_receiver_and_alive(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-this-context-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, THIS_CONTEXT_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class ContextReporter", output)
            self.assertIn("CTX", output)
            self.assertIn("sum", output)
            self.assertIn("truth", output)
            self.assertIn("ALIVE", output)
            self.assertIn("RCVR", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_compiled_program_contexts_bridge_into_live_methods(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-compiled-context-bridge-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, COMPILED_CONTEXT_BRIDGE_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class CompiledContextBridge", output)
            self.assertIn("PCTX", output)
            self.assertGreaterEqual(output.count("<program>"), 2)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_compiled_method_updates_expose_sender_context_on_rv32(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-compiled-method-context-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            update_payload = temp_path / "object_detail_update.bin"
            build_update = subprocess.run(
                [
                    "python3",
                    str(UPDATE_TOOL),
                    "Object",
                    str(COMPILED_METHOD_CONTEXT_UPDATE_SOURCE),
                    str(update_payload),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if build_update.returncode != 0:
                raise AssertionError(
                    "compiled method update build failed\n"
                    f"stdout:\n{build_update.stdout}\n"
                    f"stderr:\n{build_update.stderr}"
                )
            elf_path = _build_elf(build_dir, COMPILED_METHOD_CONTEXT_UPDATE_DEMO)
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
                    "-fw_cfg",
                    f"name=opt/recorz-method-update,file={update_payload}",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: applied method update", output)
            self.assertIn("<program>", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_parenthesized_method_expressions(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-parenthesized-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, PAREN_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class ParenReporter", output)
            self.assertIn("PAREN", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_multi_keyword_method_headers(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-multikeyword-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, MULTIKEYWORD_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class KeywordReporter", output)
            self.assertIn("KEYWORD", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_keyword_sends_inside_method_expressions(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-expression-keyword-send-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, EXPRESSION_KEYWORD_SEND_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class KeywordExpressionReporter", output)
            self.assertIn("EXPR", output)
            self.assertIn("RETURN", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_nil_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-nil-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, NIL_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class NilReporter", output)
            self.assertIn("NIL", output)
            self.assertIn("KEPT", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_self_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-self-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, SELF_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class SelfReporter", output)
            self.assertIn("SELF", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_small_integer_literals_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-small-integer-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, SMALL_INTEGER_LITERAL_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class LiteralReporter", output)
            self.assertIn("INT", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_string_literals_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-string-literal-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, STRING_LITERAL_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class StringReporter", output)
            self.assertIn("STR", output)
            self.assertIn("OK", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_true_and_false_literals_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-boolean-literal-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, BOOLEAN_LITERAL_METHOD_SOURCE_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class BooleanReporter", output)
            self.assertIn("OBJECT: TRUTHVALUE", output)
            self.assertIn("CLASS: TRUE", output)
            self.assertIn("OBJECT: FALSEVALUE", output)
            self.assertIn("CLASS: FALSE", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_package_comment_round_trips_through_file_out_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-package-comment-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, PACKAGE_COMMENT_ROUNDTRIP_EXAMPLE)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            output_no_newlines = output.replace("\n", "")
            self.assertIn("RecorzKernelPackage:", output_no_newlines)
            self.assertIn("Tools", output_no_newlines)
            self.assertIn("comment:", output_no_newlines)
            self.assertIn("Shared utilities", output_no_newlines)
            self.assertIn("PackageRoundtrip", output_no_newlines)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)


if __name__ == "__main__":
    unittest.main()
