# Step 1: Bare-Metal Hello World on QEMU

## What was built

A minimal bare-metal RISC-V program that prints `Hello, recorz!` to the serial
console via QEMU's emulated UART. This is the very first piece of executable
code in the recorz project — the foundation on which the virtual machine will
be built.

## Architecture decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| ISA | **RV32I** | Minimal 32-bit base integer ISA. Simpler for FPGA targets, closer to the Xerox Alto philosophy. |
| QEMU machine | **virt** | Standard virtual platform with well-documented memory map. |
| Execution mode | **M-mode** | Bare metal starts in machine mode. No OS, no supervisor. |
| Output | **UART (ns16550a)** | Memory-mapped at `0x10000000` on QEMU virt. Simple byte-at-a-time TX. |
| Toolchain | **riscv64-elf-binutils + gcc** | Homebrew packages. The 64-bit toolchain handles RV32 via `-march=rv32i -mabi=ilp32`. |

## Memory map (QEMU virt, RV32)

| Address | What |
|---------|------|
| `0x10000000` | UART (ns16550a) |
| `0x80000000` | RAM base (kernel load address) |
| `0x80200000` | Stack top (2 MB above base) |

## Files

```
vm/
  src/boot.S    — Entry point (_start), UART print loop, halt
  link.ld       — Linker script: .text at 0x80000000, stack at top of 2MB RAM
  Makefile      — Build targets: build, run, debug, disasm, clean
```

## Prerequisites

```bash
brew install riscv64-elf-binutils riscv64-elf-gcc qemu
```

## Build and run

```bash
cd vm
make build    # Assemble and link → build/recorz.elf
make run      # Launch in QEMU (Ctrl-A X to quit)
make disasm   # Show disassembly
make debug    # Run with GDB stub (-s -S), connect with riscv64-elf-gdb
make clean    # Remove build artifacts
```

## What's next

This boot stub will evolve into the recorz virtual machine — a stack-based VM
with bytecode dispatch, memory management, and eventually framebuffer output
for the graphical development environment.
