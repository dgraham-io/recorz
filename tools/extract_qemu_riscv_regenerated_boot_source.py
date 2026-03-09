#!/usr/bin/env python3
"""Extract regenerated Recorz boot source from a QEMU serial log."""

from __future__ import annotations

import argparse
import re
import time
from pathlib import Path


BEGIN_RE = re.compile(r"recorz-regenerated-boot-source-begin (\d+)")
DATA_RE = re.compile(r"^recorz-regenerated-boot-source-data ([0-9a-f]+)$", re.MULTILINE)
END_RE = re.compile(r"^recorz-regenerated-boot-source-end$", re.MULTILINE)


def extract_regenerated_boot_source(log_text: str) -> str | None:
    log_text = log_text.replace("\r\n", "\n").replace("\r", "\n")
    begin_match = BEGIN_RE.search(log_text)
    end_match = END_RE.search(log_text)

    if begin_match is None or end_match is None or end_match.start() < begin_match.end():
        return None

    expected_size = int(begin_match.group(1))
    payload_region = log_text[begin_match.end() : end_match.start()]
    payload_hex = "".join(match.group(1) for match in DATA_RE.finditer(payload_region))
    payload = bytes.fromhex(payload_hex)
    if len(payload) != expected_size:
        raise SystemExit(
            "regenerated boot source size mismatch: "
            f"expected {expected_size} bytes, extracted {len(payload)} bytes"
        )
    return payload.decode("utf-8")


def wait_for_regenerated_boot_source(log_path: Path, timeout_seconds: float) -> str:
    deadline = time.time() + timeout_seconds

    while time.time() < deadline:
        if log_path.exists():
            source_text = extract_regenerated_boot_source(log_path.read_text(encoding="utf-8", errors="replace"))
            if source_text is not None:
                return source_text
        time.sleep(0.1)
    raise SystemExit(f"timed out waiting for regenerated boot source markers in {log_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log_path", type=Path)
    parser.add_argument("output_path", type=Path)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()

    source_text = wait_for_regenerated_boot_source(args.log_path, args.timeout)
    args.output_path.parent.mkdir(parents=True, exist_ok=True)
    args.output_path.write_text(source_text, encoding="utf-8")


if __name__ == "__main__":
    main()
