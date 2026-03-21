# Bluebook VM Parity Execution Plan

This file turns [docs/bluebook_vm_parity_roadmap.md](/Users/david/repos/recorz/docs/bluebook_vm_parity_roadmap.md) into a concrete task queue that can be executed incrementally until the roadmap is complete.

It is the working plan for day-to-day progress. The roadmap remains the high-level statement of goals and guardrails; this file is the task-level breakdown.

## How To Use This Plan

1. Always take the earliest incomplete stage with a blocking open task.
2. Within that stage, take the first unblocked task in the listed order.
3. If a task touches an Integration Lane file, do not run another Integration Lane task in parallel.
4. End every completed task or tight task bundle with:
   - targeted verification,
   - an update to this file if task state changed materially,
   - a commit,
   - a push.
5. Do not skip forward into later stages just because a later task looks easier.
6. Keep RV32 and the 32 MB budget as hard constraints throughout.

## Current Position

As of `2026-03-21`, the project appears to be here:

- Stage 0: mostly established; keep as a regression lane.
- Stage 1: mostly established; primitive-boundary discipline exists, and the first debugger-visible primitive-failure path is now in place.
- Stage 2: substantially established; `Context` and `Process` are now visible runtime objects and support the current scheduler-backed process model.
- Stage 3: complete.
- Stage 4: complete.
- Stage 5: complete.
- Stage 6: complete.
- Stages 7 and 8: still open, with Stage 7 now at the head of execution.

Stage 3 status note as of `2026-03-21`:

- the development-home opening-menu to active-process debugger path is stable enough to assert directly in framebuffer coverage
- the development-home project-browser failing-test path is stable when tests are entered from package source instead of the package list
- the multi-process browser keeps `BootActiveProcess` first, the non-default-process debugger handoff is assertion-covered, and the debugger return path restores the selected process view without stale debugger status carryover
- the input monitor enters the debugger on an intended primitive/send failure instead of panicking, with direct serial coverage
- the main debugger/process-browser tests now assert instead of skipping

That closes Stage 3 and moves the main execution path to Stage 4 model extraction, while continuing to keep Stage 0 regression gates healthy.

Stage 4 status note as of `2026-03-21`:

- browser selection, browser return state, debugger state, and plain-workspace editor state now live in explicit image-side model objects (`WorkspaceBrowserModel`, `WorkspaceEditorModel`, and `WorkspaceReturnState`) instead of being spread across ad hoc tool/session globals
- `WidgetBootstrap` now routes source-editor return, debugger return, opening-menu state, and plain-workspace restore through those model objects
- the lightweight seed-side cursor/selection/origin records remain in `kernel/mvp`, while the larger mutator logic stays in image-side `TextUI` code to respect the current seed compiled-method limit
- RV32 snapshot/live-source bookkeeping was widened enough to carry the extracted model layer without changing the `32M` machine target
- the opening-menu memory-report return path and the package-browser failing-test debugger return path are both stable again
- Stage 4 render, snapshot, lowering, snapshot-inspector, and full RV32 build gates are now green

Stage 5 status note as of `2026-03-21`:

- the RV32 runtime now has scheduler-owned runnable process state instead of placeholder-only process inspection, with resumable activation records tied to real `Context` handles
- the first cooperative process lifecycle is in place: spawn, yield, suspend, resume, and terminate all run through scheduler state instead of hidden debugger aliases
- the process browser now reflects scheduler-backed process objects, and the debugger remains associated with the selected scheduled process under that runtime
- scheduler state survives snapshot save and restore through the current RV32 `v9` snapshot format, including runnable-queue continuity and resumed process completion
- Stage 5 serial, render, snapshot, lowering, and full RV32 build gates are green without widening the standard `32M` target

Stage 6 status note as of `2026-03-21`:

- selected-frame navigation, sender traversal, receiver inspection, temporaries inspection, proceed, and step controls now run through the scheduler-backed RV32 debugger instead of placeholder-only browser state
- intended development-path failures now enter the debugger with a materialized `Context` instead of falling straight into panic-only behavior, while corruption and unsupported runtime states remain hard failures
- debugger state is carried through image-owned browser return state, so process-browser and debugger handoff/return are coherent across active-process and scheduled-process flows
- the Stage 6 serial, render, lowering, and RV32 build gates are green, and the current debugger/process-browser framebuffer assertions no longer rely on the older skip-gated expectations

## Global Completion Rules

These apply to every task below:

