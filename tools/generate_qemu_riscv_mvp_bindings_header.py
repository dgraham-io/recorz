#!/usr/bin/env python3
"""Emit generated primitive binding definitions for the QEMU RISC-V MVP target."""

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
    parser.add_argument("output", type=Path, help="Path to the generated header")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    args.output.write_text(mvp.render_generated_primitive_bindings_header(), encoding="utf-8")


if __name__ == "__main__":
    main()
