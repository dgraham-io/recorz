from __future__ import annotations

import shutil
import subprocess
import tempfile
import time
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
DEFAULT_EXAMPLE = ROOT / "examples" / "qemu_riscv_fb_demo.rz"


def _build_elf(build_dir: Path) -> Path:
    result = subprocess.run(
        [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={DEFAULT_EXAMPLE}",
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
    return build_dir / "recorz-qemu-riscv32-serial.elf"


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 serial integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32SerialIntegrationTests(unittest.TestCase):
    def test_default_demo_boots_and_prints_transcript_over_serial(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-serial-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "128M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                deadline = time.time() + 20.0
                output_chunks: list[str] = []
                while time.time() < deadline:
                    line = process.stdout.readline()
                    if not line:
                        if process.poll() is not None:
                            break
                        continue
                    output_chunks.append(line)
                    if "recorz qemu-riscv32 serial: rendered" in line:
                        break
                else:
                    self.fail("QEMU RV32 serial run timed out before rendering")
            finally:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
                if process.stdout is not None:
                    process.stdout.close()

            output = "".join(output_chunks).replace("\r", "")
            self.assertIn("recorz qemu-riscv32 serial: booting", output)
            self.assertIn("HELLO FROM RECORZ", output)
            self.assertIn("SIZE: 1280 x 1024", output)
            self.assertIn("RAMFB ONLINE.", output)
            self.assertIn("recorz qemu-riscv32 serial: rendered", output)


if __name__ == "__main__":
    unittest.main()
