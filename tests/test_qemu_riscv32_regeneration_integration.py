from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
import os


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
REGENERATION_EXAMPLE = ROOT / "examples" / "qemu_riscv_emit_regenerated_boot_source_demo.rz"
REGENERATION_AFTER_CLASS_EDIT_EXAMPLE = ROOT / "examples" / "qemu_riscv_emit_regenerated_boot_source_after_class_edit_demo.rz"
DEV_FILE_IN_UPDATE_EXAMPLE = ROOT / "examples" / "qemu_riscv_image_first_transcript_show_noop_update.rz"
IMAGE_BUILDER = ROOT / "tools" / "build_qemu_riscv_mvp_image.py"
RUNTIME_BINDINGS_GENERATOR = ROOT / "tools" / "generate_qemu_riscv_mvp_runtime_bindings_header.py"
IMAGE_INSPECTOR = ROOT / "tools" / "inspect_qemu_riscv_mvp_image.py"


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 regeneration integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32RegenerationIntegrationTests(unittest.TestCase):
    def run_make(self, *args: str) -> subprocess.CompletedProcess[str]:
        result = subprocess.run(
            ["make", "-C", str(PLATFORM_DIR), *args],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            self.fail(
                "QEMU RV32 Makefile flow failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        return result

    def regenerate_boot_source(self, *, build_dir: Path, example_path: Path) -> tuple[str, Path, str, Path, str]:
        regenerated_source_path = build_dir / "regenerated_boot_source.rz"
        regenerated_kernel_source_path = build_dir / "regenerated_kernel_source.rz"
        self.run_make(
            f"BUILD_DIR={build_dir}",
            f"EXAMPLE={example_path}",
            f"REGENERATED_BOOT_SOURCE_OUTPUT={regenerated_source_path}",
            f"REGENERATED_KERNEL_SOURCE_OUTPUT={regenerated_kernel_source_path}",
            "clean",
            "regenerate-boot-source",
        )
        qemu_log = (build_dir / "qemu.log").read_text(encoding="utf-8")
        return (
            qemu_log,
            regenerated_source_path,
            regenerated_source_path.read_text(encoding="utf-8"),
            regenerated_kernel_source_path,
            regenerated_kernel_source_path.read_text(encoding="utf-8"),
        )

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

    def generate_runtime_bindings_from_kernel_source(self, *, kernel_source_path: Path, output_path: Path) -> str:
        env = dict(os.environ)
        env["RECORZ_MVP_KERNEL_SOURCE_BUNDLE"] = str(kernel_source_path)
        result = subprocess.run(
            [
                "python3",
                str(RUNTIME_BINDINGS_GENERATOR),
                str(output_path),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            env=env,
        )
        if result.returncode != 0:
            self.fail(
                "runtime bindings generation from regenerated kernel source failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        return output_path.read_text(encoding="utf-8")

    def build_image_from_regenerated_sources(
        self,
        *,
        kernel_source_path: Path,
        boot_source_path: Path,
        image_output_path: Path,
    ) -> str:
        env = dict(os.environ)
        env["RECORZ_MVP_KERNEL_SOURCE_BUNDLE"] = str(kernel_source_path)
        result = subprocess.run(
            [
                "python3",
                str(IMAGE_BUILDER),
                str(boot_source_path),
                str(image_output_path),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            env=env,
        )
        if result.returncode != 0:
            self.fail(
                "image build from regenerated sources failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        inspect_result = subprocess.run(
            [
                "python3",
                str(IMAGE_INSPECTOR),
                str(image_output_path),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if inspect_result.returncode != 0:
            self.fail(
                "image inspection for regenerated-source build failed\n"
                f"stdout:\n{inspect_result.stdout}\n"
                f"stderr:\n{inspect_result.stderr}"
            )
        return inspect_result.stdout

    def test_image_can_emit_regenerated_boot_source_that_recreates_the_same_framebuffer_state(self) -> None:
        with tempfile.TemporaryDirectory(prefix="regen-", dir=ROOT / "misc") as temp_dir:
            temp_root = Path(temp_dir)
            (
                regenerated_log,
                regenerated_source_path,
                regenerated_source,
                regenerated_kernel_source_path,
                regenerated_kernel_source,
            ) = self.regenerate_boot_source(
                build_dir=temp_root / "regen",
                example_path=REGENERATION_EXAMPLE,
            )

            self.assertIn("recorz-regenerated-kernel-source-begin", regenerated_log)
            self.assertIn("recorz-regenerated-boot-source-begin", regenerated_log)
            self.assertTrue(regenerated_source_path.exists())
            self.assertTrue(regenerated_kernel_source_path.exists())
            self.assertIn("RecorzKernelClass: #RegeneratedDemo", regenerated_source)
            self.assertIn("Workspace browsePackageNamed: 'Tools'.", regenerated_source)
            self.assertIn("Workspace setContents:", regenerated_source)
            self.assertIn("RecorzKernelClass: #Workspace", regenerated_kernel_source)
            self.assertNotIn("RecorzKernelPackage:", regenerated_kernel_source)

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

    def test_host_builder_can_use_regenerated_kernel_source_as_the_kernel_authority(self) -> None:
        with tempfile.TemporaryDirectory(prefix="regen-", dir=ROOT / "misc") as temp_dir:
            temp_root = Path(temp_dir)
            (
                _regenerated_log,
                regenerated_source_path,
                _regenerated_source,
                regenerated_kernel_source_path,
                _regenerated_kernel_source,
            ) = self.regenerate_boot_source(
                build_dir=temp_root / "regen",
                example_path=REGENERATION_EXAMPLE,
            )

            bindings_output = temp_root / "recorz_mvp_regenerated_runtime_bindings.h"
            image_output = temp_root / "regenerated_from_bundle.bin"

            header_text = self.generate_runtime_bindings_from_kernel_source(
                kernel_source_path=regenerated_kernel_source_path,
                output_path=bindings_output,
            )
            inspect_output = self.build_image_from_regenerated_sources(
                kernel_source_path=regenerated_kernel_source_path,
                boot_source_path=regenerated_source_path,
                image_output_path=image_output,
            )

            self.assertIn("RECORZ_MVP_SELECTOR_SEED_BOOT_CONTENTS = 93", header_text)
            self.assertIn("struct recorz_mvp_seed_class_source_record {", header_text)
            self.assertIn("RecorzKernelBootObject: #DefaultForm", _regenerated_kernel_source)
            self.assertIn("RecorzKernelRoot: #default_form", _regenerated_kernel_source)
            self.assertIn("RecorzKernelSelector: #show:", _regenerated_kernel_source)
            self.assertIn("RecorzKernelGlyphBitmapFamily:", _regenerated_kernel_source)
            self.assertIn("profile: RV64MVP1", inspect_output)
            self.assertIn("selector_objects=100", inspect_output)
            self.assertIn("method_entries=86", inspect_output)

    def test_regenerated_kernel_source_reflects_seeded_class_edits_accepted_in_image(self) -> None:
        with tempfile.TemporaryDirectory(prefix="regen-", dir=ROOT / "misc") as temp_dir:
            temp_root = Path(temp_dir)
            (
                regenerated_log,
                _regenerated_source_path,
                _regenerated_source,
                _regenerated_kernel_source_path,
                regenerated_kernel_source,
            ) = self.regenerate_boot_source(
                build_dir=temp_root / "regen-edit",
                example_path=REGENERATION_AFTER_CLASS_EDIT_EXAMPLE,
            )

            self.assertIn("recorz-regenerated-kernel-source-begin", regenerated_log)
            self.assertIn("RecorzKernelClass: #Display", regenerated_kernel_source)
            self.assertIn("Transcript show: 'RGEN'.", regenerated_kernel_source)

    def test_regenerated_boot_source_recreates_seeded_class_edit_state_after_cold_boot(self) -> None:
        with tempfile.TemporaryDirectory(prefix="regen-", dir=ROOT / "misc") as temp_dir:
            temp_root = Path(temp_dir)
            (
                regenerated_log,
                regenerated_source_path,
                regenerated_source,
                _regenerated_kernel_source_path,
                regenerated_kernel_source,
            ) = self.regenerate_boot_source(
                build_dir=temp_root / "regen-edit",
                example_path=REGENERATION_AFTER_CLASS_EDIT_EXAMPLE,
            )

            original_log, original_ppm = self.render_demo(
                build_dir=temp_root / "orig-edit",
                example_path=REGENERATION_AFTER_CLASS_EDIT_EXAMPLE,
            )
            rebuilt_log, rebuilt_ppm = self.render_demo(
                build_dir=temp_root / "rebld-edit",
                example_path=regenerated_source_path,
            )

            self.assertIn("recorz-regenerated-boot-source-begin", regenerated_log)
            self.assertIn("Workspace fileIn: 'RecorzKernelClass: #Transcript", regenerated_source)
            self.assertIn("Workspace fileOutClassNamed: 'Display'.", regenerated_source)
            self.assertIn("Transcript show: ''RGEN''.", regenerated_source)
            self.assertIn("Transcript show: 'RGEN'.", regenerated_kernel_source)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", original_log)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", rebuilt_log)
            self.assertIn("RGEN", original_log)
            self.assertIn("RGEN", rebuilt_log)
            self.assertIn("CLASS: DISPLAY", rebuilt_log)
            self.assertIn("VIEW: SOURCE", rebuilt_log)
            self.assertIn(
                "NEWLINETRANSCRIPT SHOW: 'RGEN'.TRANSCRIPT CR.",
                rebuilt_log.replace("\r", "").replace("\n", ""),
            )
            self.assertNotEqual(len(original_ppm), 0)
            self.assertNotEqual(len(rebuilt_ppm), 0)

    def test_dev_snapshot_can_regenerate_sources_from_current_image_state(self) -> None:
        with tempfile.TemporaryDirectory(prefix="regen-", dir=ROOT / "misc") as temp_dir:
            temp_root = Path(temp_dir)
            build_dir = temp_root / "dev-loop"
            snapshot_path = temp_root / "dev" / "live.bin"
            regenerated_source_path = build_dir / "regenerated_boot_source.rz"
            regenerated_kernel_source_path = build_dir / "regenerated_kernel_source.rz"

            self.run_make(
                f"BUILD_DIR={build_dir}",
                f"DEV_SNAPSHOT={snapshot_path}",
                "dev-init",
            )
            self.run_make(
                f"BUILD_DIR={build_dir}",
                f"DEV_SNAPSHOT={snapshot_path}",
                f"FILE_IN_PAYLOAD={DEV_FILE_IN_UPDATE_EXAMPLE}",
                "dev-file-in",
            )
            result = self.run_make(
                f"BUILD_DIR={build_dir}",
                f"DEV_SNAPSHOT={snapshot_path}",
                f"REGENERATED_BOOT_SOURCE_OUTPUT={regenerated_source_path}",
                f"REGENERATED_KERNEL_SOURCE_OUTPUT={regenerated_kernel_source_path}",
                "dev-regenerate-boot-source",
            )

            qemu_log = (build_dir / "qemu.log").read_text(encoding="utf-8")
            regenerated_source = regenerated_source_path.read_text(encoding="utf-8")
            regenerated_kernel_source = regenerated_kernel_source_path.read_text(encoding="utf-8")

            self.assertIn("wrote", result.stdout)
            self.assertIn("recorz-regenerated-kernel-source-begin", qemu_log)
            self.assertIn("recorz-regenerated-boot-source-begin", qemu_log)
            self.assertTrue(regenerated_source_path.exists())
            self.assertTrue(regenerated_kernel_source_path.exists())
            self.assertIn("RecorzKernelClass: #Transcript", regenerated_source)
            self.assertIn("show: text\n^self", regenerated_source)
            self.assertIn("RecorzKernelClass: #Transcript", regenerated_kernel_source)
            self.assertIn("show: text\n^self", regenerated_kernel_source)


if __name__ == "__main__":
    unittest.main()
