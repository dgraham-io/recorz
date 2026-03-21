# Post Phase 1 Execution Plan

This file turns [docs/post_phase_1_roadmap.md](/Users/david/repos/recorz/docs/post_phase_1_roadmap.md) into a concrete task queue that can be executed incrementally after the completed Bluebook-parity program.

It is the working plan for the next development program. The roadmap remains the high-level sequence and guardrails; this file is the task-level breakdown intended to be followed without re-planning the project every turn.

## How To Use This Plan

1. Always take the earliest incomplete stage with a blocking open task.
2. Within that stage, take the first unblocked task in the listed order.
3. If a task touches an Integration Lane file, do not run another Integration Lane task in parallel.
4. End every completed task or tight task bundle with:
   - targeted verification,
   - an update to this file if task state changed materially,
   - a commit,
   - a push.
5. Do not skip ahead into later stages because a later task looks easier.
6. Keep RV32 and the `32M` machine target as hard constraints throughout.
7. Keep RV64 honest, but do not let RV64 pull the project into a second product line.

## Current Position

As of `2026-03-21`, the project appears to be here:

- Bluebook-parity execution plan: complete on the RV32 primary path.
- Stage 1: complete.
- Stage 2: open.
- Stage 3: open.
- Stage 4: open.
- Stage 5: open.

Immediate status note as of `2026-03-21`:

- Stage 1 is now closed on the RV32 primary path with targeted coverage for fresh `dev-loop` reset/restore, stale snapshot diagnostics, opening-menu exit flows, detail/source/editor returns, repeated save/reopen, regeneration alignment, and the RV32 build gate
- two explicit Stage 1 non-goals remain deferred rather than hidden:
  - `Workspace saveAndReopen` does not yet preserve class-browser restore
  - regenerated boot-source browser snapshot emission remains outside the minimum reopen contract
- the native VM is narrower than it was before the Bluebook-parity program, but `vm.c` still carries tool/session behavior that should move upward into image-side models
- RV64 is currently an honest minimal validation target with build coverage and a base smoke suite, not full default-TextUI parity
- the host builders are still intentionally mechanical, but the next meaningful architectural gain is to shift more compiler/regeneration responsibility into the image

## Working Lanes

### Lane A: RV32 Product Reliability

Owns:

- `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
- `/Users/david/repos/recorz/platform/qemu-riscv32/Makefile`

Responsibilities:

- cold-start and `dev-loop` reliability
- save/resume/recovery behavior
- opening-menu and return-path coverage
- redraw and framebuffer regressions

### Lane B: Runtime Boundary And Native VM Reduction

Owns:

- `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
- `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
- `/Users/david/repos/recorz/platform/shared/`
- `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
- `/Users/david/repos/recorz/kernel/mvp/WorkspaceBrowserModel.rz`
- `/Users/david/repos/recorz/kernel/mvp/WorkspaceEditorModel.rz`
- `/Users/david/repos/recorz/kernel/mvp/WorkspaceReturnState.rz`

Responsibilities:

- primitive-boundary discipline
- remaining tool/session policy extraction
- RV32/RV64 shared helper consolidation
- session redraw contract cleanup

### Lane C: RV64 Validation

Owns:

- `/Users/david/repos/recorz/platform/qemu-riscv64/`
- `/Users/david/repos/recorz/tests/test_qemu_riscv64_validation_smoke.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv_render_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv_snapshot_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv_dev_loop_integration.py`

Responsibilities:

- RV64 build integrity
- smoke-suite parity against the shared runtime contract
- explicit documentation of remaining RV64 gaps

### Lane D: Self-Hosting And Bootstrap Thinning

Owns:

- `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
- `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv_mvp_lowering.py`
- `/Users/david/repos/recorz/docs/`

Responsibilities:

- regeneration and builder ownership
- image-visible compiler/bootstrap state
- keeping the host path mechanical
- preparing the next self-hosting slice

### Integration Lane

These files should be treated as shared integration points and changed by one agent at a time:

- `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
- `/Users/david/repos/recorz/kernel/mvp/Selector.rz`
- `/Users/david/repos/recorz/README.md`
- `/Users/david/repos/recorz/TODO.md`
- `/Users/david/repos/recorz/docs/post_phase_1_roadmap.md`

