import shutil
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv64"
BUILD_DIR = ROOT / "misc" / "qemu-riscv64-panic-test"
ELF_PATH = BUILD_DIR / "recorz-qemu-riscv64-mvp.elf"
PANIC_EXAMPLE_PATH = ROOT / "examples" / "qemu_riscv_panic_demo.rz"
LOOKUP_PANIC_EXAMPLE_PATH = ROOT / "examples" / "qemu_riscv_lookup_panic_demo.rz"


def _timeout_output(exc: subprocess.TimeoutExpired) -> str:
    parts: list[str] = []
    for chunk in (exc.stdout, exc.stderr):
        if chunk is None:
            continue
        if isinstance(chunk, bytes):
            parts.append(chunk.decode("utf-8", errors="replace"))
        else:
            parts.append(chunk)
    return "".join(parts)


@unittest.skipUnless(
    shutil.which("qemu-system-riscv64") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RISC-V panic integration test requires qemu-system-riscv64 and riscv64-unknown-elf-gcc",
)
class QemuRiscvPanicIntegrationTests(unittest.TestCase):
    def build_and_capture_timeout_output(self, example_path: Path) -> str:
        build = subprocess.run(
            [
                "make",
                "-C",
                str(PLATFORM_DIR),
                f"BUILD_DIR={BUILD_DIR}",
                f"EXAMPLE={example_path}",
                "clean",
                "all",
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if build.returncode != 0:
            self.fail(f"panic demo build failed\nstdout:\n{build.stdout}\nstderr:\n{build.stderr}")

        try:
            subprocess.run(
                [
                    "qemu-system-riscv64",
                    "-machine",
                    "virt",
                    "-m",
                    "128M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(ELF_PATH),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
                timeout=5,
                check=False,
            )
            self.fail("panic demo unexpectedly exited without timing out")
        except subprocess.TimeoutExpired as exc:
            return _timeout_output(exc)

    def test_runtime_panic_reports_vm_context(self) -> None:
        output = self.build_and_capture_timeout_output(PANIC_EXAMPLE_PATH)

        self.assertIn("panic: BitBlt fillForm:color: expects a form receiver argument", output)
        self.assertIn("vm: phase=send", output)
        self.assertIn("vm: pc=", output)
        self.assertIn("opcode=send", output)
        self.assertIn("vm: send selector=fillForm:color: args=2", output)
        self.assertIn("vm: receiver=object#", output)
        self.assertIn("vm: arg[0]=smallInteger(1)", output)
        self.assertIn("vm: arg[1]=smallInteger(0)", output)
        self.assertIn("vm: stack_size=0", output)

    def test_missing_seeded_method_reports_lookup_panic(self) -> None:
        output = self.build_and_capture_timeout_output(LOOKUP_PANIC_EXAMPLE_PATH)

        self.assertIn("panic: selector is not understood by receiver class", output)
        self.assertIn("vm: phase=send", output)
        self.assertIn("vm: send selector=width args=0", output)
        self.assertIn("vm: receiver=object#", output)


if __name__ == "__main__":
    unittest.main()
