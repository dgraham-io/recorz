#!/usr/bin/env python3
"""Extract a live Recorz snapshot blob from a QEMU serial log."""

from __future__ import annotations

import argparse
import re
import time
from pathlib import Path


BEGIN_RE = re.compile(r"^recorz-snapshot-begin (\d+)$", re.MULTILINE)
DATA_RE = re.compile(r"^recorz-snapshot-data ([0-9a-f]+)$", re.MULTILINE)
END_RE = re.compile(r"^recorz-snapshot-end$", re.MULTILINE)


def extract_snapshot_bytes(log_text: str) -> bytes | None:
    begin_match = BEGIN_RE.search(log_text)
    end_match = END_RE.search(log_text)

    if begin_match is None or end_match is None or end_match.start() < begin_match.end():
        return None

    expected_size = int(begin_match.group(1))
    payload_region = log_text[begin_match.end() : end_match.start()]
    payload_hex = "".join(match.group(1) for match in DATA_RE.finditer(payload_region))
    snapshot = bytes.fromhex(payload_hex)
    if len(snapshot) != expected_size:
        raise SystemExit(
            f"snapshot size mismatch: expected {expected_size} bytes, extracted {len(snapshot)} bytes"
        )
    return snapshot


def wait_for_snapshot(log_path: Path, timeout_seconds: float) -> bytes:
    deadline = time.time() + timeout_seconds

    while time.time() < deadline:
        if log_path.exists():
            snapshot = extract_snapshot_bytes(log_path.read_text(encoding="utf-8", errors="replace"))
            if snapshot is not None:
                return snapshot
        time.sleep(0.1)
    raise SystemExit(f"timed out waiting for snapshot markers in {log_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log_path", type=Path)
    parser.add_argument("output_path", type=Path)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()

    snapshot = wait_for_snapshot(args.log_path, args.timeout)
    args.output_path.parent.mkdir(parents=True, exist_ok=True)
    args.output_path.write_bytes(snapshot)


if __name__ == "__main__":
    main()
