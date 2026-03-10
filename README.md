# Recorz

Recorz is an experimental Smalltalk/Strongtalk-inspired system built around a live image, a minimal inspectable VM, and a path toward self-hosted development on RISC-V. The project is focused on bringing up a clean RV32-first environment where source, tools, and runtime behavior stay visible and understandable as the system grows.

Today, Recorz already boots on QEMU RISC-V with framebuffer output, live source loading, snapshots, and an in-image workspace/editor. The long-term goal is a compact but expressive system that remains faithful to classic image-based development while opening room for gradual typing, systems work, hardware description, and capability-oriented isolation.
