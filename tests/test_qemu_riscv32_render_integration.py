import shutil
import subprocess
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
    def render_example(self, example_path: Optional[Path] = None) -> tuple[str, int, int, bytes]:
        command = ["make", "-C", str(ROOT / "platform" / "qemu-riscv32")]
        if example_path is not None:
            command.append(f"EXAMPLE={example_path}")
        command.extend(["clean", "screenshot"])
        result = subprocess.run(
            command,
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            self.fail(f"QEMU RV32 screenshot flow failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        qemu_log = QEMU_LOG_PATH.read_text(encoding="utf-8")
        width, height, data = _read_ppm(PPM_PATH)
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
        self.assertIn("IMAGE RENDERER", normalized_log)
        self.assertEqual((width, height), (1024, 768))

        text_histogram = _region_histogram(data, width, 24, 24, 360, 140)
        self.assertGreater(text_histogram[(31, 41, 51)], 1800)
        self.assertGreater(text_histogram[(247, 243, 232)], 1800)


if __name__ == "__main__":
    unittest.main()
