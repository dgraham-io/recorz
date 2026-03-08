import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
IMAGE_FIRST_UPDATE_PATH = ROOT / "examples" / "qemu_riscv_image_first_transcript_show_noop_update.rz"


class QemuRiscv32MakefileTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_run_headless_uses_rv32_framebuffer_target(self) -> None:
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
            self.assertIn("-m 32M", result.stdout)
            self.assertIn("-march=rv32im -mabi=ilp32", result.stdout)
            self.assertIn("-device ramfb", result.stdout)
            self.assertNotIn("-fw_cfg name=", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_continue_snapshot_uses_a_temporary_output_before_replacing_input_snapshot(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-continue-snapshot-") as temp_dir:
            build_dir = Path(temp_dir)
            snapshot_path = build_dir / "live.bin"
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    f"SNAPSHOT_PAYLOAD={snapshot_path}",
                    "continue-snapshot",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n continue-snapshot failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            temp_snapshot_path = f"{snapshot_path}.tmp"
            self.assertIn("extract_qemu_riscv_snapshot.py --timeout", result.stdout)
            self.assertIn(f"{build_dir / 'qemu.log'} {temp_snapshot_path}", result.stdout)
            self.assertIn(f"mv {temp_snapshot_path} {snapshot_path}", result.stdout)
            self.assertNotIn(f"rm -f {snapshot_path}", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_dev_file_in_routes_through_continue_snapshot_with_rv32_examples(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-dev-file-in-") as temp_dir:
            build_dir = Path(temp_dir)
            snapshot_path = build_dir / "dev" / "live.bin"
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    f"DEV_SNAPSHOT={snapshot_path}",
                    f"FILE_IN_PAYLOAD={IMAGE_FIRST_UPDATE_PATH}",
                    "dev-file-in",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n dev-file-in failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn("qemu_riscv_image_first_save.rz", result.stdout)
            self.assertIn(f"SNAPSHOT_PAYLOAD={snapshot_path}", result.stdout)
            self.assertIn(f"FILE_IN_PAYLOAD={IMAGE_FIRST_UPDATE_PATH}", result.stdout)
            self.assertIn("continue-snapshot", result.stdout)


if __name__ == "__main__":
    unittest.main()
