# Deeper Self-Hosting Execution Plan

This file is the follow-on execution plan selected by Stage 5 of [docs/post_phase_1_execution_plan.md](/Users/david/repos/recorz/docs/post_phase_1_execution_plan.md).

It keeps the same task-level structure, dependency rules, and verification discipline as the completed post-Phase-1 plan.
The goal is to move more compiler and regeneration responsibility into the image without widening the VM or weakening the RV32 `32M` development path.

## How To Use This Plan

1. Always take the earliest incomplete stage with a blocking open task.
2. Within that stage, take the first unblocked task in the listed order.
3. If a task touches an Integration Lane file, do not run another Integration Lane task in parallel.
4. End every completed task or tight task bundle with:
   - targeted verification,
   - a docs update if task state changed materially,
   - a commit,
   - a push.
5. Keep RV32 as the primary path, RV64 as validation, and `32M` as a hard limit.
6. Keep host builders bootstrap-only.
7. Do not move tool policy or compiler ownership back into C or host Python when the image can own it honestly.

## Default Non-Blocking Decisions

- If `DS0.1` finds a regression in the current RV32 primary flows, fix it inside Stage 0 before starting Stage 1. Do not carry known red regressions forward as TODOs.
- If `DS0.2` finds a failure outside the honest RV64 smoke/build lane, record it as out of scope and continue. If RV64 smoke or `all` fails, Stage 0 is not complete.
- Unless there is a strong reason not to, use these stage note files instead of inventing new filenames mid-run:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage0_stability_bundle.md`
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage1_inventory.md`
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage2_runtime_metadata_audit.md`
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage3_manifest_boundary_note.md`
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage4_closure_matrix.md`
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage4_handoff_note.md`
- Every stage note should include:
  - scope completed
  - deferred items kept out of scope
  - verification commands run
  - remaining risks or non-goals
- If a task is docs-only, still update this plan when task state changes materially and still end the slice with a commit and push.
- If a self-hosting candidate would require widening the C VM, weakening the RV32 `32M` contract, or moving policy back into host Python, defer it and record why instead of stopping for human input.
- Stage 2 should expand the existing `Runtime Metadata` / regenerated-source surfaces before inventing new UI surfaces.
- Stage 4 should default the handoff note to `continue deeper self-hosting` unless the recorded closure matrix shows this lane is blocked or exhausted.

## Required Verification Bundles

### `DS-BUNDLE-RV32-BASE`

Run this bundle for `DS0.1` and again inside `DS4.1`:

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_can_open_the_memory_report_and_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_menu_can_open_the_runtime_metadata_browser_and_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_memory_report_absorbs_enter_and_can_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_runtime_metadata_absorbs_enter_and_can_return tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_workspace_development_home_object_inspector_detail_absorbs_enter_and_can_return`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_runtime_metadata_browser_renders_and_returns_to_the_opening_menu tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_memory_report_renders_and_returns_to_the_opening_menu tests.test_qemu_riscv32_render_integration.QemuRiscv32RenderIntegrationTests.test_development_home_object_inspector_detail_renders_and_returns_to_the_list`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_version_mismatch_reports_the_expected_rv32_profile tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_fresh_development_home_snapshot_can_return_from_read_only_views_after_returning_from_workspace tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_fresh_development_home_snapshot_runtime_metadata_absorbs_enter_and_can_return`
- `make -C /Users/david/repos/recorz/platform/qemu-riscv32 BUILD_DIR=/tmp/recorz-qemu-riscv32-deeper-self-hosting-base all`

### `DS-BUNDLE-RV64-GUARD`

Run this bundle for `DS0.2` and again inside `DS4.1`:

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv64_validation_smoke`
- `make -C /Users/david/repos/recorz/platform/qemu-riscv64 BUILD_DIR=/tmp/recorz-qemu-riscv64-deeper-self-hosting-guard all`

### `DS-BUNDLE-REGEN`

