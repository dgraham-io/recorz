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
            self.assertIn("program: instructions=129", default_output)
            self.assertIn("program: instructions=42", stateful_output)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_continue_snapshot_uses_a_temporary_output_before_replacing_input_snapshot(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-continue-snapshot-") as temp_dir:
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
            self.assertIn(
                f"extract_qemu_riscv_snapshot.py {build_dir / 'qemu.log'} {temp_snapshot_path}",
                result.stdout,
            )
            self.assertIn(f"cp \"{snapshot_path}\" \"{backup_snapshot_path}\"", result.stdout)
            self.assertIn(f"mv {temp_snapshot_path} {snapshot_path}", result.stdout)
            self.assertNotIn(f"rm -f {snapshot_path}", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_continue_snapshot_includes_external_file_in_fw_cfg_when_requested(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-file-in-") as temp_dir:
            build_dir = Path(temp_dir)
            snapshot_path = build_dir / "live.bin"
            file_in_path = build_dir / "update.rz"
            file_in_path.write_text("RecorzKernelClass: #Update descriptorOrder: 1 instanceVariableNames: ''\n", encoding="utf-8")
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    f"SNAPSHOT_PAYLOAD={snapshot_path}",
                    f"FILE_IN_PAYLOAD={file_in_path}",
                    "continue-snapshot",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n continue-snapshot with FILE_IN_PAYLOAD failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            combined_payload = build_dir / "external_file_in_payload.rz"
            bootstrap_payload = ROOT / "kernel" / "textui" / "TextRendererBootstrap.rz"
            self.assertIn(f"for path in {bootstrap_payload} {file_in_path}; do", result.stdout)
            self.assertIn(f"-fw_cfg name=opt/recorz-file-in,file={combined_payload}", result.stdout)
            self.assertIn(f"-fw_cfg name=opt/recorz-snapshot,file={snapshot_path}", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_continue_snapshot_interactive_logs_stdio_and_updates_the_snapshot_file(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-continue-snapshot-interactive-") as temp_dir:
            build_dir = Path(temp_dir)
            snapshot_path = build_dir / "live.bin"
            signal_path = build_dir / "continued.flag"
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    f"SNAPSHOT_PAYLOAD={snapshot_path}",
                    f"CONTINUE_SNAPSHOT_SIGNAL={signal_path}",
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
            self.assertIn(f"extract_qemu_riscv_snapshot.py --timeout 1 {build_dir / 'qemu.log'} {temp_snapshot_path}", result.stdout)
            self.assertIn(f"cp \"{snapshot_path}\" \"{backup_snapshot_path}\"", result.stdout)
            self.assertIn(f"touch \"{signal_path}\"", result.stdout)
            self.assertIn(f"mv {temp_snapshot_path} {snapshot_path}", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_continue_snapshot_uses_generated_combined_file_in_payload_for_multiple_sources(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-file-in-stream-") as temp_dir:
            build_dir = Path(temp_dir)
            snapshot_path = build_dir / "live.bin"
            first_file = build_dir / "First.rz"
            second_file = build_dir / "Second.rz"
            first_file.write_text("RecorzKernelClass: #First descriptorOrder: 1 instanceVariableNames: ''\n", encoding="utf-8")
            second_file.write_text("RecorzKernelClass: #Second descriptorOrder: 2 instanceVariableNames: ''\n", encoding="utf-8")
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    f"SNAPSHOT_PAYLOAD={snapshot_path}",
                    f"FILE_IN_PAYLOADS={first_file} {second_file}",
                    "continue-snapshot",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n continue-snapshot with FILE_IN_PAYLOADS failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            combined_payload = build_dir / "external_file_in_payload.rz"
            bootstrap_payload = ROOT / "kernel" / "textui" / "TextRendererBootstrap.rz"
            self.assertIn(f"for path in {bootstrap_payload} {first_file} {second_file}; do", result.stdout)
            self.assertIn(f"cat $path >> {combined_payload}", result.stdout)
            self.assertIn(f"-fw_cfg name=opt/recorz-file-in,file={combined_payload}", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_dev_interactive_routes_through_continue_snapshot_interactive(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-dev-interactive-") as temp_dir:
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

            self.assertIn("qemu_riscv_image_development_home_save.rz", result.stdout)
            self.assertIn("qemu_riscv_image_development_home_boot.rz", result.stdout)
            self.assertIn(f"DEV_SNAPSHOT={snapshot_path}", result.stdout)
            self.assertIn(f"SNAPSHOT_PAYLOAD={snapshot_path}", result.stdout)
            self.assertIn("continue-snapshot-interactive", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_dev_loop_reopens_after_snapshot_emission(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-dev-loop-") as temp_dir:
            build_dir = Path(temp_dir)
            snapshot_path = build_dir / "dev" / "live.bin"
            signal_path = build_dir / "dev-loop.snapshot"
            result = subprocess.run(
                [
                    "make",
                    "-n",
                    "-C",
                    str(PLATFORM_DIR),
                    f"BUILD_DIR={build_dir}",
                    f"DEV_SNAPSHOT={snapshot_path}",
                    f"DEV_LOOP_SIGNAL={signal_path}",
                    "dev-loop",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                self.fail(
                    "make -n dev-loop failed\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                )

            self.assertIn("while true; do", result.stdout)
            self.assertIn(f"rm -f \"{signal_path}\"", result.stdout)
            self.assertIn(f"CONTINUE_SNAPSHOT_SIGNAL=\"{signal_path}\"", result.stdout)
            self.assertIn(f"SNAPSHOT_PAYLOAD={snapshot_path}", result.stdout)
            self.assertIn("continue-snapshot-interactive", result.stdout)
            self.assertIn(f"echo \"resuming {snapshot_path}\"", result.stdout)

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_dev_reset_reinitializes_the_default_snapshot_and_clears_backup(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-dev-reset-") as temp_dir:
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

    @unittest.skipUnless(shutil.which("make"), "make is required for QEMU Makefile tests")
    def test_dev_restore_restores_the_previous_snapshot_backup(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv64-makefile-dev-restore-") as temp_dir:
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


if __name__ == "__main__":
    unittest.main()
