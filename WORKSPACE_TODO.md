# Recorz Workspace TODO

## Goal
Build a complete native Recorz workspace as an image-owned tool, guided by the Smalltalk-80 workspace model where that fits Recorz's current architecture.

The workspace should be:
- an image-owned editor/view object rather than a VM-owned mode
- able to hold and edit arbitrary text buffers
- able to evaluate, print, accept, revert, test, save, and reopen from inside the image
- stable across scrolling, snapshot/reboot, and repeated editing sessions

## Scope
- one primary workspace buffer with a clear editor/session model
- room to grow toward multiple named workspaces later
- no new VM UI policy beyond low-level drawing, input, persistence, and panic/debug support

## Phase 1 - Workspace Model
- [x] Define the canonical image-side workspace object model:
  buffer contents, cursor, selection, visible origin, status, feedback, current target, and tool/session state
- [x] Separate persistent workspace state from transient session/view state
- [x] Decide which parts of the workspace must survive snapshot/reboot and which should be recomputed on reopen
- [x] Add explicit protocol for:
  `contents`, `setContents:`, `cursor`, `selection`, `visibleOrigin`, `statusText`, `feedbackText`
- [x] Add a simple named-workspace hook so the model can grow beyond one global buffer later

Notes:
- persistent image-owned state now lives across `Workspace`, `WorkspaceCursor`, `WorkspaceSelection`, `BootWorkspaceTool`, and `BootWorkspaceSession`
- `BootWorkspaceTool` now owns status text, feedback text, workspace name, and horizontal viewport column; `escape` remains transient session input state
- first real viewport-relative page-up/page-down is now in the image session path
- explicit workspace/session protocol now exposes status, feedback, target label, modified-state, and visible top/left origin from image code; the current visible-origin API is an explicit top/left pair rather than a dedicated origin object

## Phase 2 - Editor Behavior
- [ ] Make the workspace editor the single source of truth for cursor movement, selection, insert/delete, and line navigation
- [ ] Support stable horizontal and vertical scrolling with a real visible-origin model
- [x] Add page-up/page-down that operate on the visible viewport, not just the cursor
- [x] Add top/bottom navigation that operate on the visible viewport, not just the cursor
- [ ] Ensure large buffers keep a stable visible origin during edits, movement, and reopen
- [ ] Add explicit cursor painting and selection painting rules owned by image-side objects

## Phase 3 - Workspace Commands
- [x] Define a clean image-side command protocol for:
  do-it, print-it, accept, revert, run tests, save, reopen, and browse target
- [x] Separate workspace editing commands from browser/package/class source-editor commands
- [x] Make command dispatch live in workspace/tool objects instead of VM-side mode branches
- [x] Add clear refusal/status behavior for commands that require a browser-backed target
- [x] Support command binding updates without hard-wiring more policy into the VM

## Phase 4 - Rendering And Layout
- [ ] Keep workspace rendering image-owned:
  header, source pane, status pane, feedback pane, cursor, selection, and scroll position
- [ ] Unify workspace rendering with the existing view/widget/layout objects rather than special-casing the workspace
- [ ] Add explicit dirty-region redraw for:
  cursor-only movement, line edits, viewport scroll, and status updates
- [x] Preserve readable default workspace typography and pane spacing on the RV32 framebuffer target
- [x] Ensure the workspace remains usable at both the primary and comfort text densities

## Phase 5 - Persistence And Reopen
- [x] Make workspace reopen restore the image-owned workspace session instead of reconstructing a VM-owned tool mode
- [x] Preserve buffer contents, cursor, selection, visible origin, status, and feedback across snapshot/reboot
- [x] Distinguish reopening a plain workspace buffer from reopening a browser-backed source editor
- [x] Add recovery behavior for malformed or stale workspace session state
- [x] Keep snapshot format expectations explicit and covered by tests

## Phase 6 - Workflow Integration
- [x] Make the workspace the default place to stage and run exploratory code from inside the image
- [ ] Support jumping cleanly between workspace and browser/source-editor contexts
- [ ] Allow browser-opened source to return to the workspace without losing the workspace buffer
- [ ] Add save/resume behavior that feels like continuing one live workspace session
- [ ] Ensure regeneration, recovery, and package/class editing flows do not corrupt workspace state

## Phase 7 - Ergonomics
- [x] Add a clear visual indicator for focus, cursor, modified state, and current target
- [x] Add a visible command summary that fits the workspace layout without causing wrap corruption
- [x] Add explicit empty-workspace and first-run behavior
- [x] Decide whether the first serious workspace should support multiple buffers, scratch panes, or one canonical buffer only
- [x] Document the intended user flow for the native workspace in `implementation.md` or `/docs`

## Proof And Regression Coverage
- [x] Add framebuffer integration tests for:
  workspace open, edit, scroll, page movement, save/reopen, and snapshot restore
- [x] Add snapshot tests proving workspace buffer, cursor, and visible origin survive reboot
- [x] Add stress tests for held-key repeat on large workspace buffers
- [x] Add outcome tests for command behavior:
  do-it, print-it, accept, revert, test, save, reopen
- [x] Add lowering/runtime checks to keep new workspace protocol within the current selector/primitive model

## Definition Of Done
- [ ] The workspace is an image-owned tool object with a stable editor/session model
- [ ] The visible origin is owned by workspace/image objects and remains stable under scrolling and editing
- [x] Workspace command behavior is owned by image-side tool objects, not VM-side mode policy
- [x] Reopen/snapshot restore lands back in the same workspace session with correct buffer, cursor, and viewport state
- [x] The workspace is covered by live RV32 framebuffer proofs for open, edit, scroll, save, and restore
