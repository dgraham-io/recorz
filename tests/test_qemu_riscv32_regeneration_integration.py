from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
REGENERATION_EXAMPLE = ROOT / "examples" / "qemu_riscv_emit_regenerated_boot_source_demo.rz"


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 regeneration integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32RegenerationIntegrationTests(unittest.TestCase):
    def regenerate_boot_source(self, *, build_dir: Path, example_path: Path) -> tuple[str, Path, str]:
        regenerated_source_path = build_dir / "regenerated_boot_source.rz"
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            f"REGENERATED_BOOT_SOURCE_OUTPUT={regenerated_source_path}",
            "clean",
            "regenerate-boot-source",
        ]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(
                "QEMU RV32 regenerate-boot-source flow failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        qemu_log = (build_dir / "qemu.log").read_text(encoding="utf-8")
        return qemu_log, regenerated_source_path, regenerated_source_path.read_text(encoding="utf-8")

    def render_demo(self, *, build_dir: Path, example_path: Path) -> tuple[str, bytes]:
        ppm_path = build_dir / "recorz-qemu-riscv32-mvp.ppm"
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            "clean",
            "screenshot",
        ]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(
                "QEMU RV32 screenshot flow failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        qemu_log = (build_dir / "qemu.log").read_text(encoding="utf-8")
        return qemu_log, ppm_path.read_bytes()

    def test_image_can_emit_regenerated_boot_source_that_recreates_the_same_framebuffer_state(self) -> None:
        with tempfile.TemporaryDirectory(prefix="regen-", dir=ROOT / "misc") as temp_dir:
            temp_root = Path(temp_dir)
            regenerated_log, regenerated_source_path, regenerated_source = self.regenerate_boot_source(
                build_dir=temp_root / "regen",
                example_path=REGENERATION_EXAMPLE,
            )

            self.assertIn("recorz-regenerated-boot-source-begin", regenerated_log)
            self.assertTrue(regenerated_source_path.exists())
            self.assertIn("RecorzKernelClass: #RegeneratedDemo", regenerated_source)
            self.assertIn("Workspace browsePackageNamed: 'Tools'.", regenerated_source)
            self.assertIn("Workspace setContents:", regenerated_source)

            original_log, original_ppm = self.render_demo(
                build_dir=temp_root / "orig",
                example_path=REGENERATION_EXAMPLE,
            )
            rebuilt_log, rebuilt_ppm = self.render_demo(
                build_dir=temp_root / "rebld",
                example_path=regenerated_source_path,
            )

            self.assertIn("recorz qemu-riscv32 mvp: rendered", original_log)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", rebuilt_log)
            self.assertEqual(original_ppm, rebuilt_ppm)


if __name__ == "__main__":
    unittest.main()
