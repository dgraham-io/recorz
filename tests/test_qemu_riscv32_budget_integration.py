from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"


def _build_elf(build_dir: Path, example_path: Path, *, profile: str = "target") -> Path:
    result = subprocess.run(
        [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"RV32_PROFILE={profile}",
            f"EXAMPLE={example_path}",
            "clean",
            "all",
        ],
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


def _run_serial_elf(elf_path: Path) -> str:
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
    return output.replace("\r", "")


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 budget integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32BudgetIntegrationTests(unittest.TestCase):
    def run_source(self, source: str) -> str:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-budget-") as temp_dir:
            temp_path = Path(temp_dir)
            example_path = temp_path / "budget_test.rz"
            example_path.write_text(source, encoding="utf-8")
            elf_path = _build_elf(temp_path / "build", example_path)
            return _run_serial_elf(elf_path)

    def test_dynamic_class_registry_overflow_reports_target_limit(self) -> None:
        parts = ["Display clear."]
        for index in range(1, 10):
            parts.append(
                "KernelInstaller fileInClassChunks: "
                f"'RecorzKernelClass: #Dyn{index} superclass: #Object instanceVariableNames: ''''\n!\n"
                "value\n    ^self'."
            )
        parts.append("Transcript show: 'DONE'.")

        output = self.run_source("\n".join(parts))

        self.assertIn("recorz qemu-riscv32 mvp: created class Dyn8", output)
        self.assertIn("panic: dynamic class registry overflow", output)
        self.assertIn("send selector=fileInClassChunks:", output)

    def test_named_object_registry_overflow_reports_target_limit(self) -> None:
        parts = ["Display clear."]
        for index in range(1, 10):
            parts.append(f"KernelInstaller rememberObject: Display named: 'o{index}'.")
        parts.append("Transcript show: 'DONE'.")

        output = self.run_source("\n".join(parts))

        self.assertIn("panic: named object registry overflow", output)
        self.assertIn("send selector=rememberObject:named:", output)

    def test_object_heap_overflow_reports_target_limit(self) -> None:
        parts = ["Display clear."]
        for index in range(1, 8):
            parts.append(
                "KernelInstaller fileInClassChunks: "
                f"'RecorzKernelClass: #Heap{index} superclass: #Object instanceVariableNames: ''''\n!\n"
                "value\n    ^self\n!\n"
                f"RecorzKernelClassSide: #Heap{index}\n!\n"
                "detail\n    ^self'."
            )
        parts.append("Transcript show: 'DONE'.")

        output = self.run_source("\n".join(parts))

        self.assertIn("recorz qemu-riscv32 mvp: created class Heap7", output)
        self.assertIn("panic: object heap overflow", output)
        self.assertIn("send selector=fileInClassChunks:", output)

    def test_snapshot_buffer_overflow_reports_target_limit(self) -> None:
        parts = ["Display clear."]
        for index in range(1, 7):
            parts.append(
                "KernelInstaller fileInClassChunks: "
                f"'RecorzKernelClass: #Snap{index} superclass: #Object instanceVariableNames: ''''\n!\n"
                "value\n    ^self\n!\n"
                f"RecorzKernelClassSide: #Snap{index}\n!\n"
                "detail\n    ^self'."
            )
        parts.append("Workspace setContents: '" + ("A" * 7000) + "'.")
        parts.append("KernelInstaller saveSnapshot.")

        output = self.run_source("\n".join(parts))

        self.assertIn("recorz qemu-riscv32 mvp: created class Snap6", output)
        self.assertIn("panic: snapshot exceeds buffer capacity", output)
        self.assertIn("send selector=saveSnapshot", output)


if __name__ == "__main__":
    unittest.main()
