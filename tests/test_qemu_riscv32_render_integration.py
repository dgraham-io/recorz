import hashlib
import shutil
import subprocess
import time
import unittest
from collections import Counter
from pathlib import Path
from typing import Optional


ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "misc" / "qemu-riscv32-dev-mvp"
PPM_PATH = BUILD_DIR / "recorz-qemu-riscv32-mvp.ppm"
QEMU_LOG_PATH = BUILD_DIR / "qemu.log"
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
BROWSER_SURFACE_EXAMPLE = ROOT / "examples" / "qemu_riscv_browser_surface_demo.rz"


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
        qemu_log, width, height, data = self.render_example(IMAGE_SIDE_FORM_WRITER_EXAMPLE)

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
        qemu_log, width, height, data = self.render_example(
            WORKSPACE_BROWSE_INPUT_MONITOR_EXAMPLE,
            file_in_payload=IMAGE_SIDE_INPUT_MONITOR_RENDERER_FILE_IN,
        )

        self.assertIn("recorz qemu-riscv32 mvp: rendered", qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 160, 210, 204, 226)[(0, 0, 255)], 20)

    def test_workspace_input_monitor_output_pane_delegates_to_image_side_renderer(self) -> None:
        qemu_log, width, height, data = self.render_example(
            WORKSPACE_BROWSE_INPUT_MONITOR_EXAMPLE,
            file_in_payload=IMAGE_SIDE_INPUT_MONITOR_OUTPUT_RENDERER_FILE_IN,
        )

        self.assertIn("recorz qemu-riscv32 mvp: rendered", qemu_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 160, 520, 200, 536)[(255, 0, 0)], 20)

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
        highlight_histogram = _region_histogram(data, width, 168, 224, 216, 232)
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

    def test_browser_surface_can_render_list_and_source_panes_in_image_code(self) -> None:
        qemu_log, width, height, data = self.render_example(BROWSER_SURFACE_EXAMPLE)

        normalized_log = " ".join(qemu_log.replace("\r", " ").split())
        self.assertIn("BROWSER SURFACE", normalized_log)
        self.assertEqual((width, height), (1024, 768))
        self.assertGreater(_region_histogram(data, width, 52, 84, 320, 112)[(31, 41, 51)], 120)
        self.assertGreater(_region_histogram(data, width, 52, 160, 276, 260)[(31, 41, 51)], 180)
        self.assertGreater(_region_histogram(data, width, 340, 160, 720, 300)[(31, 41, 51)], 180)
        self.assertGreater(_region_histogram(data, width, 52, 612, 860, 652)[(31, 41, 51)], 180)

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
