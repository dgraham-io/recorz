# Recorz

Recorz is an experimental Smalltalk/Strongtalk-inspired system built around a live image, a minimal inspectable VM, and a path toward self-hosted development on RISC-V. The project is focused on bringing up a clean RV32-first environment where source, tools, and runtime behavior stay visible and understandable as the system grows.

Today, Recorz already boots on QEMU RISC-V with framebuffer output, live source loading, snapshots, and an in-image workspace/editor. The long-term goal is a compact but expressive system that remains faithful to classic image-based development while opening room for gradual typing, systems work, hardware description, and capability-oriented isolation.

![In-image Browser/Workspace](assets/images/browser_sreenshot.png)

*The screenshot shows the live package browser and workspace editor running on the RV32 target.*

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

For the normal RV32 in-image development loop, use the auto-resume target:

```sh
make -C /Users/david/repos/recorz/platform/qemu-riscv32 dev-loop
```

`dev-loop` reopens the saved image automatically after `Ctrl-W`, so save/resume feels like one continuous development session. The older one-shot entry remains available as `dev-interactive`.

In the interactive workspace, the primary commands are:
- `Ctrl-D` do it
- `Ctrl-P` print it
- `Ctrl-X` accept
- `Ctrl-O` close/return
- `Ctrl-W` save the snapshot and continue when launched through `dev-loop`
- `Ctrl-K` save a checkpointed image state

If you need to roll back to the previous saved image:

```sh
make -C /Users/david/repos/recorz/platform/qemu-riscv32 dev-restore
```