Run this bundle whenever a task changes regeneration authority, manifest ownership, or runtime-binding summaries:

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_regeneration_integration`
- `python3 -m py_compile /Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py /Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`

## Current Position

As of `2026-03-22`, the project begins this plan from the following base:

- the RV32 native workspace/browser/debugger/process tool stack is complete and stable enough to support the next program
- Stage 4 of the post-Phase-1 program is complete, so the host-builder boundary is explicit and tested
- direct regenerated-source browsing, `dev-regenerate-boot-source`, regeneration authority checks, and runtime metadata browsing already exist
- the host builders still own source discovery, lowering, manifest emission, and runtime-binding generation
- recent RV32 stability work closed stale-snapshot compatibility drift and read-only view input regressions, so Stage 0 should treat those as baseline guarantees rather than new scope

Immediate next queue:

1. `DS2.1` Runtime-metadata surface expansion
2. `DS2.2` Image-visible runtime-binding summary
3. `DS2.3` Regenerated-source browser coherence

## Working Lanes

### Lane A: Host-Builder Thinning

Owns:

- `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
- `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv_mvp_lowering.py`
- `/Users/david/repos/recorz/docs/`

Responsibilities:

- keep host scripts mechanical
- isolate derivable metadata from emission code
- prevent policy creep back into Python

### Lane B: Image-Owned Compiler / Bootstrap State

Owns:

- `/Users/david/repos/recorz/kernel/mvp/`
- `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`

Responsibilities:

- surface compiler/bootstrap state in the image
- keep regenerated source and runtime metadata browsable
- move policy upward when a model object can own it

### Lane C: Verification And Regeneration

Owns:

- `/Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`

Responsibilities:

- regeneration smoke
- snapshot/reopen checks
- source-ownership and builder-smoke assertions

### Integration Lane

These files should be treated as shared integration points and changed by one agent at a time:

- `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
- `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
- `/Users/david/repos/recorz/README.md`
- `/Users/david/repos/recorz/TODO.md`
- `/Users/david/repos/recorz/docs/post_phase_1_roadmap.md`

## Global Completion Rules

- Prefer the smallest slice that leaves the image owning more of its compiler/bootstrap story.
- Keep regeneration, snapshot reopen, and `dev-loop` stronger than before.
- Do not weaken the explicit Stage 4 boundary just to make this plan look larger.
- When a potential self-hosting step is not yet honest, document it as deferred instead of hiding it behind a stale test.

## Stage 0. Keep The Current Base Stable

Goal:
Preserve the existing RV32 development loop while the self-hosting slice moves.

### Open Tasks

- [x] `DS0.1` Base stability recheck
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage0_stability_bundle.md`
  Work:
  - rerun the Stage 4 closure matrix before broadening self-hosting work
  - make any stale-snapshot or return-path regressions explicit before continuing
  Verify:
  - `DS-BUNDLE-RV32-BASE`
  - recorded base stability bundle

- [x] `DS0.2` RV64 validation guard
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv64_validation_smoke.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_render_integration.py`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage0_stability_bundle.md`
  Work:
  - keep the honest RV64 smoke suite green while Stage 0 lands
  - avoid letting self-hosting work silently break the validation target
  Verify:
  - `DS-BUNDLE-RV64-GUARD`

## Stage 1. Inventory Bootstrap-Only Responsibilities

Goal:
Classify what the host path truly needs to own.

### Open Tasks

- [x] `DS1.1` Host source-discovery and registry inventory
  Files:
  - `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
  - `/Users/david/repos/recorz/docs/`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage1_inventory.md`
  Depends on:
  - `DS0.1`
  Work:
  - enumerate builder-owned source discovery, kernel registry, selector scan, class scan, and boot-object scan behavior
  - classify each item as:
    - bootstrap-only host responsibility to keep
    - image-owned metadata candidate to move
    - mechanical marshalling helper
  Required sections:
  - builder-owned discovery inventory
  - bootstrap-only responsibilities to keep
  - image-owned metadata candidates
  - mechanical marshalling helpers
  Verify:
  - updated stage 1 inventory note
  - `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`

- [x] `DS1.2` Runtime-binding handoff inventory
  Files:
  - `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
  - `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
  - `/Users/david/repos/recorz/docs/`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage1_inventory.md`
  Depends on:
  - `DS1.1`
  Work:
  - document which runtime-binding facts must stay host-generated
  - identify which selector/source metadata can become image-authored inputs instead of host-owned policy
  Required sections:
  - host-generated runtime-binding facts that must remain
  - selector/source metadata candidates for image authorship
  - blockers or reasons to defer specific candidates
  Verify:
  - updated stage 1 inventory note
  - `python3 -m py_compile /Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py /Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`

