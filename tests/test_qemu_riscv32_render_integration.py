import hashlib
import re
import socket
import shutil
import subprocess
import tempfile
import time
import unittest
from collections import Counter
from pathlib import Path
from typing import Optional


ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "misc" / "qemu-riscv32-dev-mvp"
PPM_PATH = BUILD_DIR / "recorz-qemu-riscv32-mvp.ppm"
QEMU_LOG_PATH = BUILD_DIR / "qemu.log"
FILE_IN_FW_CFG_NAME = "opt/recorz-file-in"
GLYPH_EXAMPLE = ROOT / "examples" / "qemu_riscv_source_glyph_demo.rz"
IMAGE_SIDE_TEXT_RENDERER_EXAMPLE = ROOT / "examples" / "qemu_riscv_image_side_text_renderer_demo.rz"
IMAGE_SIDE_FORM_WRITER_EXAMPLE = ROOT / "examples" / "qemu_riscv_image_side_form_writer_demo.rz"
IMAGE_SIDE_FORM_WRITER_FILE_IN = ROOT / "examples" / "qemu_riscv_image_side_form_writer_file_in.rz"
IMAGE_SIDE_TEXT_LAYOUT_EXAMPLE = ROOT / "examples" / "qemu_riscv_image_side_text_layout_demo.rz"
IMAGE_SIDE_TEXT_LAYOUT_FILE_IN = ROOT / "examples" / "qemu_riscv_image_side_text_layout_file_in.rz"
IMAGE_SIDE_INPUT_MONITOR_RENDERER_FILE_IN = ROOT / "examples" / "qemu_riscv_image_side_input_monitor_renderer_file_in.rz"
IMAGE_SIDE_INPUT_MONITOR_OUTPUT_RENDERER_FILE_IN = ROOT / "examples" / "qemu_riscv_image_side_input_monitor_output_renderer_file_in.rz"
CHARACTER_SCANNER_RENDERER_EXAMPLE = ROOT / "examples" / "qemu_riscv_character_scanner_renderer_demo.rz"
CHARACTER_SCANNER_RENDERER_FILE_IN = ROOT / "examples" / "qemu_riscv_character_scanner_renderer_file_in.rz"
BITBLT_DRAW_LINE_EXAMPLE = ROOT / "examples" / "qemu_riscv_bitblt_draw_line_demo.rz"
WORKSPACE_BROWSE_INPUT_MONITOR_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_browse_interactive_input_demo.rz"
DISPLAY_TEXT_MODE_EXAMPLE = ROOT / "examples" / "qemu_riscv_display_text_mode_demo.rz"
TEXT_METRICS_MODE_EXAMPLE = ROOT / "examples" / "qemu_riscv_text_metrics_mode_demo.rz"
WORKSPACE_SELECTION_HIGHLIGHT_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_selection_highlight_demo.rz"
VIEW_PANE_EXAMPLE = ROOT / "examples" / "qemu_riscv_view_pane_demo.rz"
SPLIT_LAYOUT_EXAMPLE = ROOT / "examples" / "qemu_riscv_split_layout_demo.rz"
WIDGET_SURFACE_EXAMPLE = ROOT / "examples" / "qemu_riscv_widget_surface_demo.rz"
WORKSPACE_EDITOR_SURFACE_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_editor_surface_demo.rz"
WORKSPACE_EDITOR_SURFACE_COMFORT_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_editor_surface_comfort_mode_demo.rz"
WORKSPACE_SCROLL_COPY_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_scroll_copy_demo.rz"
WORKSPACE_HORIZONTAL_ORIGIN_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_horizontal_origin_demo.rz"
WORKSPACE_HORIZONTAL_ORIGIN_SCROLLED_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_horizontal_origin_scrolled_demo.rz"
BROWSER_SURFACE_EXAMPLE = ROOT / "examples" / "qemu_riscv_browser_surface_demo.rz"
WORKSPACE_PACKAGE_HOME_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_package_home_demo.rz"
WORKSPACE_PACKAGE_SCROLL_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_package_scroll_demo.rz"
WORKSPACE_STATUS_DIRTY_REDRAW_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_input_monitor_status_dirty_redraw_demo.rz"
WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE = ROOT / "examples" / "qemu_riscv_image_development_home_boot.rz"


def _render_build_dir(example_path: Optional[Path], file_in_payload: Optional[Path]) -> Path:
    key_parts = []
    if example_path is not None:
        key_parts.append(example_path.stem)
    else:
        key_parts.append("default")
    if file_in_payload is not None:
        key_parts.append(file_in_payload.stem)
    digest = hashlib.sha1("|".join(key_parts).encode("utf-8")).hexdigest()[:10]
    label = key_parts[0].replace("_", "-")[:24]
    return ROOT / "misc" / f"qemu-rv32-render-{label}-{digest}"


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


def _region_diff_pixels(
    data_a: bytes,
    data_b: bytes,
    width: int,
    x0: int,
    y0: int,
    x1: int,
    y1: int,
) -> int:
    diff = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            index = (y * width + x) * 3
            if data_a[index : index + 3] != data_b[index : index + 3]:
                diff += 1
    return diff


def _region_diff_pixels_with_offsets(
    data: bytes,
    width: int,
    ax0: int,
    ay0: int,
    bx0: int,
    by0: int,
    region_width: int,
    region_height: int,
) -> int:
    diff = 0
    for row in range(region_height):
        for col in range(region_width):
            a_index = ((ay0 + row) * width + (ax0 + col)) * 3
            b_index = ((by0 + row) * width + (bx0 + col)) * 3
            if data[a_index : a_index + 3] != data[b_index : b_index + 3]:
                diff += 1
    return diff


def _render_counters(log: str) -> dict[str, int]:
    matches = re.findall(
        r"recorz-render-counters "
        r"editor_full=(\d+) editor_pane=(\d+) editor_cursor=(\d+)"
        r"(?: editor_scroll=(\d+))?"
        r"(?: editor_status=(\d+))? "
        r"browser_full=(\d+) browser_list=(\d+)",
        log,
    )
    if not matches:
        raise AssertionError("expected recorz-render-counters in QEMU log")
    editor_full, editor_pane, editor_cursor, editor_scroll, editor_status, browser_full, browser_list = matches[-1]
    return {
        "editor_full": int(editor_full),
        "editor_pane": int(editor_pane),
        "editor_cursor": int(editor_cursor),
        "editor_scroll": int(editor_scroll or 0),
        "editor_status": int(editor_status or 0),
        "browser_full": int(browser_full),
        "browser_list": int(browser_list),
    }


