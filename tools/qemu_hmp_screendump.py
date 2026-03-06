#!/usr/bin/env python3
"""Send screendump and quit to a QEMU HMP unix socket."""

from __future__ import annotations

import socket
import sys
import time
from pathlib import Path


def wait_for_socket(path: Path) -> None:
    for _ in range(50):
        if path.exists():
            return
        time.sleep(0.1)
    raise SystemExit("monitor socket did not appear")


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    if len(args) != 2:
        raise SystemExit("usage: qemu_hmp_screendump.py <monitor-socket> <ppm-path>")

    socket_path = Path(args[0])
    ppm_path = Path(args[1])
    wait_for_socket(socket_path)

    monitor = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    for _ in range(50):
        try:
            monitor.connect(str(socket_path))
            break
        except OSError:
            time.sleep(0.1)
    else:
        raise SystemExit("could not connect to QEMU monitor")

    time.sleep(2.0)
    for command in [f"screendump {ppm_path}\n".encode("utf-8"), b"quit\n"]:
        monitor.sendall(command)
        time.sleep(0.5)
    monitor.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