- Prefer the smallest slice that leaves the system more correct and more testable.
- Add or strengthen tests before removing guardrails or skip-gates.
- Do not grow the C VM when image-side policy can solve the same problem.
- When a task increases memory pressure, measure and simplify before proceeding.
- Keep RV64 building, but do not let RV64 block RV32-first design unless the task is specifically the RV64 validation item.

## Stage 0. Keep The Current Base Stable

This stage is a permanent regression lane. Treat these tasks as maintenance gates that must remain green while later stages proceed.

### Open Tasks

- [ ] `BP0.1` Fresh RV32 cold-start smoke
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  Work:
  - assert fresh `dev-loop`/boot reaches the opening menu or expected workspace surface from a cold snapshot state
  - cover save, continue, and reset flows without manual intervention
  Verify:
  - targeted RV32 serial and snapshot tests

- [ ] `BP0.2` Snapshot compatibility and recovery diagnostics
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/tools/inspect_qemu_riscv_snapshot.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  Work:
  - keep profile/version mismatches explicit
  - keep recovery guidance visible in logs/tool output
  Verify:
  - mismatch fixture tests

- [ ] `BP0.3` Opening-menu and top-level browser redraw coverage
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  Work:
  - cover opening menu, class browser, project browser, object inspector, process browser, and return paths with framebuffer assertions
  Verify:
  - targeted RV32 render tests

- [ ] `BP0.4` 32 MB budget guard
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/Makefile`
  - `/Users/david/repos/recorz/tests/`
  Work:
  - keep the standard RV32 path fixed at `32M`
  - add a lightweight regression that fails if the standard path silently widens
  Verify:
  - standard RV32 build/test path still uses `-m 32M`

- [ ] `BP0.5` RV64 build validation maintenance
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv64/`
  - `/Users/david/repos/recorz/tests/`
  Work:
  - keep RV64 building as a validation target
  - add or maintain at least one smoke test per major tool/runtime area where practical
  Verify:
  - RV64 `all`

## Stage 1. Freeze The Debugging Primitive Boundary

This stage should only add substrate that the image-side debugger/process tools truly need.

### Open Tasks

- [ ] `BP1.1` Runtime primitive-boundary audit
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/docs/bluebook_vm_parity_roadmap.md`
  Work:
  - identify which debugger/process hooks are real substrate versus lingering UI/tool policy
  - document the accepted primitive surface explicitly
  Verify:
  - docs update plus lowering/build checks

- [ ] `BP1.2` Context inspection invariants
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv_mvp_lowering.py`
  Work:
  - make context stack traversal bounded and cycle-safe
  - keep `receiver`, `sender`, and liveness/detail queries explicit
  Verify:
  - lowering tests
  - context serial or render tests

- [ ] `BP1.3` Minimal process-inspection substrate
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/kernel/mvp/Selector.rz`
  Work:
  - keep only the minimum hooks for process list, active process, and context lookup
  - avoid adding browser/menu semantics to the VM
  Verify:
  - lowering tests
  - process-browser smoke tests

- [ ] `BP1.4` Debugger-visible primitive-failure entry
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/kernel/mvp/Selector.rz`
  - `/Users/david/repos/recorz/tests/`
  Work:
  - convert intended debugger-entry primitive failures from panic-only paths into inspectable debugger state
  - keep fatal corruption as hard panic
  Verify:
  - targeted failure-path tests

## Stage 2. Make Contexts And Processes First-Class Runtime Objects

This stage is mostly underway, but it is not complete until processes represent real runtime state and survive snapshots cleanly.

### Open Tasks

- [ ] `BP2.1` Lock down `Context` object contract
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/Context.rz`
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  Work:
  - formalize `thisContext`, sender traversal, receiver/detail visibility, and non-local-return observability
  Verify:
  - lowering tests
  - targeted runtime tests

- [ ] `BP2.2` Complete minimal `Process` object contract
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/Process.rz`
  - `/Users/david/repos/recorz/kernel/mvp/Selector.rz`
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  Work:
  - ensure `Process` carries label, state, priority-equivalent metadata, and top-context association
  - remove placeholder-only assumptions
  Verify:
  - lowering tests
  - serial/render process-browser tests

- [ ] `BP2.3` Snapshot continuity for context/process graphs
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  Work:
  - keep active debugger/process-visible state across save, continue, and restore
  Verify:
  - snapshot integration tests

- [ ] `BP2.4` Multi-process fixtures without scheduler semantics yet
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  Work:
  - keep stable fixtures for multiple named `Process` objects
  - use them to validate process-browser and debugger-selection behavior before the real scheduler lands
  Verify:
  - render and serial tests without skip-gates

