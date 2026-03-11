# Recorz TODO

## Goal
Build the first native Recorz development UI in the image using a Smalltalk-80-style primitive boundary:

- a tiny VM/platform surface for bitmap display, cursor/display binding, text scanning, line/shape support, input delivery, and persistence
- image-owned text composition, editor/browser rendering, tool behavior, and UI structure
- RV32 as the primary development target, with RV64 kept as a validation target

The immediate target is not a full window system. It is a credible initial in-image workspace and browser built on the same kind of primitive boundary Smalltalk-80 used.

Reference:
- Use [smalltalk80_ui_extraction_plan.md](/Users/david/repos/recorz/docs/smalltalk80_ui_extraction_plan.md) as a loose design guide for the primitive boundary, text stack, and first tool model.

## Primitive Boundary To Reach
- [x] `BitBlt>>copyBits`-class primitive path for bitmap copy, fill, clipping, and monochrome/color transfer
- [x] `Form>>beDisplay`-class primitive path to bind a `Form` to the real framebuffer
- [x] `Cursor>>beCursor`-class primitive path to install a cursor image
- [x] `CharacterScanner`-class text scan primitive for glyph placement and stop conditions
- [x] line/shape primitive support comparable to `drawLoop` / line drawing support where useful
- [x] keyboard and serial event delivery as raw input primitives, not UI policy
- [x] snapshot/load/save and boot glue kept below the image
- [x] emergency panic/debug output kept below the image

## Current State
- [x] RV32 framebuffer boot on QEMU
- [x] Interactive workspace editor in the framebuffer
- [x] Browser-backed editing for methods, classes, and packages
- [x] In-image accept, revert, test, save, resume, and regeneration hooks
- [x] Source-owned kernel/package regeneration path
- [x] Image-visible font, metrics, style, layout, cursor, and selection objects
- [x] First image-side text renderer slice
- [x] Shared `Form` string path can delegate to image-side rendering on RV32
- [x] Display/input primitive boundary cleaned up around the Smalltalk-80 model
- [ ] Image-owned workspace/browser rendering
- [ ] Real native tool surfaces in the image

## Phase 1 - Lock The Primitive Contract
- [x] Complete Pass 1 from [smalltalk80_ui_extraction_plan.md](/Users/david/repos/recorz/docs/smalltalk80_ui_extraction_plan.md):
  summarize `BitBlt`, `Form`, `Cursor`, `DisplayScreen`, and `CharacterScanner` into a Recorz primitive note
- [x] Decide and document the exact initial primitive surface Recorz will keep in C:
  `copyBits`, display binding, cursor binding, text scan, input delivery, snapshot/persistence, panic/debug
- [x] Audit current C-side text/UI helpers and classify each one as:
  primitive to keep, temporary scaffold to replace, or behavior that should move into the image immediately
- [x] Normalize existing bitmap/form operations so the future image-side renderer uses one coherent primitive path instead of multiple ad hoc text helpers
- [x] Add explicit clipping and transfer-mode decisions to the bitmap copy surface
- [x] Define how glyph blitting relates to `copyBits` so text rendering stays Smalltalk-shaped rather than hard-wired as a special VM text API
- [x] Document the RV32 performance assumptions for this primitive set

## Phase 2 - Finish The Display / Form / Cursor Primitive Layer
- [x] Introduce a real `beDisplay`-style path so a `Form` can become the active screen surface from image code
- [x] Introduce a real `beCursor`-style path so cursor shape and hotspot come from image objects
- [x] Expose cursor show/hide and position updates through a primitive boundary that does not own editor policy
- [x] Make the active display form and active cursor snapshot-safe and image-visible
- [x] Keep the existing framebuffer bootstrap working while this surface is introduced

## Phase 3 - Build The Text Scan Primitive
- [x] Add a `scanCharacters`-class primitive suitable for an image-side `CharacterScanner` / `TextScanner`
- [x] Define stop conditions for:
  end of run, right margin, tab, newline, control character, and optional selection/cursor boundaries
- [x] Return enough placement state from the scan primitive to let image code own wrapping and composition
- [x] Keep the primitive low-level: scan/advance only, not editor policy
- [x] Prove the primitive can render the current transcript/workspace text with image-owned scanner code

## Phase 4 - Move Text Layout And Composition Into The Image
- [x] Replace VM-owned margin, wrap, tab, and line-height policy with image-owned text composition objects
- [x] Move line breaking, cursor x/y calculation, and visible line traversal into image code
- [x] Move status-line and feedback-line composition into image code
- [x] Make the primary reference development font a readable 12-point image-owned font
- [x] Add a larger 16-point comfort mode once the 12-point primary font is working well
- [x] Make text metrics configurable from the image instead of fixed in the VM
- [x] Keep a temporary C fallback only until image-side composition reaches parity

