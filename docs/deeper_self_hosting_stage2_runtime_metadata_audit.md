## Stage 2 Runtime Metadata Audit

### Scope Completed

- `DS2.1` expanded the existing `Runtime Metadata` browser entry instead of adding a new tool surface.
- Added high-signal compiler/bootstrap summary facts to the live image:
  - source count (`SRCS`)
  - primitive-binding owner count (`BOWN`)
  - selector registry consistency summary
  - primitive-binding/runtime-surface summary
  - explicit regenerated-source anchors for kernel, boot, and file-in source
- `DS2.2` made runtime-binding ownership visible from the image with concise summary rows rather than raw host dumps.
- `DS2.3` kept regenerated-source browsing coherent from `Runtime Metadata` by wiring anchored browse commands through the existing return-state model and preserving snapshot reopen of regenerated kernel source.
- Increased regenerated-source buffer capacity to match the file-in buffer so the expanded source-browsing path does not panic on regenerated source size.

### Deferred Items Kept Out Of Scope

- No new standalone runtime-metadata or regeneration UI was introduced beyond the existing `Runtime Metadata` and regenerated-source views.
- No compiler or tool policy moved back into C or host Python.
- Stage 2 does not attempt to make boot/file-in regenerated-source browsing a first-class serial-harness matrix; the broad serial assertions stay focused on the kernel regenerated-source path and on the live return-path contract.
- Stage 2 does not widen RV64 beyond its validation role.

### Verification Commands Run

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_can_open_the_runtime_metadata_browser_and_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_runtime_metadata_absorbs_enter_and_can_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_runtime_metadata_can_browse_regenerated_kernel_source_and_return`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_runtime_metadata_regenerated_kernel_source_and_return tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_fresh_development_home_snapshot_runtime_metadata_absorbs_enter_and_can_return`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_regeneration_integration`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_runtime_metadata_browser_renders_and_returns_to_the_opening_menu tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_runtime_metadata_can_open_regenerated_kernel_source_and_return`
- `python3 -m py_compile /Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py /Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
- `make -C /Users/david/repos/recorz/platform/qemu-riscv32 BUILD_DIR=/tmp/recorz-qemu-riscv32-stage2 all`
- `make -C /Users/david/repos/recorz/platform/qemu-riscv64 BUILD_DIR=/tmp/recorz-qemu-riscv64-stage2 all`

### Remaining Risks Or Non-Goals

- The image can now explain more of the emitted compiler/bootstrap surface, but the host builder still owns manifest discovery and emission policy; that is Stage 3 scope.
- Runtime metadata remains summary-oriented on purpose. It is not intended to dump raw host-side structures into the image.
- Regenerated-source return/save/reopen is asserted for the kernel regenerated-source path. Broader regenerated-source path coverage can expand later if it becomes necessary for an actual product path rather than plan completeness.
