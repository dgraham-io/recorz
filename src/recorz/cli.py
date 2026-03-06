"""Textual workspace entrypoint for the Phase 1 image."""

from __future__ import annotations

import argparse
from typing import Optional

from .bootstrap import bootstrap_image
from .image import load_image_manifest, save_image_manifest
from .model import RecorzError


def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(prog="recorz")
    parser.add_argument("source", nargs="?", help="Expression or statement sequence to evaluate")
    parser.add_argument("--image", help="Load a structural image manifest before evaluation")
    parser.add_argument("--save-image", help="Write the current structural image manifest after evaluation")
    args = parser.parse_args(argv)

    if args.image:
        image, vm = load_image_manifest(args.image)
    else:
        image, vm = bootstrap_image()

    if args.source:
        try:
            result = vm.evaluate(args.source)
            rendered = vm.send(result, "printString", [])
            print(rendered)
        except RecorzError as error:
            print(f"error: {error}")
            return 1

    if args.save_image:
        save_image_manifest(image, args.save_image)
    return 0
