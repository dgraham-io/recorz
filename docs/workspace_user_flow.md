# Recorz Native Workspace User Flow

This document describes the current native Workspace flow on the RV32/QEMU path. It is a description of what the image does now, not a future design sketch.

## Primary Shape

The current Workspace behaves as one primary live text buffer with two closely related modes:

- a plain workspace buffer for exploratory code
- a browser-backed source editor for package, class, method, and regenerated source

The same image-owned workspace/session state carries buffer text, cursor, selection, viewport state, status text, feedback text, and reopen state across those modes.

## Entry Points

The current image exposes three practical ways into the workspace flow:

- `Workspace developmentHome.` boots into the editor with a starter command already staged. The current starter path is `Workspace browsePackagesInteractive.` so the first serious interaction can begin from inside the live image.
- `Workspace interactiveInputMonitor.` opens the plain workspace editor directly for scratch-buffer evaluation.
- Browser/edit entry points such as `Workspace browsePackagesInteractive.`, `Workspace editMethod:ofClassNamed:`, and `Workspace editPackageNamed:` open browser-backed source editors on top of the same workspace/session model.

## Plain Workspace Flow

The plain workspace is the default scratch surface.

- Status defaults to `WORKSPACE :: READY` unless a command overrides it.
- Feedback defaults to `D DOIT  P PRINT  T TEST  W SAVE`.
- `Ctrl-D` or `Ctrl-R` evaluates the current buffer as a do-it.
- `Ctrl-P` prints the current buffer result.
- `Ctrl-T` runs tests for the current browser-backed target when present; otherwise the tool reports a refusal status.
- `Ctrl-W` saves a snapshot/reopen path, and `Ctrl-K` saves a recovery snapshot.

The plain buffer is persistent workspace state. Contents, cursor, selection, visible top/left origin, status, feedback, and reopen context survive snapshot/reload.

## Browser And Source-Editor Flow

The package browser is the current jump-off point for structured source editing.

- Browser status is `INTERACTIVE PACKAGE LIST`.
- Browser feedback is `N/P MOVE  H/F EDGE  B/V PAGE  X OPEN  O CLOSE`.
- `Enter` or `Ctrl-X` opens the selected source into the workspace editor.

Once a source target is open, the workspace becomes a browser-backed source editor.

- Status identifies the active editor context, for example `METHOD SOURCE :: SOURCE EDITOR READY`, `CLASS SOURCE :: SOURCE EDITOR READY`, or `PACKAGE SOURCE :: SOURCE EDITOR READY`.
- Feedback defaults to `A/E LINE  H/F EDGE  B/V PAGE  X ACCEPT  Y REVERT  O RETURN`.
- `Ctrl-X` accepts the edited source back into the image and keeps the surrounding browser context.
- `Ctrl-Y` restores the original browser-backed source.
- `Ctrl-O` returns from the source editor to the browser view without discarding the workspace session.

This is the current answer to "jump from browsing into editing and back again without leaving the live image."

## Save And Resume

Save/resume is intended to feel like continuing the same live session, not launching a separate tool mode.

- `save and reopen` restores the same workspace/browser session after reload.
- `save and rerun` preserves the last source and replays it on resume.
- recovery snapshots preserve the current workspace state even without a startup hook.
- malformed reopen state is repaired back into a safe workspace session on boot.

In practice, this means a saved session can come back as:

- the package browser
- a browser-backed method/class/package source editor
- the plain workspace buffer

with the correct buffer, cursor, selection, target, and viewport state restored.

## Current Limits

The current native workspace is usable, but still intentionally narrow.

- It is still one canonical buffer-first workspace, not a multi-buffer or multi-pane editor.
- Plain workspace and browser-backed source editing share the same session model, but the visible-origin model is still an explicit top/left pair rather than a dedicated origin object.
- Workflow polish is still incomplete around focus indicators, first-run presentation, and a fuller command legend.
- The editor/browser rendering path is proven on RV32, but some render ownership and ergonomics items are still open in `WORKSPACE_TODO.md`.

## Recommended Current Workflow

For day-to-day live use in the current implementation:

1. Boot into `Workspace developmentHome.` or `Workspace browsePackagesInteractive.`.
2. Open package, class, or method source when you want to edit existing system code.
3. Use the plain workspace buffer for exploratory do-its and print-its.
4. Accept or revert browser-backed source edits from inside the same editor session.
5. Save/reopen when you want the same live session to survive a restart.