## Global Completion Rules

These apply to every task below:

- Prefer the smallest slice that leaves the system more correct and more testable.
- Add or strengthen tests before broadening behavior.
- Keep RV32 `32M` as the standard development path.
- Keep RV64 building even when a stage is primarily RV32-focused.
- Do not move tool policy back into C or host Python if image-side models can own it honestly.
- When a task exposes stale snapshot or upgrade incompatibilities, make the failure explicit before adding features.
- If a long-term topic becomes attractive early, record it and defer it unless the current stage explicitly allows it.

## Stage 1. Harden The Current RV32 Experience

Goal:
Make the current native development loop boring and reliable.

### Completed Tasks

- [x] `PP1.1` Fresh `dev-loop` cold-start and reset coverage
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  - `/Users/david/repos/recorz/platform/qemu-riscv32/Makefile`
  Work:
  - assert `dev-reset` followed by `dev-loop` reaches the expected opening tool surface from a clean state
  - cover initial snapshot creation, reopen, and `dev-restore`
  - keep the normal RV32 interactive loop reproducible without manual cleanup
  Verify:
  - targeted RV32 serial and snapshot tests

- [x] `PP1.2` Snapshot mismatch and stale-state diagnostics
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/tools/inspect_qemu_riscv_snapshot.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  Work:
  - keep profile/version mismatches explicit
  - make stale dev-snapshot incompatibility obvious in logs and inspection output
  - point users toward `dev-reset` or `dev-restore` when recovery is possible
  Verify:
  - snapshot mismatch and recovery tests

- [x] `PP1.3` Opening-menu exit matrix
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  Work:
  - add explicit open-and-return coverage for:
    - `Workspace`
    - `Class Browser`
    - `Project Browser`
    - `Memory Report`
    - `Object Inspector`
    - `Process Browser`
    - `Runtime Metadata`
  - ensure each item can exit using its intended command path
  Verify:
  - serial and framebuffer opening-menu tests

- [x] `PP1.4` Detail/source/editor return matrix
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  Work:
  - add explicit return coverage for:
    - object-inspector detail
    - debugger detail modes
    - process-browser debugger handoff/return
    - class source
    - package source
    - regenerated source
    - plain workspace/editor restore
  - ensure return paths repaint correctly, not just update serial state
  Verify:
  - serial, render, and snapshot return-path tests

- [x] `PP1.5` Repeated save/reopen reproducibility
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  - `/Users/david/repos/recorz/platform/qemu-riscv32/Makefile`
  Work:
  - prove repeated save/reopen cycles preserve the same active tool state
  - cover at least:
    - plain workspace
    - package browser/source
    - object inspector
    - process browser/debugger
  Verify:
  - targeted snapshot reopen tests

- [x] `PP1.6` Stage 1 closure matrix
  Files:
  - `/Users/david/repos/recorz/tests/`
  - `/Users/david/repos/recorz/docs/post_phase_1_roadmap.md`
  Work:
  - run a consolidated RV32 reliability matrix for serial, framebuffer, snapshot, regeneration, and build gates
  - record any intentionally deferred non-goals before moving to Stage 2
  Verify:
  - recorded RV32 matrix run