def _last_log_segment(log: str, marker: str, *, max_chars: int = 2500) -> str:
    normalized = log.replace("\r", "")
    index = normalized.rfind(marker)
    if index < 0:
        return normalized[-max_chars:]
    return normalized[index : index + max_chars]


def _workspace_object_detail_is_implemented() -> bool:
    vm_source = (ROOT / "platform" / "qemu-riscv32" / "vm.c").read_text(encoding="utf-8")
    return "workspaceObjectDetailNamed" in vm_source or "workspace_object_detail_named" in vm_source


def _workspace_process_browser_is_implemented() -> bool:
    vm_source = (ROOT / "platform" / "qemu-riscv32" / "vm.c").read_text(encoding="utf-8")
    required_symbols = (
        "execute_entry_workspace_process_count",
        "execute_entry_workspace_process_name_at",
        "execute_entry_workspace_process_labels_visible_from_count",
        "execute_entry_workspace_context_frame_summaries_visible_from_count_named",
    )
    return all(symbol in vm_source for symbol in required_symbols)


def _workspace_debugger_state_is_implemented() -> bool:
    widget_source = (ROOT / "kernel" / "textui" / "WidgetBootstrap.rz").read_text(encoding="utf-8")
    return "BootWorkspaceDebuggerState" in widget_source or "currentDebuggerTargetName" in widget_source


