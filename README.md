# Recorz

Recorz is an experimental Smalltalk/Strongtalk-inspired system built around a live image, a minimal inspectable VM, and a path toward self-hosted development on RISC-V. The project is focused on bringing up a clean RV32-first environment where source, tools, and runtime behavior stay visible and understandable as the system grows.

Today, Recorz already boots on QEMU RISC-V with framebuffer output, live source loading, snapshots, and an in-image workspace/editor. The long-term goal is a compact but expressive system that remains faithful to classic image-based development while opening room for gradual typing, systems work, hardware description, and capability-oriented isolation.

## Quick Start

Recorz currently targets QEMU RISC-V first. The main host-side prerequisites are:
- `qemu-system-riscv32`
- `riscv64-unknown-elf-gcc`
- `python3`
- `make`

To boot the current RV32 framebuffer demo:

```sh
make -C /Users/david/repos/recorz/platform/qemu-riscv32 run
```

To launch the interactive RV32 development image and continue working from the saved snapshot:

```sh
make -C /Users/david/repos/recorz/platform/qemu-riscv32 dev-interactive
```

In the interactive workspace, the primary commands are:
- `Ctrl-D` do it
- `Ctrl-P` print it
- `Ctrl-X` accept
- `Ctrl-O` close/return
- `Ctrl-W` save the snapshot and end the current session