## Stage 3. Build The First Real Inspector / Debugger / Process Browser

This is the current main execution stage.

### Open Tasks

- [ ] `BP3.1` Finish object-inspector navigation
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  Work:
  - remove any remaining special-case return glitches
  - keep object selection, detail entry, and return coherent from the opening menu and debugger-adjacent flows
  Verify:
  - serial and framebuffer object-inspector tests

- [ ] `BP3.2` Finish debugger model state
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
  - `/Users/david/repos/recorz/kernel/mvp/Selector.rz`
  Work:
  - keep selected process, selected frame, and return state image-owned
  - remove hidden alias or placeholder debugger state where it still exists
  Verify:
  - lowering tests
  - debugger serial/render tests

- [ ] `BP3.3` Remove skip-gates from process browser flows
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  Work:
  - stabilize multi-process browser selection
  - stabilize debugger handoff from a non-default process
  - convert current process-browser skip tests into asserting regressions
  Verify:
  - process-browser render and serial tests pass without skips

- [x] `BP3.4` Stabilize debugger entry from development workflows
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py`
  Work:
  - make debugger entry reliable for test failure in the development-home/browser path
  - make debugger entry reliable for primitive failure where intended
  Verify:
  - no skip-gate in the development-home `Ctrl-T` debugger-entry tests

- [x] `BP3.5` Finish debugger return-path coherence
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py`
  Work:
  - preserve list selection and scroll position when returning from debugger to process, class, and package views
  - remove any stale status/feedback carryover
  Verify:
  - framebuffer return-path tests

### Stage 3 Exit Rule

Do not start Stage 4 until all of these are true:

- the object inspector, debugger, and process browser are all reachable from the opening menu
- debugger entry works from explicit process selection and at least one real failure path
- the main debugger/process-browser tests assert instead of skipping

## Stage 4. Extract Browser And Editor Models

This is the first large structural cleanup stage. Do not start it until Stage 3 behavior is stable.

### Open Tasks

- [x] `BP4.1` Extract browser state model
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - new browser model files under `/Users/david/repos/recorz/kernel/mvp/`
  Work:
  - move browser selection, list top, target, and return state into explicit image-side model objects
  - reduce `WidgetBootstrap` ownership of browser policy
  Verify:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py::QemuRiscv32SerialIntegrationTests::test_workspace_tool_dispatch_command_returns_source_editor_to_browser_context`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py::QemuRiscv32SerialIntegrationTests::test_workspace_source_editor_return_restores_plain_workspace_state`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py::QemuRiscv32SnapshotIntegrationTests::test_snapshot_resume_can_return_from_a_source_editor_to_the_same_plain_workspace`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py::QemuRiscv32SnapshotIntegrationTests::test_object_browser_snapshot_round_trips_through_saved_state`

- [x] `BP4.2` Extract editor/source-holder model
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/kernel/mvp/WorkspaceVisibleOrigin.rz`
  - `/Users/david/repos/recorz/kernel/mvp/TextCursor.rz`
  - `/Users/david/repos/recorz/kernel/mvp/TextSelection.rz`
  - new editor model files under `/Users/david/repos/recorz/kernel/mvp/`
  Work:
  - split source holder, cursor, selection, and visible-origin coordination into reusable model objects
  Verify:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py::QemuRiscv32SerialIntegrationTests::test_live_workspace_session_redraw_routes_through_image_side_source_widget`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py::QemuRiscv32RenderIntegrationTests::test_interactive_editor_cursor_move_uses_cursor_only_redraw_path`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py::QemuRiscv32RenderIntegrationTests::test_interactive_editor_line_edit_uses_pane_redraw_path_without_pool_overflow`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py::QemuRiscv32RenderIntegrationTests::test_interactive_editor_print_uses_status_only_redraw_path`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py::QemuRiscv32RenderIntegrationTests::test_interactive_editor_page_down_keeps_text_visible`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py::QemuRiscv32SerialIntegrationTests::test_workspace_source_editor_return_restores_plain_workspace_state`

- [x] `BP4.3` Move opening-menu and return routing into model APIs
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - new browser/editor model files under `/Users/david/repos/recorz/kernel/mvp/`
  Work:
  - reduce direct menu and return branching in `WidgetBootstrap`
  - keep navigation state image-owned and smaller in surface area
  Verify:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py::QemuRiscv32SerialIntegrationTests::test_workspace_development_home_boot_opens_the_opening_menu_and_workspace_editor`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py::QemuRiscv32SerialIntegrationTests::test_workspace_development_home_opening_menu_lists_object_inspector_and_context_debugger_entries`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py::QemuRiscv32SerialIntegrationTests::test_workspace_development_home_menu_can_open_the_memory_report_and_return`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py::QemuRiscv32SnapshotIntegrationTests::test_development_home_menu_and_browser_frames_are_distinct`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py::QemuRiscv32SnapshotIntegrationTests::test_development_home_menu_paths_render_class_and_package_sources`