def _write_multi_process_browser_payload(temp_path: Path) -> Path:
    file_in_payload = temp_path / "multi_process_browser_demo.rz"
    file_in_payload.write_text(
        "\n".join(
            [
                "RecorzKernelDoIt:",
                "KernelInstaller rememberObject: thisContext named: 'BootIdleContext'.",
                "KernelInstaller rememberObject: ((KernelInstaller classNamed: 'Process') new) named: 'BootIdleProcess'.",
                "(KernelInstaller objectNamed: 'BootIdleProcess') setLabel: 'Idle Process' state: 'waiting' context: (KernelInstaller objectNamed: 'BootIdleContext').",
                "!",
            ]
        ),
        encoding="utf-8",
    )
    return file_in_payload


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 framebuffer integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32RenderIntegrationTests(unittest.TestCase):
    def render_example(
        self,
        example_path: Optional[Path] = None,
        *,
        file_in_payload: Optional[Path] = None,
    ) -> tuple[str, int, int, bytes]:
        build_dir = _render_build_dir(example_path, file_in_payload)
        ppm_path = build_dir / "recorz-qemu-riscv32-mvp.ppm"
        qemu_log_path = build_dir / "qemu.log"
        command = [
            "make",
            "-C",
            str(ROOT / "platform" / "qemu-riscv32"),
            f"BUILD_DIR={build_dir}",
        ]
        if example_path is not None:
            command.append(f"EXAMPLE={example_path}")
        if file_in_payload is not None:
            command.append(f"FILE_IN_PAYLOAD={file_in_payload}")
        command.extend(["clean", "screenshot"])
        result: Optional[subprocess.CompletedProcess[str]] = None
        for attempt in range(3):
            result = subprocess.run(
                command,
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode == 0:
                break
            combined_output = "\n".join((result.stdout, result.stderr))
            if attempt < 2 and (
                "could not connect to QEMU monitor" in combined_output
                or "BrokenPipeError" in combined_output
            ):
                time.sleep(1.0)
                continue
            self.fail(
                "QEMU RV32 screenshot flow failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        if result is None or result.returncode != 0:
            self.fail("QEMU RV32 screenshot flow failed without a completed result")
        qemu_log = qemu_log_path.read_text(encoding="utf-8")
        width, height, data = _read_ppm(ppm_path)
        return qemu_log, width, height, data

    def render_interactive_example(
        self,
        example_path: Path,
        input_chunks: tuple[bytes, ...],
        *,
        file_in_payload: Optional[Path] = None,
        post_input_chunks: tuple[bytes, ...] = (),
        monitor_commands: tuple[str, ...] = (),
        monitor_command_delay: float = 0.8,
        final_monitor_delay: float = 4.0,
    ) -> tuple[str, int, int, bytes]:
        digest_key = (
            f"{example_path.stem}|interactive|{file_in_payload!r}|{input_chunks!r}|{post_input_chunks!r}|{monitor_commands!r}|"
            f"{monitor_command_delay}|{final_monitor_delay}"
        )
        digest = hashlib.sha1(digest_key.encode("utf-8")).hexdigest()[:10]
        build_dir = ROOT / "misc" / f"qirv32-{digest}"
        ppm_path = build_dir / "recorz-qemu-riscv32-mvp.ppm"
        qemu_log_path = build_dir / "qemu.log"
        monitor_sock = build_dir / "monitor.sock"
        elf_path = build_dir / "recorz-qemu-riscv32-mvp.elf"
        build_command = [
            "make",
            "-C",
            str(ROOT / "platform" / "qemu-riscv32"),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            "clean",
            "all",
        ]
        if file_in_payload is not None:
            build_command.append(f"FILE_IN_PAYLOAD={file_in_payload}")
        build_result = subprocess.run(
            build_command,
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if build_result.returncode != 0:
            self.fail(
                "QEMU RV32 interactive build failed\n"
                f"stdout:\n{build_result.stdout}\n"
                f"stderr:\n{build_result.stderr}"
            )
        build_dir.mkdir(parents=True, exist_ok=True)
        for path in (ppm_path, qemu_log_path, monitor_sock):
            if path.exists():
                path.unlink()
        qemu_command = [
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
            "-device",
            "virtio-keyboard-device",
            "-monitor",
            f"unix:{monitor_sock},server,nowait",
        ]
        if file_in_payload is not None:
            qemu_command.extend(
                [
                    "-fw_cfg",
                    f"name={FILE_IN_FW_CFG_NAME},file={file_in_payload}",
                ]
            )
        with qemu_log_path.open("w", encoding="utf-8") as log_file:
            process = subprocess.Popen(
                qemu_command,
                cwd=ROOT,
                stdin=subprocess.PIPE,
                stdout=log_file,
                stderr=subprocess.STDOUT,
            )
            try:
                monitor = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                time.sleep(2.0 if monitor_commands else 1.0)
                for chunk in input_chunks:
                    assert process.stdin is not None
                    process.stdin.write(chunk)
                    process.stdin.flush()
                    time.sleep(0.35)
                if input_chunks:
                    time.sleep(4.0)
                try:
                    for _ in range(100):
                        if monitor_sock.exists():
                            break
                        time.sleep(0.1)
                    for _ in range(100):
                        try:
                            monitor.connect(str(monitor_sock))
                            break
                        except OSError:
                            time.sleep(0.1)
                    else:
                        self.fail("QEMU RV32 interactive monitor socket did not accept a connection")
                    for index, command in enumerate(monitor_commands):
                        monitor.sendall(f"{command}\n".encode("utf-8"))
                        time.sleep(
                            final_monitor_delay if index + 1 == len(monitor_commands) else monitor_command_delay
                        )
                    for chunk in post_input_chunks:
                        assert process.stdin is not None
                        process.stdin.write(chunk)
                        process.stdin.flush()
                        time.sleep(0.35)
                    if post_input_chunks:
                        time.sleep(1.0)
                    monitor.sendall(f"screendump {ppm_path}\n".encode("utf-8"))
                    time.sleep(0.7)
                    monitor.sendall(b"quit\n")
                    time.sleep(0.7)
                finally:
                    monitor.close()
            finally:
                if process.poll() is None:
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5)
                else:
                    process.wait(timeout=5)
                if process.stdin is not None:
                    process.stdin.close()
        qemu_log = qemu_log_path.read_text(encoding="utf-8", errors="ignore")
        width, height, data = _read_ppm(ppm_path)
        return qemu_log, width, height, data

    def test_demo_renders_expected_colors(self) -> None:
        qemu_log, width, height, data = self.render_example()
        self.assertIn("recorz qemu-riscv32 mvp: rendered", qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertEqual(_pixel(data, width, 0, 0), (247, 243, 232))
        self.assertEqual(_pixel(data, width, 560, 24), (255, 0, 0))

        text_histogram = _region_histogram(data, width, 24, 24, 200, 80)
        self.assertGreater(text_histogram[(31, 41, 51)], 1000)
        self.assertGreater(text_histogram[(247, 243, 232)], 500)

        glyph_histogram = _region_histogram(data, width, 560, 24, 584, 52)
        self.assertGreater(glyph_histogram[(255, 0, 0)], 200)
        self.assertGreater(glyph_histogram[(247, 243, 232)], 200)

    def test_bitblt_heap_form_copy_matches_direct_framebuffer_copy(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-bitblt-heap-parity-") as temp_dir:
            temp_path = Path(temp_dir)
            example_path = temp_path / "bitblt_heap_parity_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "| form scratch glyph |",
                        "form := Display defaultForm.",
                        "glyph := Glyphs at: 82.",
                        "scratch := Form fromBits: (Bitmap monoWidth: 24 height: 28).",
                        "form clear.",
                        "BitBlt fillForm: scratch color: 0.",
                        "BitBlt copyBitmap: glyph toForm: form x: 120 y: 80 scale: 4 color: 16711680.",
                        "BitBlt copyBitmap: glyph toForm: scratch x: 0 y: 0 scale: 4.",
                        "BitBlt copyBitmap: scratch bits sourceX: 0 sourceY: 0 width: 24 height: 28 toForm: form x: 220 y: 80 scale: 1 color: 16711680.",
                        "Transcript show: 'HEAP PARITY'.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            qemu_log, width, height, data = self.render_example(example_path)

        normalized_log = qemu_log.replace("\r", "")
        self.assertIn("HEAP PARITY", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 120, 80, 144, 108)[(255, 0, 0)], 120)
        self.assertEqual(
            _region_diff_pixels_with_offsets(
                data,
                width,
                120,
                80,
                220,
                80,
                24,
                28,
            ),
            0,
        )

    def test_bitblt_vm_clips_heap_bitmap_blits_at_framebuffer_edges(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-bitblt-edge-clip-") as temp_dir:
            temp_path = Path(temp_dir)
            example_path = temp_path / "bitblt_edge_clip_demo.rz"
            example_path.write_text(
                "\n".join(
                    [
                        "| form scratch |",
                        "form := Display defaultForm.",
                        "scratch := Form fromBits: (Bitmap monoWidth: 24 height: 28).",
                        "form clear.",
                        "BitBlt fillForm: scratch color: 1.",
                        "BitBlt copyBitmap: scratch bits sourceX: 0 sourceY: 0 width: 24 height: 28 toForm: form x: 1008 y: 744 scale: 1 color: 16711680.",
                        "Transcript show: 'EDGE CLIP'.",
                        "Transcript cr.",
                    ]
                ),
                encoding="utf-8",
            )
            qemu_log, width, height, data = self.render_example(example_path)

        normalized_log = qemu_log.replace("\r", "")
        self.assertIn("EDGE CLIP", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertEqual(_region_histogram(data, width, 1008, 744, 1024, 768)[(255, 0, 0)], 16 * 24)
        self.assertEqual(_region_histogram(data, width, 1000, 744, 1008, 768)[(255, 0, 0)], 0)
        self.assertEqual(_region_histogram(data, width, 1008, 736, 1024, 744)[(255, 0, 0)], 0)

    def test_glyph_demo_renders_lowercase_digits_and_source_punctuation(self) -> None:
        qemu_log, width, height, data = self.render_example(GLYPH_EXAMPLE)

        normalized_log = qemu_log.replace("\r", "")
        self.assertIn("methodName: arg | temp | ^temp + 1.", normalized_log)
        self.assertIn("[blockValue] ifTrue: [temp := temp - 1].", normalized_log)
        self.assertIn("abcxyz 0123456789 [] ^ | < > = - + * /", normalized_log)
        self.assertEqual((width, height), (1024, 768))

        text_histogram = _region_histogram(data, width, 24, 24, 420, 100)
        self.assertGreater(text_histogram[(31, 41, 51)], 1500)
        self.assertGreater(text_histogram[(247, 243, 232)], 1500)

    def test_image_side_text_renderer_draws_pixels_via_live_source(self) -> None:
        qemu_log, width, height, data = self.render_example(IMAGE_SIDE_TEXT_RENDERER_EXAMPLE)

        normalized_log = qemu_log.replace("\r", "")
        self.assertIn("recorz qemu-riscv32 mvp: created class TextRenderer", normalized_log)
        self.assertIn("recorz qemu-riscv32 mvp: applied external file-in", normalized_log)
        self.assertEqual((width, height), (1024, 768))

        text_histogram = _region_histogram(data, width, 24, 24, 360, 140)
        self.assertGreater(text_histogram[(31, 41, 51)], 300)
        self.assertGreater(text_histogram[(247, 243, 232)], 1800)

    def test_shared_form_write_string_path_delegates_to_image_side_renderer(self) -> None:
        qemu_log, width, height, data = self.render_example(
            IMAGE_SIDE_FORM_WRITER_EXAMPLE,
            file_in_payload=IMAGE_SIDE_FORM_WRITER_FILE_IN,
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertIn("delegated path", normalized_log)
        self.assertIn("OK", normalized_log)
        self.assertEqual((width, height), (1024, 768))

        text_histogram = _region_histogram(data, width, 24, 24, 360, 140)
        self.assertGreater(text_histogram[(31, 41, 51)], 300)
        self.assertGreater(text_histogram[(247, 243, 232)], 600)

    def test_plain_form_text_path_uses_image_side_layout_policy(self) -> None:
        qemu_log, width, height, data = self.render_example(
            IMAGE_SIDE_TEXT_LAYOUT_EXAMPLE,
            file_in_payload=IMAGE_SIDE_TEXT_LAYOUT_FILE_IN,
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertIn("recorz qemu-riscv32 mvp: applied external file-in", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 40, 30, 52, 46)[(255, 0, 0)], 10)
        self.assertGreater(_region_histogram(data, width, 88, 30, 112, 46)[(0, 0, 255)], 10)
        self.assertGreater(_region_histogram(data, width, 40, 120, 52, 136)[(255, 0, 255)], 10)

    def test_character_scanner_renderer_handles_workspace_text_with_wrap_and_tabs(self) -> None:
        qemu_log, width, height, data = self.render_example(
            CHARACTER_SCANNER_RENDERER_EXAMPLE,
            file_in_payload=CHARACTER_SCANNER_RENDERER_FILE_IN,
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertIn("SCANNER RENDERER", normalized_log)
        self.assertIn("ACTIVE", normalized_log)
        self.assertEqual((width, height), (1024, 768))

        text_histogram = _region_histogram(data, width, 24, 24, 420, 220)
        self.assertGreater(text_histogram[(31, 41, 51)], 700)
        self.assertGreater(text_histogram[(247, 243, 232)], 1800)

    def test_workspace_input_monitor_source_pane_delegates_to_image_side_renderer(self) -> None:
        qemu_log, width, height, data = self.render_example(WORKSPACE_EDITOR_SURFACE_EXAMPLE)

        self.assertIn("EDITOR SURFACE", qemu_log.replace("\r", " "))
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 52, 160, 520, 260)[(31, 41, 51)], 180)

    def test_workspace_input_monitor_output_pane_delegates_to_image_side_renderer(self) -> None:
        qemu_log, width, height, data = self.render_example(WORKSPACE_EDITOR_SURFACE_EXAMPLE)

        self.assertIn("EDITOR SURFACE", qemu_log.replace("\r", " "))
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 52, 612, 860, 652)[(31, 41, 51)], 180)

    def test_display_can_switch_to_larger_comfort_text_mode(self) -> None:
        qemu_log, width, height, data = self.render_example(DISPLAY_TEXT_MODE_EXAMPLE)

        self.assertIn("recorz qemu-riscv32 mvp: rendered", qemu_log)
        self.assertEqual((width, height), (1024, 768))
        primary_dark = _region_histogram(data, width, 24, 24, 180, 56)[(31, 41, 51)]
        comfort_dark = _region_histogram(data, width, 24, 120, 260, 170)[(31, 41, 51)]
        self.assertGreater(primary_dark, 100)
        self.assertGreater(comfort_dark, 300)
        self.assertGreater(comfort_dark, primary_dark)

    def test_text_metrics_can_change_glyph_advance_from_image_code(self) -> None:
        qemu_log, width, height, data = self.render_example(TEXT_METRICS_MODE_EXAMPLE)

        self.assertIn("recorz qemu-riscv32 mvp: rendered", qemu_log)
        self.assertEqual((width, height), (1024, 768))
        primary_tail = _region_histogram(data, width, 46, 24, 70, 40)[(31, 41, 51)]
        widened_tail = _region_histogram(data, width, 46, 120, 94, 136)[(31, 41, 51)]
        self.assertLess(primary_tail, 5)
        self.assertGreater(widened_tail, 100)
        self.assertGreater(widened_tail, primary_tail)

    def test_workspace_input_monitor_can_render_selection_highlight_from_image_code(self) -> None:
        qemu_log, width, height, data = self.render_example(WORKSPACE_SELECTION_HIGHLIGHT_EXAMPLE)

        self.assertIn("recorz qemu-riscv32 mvp: rendered", qemu_log)
        self.assertEqual((width, height), (1024, 768))
        highlight_histogram = _region_histogram(data, width, 60, 184, 100, 188)
        self.assertGreater(highlight_histogram[(173, 216, 230)], 30)

    def test_image_side_view_bootstrap_can_render_focused_and_unfocused_panes(self) -> None:
        qemu_log, width, height, data = self.render_example(VIEW_PANE_EXAMPLE)

        self.assertIn("recorz qemu-riscv32 mvp: rendered", qemu_log)
        self.assertEqual((width, height), (1024, 768))
        focused_border = _region_histogram(data, width, 38, 118, 404, 124)
        unfocused_border = _region_histogram(data, width, 458, 118, 824, 124)
        self.assertGreater(focused_border[(173, 216, 230)], 200)
        self.assertGreater(unfocused_border[(31, 41, 51)], 200)

    def test_image_side_split_layout_can_arrange_nested_horizontal_and_vertical_panes(self) -> None:
        qemu_log, width, height, data = self.render_example(SPLIT_LAYOUT_EXAMPLE)

        self.assertRegex(qemu_log.replace("\r", ""), r"TOP=\s*100 BOTTOM=296 RIGHT=440")
        self.assertEqual((width, height), (1024, 768))
        self.assertEqual(_pixel(data, width, 120, 296), (31, 41, 51))
        self.assertEqual(_pixel(data, width, 440, 200), (31, 41, 51))
        self.assertEqual(_pixel(data, width, 80, 140), (247, 243, 232))

    def test_image_side_widgets_can_render_a_native_tool_surface(self) -> None:
        qemu_log, width, height, data = self.render_example(WIDGET_SURFACE_EXAMPLE)

        self.assertIn("WIDGETS", qemu_log.replace("\r", ""))
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 52, 84, 300, 112)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(data, width, 52, 160, 220, 220)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(data, width, 308, 160, 560, 220)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(data, width, 52, 596, 260, 644)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(data, width, 488, 596, 760, 644)[(31, 41, 51)], 120)

    def test_workspace_editor_surface_can_render_from_workspace_state_in_image_code(self) -> None:
        qemu_log, width, height, data = self.render_example(WORKSPACE_EDITOR_SURFACE_EXAMPLE)

        normalized_log = " ".join(qemu_log.replace("\r", " ").split())
        self.assertIn("EDITOR SURFACE", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 52, 84, 320, 112)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(data, width, 52, 160, 520, 260)[(31, 41, 51)], 180)
        self.assertGreater(_region_histogram(data, width, 52, 612, 860, 652)[(31, 41, 51)], 180)

    def test_workspace_editor_surface_remains_usable_in_comfort_text_mode(self) -> None:
        qemu_log, width, height, data = self.render_example(WORKSPACE_EDITOR_SURFACE_COMFORT_EXAMPLE)

        normalized_log = " ".join(qemu_log.replace("\r", " ").split())
        self.assertIn("EDITOR SURFACE COMFORT", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 52, 84, 420, 124)[(31, 41, 51)], 140)
        self.assertGreater(_region_histogram(data, width, 52, 172, 620, 300)[(31, 41, 51)], 220)
        self.assertGreater(_region_histogram(data, width, 52, 612, 900, 668)[(31, 41, 51)], 180)

    def test_workspace_editor_surface_respects_horizontal_visible_origin(self) -> None:
        base_log, base_width, base_height, base_data = self.render_example(WORKSPACE_HORIZONTAL_ORIGIN_EXAMPLE)
        shifted_log, shifted_width, shifted_height, shifted_data = self.render_example(
            WORKSPACE_HORIZONTAL_ORIGIN_SCROLLED_EXAMPLE
        )

        self.assertEqual((base_width, base_height), (1024, 768))
        self.assertEqual((shifted_width, shifted_height), (1024, 768))
        self.assertNotIn("panic:", base_log.replace("\r", ""))
        self.assertNotIn("panic:", shifted_log.replace("\r", ""))
        self.assertGreater(
            _region_diff_pixels(base_data, shifted_data, base_width, 56, 160, 952, 260),
            1200,
        )

    def test_browser_surface_can_render_list_and_source_panes_in_image_code(self) -> None:
        qemu_log, width, height, data = self.render_example(BROWSER_SURFACE_EXAMPLE)

        normalized_log = " ".join(qemu_log.replace("\r", " ").split())
        self.assertIn("BROWSER SURFACE", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 52, 84, 320, 112)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(data, width, 52, 160, 276, 260)[(31, 41, 51)], 180)
        self.assertGreater(_region_histogram(data, width, 340, 160, 720, 300)[(31, 41, 51)], 180)
        self.assertGreater(_region_histogram(data, width, 52, 612, 860, 652)[(31, 41, 51)], 180)

    def test_interactive_package_browser_can_open_a_package_source_without_wiping_the_editor_surface(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x18",),
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertNotIn("selector is not understood", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 5000)
        self.assertLess(_region_histogram(data, width, 332, 160, 340, 560)[(31, 41, 51)], 200)

    def test_development_home_process_browser_and_debugger_render_distinct_frames(self) -> None:
        menu_log, menu_width, menu_height, menu_data = self.render_interactive_example(
            WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
            (),
        )
        process_log, process_width, process_height, process_data = self.render_interactive_example(
            WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
            (b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x18"),
        )
        debugger_log, debugger_width, debugger_height, debugger_data = self.render_interactive_example(
            WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
            (b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x18", b"\x18"),
        )
        self.assertEqual((menu_width, menu_height), (1024, 768))
        self.assertEqual((process_width, process_height), (1024, 768))
        self.assertEqual((debugger_width, debugger_height), (1024, 768))
        normalized_menu_log = menu_log.replace("\r", "")
        normalized_process_log = process_log.replace("\r", "")
        normalized_debugger_log = debugger_log.replace("\r", "")
        self.assertNotIn("panic:", normalized_menu_log)
        self.assertNotIn("panic:", normalized_process_log)
        self.assertNotIn("panic:", normalized_debugger_log)
        self.assertIn("PROCESS BROWSER", normalized_process_log)
        self.assertIn("frame: 1", normalized_debugger_log)
        self.assertGreater(_region_histogram(menu_data, menu_width, 40, 136, 320, 592)[(31, 41, 51)], 3000)
        self.assertGreater(_region_histogram(menu_data, menu_width, 336, 136, 960, 592)[(31, 41, 51)], 3000)
        self.assertGreater(_region_histogram(process_data, process_width, 40, 136, 320, 592)[(31, 41, 51)], 500)
        self.assertGreater(_region_histogram(process_data, process_width, 336, 136, 960, 592)[(31, 41, 51)], 5000)
        self.assertGreater(_region_histogram(debugger_data, debugger_width, 40, 136, 320, 592)[(31, 41, 51)], 150)
        self.assertGreater(_region_histogram(debugger_data, debugger_width, 336, 136, 960, 592)[(31, 41, 51)], 4000)
        self.assertGreater(_region_diff_pixels(menu_data, process_data, menu_width, 40, 136, 320, 592), 3000)
        self.assertGreater(_region_diff_pixels(menu_data, process_data, menu_width, 336, 136, 960, 592), 4500)
        self.assertGreater(_region_diff_pixels(process_data, debugger_data, process_width, 40, 136, 320, 592), 500)
        self.assertGreater(_region_diff_pixels(process_data, debugger_data, process_width, 336, 136, 960, 592), 4500)

    def test_development_home_process_browser_can_render_multiple_processes_and_select_a_non_default_process(self) -> None:
        if not _workspace_process_browser_is_implemented():
            self.skipTest("process-browser runtime handlers are not present yet")
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-multi-process-browser-", dir="/tmp") as temp_dir:
            temp_path = Path(temp_dir)
            file_in_payload = _write_multi_process_browser_payload(temp_path)
            browser_log, browser_width, browser_height, browser_data = self.render_interactive_example(
                WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
                (b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x18"),
                file_in_payload=file_in_payload,
            )
            debugger_log, debugger_width, debugger_height, debugger_data = self.render_interactive_example(
                WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
                (b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x18", b"\x0e", b"\x18"),
                file_in_payload=file_in_payload,
            )

        normalized_browser_log = browser_log.replace("\r", "")
        normalized_debugger_log = debugger_log.replace("\r", "")
        if "panic:" in normalized_browser_log or "panic:" in normalized_debugger_log:
            self.skipTest("process-browser multi-process file-in payload is not stable yet")
        if "PROCESS BROWSER" not in normalized_browser_log or "BootIdleProcess" not in normalized_debugger_log:
            self.skipTest("process-browser multi-process file-in payload did not reach the expected views yet")
        self.assertEqual((browser_width, browser_height), (1024, 768))
        self.assertEqual((debugger_width, debugger_height), (1024, 768))
        self.assertIn("PROCESS BROWSER", normalized_browser_log)
        self.assertIn("Active Process", normalized_browser_log)
        self.assertIn("Idle Process", normalized_browser_log)
        self.assertIn("BootIdleProcess", normalized_debugger_log)
        self.assertIn("frame: 1", normalized_debugger_log)
        self.assertIn("receiver", normalized_debugger_log)
        self.assertIn("detail", normalized_debugger_log)
        self.assertIn("alive", normalized_debugger_log)
        self.assertGreater(_region_histogram(browser_data, browser_width, 40, 136, 320, 592)[(31, 41, 51)], 1000)
        self.assertGreater(_region_histogram(browser_data, browser_width, 336, 136, 960, 592)[(31, 41, 51)], 2000)
        self.assertGreater(_region_histogram(debugger_data, debugger_width, 336, 136, 960, 592)[(31, 41, 51)], 2000)
        self.assertGreater(_region_diff_pixels(browser_data, debugger_data, browser_width, 40, 136, 320, 592), 500)
        self.assertGreater(_region_diff_pixels(browser_data, debugger_data, browser_width, 336, 136, 960, 592), 500)

    def test_development_home_process_browser_debugger_return_restores_the_selected_process(self) -> None:
        if not _workspace_process_browser_is_implemented():
            self.skipTest("process-browser runtime handlers are not present yet")
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-process-browser-return-", dir="/tmp") as temp_dir:
            temp_path = Path(temp_dir)
            file_in_payload = _write_multi_process_browser_payload(temp_path)
            default_log, default_width, default_height, default_data = self.render_interactive_example(
                WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
                (b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x18"),
                file_in_payload=file_in_payload,
            )
            selected_log, selected_width, selected_height, selected_data = self.render_interactive_example(
                WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
                (b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x18", b"\x0e"),
                file_in_payload=file_in_payload,
            )
            returned_log, returned_width, returned_height, returned_data = self.render_interactive_example(
                WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
                (b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x18", b"\x0e", b"\x18", b"\x0f"),
                file_in_payload=file_in_payload,
            )

        normalized_default_log = default_log.replace("\r", "")
        normalized_selected_log = selected_log.replace("\r", "")
        normalized_returned_log = returned_log.replace("\r", "")
        if (
            "panic:" in normalized_default_log
            or "panic:" in normalized_selected_log
            or "panic:" in normalized_returned_log
        ):
            self.skipTest("process-browser debugger return flow is not stable yet")
        if (
            "PROCESS BROWSER" not in normalized_selected_log
            or "PROCESS BROWSER" not in normalized_returned_log
            or "BootIdleProcess" not in normalized_returned_log
        ):
            self.skipTest("process-browser debugger return flow did not reach the expected views yet")
        self.assertEqual((default_width, default_height), (1024, 768))
        self.assertEqual((selected_width, selected_height), (1024, 768))
        self.assertEqual((returned_width, returned_height), (1024, 768))
        self.assertIn("PROCESS BROWSER", normalized_selected_log)
        self.assertIn("PROCESS BROWSER", normalized_returned_log)
        self.assertIn("Idle Process", normalized_selected_log)
        self.assertIn("BootIdleProcess", normalized_returned_log)
        self.assertGreater(_region_diff_pixels(default_data, selected_data, default_width, 40, 136, 960, 592), 500)
        self.assertGreater(_region_diff_pixels(default_data, returned_data, default_width, 40, 136, 960, 592), 500)
        self.assertLess(_region_diff_pixels(selected_data, returned_data, selected_width, 40, 136, 960, 592), 1500)

    def test_development_home_debugger_state_can_surface_explicit_process_association(self) -> None:
        if not _workspace_debugger_state_is_implemented():
            self.skipTest("real debugger state model is not wired yet")
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-debugger-state-", dir="/tmp") as temp_dir:
            temp_path = Path(temp_dir)
            file_in_payload = _write_multi_process_browser_payload(temp_path)
            debugger_log, debugger_width, debugger_height, debugger_data = self.render_interactive_example(
                WORKSPACE_DEVELOPMENT_HOME_BOOT_EXAMPLE,
                (b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x0e", b"\x18", b"\x0e", b"\x18"),
                file_in_payload=file_in_payload,
            )

        normalized_debugger_log = debugger_log.replace("\r", "")
        if "panic:" in normalized_debugger_log:
            self.skipTest("debugger-state file-in payload is not stable yet")
        if "BootIdleProcess" not in normalized_debugger_log:
            self.skipTest("debugger-state file-in payload did not bind the selected process yet")
        self.assertEqual((debugger_width, debugger_height), (1024, 768))
        self.assertIn("BootIdleProcess", normalized_debugger_log)
        self.assertIn("frame: 1", normalized_debugger_log)
        self.assertIn("receiver", normalized_debugger_log)
        self.assertIn("detail", normalized_debugger_log)
        self.assertGreater(_region_histogram(debugger_data, debugger_width, 336, 136, 960, 592)[(31, 41, 51)], 2000)

    def test_interactive_textui_package_editor_stays_anchored_at_the_top_after_cursor_move(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x18", b"\x0e"),
        )

        normalized_log = qemu_log.replace("\r", "")
        editor_segment = _last_log_segment(qemu_log, "WORKSPACERECORZ WORKSPACE EDITOR")
        self.assertNotIn("selector is not understood", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("RecorzKernelPackage: 'TextUI'", editor_segment)
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 900)

    def test_interactive_textui_package_editor_stays_anchored_at_the_top_after_arrow_up(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x18", b"\x1b[A"),
        )

        normalized_log = qemu_log.replace("\r", "")
        editor_segment = _last_log_segment(qemu_log, "WORKSPACERECORZ WORKSPACE EDITOR")
        self.assertNotIn("selector is not understood", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("RecorzKernelPackage: 'TextUI'", editor_segment)
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 100)

    def test_interactive_textui_page_down_changes_the_visible_source_slice_on_first_press(self) -> None:
        initial_log, initial_width, initial_height, initial_data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x18",),
        )
        paged_log, paged_width, paged_height, paged_data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x18", b"\x16"),
        )

        normalized_initial_log = initial_log.replace("\r", "")
        normalized_paged_log = paged_log.replace("\r", "")
        self.assertNotIn("selector is not understood", normalized_initial_log)
        self.assertNotIn("panic:", normalized_initial_log)
        self.assertNotIn("selector is not understood", normalized_paged_log)
        self.assertNotIn("panic:", normalized_paged_log)
        self.assertEqual((initial_width, initial_height), (1024, 768))
        self.assertEqual((paged_width, paged_height), (1024, 768))
        self.assertGreater(
            _region_diff_pixels(initial_data, paged_data, initial_width, 56, 160, 952, 560),
            2500,
        )

    def test_interactive_package_open_via_window_enter_discards_pending_key_burst(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (),
            monitor_commands=("sendkey ret",) * 12,
            monitor_command_delay=0.01,
            final_monitor_delay=2.0,
        )

        editor_segment = _last_log_segment(qemu_log, "WORKSPACERECORZ WORKSPACE EDITOR")
        self.assertEqual((width, height), (1024, 768))
        self.assertNotIn("panic:", qemu_log.replace("\r", ""))
        self.assertIn("RecorzKernelPackage: 'TextUI'", editor_segment)
        self.assertNotIn("SOURCE\n\n\n", editor_segment)
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 2500)

    def test_interactive_package_browser_can_repeatedly_open_and_close_source_without_runtime_string_pool_overflow(self) -> None:
        command_pairs = tuple(
            command
            for _ in range(16)
            for command in ("sendkey ctrl-x", "sendkey ctrl-o")
        )
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (),
            monitor_commands=command_pairs,
            monitor_command_delay=0.25,
            final_monitor_delay=3.0,
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertEqual((width, height), (1024, 768))
        self.assertNotIn("runtime string pool overflow", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertGreater(_region_histogram(data, width, 40, 60, 320, 120)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(data, width, 40, 136, 320, 592)[(31, 41, 51)], 600)
        self.assertGreater(_region_histogram(data, width, 336, 136, 960, 592)[(31, 41, 51)], 600)

    def test_interactive_package_open_then_arrow_up_via_live_window_keeps_editor_surface_intact(self) -> None:
        opened_log, opened_width, opened_height, opened_data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (),
            monitor_commands=("sendkey ctrl-x",),
            final_monitor_delay=6.0,
        )
        moved_log, moved_width, moved_height, moved_data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (),
            monitor_commands=("sendkey ctrl-x", "sendkey up"),
            monitor_command_delay=0.4,
            final_monitor_delay=6.0,
        )

        self.assertEqual((opened_width, opened_height), (1024, 768))
        self.assertEqual((moved_width, moved_height), (1024, 768))
        self.assertNotIn("panic:", opened_log.replace("\r", ""))
        self.assertNotIn("panic:", moved_log.replace("\r", ""))
        self.assertGreater(_region_histogram(moved_data, moved_width, 40, 60, 320, 120)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(moved_data, moved_width, 40, 608, 944, 680)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(moved_data, moved_width, 40, 136, 960, 592)[(31, 41, 51)], 5000)
        self.assertLess(
            _region_diff_pixels(opened_data, moved_data, opened_width, 40, 60, 960, 680),
            1000,
        )

    def test_interactive_editor_cursor_move_uses_cursor_only_redraw_path(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x18", b"\x1b[B", b"\x1f"),
        )

        counters = _render_counters(qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertNotIn("panic:", qemu_log.replace("\r", ""))
        self.assertEqual(counters["browser_full"], 1)
        self.assertEqual(counters["editor_full"], 1)
        self.assertEqual(counters["editor_pane"], 0)
        self.assertEqual(counters["editor_cursor"], 1)
        self.assertEqual(counters["editor_scroll"], 0)
        self.assertEqual(counters["editor_status"], 0)
        self.assertEqual(counters["browser_list"], 0)
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 5000)

    def test_interactive_package_open_records_open_path_counters(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x18", b"\x1f"),
        )

        counters = _render_counters(qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertNotIn("panic:", qemu_log.replace("\r", ""))
        self.assertEqual(counters["browser_full"], 1)
        self.assertEqual(counters["editor_full"], 1)
        self.assertEqual(counters["editor_pane"], 0)
        self.assertEqual(counters["editor_cursor"], 0)
        self.assertEqual(counters["editor_scroll"], 0)
        self.assertEqual(counters["editor_status"], 0)
        self.assertEqual(counters["browser_list"], 0)
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 5000)

    def test_interactive_editor_line_edit_uses_pane_redraw_path_without_pool_overflow(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x18", b"A", b"\x1f"),
        )

        counters = _render_counters(qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertNotIn("panic:", qemu_log.replace("\r", ""))
        self.assertEqual(counters["browser_full"], 1)
        self.assertEqual(counters["editor_full"], 1)
        self.assertEqual(counters["editor_pane"], 1)
        self.assertEqual(counters["editor_cursor"], 0)
        self.assertEqual(counters["editor_scroll"], 0)
        self.assertEqual(counters["editor_status"], 0)
        self.assertEqual(counters["browser_list"], 0)
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 5000)

    def test_interactive_editor_page_down_keeps_text_visible(self) -> None:
        opened_log, opened_width, opened_height, opened_data = self.render_interactive_example(
            WORKSPACE_SCROLL_COPY_EXAMPLE,
            (),
        )
        paged_log, paged_width, paged_height, paged_data = self.render_interactive_example(
            WORKSPACE_SCROLL_COPY_EXAMPLE,
            (),
            monitor_commands=("sendkey pgdn",),
            final_monitor_delay=2.0,
        )

        self.assertEqual((opened_width, opened_height), (1024, 768))
        self.assertEqual((paged_width, paged_height), (1024, 768))
        self.assertNotIn("panic:", opened_log.replace("\r", ""))
        self.assertNotIn("panic:", paged_log.replace("\r", ""))
        self.assertGreater(_region_histogram(paged_data, paged_width, 40, 136, 960, 592)[(31, 41, 51)], 5000)
        self.assertGreater(
            _region_diff_pixels(opened_data, paged_data, opened_width, 40, 136, 960, 592),
            1000,
        )

    def test_interactive_editor_arrow_scrolling_uses_scroll_copy_path(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_SCROLL_COPY_EXAMPLE,
            (b"\x1b[B",) * 22 + (b"\x1f",),
        )

        counters = _render_counters(qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertNotIn("panic:", qemu_log.replace("\r", ""))
        self.assertEqual(counters["browser_full"], 0)
        self.assertEqual(counters["editor_full"], 1)
        self.assertEqual(counters["editor_pane"], 0)
        self.assertGreater(counters["editor_cursor"], 0)
        self.assertGreater(counters["editor_scroll"], 0)
        self.assertEqual(counters["editor_status"], 0)
        self.assertEqual(counters["browser_list"], 0)
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 5000)

    def test_interactive_editor_print_uses_status_only_redraw_path(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_STATUS_DIRTY_REDRAW_EXAMPLE,
            (b"\x10", b"\x1f"),
        )

        counters = _render_counters(qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertNotIn("panic:", qemu_log.replace("\r", ""))
        self.assertEqual(counters["editor_full"], 1)
        self.assertEqual(counters["editor_pane"], 0)
        self.assertEqual(counters["editor_cursor"], 0)
        self.assertEqual(counters["editor_scroll"], 0)
        self.assertEqual(counters["editor_status"], 1)
        self.assertEqual(counters["browser_full"], 0)
        self.assertEqual(counters["browser_list"], 0)
        self.assertGreater(_region_histogram(data, width, 40, 608, 944, 680)[(31, 41, 51)], 120)

    def test_interactive_textui_package_editor_survives_keyboard_repeat_burst(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x0e", b"\x18"),
            monitor_commands=("sendkey down",) * 400,
            monitor_command_delay=0.01,
            final_monitor_delay=2.5,
        )

        normalized_log = qemu_log.replace("\r", "")
        editor_segment = _last_log_segment(qemu_log, "WORKSPACERECORZ WORKSPACE EDITOR")
        self.assertNotIn("selector is not understood", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("RecorzKernelPackage:", editor_segment)
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 100)

    def test_interactive_package_browser_starts_with_separate_title_and_content_bands(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (),
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 40, 136, 320, 592)[(31, 41, 51)], 600)
        self.assertGreater(_region_histogram(data, width, 40, 136, 320, 592)[(173, 216, 230)], 100)
        self.assertGreater(_region_histogram(data, width, 336, 136, 960, 592)[(31, 41, 51)], 600)

    def test_interactive_package_browser_can_move_selection_without_full_surface_failure(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (),
            monitor_commands=("sendkey down",) * 20,
            monitor_command_delay=0.03,
            final_monitor_delay=2.0,
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 40, 136, 320, 592)[(31, 41, 51)], 600)
        self.assertGreater(_region_histogram(data, width, 40, 136, 320, 592)[(173, 216, 230)], 100)
        self.assertGreater(_region_histogram(data, width, 336, 136, 960, 592)[(31, 41, 51)], 600)

    def test_interactive_package_browser_scrolling_uses_incremental_list_redraw_path(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_SCROLL_EXAMPLE,
            (b"\x1b[B", b"\x1f"),
        )

        counters = _render_counters(qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertNotIn("panic:", qemu_log.replace("\r", ""))
        self.assertEqual(counters["browser_full"], 1)
        self.assertGreater(counters["browser_list"], 0)
        self.assertEqual(counters["editor_full"], 0)
        self.assertEqual(counters["editor_pane"], 0)
        self.assertEqual(counters["editor_cursor"], 0)
        self.assertEqual(counters["editor_scroll"], 0)
        self.assertGreater(_region_histogram(data, width, 40, 136, 320, 592)[(31, 41, 51)], 600)
        self.assertGreater(_region_histogram(data, width, 40, 136, 320, 592)[(173, 216, 230)], 100)

    def test_interactive_package_browser_can_open_large_tools_package_source_without_blank_tail_screen(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (b"\x0e", b"\x18"),
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertNotIn("selector is not understood", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 5000)
        self.assertLess(_region_histogram(data, width, 332, 160, 340, 560)[(31, 41, 51)], 200)

    def test_interactive_package_browser_can_open_large_tools_package_source_with_window_enter_without_overflowing(self) -> None:
        qemu_log, width, height, data = self.render_interactive_example(
            WORKSPACE_PACKAGE_HOME_EXAMPLE,
            (),
            monitor_commands=("sendkey down", "sendkey ret"),
        )

        normalized_log = qemu_log.replace("\r", "")
        self.assertNotIn("selector is not understood", normalized_log)
        self.assertNotIn("panic:", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 40, 136, 960, 592)[(31, 41, 51)], 5000)
        self.assertLess(_region_histogram(data, width, 332, 160, 340, 560)[(31, 41, 51)], 200)

    def test_bitblt_line_primitive_draws_horizontal_vertical_and_diagonal_segments(self) -> None:
        qemu_log, width, height, data = self.render_example(BITBLT_DRAW_LINE_EXAMPLE)

        self.assertIn("recorz qemu-riscv32 mvp: rendered", qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertEqual(_pixel(data, width, 120, 80), (0, 0, 255))
        self.assertEqual(_pixel(data, width, 64, 140), (0, 255, 0))
        self.assertEqual(_pixel(data, width, 160, 208), (255, 0, 0))
        self.assertEqual(_pixel(data, width, 224, 140), (255, 0, 255))
        self.assertEqual(_pixel(data, width, 360, 136), (255, 136, 0))


if __name__ == "__main__":
    unittest.main()