Recorded Stage 1 closure matrix on `2026-03-21`:

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_dev_loop_integration`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_version_mismatch_reports_the_expected_rv32_profile`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_browser_state_without_demo_specific_program`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_class_source_browser_state_without_demo_specific_program tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_package_source_browser_state_without_demo_specific_program`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_object_inspector_state_across_repeated_cycles tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_debugger_state_across_repeated_cycles tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_browser_state_via_workspace_save_and_reopen tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_regenerated_boot_source_browser_state_without_demo_specific_program`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_workspace_can_return_to_the_opening_menu tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_project_browser_can_return_to_the_opening_menu tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_package_source_can_return_to_the_project_browser tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_class_browser_can_return_to_the_opening_menu tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_class_source_can_return_to_the_class_browser tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_can_open_the_memory_report_and_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_can_open_the_runtime_metadata_browser_and_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_object_inspector_can_return_to_the_opening_menu tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_object_inspector_detail_can_return_to_the_list tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_process_browser_can_return_to_the_opening_menu tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_process_browser_debugger_can_return_to_the_process_browser`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_runtime_metadata_browser_renders_and_returns_to_the_opening_menu`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_class_browser_renders_and_returns_to_the_opening_menu`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_memory_report_renders_and_returns_to_the_opening_menu`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_class_source_renders_and_returns_to_the_class_browser`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_package_source_renders_and_returns_to_the_project_browser`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_object_inspector_detail_renders_and_returns_to_the_list`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_process_browser_debugger_return_restores_the_selected_process`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_regeneration_integration.QemuRiscv32RegenerationIntegrationTests.test_host_builder_can_use_regenerated_kernel_source_as_the_kernel_authority tests.test_qemu_riscv32_regeneration_integration.QemuRiscv32RegenerationIntegrationTests.test_regenerated_boot_source_recreates_seeded_class_edit_state_after_cold_boot tests.test_qemu_riscv32_regeneration_integration.QemuRiscv32RegenerationIntegrationTests.test_image_can_emit_regenerated_boot_source_that_recreates_the_same_framebuffer_state tests.test_qemu_riscv32_regeneration_integration.QemuRiscv32RegenerationIntegrationTests.test_source_authority_and_runtime_bindings_keep_debugger_inspector_and_process_browser_metadata_aligned tests.test_qemu_riscv32_regeneration_integration.QemuRiscv32RegenerationIntegrationTests.test_dev_snapshot_can_regenerate_sources_from_current_image_state`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`
- `make -C /Users/david/repos/recorz/platform/qemu-riscv32 BUILD_DIR=/tmp/recorz-qemu-riscv32-stage1-closure all`

## Stage 2. Reduce Native VM Policy

Goal:
Keep shrinking the C runtime toward a primitive boundary.

### Open Tasks

- [x] `PP2.1` Runtime policy inventory
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/docs/post_phase_1_roadmap.md`
  Work:
  - enumerate remaining tool/session decisions still owned by C
  - classify each item as:
    - primitive/runtime substrate to keep
    - image-side model behavior to move
    - mechanical helper to factor
  Verify:
  - docs update plus lowering/build checks

  Current inventory after the first Stage 2 slice:
  - moved out of direct C policy ownership:
    - plain-workspace capture now delegates to image-side `BootWorkspaceSession>>rememberPlainWorkspaceStateIfNeeded` through the shared helper in `/Users/david/repos/recorz/platform/shared/recorz_mvp_workspace_plain_state_impl.h`
    - RV32 incremental image-session list redraw now asks `BootWorkspaceSession>>currentBrowserItemsVisibleFrom:count:` for visible items instead of hardcoding opening-menu/package/class contents in `vm.c`
  - still C-owned and remaining:
    - `workspace_remember_input_monitor_view` on both arches
    - `workspace_reopen_in_place` on both arches
    - RV32 image-session mode routing and some redraw-category interpretation
    - RV64 native interactive input-monitor command loop
    - browser/source target normalization in `workspace_source_text_for_browser_target`

- [ ] `PP2.2` Move remaining return/detail routing into image-side models
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/kernel/mvp/WorkspaceBrowserModel.rz`
  - `/Users/david/repos/recorz/kernel/mvp/WorkspaceEditorModel.rz`
  - `/Users/david/repos/recorz/kernel/mvp/WorkspaceReturnState.rz`
  - `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
  Work:
  - move browser return, opening-menu state, and read-only detail exit policy into model APIs where possible
  - keep `vm.c` concerned with render/update/input primitives, not menu semantics
  Verify:
  - serial, render, and snapshot return-path tests

