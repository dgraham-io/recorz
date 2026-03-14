#!/usr/bin/env python3

import argparse
import hashlib
import json
import re
import shutil
import socket
import statistics
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
QEMU_PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
WORKSPACE_PACKAGE_HOME_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_package_home_demo.rz"
WORKSPACE_SCROLL_COPY_EXAMPLE = ROOT / "examples" / "qemu_riscv_workspace_scroll_copy_demo.rz"

SCENARIOS = {
    "full-pane-redraw": {
        "example": WORKSPACE_PACKAGE_HOME_EXAMPLE,
        "input_chunks": (b"\x18", b"A", b"\x1f"),
        "description": "Open a package source editor, type one character, dump render counters.",
    },
    "cursor-only-redraw": {
        "example": WORKSPACE_PACKAGE_HOME_EXAMPLE,
        "input_chunks": (b"\x18", b"\x1b[B", b"\x1f"),
        "description": "Open a package source editor, move the cursor once, dump render counters.",
    },
    "package-source-open": {
        "example": WORKSPACE_PACKAGE_HOME_EXAMPLE,
        "input_chunks": (b"\x18", b"\x1f"),
        "description": "Open the selected package source and dump render counters.",
    },
    "pane-scroll-copy": {
        "example": WORKSPACE_SCROLL_COPY_EXAMPLE,
        "input_chunks": (b"\x1b[B",) * 22 + (b"\x1f",),
        "description": "Scroll the editor viewport until it uses rect-copy and dump render counters.",
    },
}


def parse_render_counters(log: str) -> dict[str, int]:
    matches = re.findall(
        r"recorz-render-counters "
        r"editor_full=(\d+) editor_pane=(\d+) editor_cursor=(\d+)"
        r"(?: editor_scroll=(\d+))?"
        r"(?: editor_status=(\d+))? "
        r"browser_full=(\d+) browser_list=(\d+)",
        log,
    )
    if not matches:
        raise RuntimeError("expected recorz-render-counters in QEMU log")
    editor_full, editor_pane, editor_cursor, editor_scroll, editor_status, browser_full, browser_list = matches[-1]
    return {
        "editor_full": int(editor_full),
        "editor_pane": int(editor_pane),
        "editor_cursor": int(editor_cursor),
        "editor_scroll": int(editor_scroll or 0),
        "editor_status": int(editor_status or 0),
        "browser_full": int(browser_full),
        "browser_list": int(browser_list),
    }


def benchmark_build_dir(example_path: Path, scenario_name: str) -> Path:
    digest = hashlib.sha1(f"{example_path.stem}|{scenario_name}".encode("utf-8")).hexdigest()[:10]
    return ROOT / "misc" / f"qirv32b-{digest}"


