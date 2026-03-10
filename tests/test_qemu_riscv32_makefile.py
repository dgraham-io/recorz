import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
IMAGE_FIRST_UPDATE_PATH = ROOT / "examples" / "qemu_riscv_image_first_transcript_show_noop_update.rz"
REGENERATE_EXAMPLE = ROOT / "examples" / "qemu_riscv_emit_regenerated_boot_source_file_in.rz"


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
            self.assertIn("-DRECORZ_MVP_PROFILE_DEV=1", result.stdout)
            self.assertIn("-device ramfb", result.stdout)
            self.assertNotIn("-fw_cfg name=", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_target_profile_can_be_selected_explicitly(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-target-profile-") as temp_dir:
            build_dir = Path(temp_dir)
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    "RV32_PROFILE=target",
                    "run-headless",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n run-headless with RV32_PROFILE=target failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn("-DRECORZ_MVP_PROFILE_TARGET=1", result.stdout)
            self.assertNotIn("-DRECORZ_MVP_PROFILE_DEV=1", result.stdout)

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
            backup_snapshot_path = f"{snapshot_path}.bak"
            self.assertIn("extract_qemu_riscv_snapshot.py --timeout", result.stdout)
            self.assertIn(f"{build_dir / 'qemu.log'} {temp_snapshot_path}", result.stdout)
            self.assertIn(f"cp \"{snapshot_path}\" \"{backup_snapshot_path}\"", result.stdout)
            self.assertIn(f"mv {temp_snapshot_path} {snapshot_path}", result.stdout)
            self.assertNotIn(f"rm -f {snapshot_path}", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_continue_snapshot_interactive_logs_stdio_and_updates_the_snapshot_file(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-continue-snapshot-interactive-") as temp_dir:
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
                    "continue-snapshot-interactive",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n continue-snapshot-interactive failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            temp_snapshot_path = f"{snapshot_path}.tmp"
            backup_snapshot_path = f"{snapshot_path}.bak"
            self.assertIn(
                f"-chardev stdio,id=recorzio,signal=off,logfile={build_dir / 'qemu.log'},logappend=off",
                result.stdout,
            )
            self.assertIn("-serial chardev:recorzio -monitor none", result.stdout)
            self.assertIn("grep -q 'recorz-snapshot-end' ", result.stdout)
            self.assertIn("extract_qemu_riscv_snapshot.py --timeout 1", result.stdout)
            self.assertIn(f"cp \"{snapshot_path}\" \"{backup_snapshot_path}\"", result.stdout)
            self.assertIn(f"{build_dir / 'qemu.log'} {temp_snapshot_path}", result.stdout)
            self.assertIn(f"mv {temp_snapshot_path} {snapshot_path}", result.stdout)

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

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_dev_interactive_routes_through_continue_snapshot_interactive(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-dev-interactive-") as temp_dir:
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
                    "dev-interactive",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n dev-interactive failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn("qemu_riscv_image_first_save.rz", result.stdout)
            self.assertIn("qemu_riscv_image_first_boot.rz", result.stdout)
            self.assertIn(f"DEV_SNAPSHOT={snapshot_path}", result.stdout)
            self.assertIn(f"SNAPSHOT_PAYLOAD={snapshot_path}", result.stdout)
            self.assertIn("continue-snapshot-interactive", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_dev_regenerate_boot_source_routes_through_regenerate_boot_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-dev-regenerate-") as temp_dir:
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
                    "dev-regenerate-boot-source",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n dev-regenerate-boot-source failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn("qemu_riscv_image_first_boot.rz", result.stdout)
            self.assertIn(f"SNAPSHOT_PAYLOAD={snapshot_path}", result.stdout)
            self.assertIn(f"FILE_IN_PAYLOAD={REGENERATE_EXAMPLE}", result.stdout)
            self.assertIn("regenerate-boot-source", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_dev_regenerate_image_routes_through_dev_regenerate_boot_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-dev-regenerate-image-") as temp_dir:
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
                    "dev-regenerate-image",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n dev-regenerate-image failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn("dev-regenerate-boot-source", result.stdout)
            self.assertIn(f"RECORZ_MVP_KERNEL_SOURCE_BUNDLE={build_dir / 'regenerated_kernel_source.rz'}", result.stdout)
            self.assertIn(str(build_dir / "regenerated_boot_source.rz"), result.stdout)
            self.assertIn(str(build_dir / "regenerated_image.bin"), result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_dev_reset_reinitializes_the_default_snapshot_and_clears_backup(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-dev-reset-") as temp_dir:
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
                    "dev-reset",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n dev-reset failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn(f"rm -f \"{snapshot_path}\" \"{snapshot_path}.bak\"", result.stdout)
            self.assertIn("dev-init", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_dev_restore_restores_the_previous_snapshot_backup(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-dev-restore-") as temp_dir:
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
                    "dev-restore",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n dev-restore failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn(f"test -f \"{snapshot_path}.bak\"", result.stdout)
            self.assertIn(f"cp \"{snapshot_path}.bak\" \"{snapshot_path}\"", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_regenerate_boot_source_uses_a_temporary_output_before_replacing_final_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-regenerate-boot-source-") as temp_dir:
            build_dir = Path(temp_dir)
            output_path = build_dir / "regenerated_boot_source.rz"
            kernel_output_path = build_dir / "regenerated_kernel_source.rz"
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    f"REGENERATED_BOOT_SOURCE_OUTPUT={output_path}",
                    f"REGENERATED_KERNEL_SOURCE_OUTPUT={kernel_output_path}",
                    "regenerate-boot-source",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n regenerate-boot-source failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            temp_output_path = f"{output_path}.tmp"
            temp_kernel_output_path = f"{kernel_output_path}.tmp"
            self.assertIn("extract_qemu_riscv_regenerated_kernel_source.py --timeout", result.stdout)
            self.assertIn(f"{build_dir / 'qemu.log'} {temp_kernel_output_path}", result.stdout)
            self.assertIn(f"mv {temp_kernel_output_path} {kernel_output_path}", result.stdout)
            self.assertIn("extract_qemu_riscv_regenerated_boot_source.py --timeout", result.stdout)
            self.assertIn(f"{build_dir / 'qemu.log'} {temp_output_path}", result.stdout)
            self.assertIn(f"mv {temp_output_path} {output_path}", result.stdout)
            self.assertNotIn(f"rm -f {output_path}", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU RISC-V Makefile tests")
    def test_regenerate_runtime_bindings_uses_the_regenerated_kernel_source_bundle(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-makefile-regenerate-bindings-") as temp_dir:
            build_dir = Path(temp_dir)
            kernel_output_path = build_dir / "regenerated_kernel_source.rz"
            bindings_output_path = build_dir / "recorz_mvp_regenerated_runtime_bindings.h"
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    f"REGENERATED_KERNEL_SOURCE_OUTPUT={kernel_output_path}",
                    f"REGENERATED_RUNTIME_BINDINGS_OUTPUT={bindings_output_path}",
                    "regenerate-runtime-bindings",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n regenerate-runtime-bindings failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn("extract_qemu_riscv_regenerated_kernel_source.py --timeout", result.stdout)
            self.assertIn(f"RECORZ_MVP_KERNEL_SOURCE_BUNDLE={kernel_output_path}", result.stdout)
            self.assertIn("generate_qemu_riscv_mvp_runtime_bindings_header.py", result.stdout)
            self.assertIn(str(bindings_output_path), result.stdout)


if __name__ == "__main__":
    unittest.main()
