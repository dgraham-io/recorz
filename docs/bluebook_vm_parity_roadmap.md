# Bluebook VM And Environment Parity Roadmap

The step-by-step task breakdown for executing this roadmap lives in [docs/bluebook_vm_parity_execution_plan.md](/Users/david/repos/recorz/docs/bluebook_vm_parity_execution_plan.md).

Status:

- the roadmap is complete as of `2026-03-21`
- RV32 is the fully validated primary path under the unchanged `32M` machine target
- RV64 remains an honest validation target through shared-build and minimal smoke-suite coverage rather than full default-TextUI parity

## Purpose

This roadmap defines the long-running plan for duplicating the practical functionality people associate with the Bluebook Smalltalk system, while preserving Recorz's current goals:

- RV32 remains the primary development target.
- The current 32 MB machine budget remains in force.
- The VM stays narrow: execution, object memory, persistence, input/display, GC, and minimal inspection/debug primitives.
- Tool policy continues moving into image-side objects rather than back into C.
- Recorz keeps its path toward self-hosting, future isolation, and future multiprocessing without pretending those later goals are already the current implementation boundary.

This is not a plan to clone the historical Bluebook implementation line-for-line.
It is a plan to reach equivalent live-system capability in Recorz:

- inspectable objects,
- inspectable contexts,
- inspectable processes,
- debugger entry and control,
- browser/workspace/editor continuity,
- process tools,
- snapshot-safe live development,
- image-owned tool behavior above a Smalltalk-shaped primitive boundary.

## Guardrails

Every stage in this roadmap must obey these rules:

1. Do not widen the VM into a general tool framework.
The VM may grow only where a missing primitive or runtime hook is required for concrete image-side debugger, inspector, process, or persistence work.

2. Do not compromise the current RV32 path.
The normal RV32 development loop, save/resume, and framebuffer/browser flows must remain working at each stage.

3. Do not break the 32 MB budget to buy convenience.
New tool/runtime structures must fit inside the current memory target.
If a feature cannot fit, the next task is to simplify the design, share state, or reduce duplication.

4. Do not clone the full historical MVC stack.
Use Bluebook Smalltalk as a reference for responsibilities and tool capabilities, not as a mandate to reproduce every controller/view abstraction.

5. Do not pull long-term topics into the critical path.
Gradual typing, capability enforcement, HDL work, bare-metal expansion, and broader multiprocessing stay out of the main line until the Bluebook-parity runtime and tool layers are stable.

6. Do not let RV64 become a second product line.
RV64 remains a validation target.
It should track the same contracts, not drive the roadmap.

## Bluebook Parity Target

For Recorz, "Bluebook parity" means the system can support the following in a recognizably Smalltalk-like way:

- object inspection,
- context inspection,
- debugger-visible execution state,
- process creation and process browsing,
- cooperative scheduling suitable for a live image,
- browser/workspace/editor flows built from image-side models,
- source-linked live method replacement,
- snapshot-safe persistence of the active environment,
- self-explanatory runtime and tool state.

It does not require:

- exact Bluebook object memory layout,
- exact Bluebook bytecodes,
- a full historical MVC or window manager clone,
- immediate pursuit of multi-core or capability policy,
- a wholesale rewrite of the current toolchain.

## Program Structure

The work should proceed in three lanes with a single integration point:

### Lane A: Runtime And VM Substrate

Owns:

- `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
- `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
- new kernel runtime model files such as `/Users/david/repos/recorz/kernel/mvp/Process.rz`

Responsibilities:

- process model,
- scheduler surface,
- context/process inspection primitives,
- snapshot/runtime invariants,
- GC/rooting correctness for debugger-visible state.

### Lane B: Image-Side Tools And Models

Owns:

