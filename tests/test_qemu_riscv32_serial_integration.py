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
TEXT_RENDERER_BOOTSTRAP = ROOT / "kernel" / "textui" / "TextRendererBootstrap.rz"
VIEW_BOOTSTRAP = ROOT / "kernel" / "textui" / "ViewBootstrap.rz"
WIDGET_BOOTSTRAP = ROOT / "kernel" / "textui" / "WidgetBootstrap.rz"
DEFAULT_EXAMPLE = ROOT / "examples" / "qemu_riscv_fb_demo.rz"
DISPLAY_FONT_EXAMPLE = ROOT / "examples" / "qemu_riscv_display_font_demo.rz"
DISPLAY_FONT_METRICS_EXAMPLE = ROOT / "examples" / "qemu_riscv_display_font_metrics_demo.rz"
DISPLAY_STYLED_TEXT_EXAMPLE = ROOT / "examples" / "qemu_riscv_display_styled_text_demo.rz"
DISPLAY_LAYOUT_EXAMPLE = ROOT / "examples" / "qemu_riscv_display_layout_demo.rz"
IMAGE_SIDE_TEXT_LAYOUT_EXAMPLE = ROOT / "examples" / "qemu_riscv_image_side_text_layout_demo.rz"
IMAGE_SIDE_TEXT_LAYOUT_FILE_IN = ROOT / "examples" / "qemu_riscv_image_side_text_layout_file_in.rz"
FORM_BE_DISPLAY_EXAMPLE = ROOT / "examples" / "qemu_riscv_form_be_display_demo.rz"
CURSOR_BE_CURSOR_EXAMPLE = ROOT / "examples" / "qemu_riscv_cursor_be_cursor_demo.rz"
CURSOR_VISIBILITY_EXAMPLE = ROOT / "examples" / "qemu_riscv_cursor_visibility_demo.rz"
CHARACTER_SCANNER_STOP_EXAMPLE = ROOT / "examples" / "qemu_riscv_character_scanner_stop_demo.rz"
WORKSPACE_CURSOR_SELECTION_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_cursor_selection_demo.rz"
STRING_SIZE_AT_EXAMPLE = ROOT / "examples" / "qemu_riscv_string_size_at_demo.rz"
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
WORKSPACE_ACCEPT_CURRENT_CLASS_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_accept_current_class_demo.rz"
WORKSPACE_RECOVER_LAST_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_workspace_recover_last_source_demo.rz"
REGENERATED_KERNEL_SOURCE_BROWSER_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_regenerated_kernel_source_browser_demo.rz"
WORKSPACE_INPUT_MONITOR_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_demo.rz"
WORKSPACE_INPUT_MONITOR_EVALUATE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_evaluate_demo.rz"
WORKSPACE_INPUT_MONITOR_ACCEPT_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_accept_demo.rz"
WORKSPACE_INPUT_MONITOR_RUN_TESTS_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_run_tests_demo.rz"
WORKSPACE_INPUT_MONITOR_BROWSER_BRIDGE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_browser_bridge_demo.rz"
WORKSPACE_INPUT_MONITOR_PACKAGE_BRIDGE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_package_bridge_demo.rz"
WORKSPACE_INPUT_MONITOR_EMIT_REGENERATED_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_emit_regenerated_source_demo.rz"
WORKSPACE_INPUT_MONITOR_REGENERATED_BOOT_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_regenerated_boot_demo.rz"
WORKSPACE_INPUT_MONITOR_REGENERATED_KERNEL_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_regenerated_kernel_demo.rz"
WORKSPACE_INPUT_MONITOR_REGENERATED_FILE_IN_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_regenerated_file_in_demo.rz"
WORKSPACE_PACKAGE_HOME_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_package_home_demo.rz"
WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE = ROOT / "examples" / "qemu_riscv_image_development_home_boot.rz"
WORKSPACE_EDIT_METHOD_ENTRY_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_edit_method_entry_demo.rz"
WORKSPACE_EDIT_PACKAGE_ENTRY_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_edit_package_entry_demo.rz"
WORKSPACE_EDIT_CURRENT_CLASS_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_edit_current_class_demo.rz"
WORKSPACE_EDIT_CURRENT_METHOD_LIST_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_edit_current_method_list_demo.rz"
WORKSPACE_EDIT_CURRENT_PACKAGE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_edit_current_package_demo.rz"
WORKSPACE_EDIT_CURRENT_PROTOCOL_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_edit_current_protocol_demo.rz"
VIEW_ROUTER_EXAMPLE = ROOT / "examples" / "qemu_riscv_view_router_demo.rz"
BROWSER_SURFACE_LIVE_BRIDGE_EXAMPLE = ROOT / "examples" / "qemu_riscv_browser_surface_live_bridge_demo.rz"
WORKSPACE_EDITOR_SURFACE_LIVE_BRIDGE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_editor_surface_live_bridge_demo.rz"
WORKSPACE_SESSION_WIDGET_PROBE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_session_widget_probe_demo.rz"
TEXTUI_COMPONENT_PROBE_FILE_IN = ROOT / "examples" / "qemu_riscv_textui_component_probe_file_in.rz"
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