## Phase 5 - Complete The Image-Side Text Renderer
- [x] Rebuild the current transcript/workspace text painting path around image-side scanner/renderer objects
- [x] Render cursor, selection, status lines, and highlighted regions from image code
- [x] Support at least:
  plain text, styled text, cursor painting, and single-region selection highlighting
- [x] Keep the VM at the primitive boundary:
  bitmap copy, display/cursor binding, text scan, input delivery
- [x] Prove that normal workspace text output no longer depends on C-side text layout rules

## Phase 6 - Introduce Image-Owned UI Primitives
- [x] Decide the first image-side UI structure:
  minimal pane/window system or minimal declarative view tree on top of the same primitive layer
- [ ] Add image-side view objects for bounds, invalidation, redraw, and focus
- [ ] Add image-side input routing for keyboard focus and command dispatch
- [ ] Add image-side layout objects for vertical, horizontal, and split arrangements
- [ ] Add core widgets needed for the first tools:
  label, list, text editor, status area, command surface
- [ ] Keep event transport and primitive drawing in the VM only

## Phase 7 - Replace The C Workspace / Browser Renderer
- [ ] Rebuild the workspace as an image-side editor component
- [ ] Rebuild the browser as image-side list/source components
- [ ] Move do-it, print-it, accept, revert, test, save, and regenerate behind image-side tool objects
- [ ] Make browser navigation and source editing use the same editor/view components
- [ ] Remove the special-case C workspace/browser renderers once the image-side tools reach parity

## Phase 8 - Reach The First Native UI
- [ ] Support a functional workspace surface and a functional browser surface from inside the image
- [ ] Allow switching between them without falling back to C-side tool policy
- [ ] Preserve open tool state, focus, cursor, and scroll position across snapshot save/load
- [ ] Make the normal development loop:
  browse, edit, do it / print it / accept, test, save, reboot, continue
- [ ] Reopen into the same native tools after reboot

## Phase 9 - Garbage Collector Work Needed For Native Tools
- [ ] Decide whether the current fixed heap is sufficient for the initial native tool milestone or whether GC must land first
- [ ] If needed, introduce a Smalltalk-appropriate managed heap/GC before the image-side UI becomes the default tool surface
- [ ] Keep the collector compatible with:
  handles/object identity, snapshots, live method source tables, named objects, contexts, and display/cursor roots
- [ ] Ensure the collector can trace:
  activation/context stacks, live editor state, source buffers, text/layout objects, and view trees
- [ ] Keep GC policy below the image, but make the image-side tool set safe to run under it
- [ ] Add recovery/debug tooling so UI work is not fragile while the collector matures

## Phase 10 - Source-Own The UI Stack
- [ ] File out and file in the text/UI tool classes in canonical source form
- [ ] Include the tool classes in regenerated kernel / boot source
- [ ] Prove that a regenerated image boots directly into the native workspace/browser tools
- [ ] Reduce remaining builder-side ownership of text and UI behavior

## Near-Term Experiments
- [x] Add a temporary transcript/browser demo built directly on `beDisplay` + `copyBits` + image-side scanner code
- [ ] Measure 12-point primary and 16-point comfort-mode text density on 1024x768 and current QEMU targets
- [x] Add a simple cursor-form proof using `beCursor`
- [x] Add a simple scanner proof that stops at wrap margin and tab/newline boundaries
- [x] Evaluate whether the first native UI should stay Smalltalk-pane-like or move toward a declarative surface after the primitive layer is proven

## Milestones To Watch
- [x] Milestone A: Smalltalk-80-style primitive boundary documented and implemented
- [x] Milestone B: image-owned text composition and rendering for the workspace
- [ ] Milestone C: image-owned workspace and browser replacing the C renderer
- [ ] Milestone D: native workspace and browser survive snapshot/reboot
- [ ] Milestone E: regenerated image boots directly into image-owned development tools

## Definition Of Done
- [ ] Recorz UI drawing depends on a tiny primitive set comparable in spirit to Smalltalk-80
- [ ] Text composition and rendering decisions come from image objects, not C-side UI policy
- [ ] Workspace and browser are implemented as image-side tools
- [ ] The VM provides only low-level drawing, display/cursor binding, input, GC/persistence support, and panic/debug paths
- [ ] Development can continue from inside the image without depending on C-side workspace/browser behavior