- `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
- `/Users/david/repos/recorz/kernel/textui/ViewBootstrap.rz`
- `/Users/david/repos/recorz/kernel/textui/TextRendererBootstrap.rz`
- browser/editor/process/debugger model files under `/Users/david/repos/recorz/kernel/mvp/`

Responsibilities:

- process browser,
- debugger UI,
- inspector flows,
- browser/editor model extraction,
- navigation/return/menu behavior,
- keeping tool policy out of C.

### Lane C: Verification, Bootstrap, And Docs

Owns:

- `/Users/david/repos/recorz/tests/`
- `/Users/david/repos/recorz/tools/inspect_qemu_riscv_snapshot.py`
- `/Users/david/repos/recorz/tools/`
- roadmap and acceptance docs under `/Users/david/repos/recorz/docs/`

Responsibilities:

- serial/framebuffer/snapshot/regeneration gates,
- cold-start dev-loop reliability,
- snapshot compatibility/versioning checks,
- regeneration and source-ownership checks,
- keeping the docs honest about what is actually implemented.

### Integration Lane

These files should be treated as shared-integration points and changed by one agent at a time:

- `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
- `/Users/david/repos/recorz/kernel/mvp/Selector.rz`
- `/Users/david/repos/recorz/TODO.md`
- `/Users/david/repos/recorz/README.md`

## Stage Plan

### Stage 0. Keep The Current Base Stable

Goal:
Make the current RV32 environment the non-negotiable baseline while the rest of the roadmap proceeds.

Tasks:

1. Keep cold-start `dev-loop`, save/resume, and opening-menu flows covered by serial and framebuffer tests.
2. Keep snapshot profile/version mismatches explicit and recoverable.
3. Preserve the 32 MB target in the standard RV32 test/build path.
4. Keep RV64 building as a validation target, but do not let RV64 drive stage design.

Exit criteria:

- fresh RV32 snapshots boot predictably,
- stale snapshots fail with explicit messages,
- current browser/workspace/object-inspector/process-browser flows stay covered,
- RV32 remains the main gate for new work.

### Stage 1. Freeze The Debugging Primitive Boundary

Goal:
Set the minimum VM/runtime surface needed for debugger and process work without turning the VM into a tool layer.

Tasks:

1. Keep `Context` inspection bounded, explicit, and cycle-safe.
2. Define the minimal primitive/runtime surface for:
   - context stack text or equivalent structured inspection,
   - process list inspection,
   - active process lookup,
   - debugger-visible primitive failure entry.
3. Document which current helper paths are legitimate runtime substrate versus temporary scaffolding.
4. Keep all user-visible debugger/process policy above the primitive layer.

Primary files:

- `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
- `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
- `/Users/david/repos/recorz/kernel/mvp/Selector.rz`

Dependencies:

- none beyond Stage 0.

Parallelism:

- Lane A can work while Lane C expands regression gates.
- Lane B should avoid large tool refactors until the primitive surface is settled.

Exit criteria:

- the runtime exposes the minimum honest inspection/debug hooks,
- no new browser/editor policy has been added to the VM,
- the primitive boundary is documented and testable.

### Stage 2. Make Contexts And Processes First-Class Runtime Objects

Goal:
Turn the current debugger/context scaffolding into real runtime-visible structures and add the smallest honest process model.

Tasks:

1. Formalize context invariants for:
   - `thisContext`,
   - sender traversal,
   - receiver/detail/alive visibility,
   - non-local return observability.
2. Introduce a minimal inspectable `Process` class.
3. Expose process state sufficient for:
   - active process,
   - runnable/waiting/suspended state,
   - priority or equivalent scheduling metadata,
   - associated top context.
4. Keep the first scheduler cooperative and single-core.
5. Ensure contexts and processes survive snapshot save/restore cleanly.

Primary files:

