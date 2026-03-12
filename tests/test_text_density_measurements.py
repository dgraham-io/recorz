from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DISPLAY_HEADER = ROOT / "platform" / "qemu-riscv32" / "display.h"
TEXT_RENDERER_BOOTSTRAP = ROOT / "kernel" / "textui" / "TextRendererBootstrap.rz"
TEXT_METRICS_SOURCE = ROOT / "kernel" / "mvp" / "TextMetrics.rz"
TEXT_VERTICAL_METRICS_SOURCE = ROOT / "kernel" / "mvp" / "TextVerticalMetrics.rz"
WIDGET_BOOTSTRAP = ROOT / "kernel" / "textui" / "WidgetBootstrap.rz"


def _parse_macro(text: str, name: str) -> int:
    match = re.search(rf"#define {name} (\d+)U", text)
    if match is None:
        raise AssertionError(f"missing macro {name}")
    return int(match.group(1))


def _parse_mode(text: str, selector: str) -> tuple[int, int, int]:
    pattern = (
        rf"{selector}\n"
        r"    Display font setPointSize: (\d+)\.\n"
        r"    Display layout scale: (\d+) lineSpacing: \d+\.\n"
        r"    Display layout margins left: (\d+) top: \d+ right: \d+ bottom: \d+\."
    )
    match = re.search(pattern, text)
    if match is None:
        raise AssertionError(f"missing text mode {selector}")
    return int(match.group(1)), int(match.group(2)), int(match.group(3))


def _parse_boot_field_values(text: str, boot_object: str) -> tuple[int, ...]:
    match = re.search(rf"RecorzKernelBootObject: #{boot_object}.*?\nfields: '([^']+)'", text, re.DOTALL)
    if match is None:
        raise AssertionError(f"missing boot object {boot_object}")
    values: list[int] = []
    for field in match.group(1).split():
        if not field.startswith("small:"):
            continue
        values.append(int(field.removeprefix("small:")))
    return tuple(values)


def _parse_workspace_source_bounds(text: str) -> tuple[int, int]:
    match = re.search(
        r"rect setLeft: 40 top: 136 width: (\d+) height: (\d+)\.\n"
        r"view := \(KernelInstaller classNamed: 'View'\) new\.\n"
        r"view setBounds: rect title: 'SOURCE'\.\n"
        r"KernelInstaller rememberObject: view named: 'BootWorkspaceSourceView'\.",
        text,
    )
    if match is None:
        raise AssertionError("missing BootWorkspaceSourceView bounds")
    return int(match.group(1)), int(match.group(2))


class TextDensityMeasurementTests(unittest.TestCase):
    def test_primary_and_comfort_text_density_on_qemu_riscv32(self) -> None:
        display_text = DISPLAY_HEADER.read_text(encoding="utf-8")
        bootstrap_text = TEXT_RENDERER_BOOTSTRAP.read_text(encoding="utf-8")
        metrics_text = TEXT_METRICS_SOURCE.read_text(encoding="utf-8")
        vertical_metrics_text = TEXT_VERTICAL_METRICS_SOURCE.read_text(encoding="utf-8")
        widget_text = WIDGET_BOOTSTRAP.read_text(encoding="utf-8")

        display_width = _parse_macro(display_text, "RECORZ_DISPLAY_WIDTH")
        display_height = _parse_macro(display_text, "RECORZ_DISPLAY_HEIGHT")
        _, primary_scale, primary_margin = _parse_mode(bootstrap_text, "usePrimaryTextMode")
        _, comfort_scale, comfort_margin = _parse_mode(bootstrap_text, "useComfortTextMode")
        cell_width, _, _ = _parse_boot_field_values(metrics_text, "TranscriptMetrics")
        _, _, line_height = _parse_boot_field_values(vertical_metrics_text, "TranscriptVerticalMetrics")
        source_width, source_height = _parse_workspace_source_bounds(widget_text)

        primary_character_width = cell_width * primary_scale
        primary_line_height = line_height * primary_scale
        comfort_character_width = cell_width * comfort_scale
        comfort_line_height = line_height * comfort_scale

        primary_display_density = (
            (display_width - (primary_margin * 2)) // primary_character_width,
            (display_height - (primary_margin * 2)) // primary_line_height,
        )
        comfort_display_density = (
            (display_width - (comfort_margin * 2)) // comfort_character_width,
            (display_height - (comfort_margin * 2)) // comfort_line_height,
        )
        primary_source_density = (
            (source_width - 24) // primary_character_width,
            (source_height - 24) // primary_line_height,
        )
        comfort_source_density = (
            (source_width - 24) // comfort_character_width,
            (source_height - 24) // comfort_line_height,
        )

        self.assertEqual(primary_display_density, (82, 36))
        self.assertEqual(primary_source_density, (74, 21))
        self.assertEqual(comfort_display_density, (54, 24))
        self.assertEqual(comfort_source_density, (49, 14))
        self.assertLess(comfort_display_density[0], primary_display_density[0])
        self.assertLess(comfort_display_density[1], primary_display_density[1])


if __name__ == "__main__":
    unittest.main()
