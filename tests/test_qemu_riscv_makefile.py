import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv64"
DEFAULT_EXAMPLE = ROOT / "examples" / "qemu_riscv_fb_demo.rz"
STATEFUL_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_stateful_class_demo.rz"


def _inspect_image_output(build_dir: Path, example_path: Path) -> str:
    result = subprocess.run(
        [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            "inspect-image",
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise AssertionError(
            "make inspect-image failed\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return result.stdout


def _checksum(output: str) -> str:
    match = re.search(r"^checksum: (0x[0-9a-f]+)$", output, re.MULTILINE)
    if match is None:
        raise AssertionError(f"inspect-image output did not contain checksum:\n{output}")
    return match.group(1)


class QemuRiscvMakefileTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_switching_example_rebuilds_generated_image_without_clean(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-example-switch-") as temp_dir:
            build_dir = Path(temp_dir)
            default_output = _inspect_image_output(build_dir, DEFAULT_EXAMPLE)
            stateful_output = _inspect_image_output(build_dir, STATEFUL_EXAMPLE)

            self.assertNotEqual(_checksum(default_output), _checksum(stateful_output))
            self.assertIn("program: instructions=125", default_output)
            self.assertIn("program: instructions=38", stateful_output)


if __name__ == "__main__":
    unittest.main()