- `/Users/david/repos/recorz/kernel/mvp/Context.rz`
- `/Users/david/repos/recorz/kernel/mvp/Process.rz`
- `/Users/david/repos/recorz/kernel/mvp/Selector.rz`
- `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
- `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`

Dependencies:

- Stage 1.

Parallelism:

- Lane A owns runtime/process structure.
- Lane C can build snapshot and runtime invariants around that work.
- Lane B can prepare process/debugger models but should avoid locking UI details until the `Process` contract lands.

Exit criteria:

- `Context` and `Process` are inspectable objects,
- active execution is visible through those objects,
- snapshots retain process/context continuity without hidden runtime-only state.

### Stage 3. Build The First Real Inspector / Debugger / Process Browser

Goal:
Replace one-off text views with coherent image-side tools for objects, contexts, and processes.

Tasks:

1. Turn the object inspector into a stable object-detail tool with return/navigation state.
2. Turn the current debugger detail flow into a real debugger model with:
   - selected frame,
   - sender traversal,
   - receiver/detail display,
   - process association.
3. Replace the single-item process browser placeholder with a real list of live processes.
4. Add debugger entry on at least:
   - explicit menu entry,
   - selected process,
   - primitive failure,
   - test failure or equivalent development error path where practical.
5. Keep rendering and navigation image-owned.

Primary files:

- `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
- `/Users/david/repos/recorz/kernel/mvp/WorkspaceTool.rz`
- `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
- new inspector/debugger/process model files under `/Users/david/repos/recorz/kernel/mvp/`

Dependencies:

- Stage 2.

Parallelism:

- Lane B owns the tools.
- Lane C grows serial/framebuffer/snapshot coverage in parallel.
- Lane A only changes runtime when a concrete missing hook blocks a tool behavior.

Exit criteria:

- the system can browse objects, contexts, and processes from inside the image,
- the debugger is no longer just a string dump,
- user navigation among workspace, browser, inspector, and debugger is coherent.

### Stage 4. Extract Browser And Editor Models

Goal:
Reach a smaller Recorz-native equivalent of `StringHolder`, `Browser`, and `ParagraphEditor` responsibilities without importing the full historical framework.

Tasks:

1. Split browser state out of `WidgetBootstrap` into explicit model objects.
2. Split editor/workspace state into model objects for:
   - source holder,
   - cursor,
   - selection,
   - visible origin,
   - reopen/return state.
3. Move opening-menu command selection and browser return handling into smaller image-side model APIs.
4. Keep `ViewBootstrap` as a thin composition/focus layer, not a policy sink.
5. Move more cursor/selection/layout behavior out of renderer bootstrap code into editor/text model objects.

Primary files:

- `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
- `/Users/david/repos/recorz/kernel/mvp/WorkspaceVisibleOrigin.rz`
- `/Users/david/repos/recorz/kernel/mvp/TextCursor.rz`
- `/Users/david/repos/recorz/kernel/mvp/TextSelection.rz`
- `/Users/david/repos/recorz/kernel/mvp/TextLayout.rz`
- `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
- `/Users/david/repos/recorz/kernel/textui/ViewBootstrap.rz`
- `/Users/david/repos/recorz/kernel/textui/TextRendererBootstrap.rz`

Dependencies:

- Stage 3.

Parallelism:

- Lane B can split browser and editor models in separate slices if they do not touch the same files.
- Lane C should extend framebuffer regressions for open/return/redraw/save paths after each slice.

Exit criteria:

- browser/editor state is model-driven rather than embedded in routing code,
- redraw/navigation regressions remain covered,
- the system stays within the current RV32 memory budget.

### Stage 5. Add Real Scheduler And Process Control Semantics

Goal:
Move from inspectable single-process scaffolding to an honest early Smalltalk-style process system.

Tasks:

1. Add cooperative scheduling with runnable and waiting process queues.
2. Add process creation from blocks or equivalent executable objects.
3. Add process control protocol suitable for a live image:
   - suspend,
   - resume,
   - terminate,
   - yield.
4. Ensure debugger entry and process browser behavior remain coherent when multiple processes exist.
5. Keep the architecture compatible with later multi-core/domain evolution without exposing those later concerns yet.

Primary files:

- `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
- `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`
- `/Users/david/repos/recorz/kernel/mvp/Process.rz`
- `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
- `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`

Dependencies:

- Stages 2 through 4.

Parallelism:

- Lane A owns the scheduler/runtime.
- Lane B adapts process browser/debugger UI after each runtime slice.
- Lane C adds process/snapshot/tests continuously.

Exit criteria:

- the image can host multiple inspectable processes,
- the process browser reflects real scheduler state,
- debugger/process control works without breaking the RV32 development loop.

### Stage 6. Deepen Debugger Functionality

Goal:
Make the debugger genuinely useful for live development instead of a context reporter.

Tasks:

1. Add selected-frame navigation and sender traversal.
2. Add frame-local object inspection.
3. Add basic stepping controls:
   - resume,
   - step over,
   - step into,
   - proceed from failure,
   when the runtime representation can support them honestly.
4. Make primitive failures debugger-visible in the normal development path.
5. Preserve source linkage for methods and frame reporting.

Primary files:

- `/Users/david/repos/recorz/kernel/mvp/Context.rz`
- `/Users/david/repos/recorz/kernel/mvp/Workspace.rz`
- `/Users/david/repos/recorz/kernel/textui/WidgetBootstrap.rz`
- `/Users/david/repos/recorz/platform/qemu-riscv32/vm.c`
- `/Users/david/repos/recorz/platform/qemu-riscv64/vm.c`

Dependencies:

- Stage 5.

Parallelism:

- Lane A and Lane B work in smaller alternating slices.
- Lane C owns step/control/failure-path tests.

Exit criteria:

- the debugger can inspect and control active execution in a practical way,
- failures lead to inspectable state instead of opaque panic-only behavior where debugger entry is intended.

### Stage 7. Tighten Self-Hosting And Source Ownership Around The New Tools

Goal:
Ensure the expanded debugger/process/tool environment is still consistent with Recorz's self-hosting direction.

Tasks:

1. Keep tool classes file-out/file-in clean and regeneration-safe.
2. Make regenerated source and runtime metadata easier to inspect from inside the image.
3. Keep host builders mechanical and bootstrap-oriented.
4. Expand image-owned compiler/regeneration inspection where it supports debugger/process work.
5. Keep docs aligned with what is actually implemented.

Primary files:

- `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py`
- `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py`
- `/Users/david/repos/recorz/tests/test_qemu_riscv32_snapshot_integration.py`
- `/Users/david/repos/recorz/docs/`

Dependencies:

- Stages 3 through 6.

Parallelism:

- Lane C owns most of this stage.
- Lane B participates where tool classes and regeneration output meet.

Exit criteria:

- debugger/process/browser tools survive regeneration and snapshot cycles,
- host-side ownership is thinner, not thicker,
- the documentation still matches the implementation boundary.

### Stage 8. Revalidate The Whole System And Reopen Deferred Topics

Goal:
Finish the parity push with a stable, validated base and then decide what comes next.

Tasks:

1. Run the full RV32 validation matrix for:
   - browser,
   - workspace,
   - object inspector,
   - debugger,
   - process browser,
   - save/resume,
   - regeneration.
2. Keep RV64 building and running the same core smoke suite.
3. Re-measure the 32 MB target with the expanded tool/runtime stack.
4. Only after this stage, decide whether to prioritize:
   - deeper self-hosting,
   - richer process semantics,
   - capability hooks,
   - or typed/tooling work.

Exit criteria:

- the Bluebook-parity target is usable on the RV32 primary path,
- RV64 is still an honest validation target,
- the next roadmap can reopen broader Recorz goals from a stable base.

Status as of `2026-03-21`:

- complete
- the full RV32 parity matrix is green across workspace, browser, inspector, debugger, process browser, snapshot, regeneration, and framebuffer redraw/return flows
- RV64 builds and passes the minimal base render plus snapshot save/reload smoke suite used for the current validation lane

## Required Test Gates

Every stage should leave these gates stronger than it found them:

1. lowering and selector/primitive binding tests
2. serial integration for tool/menu/process/debugger navigation
3. framebuffer integration for redraw and return-path regressions
4. snapshot integration for cold start, save, continue, restore, and compatibility failures
5. regeneration integration for tool/source round-trips
6. RV64 build validation, with targeted smoke tests where practical

No large stage should be merged if it weakens cold-start RV32, snapshot compatibility, or framebuffer redraw confidence.

## What To Defer

These topics are still valid Recorz goals, but they should remain deferred until the roadmap above is substantially complete:

- gradual typing and richer type analysis
- capability-oriented isolation enforcement
- HDL and systems-subset expansion
- multiprocessor scheduling
- accelerator-aware execution
- a large desktop-style window system

## Definition Of Done

This roadmap is complete when Recorz can support the following from inside the live image without violating the guardrails above:

- browse and edit source,
- inspect ordinary objects,
- inspect contexts,
- browse and control live processes,
- enter and use a practical debugger,
- save and resume that live environment safely,
- regenerate and recover the tool/source stack,
- do all of the above on the RV32 primary path within the current memory target and primitive-boundary discipline.