- [x] `BP4.4` Thin `ViewBootstrap` and renderer responsibilities
  Files:
  - `/Users/david/repos/recorz/kernel/textui/ViewBootstrap.rz`
  - `/Users/david/repos/recorz/kernel/textui/TextRendererBootstrap.rz`
  Work:
  - keep these files focused on composition, focus, and drawing
  - remove policy that belongs in model objects
  Verify:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py::QemuRiscv32RenderIntegrationTests::test_interactive_editor_cursor_move_uses_cursor_only_redraw_path`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py::QemuRiscv32RenderIntegrationTests::test_interactive_editor_line_edit_uses_pane_redraw_path_without_pool_overflow`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py::QemuRiscv32RenderIntegrationTests::test_interactive_editor_print_uses_status_only_redraw_path`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_render_integration.py::QemuRiscv32RenderIntegrationTests::test_interactive_package_browser_scrolling_uses_incremental_list_redraw_path`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py::QemuRiscv32SnapshotIntegrationTests::test_development_home_menu_and_browser_frames_are_distinct`

- [x] `BP4.5` Re-run memory budget after model extraction
  Files:
  - `/Users/david/repos/recorz/tests/`
  - `/Users/david/repos/recorz/docs/`
  Work:
  - verify the extracted model layer still fits the RV32 `32M` target
  Verify:
  - `make -C /Users/david/repos/recorz/platform/qemu-riscv32 BUILD_DIR=/tmp/recorz-qemu-riscv32-stage4 all`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_serial_integration.py::QemuRiscv32SerialIntegrationTests::test_workspace_development_home_menu_can_open_the_memory_report_and_return`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py::QemuRiscv32SnapshotIntegrationTests::test_snapshot_can_reopen_workspace_package_browser_state_without_demo_specific_program`

## Stage 5. Add Real Scheduler And Process Control Semantics

This stage turns the inspectable process scaffolding into a real early Smalltalk-style process system.

### Open Tasks

- [x] `BP5.1` Add cooperative runnable/waiting queues
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/kernel/mvp/Process.rz`
  Work:
  - add scheduler-owned queues and state transitions for a single-core cooperative model
  Verify:
  - runtime tests
  - process-browser tests

- [x] `BP5.2` Add process creation protocol
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/Process.rz`
  - `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  Work:
  - support creation from blocks or equivalent executable objects
  Verify:
  - process creation tests

- [x] `BP5.3` Add process control protocol
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/Process.rz`
  - `/Users/david/repos/recorz/kernel/mvp/Selector.rz`
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  Work:
  - add `suspend`, `resume`, `terminate`, and `yield`
  Verify:
  - process-control tests

- [x] `BP5.4` Process browser and debugger coherence under real scheduling
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/tests/`
  Work:
  - make the process browser reflect scheduler state
  - keep debugger process association coherent while processes run and suspend
  Verify:
  - render, serial, and snapshot tests

- [x] `BP5.5` Snapshot-safe scheduler state
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  Work:
  - ensure runnable/waiting queues survive save, continue, and restore
  Verify:
  - snapshot integration tests

## Stage 6. Deepen Debugger Functionality

This stage should only begin once Stage 5 yields a stable multi-process runtime.

### Open Tasks

- [x] `BP6.1` Selected-frame navigation and sender traversal
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/kernel/mvp/Context.rz`
  Work:
  - add explicit frame movement and sender traversal controls
  Verify:
  - debugger tests

- [x] `BP6.2` Frame-local object inspection
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - new debugger/inspector model files under `/Users/david/repos/recorz/kernel/mvp/`
  Work:
  - inspect receiver, temporaries, and related objects from the selected frame
  Verify:
  - debugger/object-inspector integration tests

- [x] `BP6.3` Resume/proceed control
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  Work:
  - support leaving the debugger and continuing execution coherently
  Verify:
  - failure and continue tests

- [x] `BP6.4` Honest stepping support
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  Work:
  - add `step over` and `step into` only if the runtime representation can support them without lying
  Verify:
  - debugger stepping tests

- [x] `BP6.5` Replace intended panic-only failure paths
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
  - `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
  - `/Users/david/repos/recorz/tests/`
  Work:
  - move intended debugger-visible failures out of panic-only behavior
  - leave corruption and unsupported states as hard failures
  Verify:
  - failure-path tests

