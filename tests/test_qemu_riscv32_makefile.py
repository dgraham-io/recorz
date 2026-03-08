import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"


class QemuRiscv32MakefileTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_run_headless_uses_rv32_serial_only_target(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-") as temp_dir:
            build_dir = Path(temp_dir)
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    "run-headless",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n run-headless failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn("qemu-system-riscv32", result.stdout)
            self.assertIn("-march=rv32im -mabi=ilp32", result.stdout)
            self.assertNotIn("-device ramfb", result.stdout)
            self.assertNotIn("-fw_cfg", result.stdout)


if __name__ == "__main__":
    unittest.main()
