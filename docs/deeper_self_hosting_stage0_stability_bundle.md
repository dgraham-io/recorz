# Stage 0 Stability Bundle

This note records the completed Stage 0 baseline for the deeper self-hosting program.

## Scope Completed

- Re-ran the RV32 base bundle against the current development-home, read-only detail, snapshot reopen, and lowering surfaces.
- Re-ran the honest RV64 smoke/build guard to keep the validation lane from silently drifting red before Stage 2 begins.
- Reconfirmed that the current self-hosting program starts from the post-Phase-1 baseline rather than from a partially degraded dev-loop state.

## Deferred Items Kept Out Of Scope

- RV64 full TextUI parity remains out of scope. Stage 0 only requires the existing RV64 smoke test and `all` build lane to stay green.
- Existing compiler/bootstrap ownership still lives in the host builder. Stage 0 intentionally did not widen into Stage 2 or Stage 3 implementation work.

## Verification Commands Run

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_can_open_the_memory_report_and_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_can_open_the_runtime_metadata_browser_and_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_memory_report_absorbs_enter_and_can_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_runtime_metadata_absorbs_enter_and_can_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_object_inspector_detail_absorbs_enter_and_can_return`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_runtime_metadata_browser_renders_and_returns_to_the_opening_menu tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_memory_report_renders_and_returns_to_the_opening_menu tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_object_inspector_detail_renders_and_returns_to_the_list`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_version_mismatch_reports_the_expected_rv32_profile tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_fresh_development_home_snapshot_can_return_from_read_only_views_after_returning_from_workspace tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_fresh_development_home_snapshot_runtime_metadata_absorbs_enter_and_can_return`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv64_validation_smoke`
- `make -C /Users/david/repos/recorz/platform/qemu-riscv32 BUILD_DIR=/tmp/recorz-qemu-riscv32-deeper-self-hosting-base all`
- `make -C /Users/david/repos/recorz/platform/qemu-riscv64 BUILD_DIR=/tmp/recorz-qemu-riscv64-deeper-self-hosting-guard all`

## Remaining Risks Or Non-Goals

- The RV64 build still produces warnings in `vm.c`, but the honest validation contract for this plan is smoke/build health, not warning elimination.
- The RV32 tool surface is stable for the Stage 0 matrix, but Stage 2 still needs to expand runtime metadata and regenerated-source coherence without reintroducing the recent read-only input regressions.
