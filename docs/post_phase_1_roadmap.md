# Post Phase 1 Roadmap

Recorz has already crossed the current Phase 1 contract. This roadmap defines the next sequence of work so the project keeps moving forward without collapsing back into an open-ended rewrite.

## Guiding Rules

- Keep RV32 as the primary development path until the next milestone is stable.
- Treat RV64 as a validation target, not a separate product line.
- Keep VM changes narrow: primitives, persistence, display/input, and the minimum runtime support required by the image.
- Prefer image-owned policy over C-side policy whenever a boundary can move safely.
- Add tests before broadening behavior, especially around save/resume and screen updates.

## Stage 1 - Harden The Current RV32 Experience

Goal: make the current native development loop boring and reliable.

Work items:
- stabilize the opening menu, class browser, project browser, and `editCurrent` flows
- make snapshot compatibility failures explicit and easy to diagnose
- add framebuffer-level regression coverage for browser/menu/source redraw
- keep `dev-loop` and save/resume behavior reproducible from a cold start

Boundary:
- do not add new language features
- do not expand the VM surface beyond current tool and primitive needs
- do not start RV64 parity work until RV32 save/resume and redraw behavior are steady

Exit criteria:
- fresh snapshots boot into the expected menu or tool without manual recovery
- browser/source transitions do not regress visual refresh
- targeted serial and framebuffer tests cover the main user-visible flows

## Stage 2 - Reduce Native VM Policy

Goal: keep shrinking the C runtime toward a primitive boundary.

Work items:
- move any remaining browser-return and tool routing policy into image-side code
- remove duplicated RV32/RV64 helper logic where it is mechanically shared
- keep snapshot, input, and drawing logic below the image while tool behavior moves upward
- simplify the native tool surface so the VM no longer owns browser/workspace decisions

Boundary:
- do not delete functionality before the image-side replacement exists
- do not mix in self-hosting work here
- do not widen the primitive set unless a missing primitive blocks a concrete image-side migration

Exit criteria:
- the VM contains less tool policy and more narrow primitives
- browser/workspace behavior is driven by image code rather than C-side branching
- shared native helper logic is consolidated where practical

## Stage 3 - Restore RV64 As Validation

Goal: keep RV64 honest so RV32 does not become the only maintained target.

Work items:
- keep RV64 building against the current selector and snapshot surface
- run the same core smoke tests on RV64 that validate RV32 behavior
- fix parity gaps that block loading, rendering, or method updates
- ensure snapshot and regeneration tooling still describe the same core contract on both targets

Boundary:
- do not invent RV64-only features
- do not let RV64 drag the project into a separate architecture track
- fix validation gaps, not speculative optimization

Exit criteria:
- RV64 can still build the current image and run the core validation suite
- RV64 failures are documented as explicit gaps rather than silent drift

## Stage 4 - Expand Self-Hosting

Goal: move more compiler and regeneration responsibility into the image.

Work items:
- shift more of the source/compiler/bootstrap workflow out of host-side ownership
- make regenerated source and tool output easier to inspect from inside the image
- keep the host build path as a bootstrap path, not the main development path

Boundary:
- do not introduce typing or capability work as a prerequisite
- do not rewrite the compiler wholesale
- keep this step incremental so the image remains usable throughout

Exit criteria:
- more of the toolchain can be exercised from inside the live image
- the host build becomes thinner and more mechanical

## Stage 5 - Add Inspector / Debugger Foundations

Goal: make live state easier to inspect before adding bigger semantic features.

Work items:
- expose core runtime state in an inspectable way
- add minimal debugger or inspector entry points for contexts and objects
- make process or execution-state visibility practical enough for development use

Boundary:
- do not attempt capability enforcement yet
- do not expand into a full debugger suite before the basic inspection path exists

Exit criteria:
- core runtime objects and execution state can be examined from the image
- debugger-oriented workflows become a normal part of development instead of a future promise

## Stage 6 - Reopen Long-Term Directions

Goal: revisit the broader spec items only after the runtime and toolchain are steady.

Candidate topics:
- gradual typing and richer type analysis
- capability-oriented isolation and sandboxing
- systems-subset and HDL-subset lowering
- multiprocessing and accelerator support

Boundary:
- do not move these into the critical path until the earlier stages are stable
- keep them behind explicit design gates so they do not disrupt the native UI and image ownership work

