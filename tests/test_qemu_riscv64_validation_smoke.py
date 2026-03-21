from __future__ import annotations

import shutil
import subprocess
import unittest
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv64"
TEXT_FOREGROUND = (31, 41, 51)


def _read_ppm(path: Path) -> tuple[int, int, bytes]:
    with path.open("rb") as ppm_file:
        if ppm_file.readline().strip() != b"P6":
            raise AssertionError("expected a binary PPM screendump")
        line = ppm_file.readline()
        while line.startswith(b"#"):
            line = ppm_file.readline()
        width, height = map(int, line.split())
        if int(ppm_file.readline()) != 255:
            raise AssertionError("expected 8-bit PPM data")
        data = ppm_file.read()
    return width, height, data


def _pixel(data: bytes, width: int, x: int, y: int) -> tuple[int, int, int]:
    index = (y * width + x) * 3
    return tuple(data[index : index + 3])


def _region_histogram(data: bytes, width: int, x0: int, y0: int, x1: int, y1: int) -> Counter[tuple[int, int, int]]:
    histogram: Counter[tuple[int, int, int]] = Counter()
    for y in range(y0, y1):
        for x in range(x0, x1):
            histogram[_pixel(data, width, x, y)] += 1
    return histogram


@unittest.skipUnless(
    shutil.which("qemu-system-riscv64") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RISC-V validation smoke requires qemu-system-riscv64 and riscv64-unknown-elf-gcc",
)
class QemuRiscv64ValidationSmokeTests(unittest.TestCase):
    """Minimal RV64 validation contract without the default TextUI file-in payload."""

    def save_snapshot(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        snapshot_output: Path,
    ) -> str:
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            "DEFAULT_FILE_IN_PAYLOADS=",
            f"EXAMPLE={example_path}",
            f"SNAPSHOT_OUTPUT={snapshot_output}",
            "clean",
            "save-snapshot",
        ]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(f"RV64 validation save-snapshot failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        return (build_dir / "qemu.log").read_text(encoding="utf-8")

    def render_snapshot(
        self,
        *,
        build_dir: Path,
        example_path: Path,
        snapshot_payload: Path,
    ) -> tuple[str, int, int, bytes]:
        ppm_path = build_dir / "recorz-qemu-riscv64-mvp.ppm"
        command = [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            "DEFAULT_FILE_IN_PAYLOADS=",
            f"EXAMPLE={example_path}",
            f"SNAPSHOT_PAYLOAD={snapshot_payload}",
            "clean",
            "screenshot",
        ]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(f"RV64 validation screenshot failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        qemu_log = (build_dir / "qemu.log").read_text(encoding="utf-8")
        width, height, data = _read_ppm(ppm_path)
        return qemu_log, width, height, data

    def test_minimal_snapshot_round_trip_renders_without_default_textui_payload(self) -> None:
        build_dir = ROOT / "misc" / "qemu-riscv64-validation-smoke-save"
        reload_build_dir = ROOT / "misc" / "qemu-riscv64-validation-smoke-reload"
        snapshot_output = ROOT / "misc" / "qemu-riscv64-snapshots" / "validation-smoke-live-image.bin"

        save_log = self.save_snapshot(
            build_dir=build_dir,
            example_path=ROOT / "examples" / "qemu_riscv_snapshot_save_demo.rz",
            snapshot_output=snapshot_output,
        )
        self.assertIn("recorz-snapshot-begin", save_log)
        self.assertIn("recorz qemu-riscv64 mvp: snapshot saved, shutting down", save_log)
        self.assertTrue(snapshot_output.exists())

        reload_log, width, height, data = self.render_snapshot(
            build_dir=reload_build_dir,
            example_path=ROOT / "examples" / "qemu_riscv_snapshot_reload_demo.rz",
            snapshot_payload=snapshot_output,
        )
        self.assertEqual((width, height), (1024, 768))
        self.assertIn("recorz qemu-riscv64 mvp: loaded snapshot", reload_log)
        self.assertIn("recorz qemu-riscv64 mvp: rendered", reload_log)
        self.assertNotIn("panic:", reload_log)
        self.assertEqual(_pixel(data, width, 0, 0), (247, 243, 232))
        self.assertGreater(_region_histogram(data, width, 24, 24, 220, 96)[TEXT_FOREGROUND], 300)


if __name__ == "__main__":
    unittest.main()