def build_example(build_dir: Path, example_path: Path) -> float:
    command = [
        "make",
        "-C",
        str(QEMU_PLATFORM_DIR),
        f"BUILD_DIR={build_dir}",
        f"EXAMPLE={example_path}",
        "clean",
        "all",
    ]
    start = time.perf_counter()
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    elapsed = time.perf_counter() - start
    if result.returncode != 0:
        raise RuntimeError(
            "benchmark build failed\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return elapsed


def run_interactive_session(
    build_dir: Path,
    example_path: Path,
    input_chunks: tuple[bytes, ...],
) -> tuple[float, dict[str, int], str]:
    ppm_path = build_dir / "recorz-qemu-riscv32-mvp.ppm"
    qemu_log_path = build_dir / "qemu.log"
    monitor_sock = build_dir / "monitor.sock"
    elf_path = build_dir / "recorz-qemu-riscv32-mvp.elf"
    qemu_command = [
        "qemu-system-riscv32",
        "-machine",
        "virt",
        "-m",
        "32M",
        "-smp",
        "1",
        "-kernel",
        str(elf_path),
        "-serial",
        "stdio",
        "-display",
        "none",
        "-device",
        "ramfb",
        "-device",
        "virtio-keyboard-device",
        "-monitor",
        f"unix:{monitor_sock},server,nowait",
    ]

    build_dir.mkdir(parents=True, exist_ok=True)
    for path in (ppm_path, qemu_log_path, monitor_sock):
        if path.exists():
            path.unlink()

    with qemu_log_path.open("w", encoding="utf-8") as log_file:
        process = subprocess.Popen(
            qemu_command,
            cwd=ROOT,
            stdin=subprocess.PIPE,
            stdout=log_file,
            stderr=subprocess.STDOUT,
        )
        start = time.perf_counter()
        try:
            monitor = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            time.sleep(1.0)
            for chunk in input_chunks:
                assert process.stdin is not None
                process.stdin.write(chunk)
                process.stdin.flush()
                time.sleep(0.35)
            if input_chunks:
                time.sleep(4.0)
            try:
                for _ in range(100):
                    if monitor_sock.exists():
                        break
                    time.sleep(0.1)
                for _ in range(100):
                    try:
                        monitor.connect(str(monitor_sock))
                        break
                    except OSError:
                        time.sleep(0.1)
                else:
                    raise RuntimeError("benchmark monitor socket did not accept a connection")
                monitor.sendall(f"screendump {ppm_path}\n".encode("utf-8"))
                time.sleep(0.7)
                monitor.sendall(b"quit\n")
                time.sleep(0.7)
            finally:
                monitor.close()
        finally:
            if process.poll() is None:
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
            else:
                process.wait(timeout=5)
            if process.stdin is not None:
                try:
                    process.stdin.close()
                except BrokenPipeError:
                    pass
        run_seconds = time.perf_counter() - start

    qemu_log = qemu_log_path.read_text(encoding="utf-8", errors="ignore")
    if "panic:" in qemu_log.replace("\r", ""):
        raise RuntimeError(f"benchmark scenario {example_path.name} hit a panic\n{qemu_log}")
    return run_seconds, parse_render_counters(qemu_log), qemu_log


def benchmark_scenario(name: str, repeats: int) -> dict[str, object]:
    scenario = SCENARIOS[name]
    example_path = scenario["example"]
    input_chunks = scenario["input_chunks"]
    build_dir = benchmark_build_dir(example_path, name)
    build_seconds = build_example(build_dir, example_path)
    run_seconds = []
    counters = []

    for _ in range(repeats):
        elapsed, render_counters, _ = run_interactive_session(build_dir, example_path, input_chunks)
        run_seconds.append(elapsed)
        counters.append(render_counters)

    return {
        "scenario": name,
        "description": scenario["description"],
        "example": str(example_path.relative_to(ROOT)),
        "repeats": repeats,
        "build_seconds": round(build_seconds, 3),
        "run_seconds": [round(value, 3) for value in run_seconds],
        "median_run_seconds": round(statistics.median(run_seconds), 3),
        "counters": counters,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Benchmark focused RV32 render paths with QEMU counter dumps.")
    parser.add_argument(
        "scenarios",
        nargs="*",
        help="render scenarios to run",
    )
    parser.add_argument("--repeat", type=int, default=1, help="number of QEMU runs per scenario")
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    args = parser.parse_args()

    if args.repeat <= 0:
        print("--repeat must be positive", file=sys.stderr)
        return 2
    for scenario_name in args.scenarios:
        if scenario_name not in SCENARIOS:
            print(
                f"unknown scenario: {scenario_name} (choose from {', '.join(sorted(SCENARIOS))})",
                file=sys.stderr,
            )
            return 2
    if shutil.which("qemu-system-riscv32") is None or shutil.which("riscv64-unknown-elf-gcc") is None:
        print("qemu-system-riscv32 and riscv64-unknown-elf-gcc are required", file=sys.stderr)
        return 2

    scenario_names = args.scenarios or [
        "full-pane-redraw",
        "cursor-only-redraw",
        "package-source-open",
        "pane-scroll-copy",
    ]
    results = [benchmark_scenario(name, args.repeat) for name in scenario_names]
    if args.json:
        print(json.dumps(results, indent=2))
        return 0

    for result in results:
        print(result["scenario"])
        print(f"  example: {result['example']}")
        print(f"  description: {result['description']}")
        print(f"  build_seconds: {result['build_seconds']}")
        print(f"  run_seconds: {result['run_seconds']}")
        print(f"  median_run_seconds: {result['median_run_seconds']}")
        print("  counters:")
        for index, counter_set in enumerate(result["counters"], start=1):
            print(f"    run {index}: {json.dumps(counter_set, sort_keys=True)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