- [x] `DS1.3` Stage 1 closure note
  Files:
  - `/Users/david/repos/recorz/docs/`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage1_inventory.md`
  Depends on:
  - `DS1.1`
  - `DS1.2`
  Work:
  - record the bootstrap-only inventory and the next image-owned candidates
  Required sections:
  - recommended Stage 2 scope
  - explicit non-goals to keep out of Stage 2
  Verify:
  - docs review

## Stage 2. Expose Compiler / Bootstrap State In The Image

Goal:
Make the live image able to browse the state that the host currently emits.

### Open Tasks

- [ ] `DS2.1` Runtime-metadata surface expansion
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage2_runtime_metadata_audit.md`
  Depends on:
  - `DS1.3`
  Work:
  - expand the current runtime-metadata surface only with high-signal compiler/bootstrap facts
  - keep the browser readable and avoid dumping opaque host internals
  Default scope:
  - extend the existing `Runtime Metadata` browser entry
  - prefer counts, ownership summaries, and browsable anchors over raw dumps
  Verify:
  - serial and render metadata tests

- [ ] `DS2.2` Image-visible runtime-binding summary
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage2_runtime_metadata_audit.md`
  Depends on:
  - `DS2.1`
  Work:
  - make the image able to explain the binding/runtime surface that the host builder currently emits
  - prefer concise summaries and browsable source anchors over raw host output dumps
  Verify:
  - regeneration and serial metadata tests
  - `DS-BUNDLE-REGEN`

- [ ] `DS2.3` Regenerated-source browser coherence
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage2_runtime_metadata_audit.md`
  Depends on:
  - `DS2.1`
  Work:
  - keep regenerated-source browse/return/save flows coherent as the metadata surface grows
  - preserve the explicit Stage 4 deferred non-goal around source-editor shortcut smoke unless that path is intentionally restored
  Verify:
  - snapshot and render tests

## Stage 3. Move Derivable Manifest Generation Upward

Goal:
Reduce the amount of policy hidden in host Python.

### Open Tasks

- [ ] `DS3.1` Selector/source manifest candidate extraction
  Files:
  - `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
  - `/Users/david/repos/recorz/kernel/mvp/`
  - `/Users/david/repos/recorz/docs/`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage3_manifest_boundary_note.md`
  Depends on:
  - `DS2.3`
  Work:
  - identify derivable selector/source manifest data that can move to image-owned descriptions
  - keep binary/image emission in Python while reducing policy ownership there
  Required sections:
  - selector/source facts that remain bootstrap-only
  - selector/source facts that can become image-authored
  - why each retained host responsibility is still necessary
  Verify:
  - updated stage 3 boundary note
  - `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`

- [ ] `DS3.2` Host marshalling boundary shrink
  Files:
  - `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
  - `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_mvp_lowering.py`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage3_manifest_boundary_note.md`
  Depends on:
  - `DS3.1`
  Work:
  - replace policy-heavy helpers with input/serialization helpers where practical
  - keep the host builder consuming explicit source and metadata rather than inventing them
  Verify:
  - lowering tests
  - `python3 -m py_compile /Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py /Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`

- [ ] `DS3.3` Regeneration authority recheck
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage3_manifest_boundary_note.md`
  Depends on:
  - `DS3.2`
  Work:
  - prove the updated self-hosting boundary still preserves source authority, regenerated output alignment, and runtime-binding consistency
  Verify:
  - `DS-BUNDLE-REGEN`

## Stage 4. Closure Matrix And Hand-Off

Goal:
Close the deeper self-hosting tranche cleanly and decide the next program.

### Open Tasks

- [ ] `DS4.1` Closure matrix
  Files:
  - `/Users/david/repos/recorz/tests/`
  - `/Users/david/repos/recorz/docs/`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage4_closure_matrix.md`
  Depends on:
  - `DS0.2`
  - `DS3.3`
  Work:
  - rerun lowering, regeneration, serial, render, snapshot, RV32 build, and RV64 smoke/build coverage
  - record any remaining non-goals explicitly
  Verify:
  - `DS-BUNDLE-RV32-BASE`
  - `DS-BUNDLE-RV64-GUARD`
  - `DS-BUNDLE-REGEN`
  - recorded closure matrix

- [ ] `DS4.2` Handoff note
  Files:
  - `/Users/david/repos/recorz/docs/`
  Outputs:
  - `/Users/david/repos/recorz/docs/deeper_self_hosting_stage4_handoff_note.md`
  Depends on:
  - `DS4.1`
  Work:
  - decide whether the next program continues deeper self-hosting or returns to another deferred direction
  - record that decision without re-opening the old backlog
  Verify:
  - docs review

## Program Complete When

This plan is complete when the image can explain and rebuild more of its own compiler/bootstrap state, while the host path remains clearly bootstrap-only and the RV32 `32M` contract stays intact.
