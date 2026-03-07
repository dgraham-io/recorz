#!/usr/bin/env python3
"""Build a tiny compiled-method replacement payload for the QEMU RISC-V MVP target."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / "src") not in sys.path:
    sys.path.insert(0, str(ROOT / "src"))

import build_qemu_riscv_mvp_image as mvp  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("class_name", help="Kernel class name that owns the method")
    parser.add_argument("source", type=Path, help="Path to a single method chunk source file")
    parser.add_argument("output", type=Path, help="Path to the generated method update payload")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    source_text = args.source.read_text(encoding="utf-8").strip()
    if not source_text:
        raise SystemExit("method update source file is empty")
    args.output.write_bytes(mvp.build_method_update_manifest(args.class_name, source_text))


if __name__ == "__main__":
    main()
