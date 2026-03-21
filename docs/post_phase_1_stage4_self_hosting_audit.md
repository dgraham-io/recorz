# Stage 4 Self-Hosting Audit

This note records the completed Stage 4 boundary after the host-builder slice and the image-side regeneration/runtime-metadata checks were reconciled.

## Current Host Boundary

- `tools/build_qemu_riscv_mvp_image.py` remains the mechanical source of truth for the demo image, runtime bindings header, image manifest packing, and bootstrap/source lowering.
- `tools/generate_qemu_riscv_mvp_runtime_bindings_header.py` is still a thin wrapper around the builder.
- The builder now exposes a structured ownership summary and image-manifest layout metadata so the current host responsibilities are explicit and testable.

## PP4.1 Completion

- The host-builder role is now explicit in code and in tests.
- The builder still owns lowering and serialization, but the derivable metadata it consumes is surfaced as a structured summary rather than being only implicit in the byte packing path.

## PP4.4 Completion

- Manifest layout metadata is now factored out of the raw packing path in the builder.
- This keeps the host path mechanical while making section offsets and lengths testable before any image-side consumer changes.

## PP4.2 / PP4.3 Completion

- Direct regenerated-source browsing remains usable from inside the image.
- `dev-regenerate-boot-source` remains aligned with the current image state and regeneration authority.
- Recovery snapshots after regenerated-source return preserve the plain workspace state captured before the browse.
- Runtime metadata remains the image-visible high-signal compiler/bootstrap surface for Stage 4.
- Explicit deferred non-goal:
  - source-editor regenerated-source shortcut smoke is not part of the current Stage 4 contract.

## Verification

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_can_browse_regenerated_kernel_source_inside_the_image tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_can_open_the_runtime_metadata_browser_and_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_object_inspector_detail_can_return_to_the_list`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_runtime_metadata_browser_renders_and_returns_to_the_opening_menu tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_object_inspector_detail_renders_and_returns_to_the_list`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_recovery_snapshot_after_regenerated_source_return_preserves_plain_workspace_state`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_regeneration_integration.QemuRiscv32RegenerationIntegrationTests.test_host_builder_can_use_regenerated_kernel_source_as_the_kernel_authority tests.test_qemu_riscv32_regeneration_integration.QemuRiscv32RegenerationIntegrationTests.test_source_authority_and_runtime_bindings_keep_debugger_inspector_and_process_browser_metadata_aligned tests.test_qemu_riscv32_regeneration_integration.QemuRiscv32RegenerationIntegrationTests.test_dev_snapshot_can_regenerate_sources_from_current_image_state`
- `python3 -m py_compile /Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py /Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
- `make -C /Users/david/repos/recorz/platform/qemu-riscv32 BUILD_DIR=/tmp/recorz-qemu-riscv32-stage4-closure all`