def _build_elf(
    build_dir: Path,
    example_path: Path = DEFAULT_EXAMPLE,
    *,
    profile: str = "dev",
    file_in_payload: Path | None = None,
) -> Path:
    command = [
        "make",
        "-C",
        str(PLATFORM_DIR),
        f"BUILD_DIR={build_dir}",
        f"RV32_PROFILE={profile}",
        f"EXAMPLE={example_path}",
    ]
    if file_in_payload is not None:
        command.append(f"FILE_IN_PAYLOAD={file_in_payload}")
    command.extend(["clean", "all"])
    result = subprocess.run(
        command,
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


def _read_until_any(process: subprocess.Popen[str], markers: tuple[str, ...], *, timeout: float) -> str:
    if process.stdout is None:
        raise AssertionError("QEMU process stdout is not available")

    output = ""
    deadline = time.monotonic() + timeout
    while not any(marker in output for marker in markers):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise AssertionError(f"timed out waiting for one of {markers!r}\ncurrent output:\n{output}")
        ready, _, _ = select.select([process.stdout], [], [], remaining)
        if not ready:
            continue
        chunk = process.stdout.read(1)
        if chunk == "":
            break
        output += chunk
    if not any(marker in output for marker in markers):
        raise AssertionError(f"did not observe any of {markers!r}\ncurrent output:\n{output}")
    return output


def _read_until_workspace_input_ready(process: subprocess.Popen[str], *, timeout: float) -> str:
    return _read_until_any(process, ("VIEW: INPUT", "STATUS:"), timeout=timeout)


def _write_workspace_session_probe_example(
    temp_path: Path,
    name: str,
    *,
    initial_contents: str = "",
    body_lines: tuple[str, ...],
    include_buffer: bool = False,
) -> Path:
    example_path = temp_path / name
    escaped_contents = initial_contents.replace("'", "''")
    lines = [
        "Display clear.",
        f"Workspace setContents: '{escaped_contents}'.",
        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
    ]
    lines.extend(body_lines)
    lines.extend(
        [
            "Transcript show: 'LINE: '.",
            "Transcript show: Workspace cursor line printString.",
            "Transcript cr.",
            "Transcript show: 'TOP: '.",
            "Transcript show: Workspace visibleTopLine printString.",
            "Transcript cr.",
        ]
    )
    if include_buffer:
        lines.extend(
            [
                "Transcript show: 'BUFFER='.",
                "Transcript show: Workspace contents.",
                "Transcript cr.",
            ]
        )
    example_path.write_text("\n".join(lines), encoding="utf-8")
    return example_path


def _read_until_bytes(process: subprocess.Popen[bytes], marker: bytes, *, timeout: float) -> str:
    if process.stdout is None:
        raise AssertionError("QEMU process stdout is not available")

    output = b""
    deadline = time.monotonic() + timeout
    while marker not in output:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise AssertionError(
                f"timed out waiting for {marker!r}\ncurrent output:\n{output.decode('utf-8', errors='replace')}"
            )
        ready, _, _ = select.select([process.stdout], [], [], remaining)
        if not ready:
            continue
        chunk = os.read(process.stdout.fileno(), 4096)
        if chunk == b"":
            break
        output += chunk
    if marker not in output:
        raise AssertionError(
            f"did not observe {marker!r}\ncurrent output:\n{output.decode('utf-8', errors='replace')}"
        )
    return output.decode("utf-8", errors="replace")


def _write_combined_file_in_payload(build_dir: Path, *payloads: Path) -> Path:
    payload_path = build_dir / "external_file_in_payload.rz"
    parts = []
    for path in payloads:
        parts.append(path.read_text(encoding="utf-8"))
    payload_path.write_text("\n!\n".join(parts), encoding="utf-8")
    return payload_path


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

    def test_view_router_routes_focus_and_commands_from_image_code(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-view-router-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, VIEW_ROUTER_EXAMPLE)
            file_in_payload = _write_combined_file_in_payload(
                build_dir,
                TEXT_RENDERER_BOOTSTRAP,
                VIEW_BOOTSTRAP,
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
                    "-fw_cfg",
                    f"name=opt/recorz-file-in,file={file_in_payload}",
                ],
                cwd=ROOT,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                output = _read_until(process, "COMMAND: SPACE", timeout=8.0)
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
            self.assertRegex(output, r"FOC\s*US:\s*2")
            self.assertIn("COMMAND: SPACE", output)
            self.assertNotIn("panic:", output)

    def test_live_browser_navigation_routes_through_image_side_browser_surface(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-browser-surface-bridge-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, BROWSER_SURFACE_LIVE_BRIDGE_EXAMPLE)
            file_in_payload = _write_combined_file_in_payload(
                build_dir,
                TEXT_RENDERER_BOOTSTRAP,
                VIEW_BOOTSTRAP,
                WIDGET_BOOTSTRAP,
                TEXTUI_COMPONENT_PROBE_FILE_IN,
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
                    "-fw_cfg",
                    f"name=opt/recorz-file-in,file={file_in_payload}",
                ],
                cwd=ROOT,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                output = _read_until(process, "TEXT EDITOR COMPONENT", timeout=8.0)
                output += _read_until(process, "newline", timeout=8.0)
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
            self.assertIn("TEXT EDITOR COMPONENT", output)
            self.assertIn("newline", output)
            self.assertNotIn("panic:", output)

    def test_live_source_editor_routes_through_image_side_workspace_editor_surface(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-editor-surface-bridge-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_EDITOR_SURFACE_LIVE_BRIDGE_EXAMPLE)
            file_in_payload = _write_combined_file_in_payload(
                build_dir,
                TEXT_RENDERER_BOOTSTRAP,
                VIEW_BOOTSTRAP,
                WIDGET_BOOTSTRAP,
                TEXTUI_COMPONENT_PROBE_FILE_IN,
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
                    "-fw_cfg",
                    f"name=opt/recorz-file-in,file={file_in_payload}",
                ],
                cwd=ROOT,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                output = _read_until(process, "TEXT EDITOR COMPONENT", timeout=8.0)
                output += _read_until(process, "MODE: METHOD SOURCE", timeout=8.0)
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
            self.assertIn("TEXT EDITOR COMPONENT", output)
            self.assertIn("MODE: METHOD SOURCE", output)
            self.assertNotIn("panic:", output)

    def test_live_workspace_session_redraw_routes_through_image_side_source_widget(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-session-bridge-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_SESSION_WIDGET_PROBE_EXAMPLE)
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
                output = _read_until(process, "TEXT EDITOR COMPONENT", timeout=8.0)
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
            self.assertIn("TEXT EDITOR COMPONENT", output)
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
                stdin=subprocess.PIPE,
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

    def test_display_font_point_size_is_visible_from_the_image(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-display-font-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, DISPLAY_FONT_EXAMPLE)
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
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("16", output)
            self.assertNotIn("panic:", output)

    def test_form_be_display_rebinds_display_default_form(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-form-be-display-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, FORM_BE_DISPLAY_EXAMPLE)
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
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("DISPLAY REBOUND", output)
            self.assertNotIn("panic:", output)

    def test_cursor_be_cursor_binds_the_active_cursor(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-cursor-be-cursor-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, CURSOR_BE_CURSOR_EXAMPLE)
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
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("CURSOR BOUND", output)
            self.assertNotIn("panic:", output)

    def test_cursor_factory_can_show_hide_and_move_the_active_cursor(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-cursor-visibility-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, CURSOR_VISIBILITY_EXAMPLE)
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
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("12", output)
            self.assertIn("34", output)
            self.assertIn("CURSOR SHOWN", output)
            self.assertIn("CURSOR HIDDEN", output)
            self.assertNotIn("panic:", output)

    def test_character_scanner_reports_wrap_tab_newline_and_boundaries(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-character-scanner-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, CHARACTER_SCANNER_STOP_EXAMPLE)
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
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("RIGHT", output)
            self.assertIn("TAB", output)
            self.assertIn("NEWLINE", output)
            self.assertIn("SELECTION", output)
            self.assertIn("CURSOR", output)
            self.assertIn("END", output)
            self.assertNotIn("panic:", output)

    def test_display_font_metrics_are_visible_from_the_image(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-display-font-metrics-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, DISPLAY_FONT_METRICS_EXAMPLE)
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
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("6", output)
            self.assertIn("2", output)
            self.assertIn("10", output)
            self.assertNotIn("panic:", output)

    def test_display_can_render_styled_text_from_image_objects(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-display-styled-text-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, DISPLAY_STYLED_TEXT_EXAMPLE)
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
            self.assertIn("STYLE", output)
            self.assertIn("2042163", output)
            self.assertNotIn("panic:", output)

    def test_display_layout_objects_are_visible_from_the_image(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-display-layout-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, DISPLAY_LAYOUT_EXAMPLE)
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
            for expected in ("24", "4", "2", "48", "1"):
                self.assertIn(expected, output)
            self.assertNotIn("panic:", output)

    def test_strings_support_size_and_at_from_the_image(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-string-size-at-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, STRING_SIZE_AT_EXAMPLE)
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
            self.assertIn("SIZE=3", output)
            self.assertIn("AT=66", output)
            self.assertNotIn("panic:", output)

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
                process.stdin.write("ab\x02C\x06D\x0f")
                process.stdin.flush()
                output += _read_until(process, "BUFFER=aCbD", timeout=8.0)
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
            self.assertIn("MODE: WORKSPACE", output)
            self.assertIn("TARGET: DOIT BUFFER", output)
            self.assertIn("MOVE: ARROWS CTRL-B/F/N", output)
            self.assertIn("PRINT: CTRL-P", output)
            self.assertIn("DOIT: CTRL-D/R", output)
            self.assertIn("LINE: 1", output)
            self.assertIn("TOP: 1", output)
            self.assertIn("CLOSE: CTRL-O", output)
            self.assertIn("BUFFER=aCbD", output.replace("\n", ""))
            self.assertNotIn("panic:", output)

    def test_workspace_cursor_and_selection_objects_are_visible_from_the_image(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-cursor-selection-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_CURSOR_SELECTION_EXAMPLE)
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
                process.stdin.write("ab\x02C\x0f")
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
            self.assertIn("INDEX=2", output)
            self.assertIn("LINE=0", output)
            self.assertIn("COLUMN=2", output)
            self.assertIn("TOP=0", output)
            self.assertIn("STARTLINE=0", output)
            self.assertIn("STARTCOLUMN=2", output)
            self.assertIn("ENDLINE=0", output)
            self.assertIn("ENDCOLUMN=2", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_supports_vertical_cursor_movement(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-vertical-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = _write_workspace_session_probe_example(
                temp_path,
                "workspace_input_monitor_vertical_demo.rz",
                body_lines=(
                    "Workspace insertCodePoint: 65.",
                    "Workspace insertCodePoint: 10.",
                    "Workspace insertCodePoint: 66.",
                    "Workspace insertCodePoint: 67.",
                    "Workspace insertCodePoint: 10.",
                    "Workspace insertCodePoint: 68.",
                    "Workspace moveCursorUp.",
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                    "Workspace insertCodePoint: 88.",
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                    "Workspace moveCursorDown.",
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                    "Workspace insertCodePoint: 89.",
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                ),
                include_buffer=True,
            )
            elf_path = _build_elf(build_dir, example_path)
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
            self.assertIn("LINE: 2", output)
            self.assertIn("TOP: 0", output)
            self.assertIn("BUFFER=A\nBXC\nDY", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_scrolls_multiline_buffers(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-scroll-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            move_lines = tuple("Workspace moveCursorDown." for _ in range(39))
            ensure_lines = tuple(
                "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible."
                for _ in range(39)
            )
            body_lines = tuple(
                line
                for pair in zip(move_lines, ensure_lines)
                for line in pair
            )
            example_path = _write_workspace_session_probe_example(
                temp_path,
                "workspace_input_monitor_scroll_demo.rz",
                initial_contents=("A\n" * 40).rstrip(),
                body_lines=body_lines,
            )
            elf_path = _build_elf(build_dir, example_path)
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
            line_match = re.search(r"LINE: (\d+)", output)
            top_match = re.search(r"TOP: (\d+)", output)
            self.assertIsNotNone(line_match)
            self.assertIsNotNone(top_match)
            self.assertGreater(int(line_match.group(1)), 0)
            self.assertGreater(int(top_match.group(1)), 0)
            self.assertGreaterEqual(int(line_match.group(1)), int(top_match.group(1)))
            self.assertNotIn("panic:", output)

    def test_workspace_explicit_protocol_exposes_status_feedback_visible_origin_and_target(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-protocol-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_protocol_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: 'zero",
                        "one",
                        "two",
                        "three",
                        "four'.",
                        "Workspace moveCursorDown.",
                        "Workspace moveCursorDown.",
                        "Workspace moveCursorDown.",
                        "Workspace moveCursorDown.",
                        "Workspace setWorkspaceName: 'Scratch'.",
                        "Workspace setStatusText: 'READY'.",
                        "Workspace setFeedbackText: 'SHORT HELP'.",
                        "Workspace setVisibleOriginTop: 3 left: 5.",
                        "Workspace setCurrentTargetName: 'ScratchBuffer'.",
                        "Transcript show: 'NAME='.",
                        "Transcript show: Workspace workspaceName.",
                        "Transcript cr.",
                        "Transcript show: 'STATUS='.",
                        "Transcript show: Workspace statusText.",
                        "Transcript cr.",
                        "Transcript show: 'FEEDBACK='.",
                        "Transcript show: Workspace feedbackText.",
                        "Transcript cr.",
                        "Transcript show: 'TOP='.",
                        "Transcript show: Workspace visibleTopLine printString.",
                        "Transcript cr.",
                        "Transcript show: 'LEFT='.",
                        "Transcript show: Workspace visibleLeftColumn printString.",
                        "Transcript cr.",
                        "Transcript show: 'ORIGINTOP='.",
                        "Transcript show: Workspace visibleOrigin topLine printString.",
                        "Transcript cr.",
                        "Transcript show: 'ORIGINLEFT='.",
                        "Transcript show: Workspace visibleOrigin visibleLeftColumn printString.",
                        "Transcript cr.",
                        "Transcript show: 'TARGET='.",
                        "Transcript show: Workspace currentTargetLabel.",
                        "Transcript cr.",
                        "Transcript show: 'MOD='.",
                        "Transcript show: ((Workspace isModified) ifTrue: ['Y'] ifFalse: ['N']).",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("NAME=Scratch", normalized)
            self.assertIn("STATUS=READY", normalized)
            self.assertIn("FEEDBACK=SHORT HELP", normalized)
            self.assertIn("TOP=3", normalized)
            self.assertIn("LEFT=5", normalized)
            self.assertIn("ORIGINTOP=3", normalized)
            self.assertIn("ORIGINLEFT=5", normalized)
            self.assertIn("TARGET=ScratchBuffer", normalized)
            self.assertIn("MOD=Y", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_session_keeps_visible_origin_stable_during_in_view_edits_and_movement(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-visible-origin-stable-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            line_text = "0123456789" * 9
            buffer_lines = [line_text for _ in range(30)]
            buffer_text = "\n".join(buffer_lines)
            cursor_line = 20
            cursor_column = 70
            cursor_index = sum(len(line) + 1 for line in buffer_lines[:cursor_line]) + cursor_column
            example_path = temp_path / "workspace_visible_origin_stable_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        f"Workspace setContents: '{buffer_text}'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        f"Workspace cursor moveToIndex: {cursor_index} line: {cursor_line} column: {cursor_column} topLine: 10.",
                        f"Workspace selection collapseToLine: {cursor_line} column: {cursor_column}.",
                        "Workspace setVisibleOriginTop: 10 left: 10.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                        "Transcript show: 'STEP1TOP='.",
                        "Transcript show: Workspace visibleTopLine printString.",
                        "Transcript cr.",
                        "Transcript show: 'STEP1LEFT='.",
                        "Transcript show: Workspace visibleLeftColumn printString.",
                        "Transcript cr.",
                        "Workspace insertCodePoint: 88.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                        "Transcript show: 'STEP2TOP='.",
                        "Transcript show: Workspace visibleTopLine printString.",
                        "Transcript cr.",
                        "Transcript show: 'STEP2LEFT='.",
                        "Transcript show: Workspace visibleLeftColumn printString.",
                        "Transcript cr.",
                        "Workspace deleteBackward.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                        "Transcript show: 'STEP3TOP='.",
                        "Transcript show: Workspace visibleTopLine printString.",
                        "Transcript cr.",
                        "Transcript show: 'STEP3LEFT='.",
                        "Transcript show: Workspace visibleLeftColumn printString.",
                        "Transcript cr.",
                        "Workspace moveCursorDown.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                        "Workspace moveCursorUp.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                        "Transcript show: 'STEP4TOP='.",
                        "Transcript show: Workspace visibleTopLine printString.",
                        "Transcript cr.",
                        "Transcript show: 'STEP4LEFT='.",
                        "Transcript show: Workspace visibleLeftColumn printString.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("STEP1TOP=10", normalized)
            self.assertIn("STEP1LEFT=10", normalized)
            self.assertIn("STEP2TOP=10", normalized)
            self.assertIn("STEP2LEFT=10", normalized)
            self.assertIn("STEP3TOP=10", normalized)
            self.assertIn("STEP3LEFT=10", normalized)
            self.assertIn("STEP4TOP=10", normalized)
            self.assertIn("STEP4LEFT=10", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_tool_dispatch_command_updates_status_feedback_and_refusals(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-commands-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_command_protocol_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: '1 + 2'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 16.",
                        "Transcript show: 'PRINTSTATUS='.",
                        "Transcript show: Workspace statusText.",
                        "Transcript cr.",
                        "Transcript show: 'PRINTFDBK='.",
                        "Transcript show: Workspace feedbackText.",
                        "Transcript cr.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 20.",
                        "Transcript show: 'TESTSTATUS='.",
                        "Transcript show: Workspace statusText.",
                        "Transcript cr.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 24.",
                        "Transcript show: 'ACCEPTSTATUS='.",
                        "Transcript show: Workspace statusText.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("PRINTSTATUS=PRINT COMPLETE", normalized)
            self.assertIn("PRINTFDBK=3", normalized)
            self.assertIn("TESTSTATUS=TESTS NEED TARGET", normalized)
            self.assertIn("ACCEPTSTATUS=ACCEPT NEEDS SOURCE TARGET", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_source_editor_state_survives_replacing_boot_workspace_session(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-session-model-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_source_editor_session_model_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace browseMethod: 'newline' ofClassNamed: 'Display'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "Workspace setContents: 'one",
                        "two'.",
                        "Workspace cursor moveToIndex: 3 line: 0 column: 3 topLine: 0.",
                        "Workspace collapseSelectionToCursor.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 88 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 8 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 27 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 91 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 66 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 27 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 91 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 68 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 27 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 91 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 67 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 27 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 91 onForm: Display defaultForm.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') handleByte: 65 onForm: Display defaultForm.",
                        "Workspace selection setStartLine: 0 startColumn: 1 endLine: 1 endColumn: 2.",
                        "Workspace setVisibleOriginTop: 0 left: 1.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceTool') setStatusText: 'EDIT STATUS'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceTool') setFeedbackText: 'EDIT FEEDBACK'.",
                        "KernelInstaller rememberObject: ((KernelInstaller classNamed: 'WorkspaceSession') new) named: 'BootWorkspaceSession'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "Transcript show: 'VIEW='.",
                        "Transcript show: Workspace currentViewKind printString.",
                        "Transcript cr.",
                        "Transcript show: 'TARGET='.",
                        "Transcript show: Workspace currentTargetName.",
                        "Transcript cr.",
                        "Transcript show: 'STATUS='.",
                        "Transcript show: Workspace statusText.",
                        "Transcript cr.",
                        "Transcript show: 'FEEDBACK='.",
                        "Transcript show: Workspace feedbackText.",
                        "Transcript cr.",
                        "Transcript show: 'BUFFER='.",
                        "Transcript show: Workspace contents.",
                        "Transcript cr.",
                        "Transcript show: 'LINE='.",
                        "Transcript show: Workspace cursor line printString.",
                        "Transcript cr.",
                        "Transcript show: 'COLUMN='.",
                        "Transcript show: Workspace cursor column printString.",
                        "Transcript cr.",
                        "Transcript show: 'LEFT='.",
                        "Transcript show: Workspace visibleLeftColumn printString.",
                        "Transcript cr.",
                        "Transcript show: 'STARTLINE='.",
                        "Transcript show: Workspace selection startLine printString.",
                        "Transcript cr.",
                        "Transcript show: 'STARTCOLUMN='.",
                        "Transcript show: Workspace selection startColumn printString.",
                        "Transcript cr.",
                        "Transcript show: 'ENDLINE='.",
                        "Transcript show: Workspace selection endLine printString.",
                        "Transcript cr.",
                        "Transcript show: 'ENDCOLUMN='.",
                        "Transcript show: Workspace selection endColumn printString.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=8.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            normalized = output.replace("\r", "")
            self.assertIn("VIEW=5", normalized)
            self.assertIn("TARGET=Display>>newline", normalized)
            self.assertIn("STATUS=EDIT STATUS", normalized)
            self.assertIn("FEEDBACK=EDIT FEEDBACK", normalized)
            self.assertIn("BUFFER=one\ntwo", normalized)
            self.assertIn("LINE=0", normalized)
            self.assertIn("COLUMN=3", normalized)
            self.assertIn("LEFT=1", normalized)
            self.assertIn("STARTLINE=0", normalized)
            self.assertIn("STARTCOLUMN=1", normalized)
            self.assertIn("ENDLINE=1", normalized)
            self.assertIn("ENDCOLUMN=2", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_tool_dispatch_command_returns_source_editor_to_browser_context(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-command-return-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_command_return_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: (KernelInstaller fileOutPackageNamed: 'TextUI').",
                        "Workspace setCurrentViewKind: 13.",
                        "Workspace setCurrentTargetName: 'TextUI'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "Transcript show: 'RESULT='.",
                        "Transcript show: ((KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 15) printString.",
                        "Transcript cr.",
                        "Transcript show: 'VIEW='.",
                        "Transcript show: Workspace currentViewKind printString.",
                        "Transcript cr.",
                        "Transcript show: 'TARGET='.",
                        "Transcript show: ((Workspace currentTargetName = nil) ifTrue: ['nil'] ifFalse: [Workspace currentTargetName]).",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("RESULT=4", normalized)
            self.assertIn("VIEW=14", normalized)
            self.assertIn("TARGET=nil", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_source_editor_return_restores_plain_workspace_state(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-plain-return-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_plain_return_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: 'plain workspace buffer'.",
                        "Workspace cursor moveToIndex: 5 line: 0 column: 5 topLine: 0.",
                        "Workspace selection setStartLine: 0 startColumn: 1 endLine: 0 endColumn: 5.",
                        "Workspace setVisibleOriginTop: 0 left: 6.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceTool') rememberPlainWorkspaceStateContents: Workspace contents.",
                        "Workspace setContents: (KernelInstaller fileOutPackageNamed: 'TextUI').",
                        "Workspace setCurrentViewKind: 13.",
                        "Workspace setCurrentTargetName: 'TextUI'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "Transcript show: 'RESULT='.",
                        "Transcript show: ((KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 15) printString.",
                        "Transcript cr.",
                        "Transcript show: 'VIEW='.",
                        "Transcript show: Workspace currentViewKind printString.",
                        "Transcript cr.",
                        "Transcript show: 'TARGET='.",
                        "Transcript show: ((Workspace currentTargetName = nil) ifTrue: ['nil'] ifFalse: [Workspace currentTargetName]).",
                        "Transcript cr.",
                        "Transcript show: 'CONTENTS='.",
                        "Transcript show: Workspace contents.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("RESULT=1", normalized)
            self.assertIn("VIEW=18", normalized)
            self.assertIn("TARGET=nil", normalized)
            self.assertIn("CONTENTS=plain workspace buffer", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_direct_method_browser_edit_return_restores_plain_workspace_state(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-direct-edit-return-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_direct_edit_return_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: 'scratch direct workspace'.",
                        "Workspace cursor moveToIndex: 6 line: 0 column: 6 topLine: 0.",
                        "Workspace selection setStartLine: 0 startColumn: 2 endLine: 0 endColumn: 6.",
                        "Workspace setVisibleOriginTop: 0 left: 4.",
                        "Workspace browseMethod: 'newline' ofClassNamed: 'Display'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "Transcript show: 'RESULT='.",
                        "Transcript show: ((KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 15) printString.",
                        "Transcript cr.",
                        "Transcript show: 'VIEW='.",
                        "Transcript show: Workspace currentViewKind printString.",
                        "Transcript cr.",
                        "Transcript show: 'TARGET='.",
                        "Transcript show: ((Workspace currentTargetName = nil) ifTrue: ['nil'] ifFalse: [Workspace currentTargetName]).",
                        "Transcript cr.",
                        "Transcript show: 'CONTENTS='.",
                        "Transcript show: Workspace contents.",
                        "Transcript cr.",
                        "Transcript show: 'CURSOR='.",
                        "Transcript show: Workspace cursor column printString.",
                        "Transcript cr.",
                        "Transcript show: 'SELECTION='.",
                        "Transcript show: Workspace selection startColumn printString.",
                        "Transcript show: '-'.",
                        "Transcript show: Workspace selection endColumn printString.",
                        "Transcript cr.",
                        "Transcript show: 'LEFT='.",
                        "Transcript show: Workspace visibleLeftColumn printString.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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
                    output, _ = process.communicate(timeout=8.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("RESULT=1", normalized)
            self.assertIn("VIEW=18", normalized)
            self.assertIn("TARGET=nil", normalized)
            self.assertIn("CONTENTS=scratch direct workspace", normalized)
            self.assertIn("CURSOR=6", normalized)
            self.assertIn("SELECTION=2-6", normalized)
            self.assertIn("LEFT=4", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_tool_command_bindings_can_be_updated_from_image_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-command-bindings-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_command_binding_update_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: '1 + 2'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "KernelInstaller fileInMethodChunks: 'printCommandByte",
                        "    ^21",
                        "!' onClass: (KernelInstaller classNamed: 'WorkspaceTool').",
                        "Transcript show: 'OLD='.",
                        "Transcript show: ((KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 16) printString.",
                        "Transcript cr.",
                        "Transcript show: 'NEW='.",
                        "Transcript show: ((KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 21) printString.",
                        "Transcript cr.",
                        "Transcript show: 'STATUS='.",
                        "Transcript show: Workspace statusText.",
                        "Transcript cr.",
                        "Transcript show: 'FEEDBACK='.",
                        "Transcript show: Workspace feedbackText.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("OLD=0", normalized)
            self.assertIn("NEW=1", normalized)
            self.assertIn("STATUS=PRINT COMPLETE", normalized)
            self.assertIn("FEEDBACK=3", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_tool_keeps_plain_workspace_and_source_editor_command_routes_separate(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-command-routes-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_command_route_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace setContents: '1 + 2'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "Workspace setCurrentViewKind: 18.",
                        "Transcript show: 'PLAINRETURN='.",
                        "Transcript show: ((KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 15) printString.",
                        "Transcript cr.",
                        "Workspace setContents: (KernelInstaller fileOutPackageNamed: 'TextUI').",
                        "Workspace setCurrentViewKind: 13.",
                        "Workspace setCurrentTargetName: 'TextUI'.",
                        "Transcript show: 'EDITORRETURN='.",
                        "Transcript show: ((KernelInstaller objectNamed: 'BootWorkspaceTool') dispatchCommandOnForm: 15) printString.",
                        "Transcript cr.",
                        "Transcript show: 'VIEW='.",
                        "Transcript show: Workspace currentViewKind printString.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("PLAINRETURN=0", normalized)
            self.assertIn("EDITORRETURN=4", normalized)
            self.assertIn("VIEW=14", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_session_open_repairs_invalid_view_kind_target_and_left_column(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-stale-session-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = temp_path / "workspace_stale_session_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Workspace fileIn: 'RecorzKernelClass: #TinyWorkspaceState superclass: #Object instanceVariableNames: ''''",
                        "!",
                        "value",
                        "    ^1'.",
                        "Workspace browseClassNamed: 'TinyWorkspaceState'.",
                        "Workspace setCurrentViewKind: 99.",
                        "Workspace setCurrentTargetName: 'StaleTarget'.",
                        "(KernelInstaller objectNamed: 'BootWorkspaceTool') setVisibleLeftColumn: (0 - 5).",
                        "(KernelInstaller objectNamed: 'BootWorkspaceSession') openOn: Workspace mode: 1.",
                        "Transcript show: 'VIEW='.",
                        "Transcript show: Workspace currentViewKind printString.",
                        "Transcript cr.",
                        "Transcript show: 'TARGET='.",
                        "Transcript show: ((Workspace currentTargetName = nil) ifTrue: ['nil'] ifFalse: [Workspace currentTargetName]).",
                        "Transcript cr.",
                        "Transcript show: 'LEFT='.",
                        "Transcript show: ((KernelInstaller objectNamed: 'BootWorkspaceTool') visibleLeftColumn) printString.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(build_dir, example_path)
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

            normalized = output.replace("\r", "").replace("\n", "")
            self.assertIn("VIEW=18", normalized)
            self.assertIn("TARGET=nil", normalized)
            self.assertIn("LEFT=0", normalized)
            self.assertNotIn("panic:", normalized)

    def test_workspace_home_moves_to_the_visible_top_of_the_viewport(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-home-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = _write_workspace_session_probe_example(
                temp_path,
                "workspace_home_demo.rz",
                initial_contents=("A\n" * 40).rstrip(),
                body_lines=(
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') scrollEditorPageBy: 1.",
                    "Workspace moveCursorDown.",
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') ensureEditorCursorVisible.",
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') moveCursorToVisibleTop.",
                ),
            )
            elf_path = _build_elf(build_dir, example_path)
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
            line_match = re.search(r"LINE: (\d+)", output)
            top_match = re.search(r"TOP: (\d+)", output)
            self.assertIsNotNone(line_match)
            self.assertIsNotNone(top_match)
            self.assertEqual(line_match.group(1), top_match.group(1))
            self.assertGreater(int(top_match.group(1)), 0)
            self.assertNotIn("panic:", output)

    def test_workspace_end_moves_to_the_visible_bottom_of_the_viewport(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-end-") as temp_dir:
            temp_path = Path(temp_dir)
            build_dir = temp_path / "build"
            example_path = _write_workspace_session_probe_example(
                temp_path,
                "workspace_end_demo.rz",
                initial_contents=("A\n" * 40).rstrip(),
                body_lines=(
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') scrollEditorPageBy: 1.",
                    "(KernelInstaller objectNamed: 'BootWorkspaceSession') moveCursorToVisibleBottom.",
                ),
            )
            elf_path = _build_elf(build_dir, example_path)
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
            line_match = re.search(r"LINE: (\d+)", output)
            top_match = re.search(r"TOP: (\d+)", output)
            self.assertIsNotNone(line_match)
            self.assertIsNotNone(top_match)
            self.assertGreater(int(line_match.group(1)), int(top_match.group(1)))
            self.assertGreater(int(top_match.group(1)), 0)
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
                process.stdin.write("Transcript show: (1 + 2) printString.\x04!\x0f")
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
            self.assertIn("DOIT: CTRL-D/R", output)
            self.assertIn("STATUS: DOIT COMPLETE", output)
            self.assertIn("OUT> 3", output)
            self.assertIn(
                "BUFFER=Transcript show: (1 + 2) printString.!",
                output.replace("\n", ""),
            )
            self.assertIn("RERUN=3", output.replace("\n", ""))
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_print_the_current_buffer_result(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-print-") as temp_dir:
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
                process.stdin.write("1 + 2\x10\x0f")
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
            self.assertIn("PRINT: CTRL-P", output)
            self.assertIn("STATUS: PRINT COMPLETE", output)
            self.assertIn("OUT> 3", output)
            self.assertIn("BUFFER=1 + 2", output.replace("\n", ""))
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
                process.stdin.write("\x18\x0f")
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
            self.assertIn("recorz qemu-riscv32 mvp: snapshot saved, shutting down", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_refuses_to_run_method_source_as_a_doit(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-run-method-") as temp_dir:
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
                process.stdin.write("\x04\x0f")
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
            self.assertIn("MODE: METHOD SOURCE", output)
            self.assertIn("STATUS: CTRL-X INSTALLS SOURCE", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_refuses_to_print_method_source_as_a_doit(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-print-method-") as temp_dir:
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
                process.stdin.write("\x10\x0f")
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
            self.assertIn("STATUS: CTRL-X INSTALLS SOURCE", output)
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

    def test_workspace_interactive_input_monitor_can_browse_regenerated_kernel_source_and_return(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-regen-kernel-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_REGENERATED_KERNEL_EXAMPLE)
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
                process.stdin.write("\x07")
                process.stdin.flush()
                output += _read_until(process, "SOURCE: KERNEL", timeout=8.0)
                output += _read_until(process, "RECORZKERNELCLASS:", timeout=8.0)
                process.stdin.write("\x0f")
                process.stdin.flush()
                output += _read_until(process, "SAVE: CTRL-W/K REGEN:G", timeout=8.0)
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
            self.assertIn("VIEW: REGEN", output)
            self.assertIn("SOURCE: KERNEL", output)
            self.assertIn("CLOSE: CTRL-O/D", output)
            self.assertIn("RECORZKERNELCLASS:", output)
            self.assertIn("SAVE: CTRL-W/K REGEN:G/L", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_browse_regenerated_boot_source_and_return(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-regen-boot-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_REGENERATED_BOOT_EXAMPLE)
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
                process.stdin.write("\x0c")
                process.stdin.flush()
                output += _read_until(process, "SOURCE: BOOT", timeout=8.0)
                output += _read_until(process, "WORKSPACE FILEIN:", timeout=8.0)
                process.stdin.write("\x0f")
                process.stdin.flush()
                output += _read_until(process, "SAVE: CTRL-W/K REGEN:G/L", timeout=8.0)
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
            self.assertIn("VIEW: REGEN", output)
            self.assertIn("SOURCE: BOOT", output)
            self.assertIn("CLOSE: CTRL-O/D", output)
            self.assertIn("WORKSPACE FILEIN:", output)
            self.assertIn("SAVE: CTRL-W/K REGEN:G/L", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_browse_regenerated_file_in_source_and_return(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-regen-file-in-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_REGENERATED_FILE_IN_EXAMPLE)
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
                process.stdin.write("\x11")
                process.stdin.flush()
                output += _read_until(process, "SOURCE: FILEIN", timeout=8.0)
                output += _read_until(process, "RECORZKERNELDOIT:", timeout=8.0)
                process.stdin.write("\x0f")
                process.stdin.flush()
                output += _read_until(process, "FILEIN: CTRL-Q", timeout=8.0)
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
            self.assertIn("VIEW: REGEN", output)
            self.assertIn("SOURCE: FILEIN", output)
            self.assertIn("CLOSE: CTRL-O/D", output)
            self.assertIn("RECORZKERNELDOIT:", output)
            self.assertIn("WORKSPACE EMITREGENERATEDBOOTSOURCE.", output)
            self.assertIn("FILEIN: CTRL-Q", output)
            self.assertNotIn("panic:", output)

    def test_workspace_development_home_boot_opens_the_interactive_editor_with_a_starter_command(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-development-home-boot-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE)
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
                output = _read_until(process, "Workspace browsePackagesInteractive.", timeout=8.0)
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
            self.assertIn("VIEW: INPUT", output)
            self.assertIn("Workspace browsePackagesInteractive.", output)
            self.assertIn("CLOSE: CTRL-O", output)
            self.assertNotIn("panic:", output)

    def test_workspace_browse_packages_interactive_can_edit_a_package_and_return(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-package-home-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_PACKAGE_HOME_EXAMPLE)
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
                output = _read_until(process, "INTERACTIVE PACKAGE LIST", timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("\x18\x0f\x0f")
                process.stdin.flush()
                output += _read_until_any(
                    process,
                    ("Workspace browsePackagesInteractive.", "panic:"),
                    timeout=8.0,
                )
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
            self.assertIn("INTERACTIVE PACKAGE LIST", output)
            self.assertIn("X OPEN  O CLOSE", output)
            self.assertIn("WORKSPACETextUI", output)
            self.assertIn("RecorzKernelPackage: 'TextUI'", output)
            self.assertIn("Workspace browsePackagesInteractive.", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_emit_regenerated_sources_and_continue(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-emit-regen-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_EMIT_REGENERATED_SOURCE_EXAMPLE)
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
            )
            try:
                output = _read_until_bytes(process, b"EMIT: CTRL-U", timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write(b"\x15")
                process.stdin.write(b"\x07")
                process.stdin.flush()
                output += _read_until_bytes(process, b"SOURCE: KERNEL", timeout=50.0)
                process.stdin.write(b"\x0f")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5.0)
                output += (process.stdout.read() or b"").decode("utf-8", errors="replace")
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5.0)
                if process.stdout is not None:
                    process.stdout.close()
                if process.stdin is not None:
                    process.stdin.close()

            output = output.replace("\r", "")
            self.assertIn("EMIT: CTRL-U", output)
            self.assertIn("recorz-regenerated-kernel-source-begin", output)
            self.assertIn("recorz-regenerated-kernel-source-end", output)
            self.assertIn("recorz-regenerated-boot-source-begin", output)
            self.assertIn("recorz-regenerated-boot-source-end", output)
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
            self.assertIn("CLOSE: CTRL-O", output)
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
            self.assertIn("CLOSE: CTRL-O", output)
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
            self.assertIn("CLOSE: CTRL-O", output)
            self.assertGreaterEqual(output.count("VIEW: INPUT"), 1)
            self.assertGreaterEqual(output.count("CLASS: DISPLAY"), 1)
            self.assertGreaterEqual(output.count("METHOD: NEWLINE"), 1)
            self.assertNotIn("panic:", output)

    def test_workspace_edit_method_entry_loads_seeded_method_source_and_accepts_without_panic(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-edit-method-seeded-source-") as temp_dir:
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
                output = _read_until(process, "Display defaultForm newline.", timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("\x04\x18\x0f")
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
            self.assertIn("Display defaultForm newline.", output)
            self.assertIn("STATUS: CTRL-X INSTALLS SOURCE", output)
            self.assertIn("STATUS: INSTALL COMPLETE", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_revert_current_browser_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-revert-") as temp_dir:
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
                output = _read_until(process, "REVERT: CTRL-Y", timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("x\x19\x18\x0f")
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
            self.assertIn("DISPLAY DEFAULTFORM NEWLINE.^SELF", output.upper())
            self.assertIn("REVERT: CTRL-Y", output)
            self.assertIn("STATUS: SOURCE RESTORED", output)
            self.assertIn("STATUS: INSTALL COMPLETE", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_save_a_recovery_snapshot(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-recovery-save-") as temp_dir:
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
                output = _read_until(process, "SAVE: CTRL-W", timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("\x0b")
                process.stdin.flush()
                time.sleep(1.0)
                if process.poll() is None:
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
            self.assertIn("SAVE: CTRL-W/K", output)
            self.assertIn("recorz-snapshot-begin", output)
            self.assertIn("recorz qemu-riscv32 mvp: snapshot saved, shutting down", output)
            self.assertNotIn("panic:", output)

    def test_workspace_interactive_input_monitor_can_run_current_tests(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-input-monitor-tests-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_INPUT_MONITOR_RUN_TESTS_EXAMPLE)
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
                output = _read_until(process, "TESTS: CTRL-T", timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("\x14")
                process.stdin.flush()
                output += _read_until(process, "STATUS: TESTS COMPLETE", timeout=8.0)
                process.stdin.write("\x0f")
                process.stdin.flush()
                time.sleep(0.5)
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
            self.assertIn("PASS TinySpec>>testPass", output)
            self.assertIn("FAIL TinySpec>>testFail", output)
            self.assertIn("SUMMARY TinySpec P=1 F=1 T=2", output)
            self.assertIn("STATUS: TESTS COMPLETE", output)
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
            self.assertIn("CLOSE: CTRL-O", output)
            self.assertGreaterEqual(output.count("VIEW: INPUT"), 1)
            self.assertGreaterEqual(output.count("PACKAGE: TOOLS"), 1)
            self.assertGreaterEqual(output.count("VIEW: SOURCE"), 1)
            self.assertNotIn("panic:", output)

    def test_workspace_edit_current_opens_the_interactive_editor_from_package_browser(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-edit-current-package-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_EDIT_CURRENT_PACKAGE_EXAMPLE)
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
                output = _read_until(process, "TESTS: CTRL-T", timeout=8.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("\x14")
                process.stdin.flush()
                output += _read_until(process, "STATUS: TESTS COMPLETE", timeout=8.0)
                process.stdin.write("\x0f")
                process.stdin.flush()
                time.sleep(0.5)
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
            self.assertIn("PASS TinySpec>>testPass", output)
            self.assertIn("FAIL TinySpec>>testFail", output)
            self.assertIn("SUMMARY Tests P=1 F=1 T=2", output)
            self.assertIn("PACKAGE: TESTS", output)
            self.assertNotIn("panic:", output)

    def test_workspace_edit_current_opens_the_interactive_editor_from_class_browser(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-edit-current-class-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_EDIT_CURRENT_CLASS_EXAMPLE)
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
                output = _read_until(process, "STATUS: CLASS SOURCE :: SOURCE EDITOR READY", timeout=12.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("\x18\x0f")
                process.stdin.flush()
                output += _read_until(process, "INSTALL COMPLETE", timeout=12.0)
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
            self.assertIn("INSTALL COMPLETE", output)
            self.assertIn("CLASS: Display", output)
            self.assertIn("VIEW: SOURCE", output)
            self.assertNotIn("panic:", output)

    def test_workspace_edit_current_opens_the_interactive_editor_from_method_list_browser(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-edit-current-method-list-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_EDIT_CURRENT_METHOD_LIST_EXAMPLE)
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
                output = _read_until(process, "RecorzKernelClass: #Display", timeout=8.0)
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
            self.assertIn("CLASS: DISPLAY", output)
            self.assertIn("METHODS", output)
            self.assertIn("VIEW: INPUT", output)
            self.assertNotIn("panic:", output)

    def test_workspace_edit_current_opens_the_interactive_editor_from_protocol_browser(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-edit-current-protocol-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_EDIT_CURRENT_PROTOCOL_EXAMPLE)
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
                output = _read_until(process, "STATUS: CLASS SOURCE :: SOURCE EDITOR READY", timeout=12.0)
                if process.stdin is None:
                    self.fail("QEMU process stdin is not available")
                process.stdin.write("\x0f")
                process.stdin.flush()
                output += _read_until(process, "STATUS: CLASS SOURCE", timeout=8.0)
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
            self.assertIn("CLASS: Grouped", output)
            self.assertIn("PROTO: accessing", output)
            self.assertIn("VIEW: INPUT", output)
            self.assertNotIn("panic:", output)

    def test_workspace_file_out_class_named_renders_the_static_browser_without_panicking(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-class-file-out-browser-") as temp_dir:
            temp_path = Path(temp_dir)
            example_path = temp_path / "workspace_class_file_out_browser.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        "Transcript show: 'MARK A'.",
                        "Transcript cr.",
                        "Workspace fileOutClassNamed: 'Display'.",
                        "Transcript show: 'MARK B'.",
                        "Transcript cr.",
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
                    "-monitor",
                    "none",
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
                output, _ = process.communicate(timeout=12.0)
            except subprocess.TimeoutExpired:
                process.kill()
                output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            normalized_output = output.replace("\r", "")
            self.assertIn("MARK A", normalized_output)
            self.assertIn("MARK B", normalized_output)
            self.assertIn("CLASS: Display", normalized_output)
            self.assertIn("VIEW: SOURCE", normalized_output)
            self.assertNotIn("panic:", normalized_output)
            self.assertNotIn("selector is not understood", normalized_output)

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
                    output, _ = process.communicate(timeout=15.0)
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
                "HEAP": 16384,
                "DCLS": 24,
                "NOBJ": 48,
                "MSRC": 512,
                "MSRP": 65536,
                "RSTR": 65536,
                "SSTR": 16384,
                "SNAP": 262144,
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

    def test_dev_profile_automatic_gc_reclaims_reinstalled_method_garbage_before_heap_overflow(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-gc-method-reinstall-") as temp_dir:
            temp_path = Path(temp_dir)
            example_path = temp_path / "automatic_gc_method_reinstall.rz"
            repeated_source = ["RecorzKernelClass: #GcProbe superclass: #Object\n!\n"]
            repeated_source.extend(
                "contents\n    ^'x'\n!\n"
                for _ in range(1024)
            )
            example_path.write_text(
                "KernelInstaller fileInClassChunks: '"
                + "".join(repeated_source).replace("'", "''")
                + "'.\n"
                + "Transcript show: KernelInstaller memoryReport.",
                encoding="utf-8",
            )
            elf_path = _build_elf(
                temp_path / "build",
                example_path,
                profile="dev",
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
                    output, _ = process.communicate(timeout=18.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("MEMORY", output)
            self.assertIn("recorz qemu-riscv32 mvp: created class GcProbe", output)
            self.assertNotIn("panic: object heap overflow", output)
            self.assertNotIn("panic: class method range contains a non-method descriptor", output)
            gc_count_match = re.search(r"GCC (\d+)", output)
            total_reclaimed_match = re.search(r"GCT (\d+)", output)
            heap_match = re.search(r"HEAP (\d+)/(\d+)", output)
            high_water_match = re.search(r"HWM (\d+)/(\d+)", output)
            self.assertIsNotNone(gc_count_match, output)
            self.assertIsNotNone(total_reclaimed_match, output)
            self.assertIsNotNone(heap_match, output)
            self.assertIsNotNone(high_water_match, output)
            self.assertGreater(int(gc_count_match.group(1)), 0, output)
            self.assertGreater(int(total_reclaimed_match.group(1)), 0, output)
            self.assertGreater(int(high_water_match.group(1)), int(heap_match.group(1)), output)

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

    def test_workspace_accept_current_installs_seeded_class_source_and_keeps_class_browser_context(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-workspace-accept-current-class-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, WORKSPACE_ACCEPT_CURRENT_CLASS_SOURCE_EXAMPLE)
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
            self.assertIn("KEDIT", output)
            self.assertIn("CLASS: Display", output)
            self.assertIn("VIEW: SOURCE", output)

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