- [x] `PP2.3` Thin the native session redraw contract
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  Work:
  - keep VM-side render codes limited to redraw categories
  - remove target-specific redraw or routing branches when image-side code can express them
  Verify:
  - lowering tests
  - targeted render tests
  - RV32 and RV64 builds

  Current result:
  - RV32 incremental image-session list redraw now asks `BootWorkspaceSession>>currentBrowserItemsVisibleFrom:count:` for visible items instead of hardcoding opening-menu/package/class contents in `vm.c`
  - the stale three-item opening-menu fallback was removed from the native redraw path

- [x] `PP2.4` Shared RV32/RV64 helper extraction
  Files:
  - `/Users/david/repos/recorz/platform/shared/`
  - `/Users/david/repos/recorz/platform/qemu-riscv32/`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/`
  Work:
  - factor identical native helpers into shared headers or sources
  - keep behavior unchanged while reducing duplication
  Verify:
  - RV32 `all`
  - RV64 `all`

  Current result:
  - `workspace_remember_view` and `workspace_capture_plain_return_state_if_needed` now live in `/Users/david/repos/recorz/platform/shared/recorz_mvp_workspace_plain_state_impl.h`
  - the shared plain-state helper now delegates to image-side session policy when `BootWorkspaceSession` exists and falls back to the older tool-based path for early bootstrap flows

- [ ] `PP2.5` Stage 2 closure matrix
  Files:
  - `/Users/david/repos/recorz/tests/`
  Work:
  - rerun the Stage 1 reliability matrix plus lowering/build checks after VM-surface cleanup
  Verify:
  - recorded RV32 matrix run
  - RV32 and RV64 builds

## Stage 3. Restore RV64 As Validation

Goal:
Keep RV64 honest so RV32 does not become the only maintained target.

### Open Tasks

- [ ] `PP3.1` Lock the current honest RV64 contract
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv64_validation_smoke.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_render_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_snapshot_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_dev_loop_integration.py`
  - `/Users/david/repos/recorz/docs/post_phase_1_roadmap.md`
  Work:
  - keep the current RV64 validation surface explicit
  - remove ambiguous skips or turn them into documented temporary non-goals
  Verify:
  - RV64 build plus current smoke tests

- [ ] `PP3.2` RV64 default-payload boot to opening menu
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_render_integration.py`
  Work:
  - restore enough selector/runtime/textui parity for RV64 to boot the current default payload to the opening menu
  - do not widen the contract beyond smoke validation in this slice
  Verify:
  - RV64 render smoke with default payload

- [ ] `PP3.3` RV64 runtime-metadata and browser smoke
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_render_integration.py`
  Work:
  - prove at least one menu-open/browser-return flow on RV64 after default-payload boot works without panic
  - prefer runtime metadata first because it is read-only and high-signal
  Verify:
  - targeted RV64 render/browser smoke