## Stage 7. Tighten Self-Hosting And Source Ownership Around The New Tools

This stage keeps the expanded tool stack aligned with Recorz's image-owned direction.

### Open Tasks

- [ ] `BP7.1` File-out/file-in cleanliness for tool classes
  Files:
  - `/Users/david/repos/recorz/kernel/mvp/`
  - `/Users/david/repos/recorz/kernel/textui/`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py`
  Work:
  - ensure debugger, inspector, and process-browser classes round-trip cleanly
  Verify:
  - regeneration tests

- [ ] `BP7.2` Regeneration-safe runtime metadata
  Files:
  - `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
  - `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
  - `/Users/david/repos/recorz/tests/`
  Work:
  - keep selectors, primitive bindings, and regenerated source aligned as tools expand
  Verify:
  - lowering and regeneration tests

- [ ] `BP7.3` In-image inspection of regenerated source and runtime state
  Files:
  - `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
  - `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
  Work:
  - make regenerated source and runtime metadata easier to browse from inside the image
  Verify:
  - serial/render regeneration tests

- [ ] `BP7.4` Keep host builders mechanical
  Files:
  - `/Users/david/repos/recorz/tools/`
  - `/Users/david/repos/recorz/docs/`
  Work:
  - avoid moving tool policy back into host-side scripts
  - document the bootstrap-only role of host builders
  Verify:
  - docs review
  - regeneration smoke tests

- [ ] `BP7.5` Snapshot and regeneration survival of new tools
  Files:
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
  - `/Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py`
  Work:
  - verify debugger/process/browser tools survive save/resume/regeneration cycles
  Verify:
  - snapshot and regeneration integration tests

## Stage 8. Revalidate The Whole System And Close The Program

This stage only begins once the earlier stages are functionally complete.

### Open Tasks

- [ ] `BP8.1` Remove remaining roadmap-related skip-gates
  Files:
  - `/Users/david/repos/recorz/tests/`
  Work:
  - convert temporary skips into real assertions or explicit documented non-goals
  Verify:
  - targeted tests with no roadmap-related skips left

- [ ] `BP8.2` Run the full RV32 validation matrix
  Files:
  - `/Users/david/repos/recorz/tests/`
  - `/Users/david/repos/recorz/docs/`
  Work:
  - validate workspace, browser, inspector, debugger, process browser, save/resume, and regeneration
  Verify:
  - recorded RV32 matrix run

- [ ] `BP8.3` RV64 smoke-suite validation
  Files:
  - `/Users/david/repos/recorz/platform/qemu-riscv64/`
  - `/Users/david/repos/recorz/tests/`
  Work:
  - keep RV64 aligned with the same core contract
  Verify:
  - RV64 build plus selected smoke tests

- [ ] `BP8.4` Final 32 MB measurement pass
  Files:
  - `/Users/david/repos/recorz/docs/`
  - `/Users/david/repos/recorz/tests/`
  Work:
  - measure the final RV32 tool/runtime stack against the `32M` target
  Verify:
  - memory report or equivalent recorded in docs

- [ ] `BP8.5` Final docs reconciliation
  Files:
  - `/Users/david/repos/recorz/README.md`
  - `/Users/david/repos/recorz/TODO.md`
  - `/Users/david/repos/recorz/docs/`
  Work:
  - align the docs with what is actually implemented when the execution plan is complete
  Verify:
  - doc review

## Immediate Next Queue

Unless a regression in Stage 0 becomes urgent, the next tasks should be tackled in this order:

1. `BP7.1` File-out/file-in cleanliness for tool classes.
2. `BP7.2` Regeneration-safe runtime metadata.
3. `BP7.3` In-image inspection of regenerated source and runtime state.

## Program Complete When

This execution plan is complete when every stage above is closed and the project can:

- browse and edit source from inside the image,
- inspect ordinary objects, contexts, and live processes,
- enter and use a practical debugger,
- save and resume that live environment safely,
- regenerate and recover the tool/source stack,
- do so on the RV32 primary path within the current memory target,
- keep RV64 as an honest validation path,
- and still preserve Recorz's narrow primitive-boundary design.