- [ ] `PP3.4` RV64 snapshot save/reload on the same payload
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_snapshot_integration.py`
  Work:
  - keep RV64 snapshot save/reload aligned with the same current UI/runtime contract being validated
  Verify:
  - targeted RV64 snapshot smoke

- [ ] `PP3.5` Stage 3 closure matrix
  Files:
  - `/Users/david/repos/recorz/tests/`
  - `/Users/david/repos/recorz/docs/`
  Work:
  - record the final RV64 validation surface
  - keep gaps explicit if any deeper RV64 tool parity remains out of scope
  Verify:
  - RV64 `all`
  - selected RV64 smoke suite

## Stage 4. Expand Self-Hosting

Goal:
Move more compiler and regeneration responsibility into the image while keeping the host path mechanical.

### Open Tasks

- [ ] `PP4.1` Host-builder ownership audit
  Files:
  - `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
  - `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
  - `/Users/david/repos/recorz/docs/post_phase_1_roadmap.md`
  Work:
  - identify which remaining builder responsibilities are truly bootstrap machinery
  - identify which ones should move into image-side source or metadata
  Verify:
  - docs update
  - builder smoke

- [ ] `PP4.2` Image-owned regeneration workflow completeness
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  Work:
  - make regeneration and regenerated-source browsing usable end-to-end from the image
  - keep save/reopen behavior coherent around those flows
  Verify:
  - regeneration and snapshot tests

- [ ] `PP4.3` Image-visible compiler/bootstrap state
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  Work:
  - expose more compile/lower/bootstrap state for browsing inside the image
  - keep the browser surface high-signal instead of dumping opaque host details
  Verify:
  - serial and render metadata/regeneration tests

- [ ] `PP4.4` Shift more manifest generation into the image
  Files:
  - `/Users/david/repos/recorz/tools/`
  - `/Users/david/repos/recorz/kernel/mvp/`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_mvp_lowering.py`
  Work:
  - move derivable selector/source metadata ownership upward where practical
  - keep host builders as marshalling tools rather than policy owners
  Verify:
  - lowering tests
  - builder smoke

- [ ] `PP4.5` Stage 4 closure matrix
  Files:
  - `/Users/david/repos/recorz/tests/`
  - `/Users/david/repos/recorz/docs/`
  Work:
  - rerun regeneration, snapshot, lowering, and build coverage after self-hosting changes
  Verify:
  - recorded self-hosting matrix run

## Stage 5. Reopen Long-Term Directions

Goal:
Reopen broader Recorz directions from a stable base without letting them collapse back into one undifferentiated backlog.

### Open Tasks

- [ ] `PP5.1` Readiness audit for deferred topics
  Files:
  - `/Users/david/repos/recorz/docs/`
  Work:
  - assess readiness for:
    - deeper self-hosting
    - richer process semantics
    - capability hooks
    - typed/tooling work
  - use the completion of Stages 1 through 4 as the gate, not ambition alone
  Verify:
  - docs review

- [ ] `PP5.2` Default priority selection rule
  Files:
  - `/Users/david/repos/recorz/docs/`
  Work:
  - if no human override is given, default the next major implementation program to deeper self-hosting
  - record why that choice is the least disruptive continuation of the current architecture
  Verify:
  - docs review

- [ ] `PP5.3` Design-gate note for each deferred direction
  Files:
  - `/Users/david/repos/recorz/docs/`
  Work:
  - define entry criteria, explicit non-goals, and expected proof artifacts for:
    - gradual typing
    - capability enforcement
    - HDL/systems-subset work
    - broader multiprocessing
  Verify:
  - docs review

- [ ] `PP5.4` Seed the next execution plan
  Files:
  - `/Users/david/repos/recorz/docs/`
  Work:
  - create the follow-on execution-plan document for whichever deferred direction is now selected
  - keep the same task-ID, dependency, and verification style used here
  Verify:
  - new execution-plan document

## Immediate Next Queue

Unless a blocking regression appears elsewhere, the next tasks should be tackled in this order:

1. `PP2.2` Move remaining return/detail routing into image-side models.
2. `PP2.5` Stage 2 closure matrix.
3. `PP3.1` Lock the current honest RV64 contract.
4. `PP3.2` RV64 default-payload boot to opening menu.

That sequence is intentional. The first Stage 2 slice is landed, but the reopen and input-monitor return policy is still the next reduction target before the program moves on to RV64 and self-hosting work.

## Program Complete When

This execution plan is complete when the project can:

- boot and reopen the RV32 development environment predictably from a cold state,
- save, restore, and recover tool state without manual guesswork,
- keep the native VM at a narrower primitive/runtime boundary than the current baseline,
- keep RV64 as an honest validation target against the same documented contract,
- move more compiler/regeneration responsibility into the image while leaving the host path mechanical,
- and choose the next long-term Recorz program from a stable, validated base instead of from an unstable development loop.
