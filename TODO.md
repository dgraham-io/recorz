# Recorz TODO

## Goal
Get Recorz to the point where text rendering and the main development UI are owned by image-side objects rather than hard-wired in the C VM, while keeping RV32 as the primary development target.

## Current State
- [x] RV32 framebuffer boot on QEMU
- [x] Interactive workspace editor in the framebuffer
- [x] Browser-backed editing for methods, classes, and packages
- [x] In-image accept, revert, test, save, resume, and regeneration hooks
- [x] Source-owned kernel/package regeneration path
- [ ] Text rendering policy owned by the image
- [ ] Image-owned workspace/browser views
- [ ] Real window or pane structure inside the image

## Keep In The Platform / VM
- [ ] Framebuffer access and low-level pixel writes
- [ ] Keyboard and serial event delivery
- [ ] BitBlt / bitmap copy / fill primitives
- [ ] Snapshot load/save transport and boot glue
- [ ] Emergency panic/debug output

## Phase 1 - Make The Native Development Loop Comfortable
- [x] Make the browser/editor workflow the default development path, not a special tool mode
- [x] Improve editor help, status, and feedback so the current command surface is self-explanatory
- [x] Tighten save/resume/recovery so continuing from a snapshot feels like a normal image action
- [x] Make package-centered edit/test/save flow routine from inside the image
- [ ] Prove the full RV32 loop repeatedly: browse, edit, accept, test, save, reboot, continue

## Phase 2 - Define The Image-Side Text Model
- [ ] Add image-visible font objects rather than relying on VM-only font decisions
- [ ] Represent glyph metrics, baseline, ascent, descent, and line height in image objects
- [ ] Represent text style separately from raw strings
- [ ] Add image-side text layout objects for line breaking, wrapping, margins, and tab handling
- [ ] Add image-side cursor and selection model objects
- [ ] Keep the C VM limited to primitive bitmap and input support while layout decisions move upward

## Phase 3 - Build Image-Side Text Rendering
- [ ] Add a Smalltalk-shaped text renderer that draws glyphs onto `Form`/`Bitmap` using existing primitives
- [ ] Move string drawing policy out of the ad hoc C workspace/browser renderer
- [ ] Render cursor, selection, status lines, and simple highlighted regions from image code
- [ ] Make the first comfortable development font target a readable 16-point size for the framebuffer workspace/browser
- [ ] Support at least two usable text sizes, with 16-point as the baseline development size and a smaller dense mode after that
- [ ] Make text metrics configurable so resolution can be used effectively on RV32 targets
- [ ] Keep a temporary C fallback only until the image-side renderer reaches parity

## Phase 4 - Introduce Image-Owned UI Primitives
- [ ] Decide the first UI structure: simple pane/window system or a minimal declarative view tree
- [ ] Add image-side view objects for bounds, invalidation, redraw, and focus
- [ ] Add image-side input routing for keyboard commands and focus changes
- [ ] Add image-side layout objects for vertical, horizontal, and split arrangements
- [ ] Add core reusable widgets: label, list, text editor, status area, and command/action surface
- [ ] Keep the VM limited to event delivery and primitive drawing instead of view policy

## Phase 5 - Replace The C Workspace / Browser Renderer
- [ ] Rebuild the workspace as an image-side editor component
- [ ] Rebuild the browser as image-side list/source components
- [ ] Move do-it, print-it, accept, revert, test, save, and regenerate commands behind image-side tool objects
- [ ] Make browser navigation, source display, and editing use the same image-owned text/UI components
- [ ] Remove the special-case C rendering paths once image-side tools are functionally equivalent

## Phase 6 - Add Real Native Tool Structure
- [ ] Support a true workspace surface and browser surface at the same time
- [ ] Add a minimal pane or window manager for switching, tiling, or stacking tool views
- [ ] Preserve open tools, focus, cursor, and scroll state across snapshot save/load
- [ ] Allow a normal development session to stay entirely inside the image
- [ ] Make the image reopen into the same development tools after reboot

## Phase 7 - Make The Tools Source-Owned
- [ ] File out and file in the text/UI tool classes in canonical source form
- [ ] Include the tool classes in regenerated kernel / boot source
- [ ] Prove that a regenerated image reboots with the same text/UI tools available
- [ ] Reduce remaining builder-side ownership of text and tool behavior

## Phase 8 - Optional But Likely Near-Term Improvements
- [ ] Add a denser bitmap font optimized for 1024x768 and similar framebuffer targets once the 16-point baseline renderer is working
- [ ] Add basic selection, copy, and paste hooks
- [ ] Add mouse input after keyboard-only tools are stable
- [ ] Experiment with a declarative UI layer once the first image-owned tool set is working
- [ ] Decide whether the long-term UI model is classic Smalltalk MVC, Morphic-like, or declarative on top of the same primitive view layer

## Milestones To Watch
- [ ] Milestone A: image-owned text layout and rendering for the workspace
- [ ] Milestone B: image-owned workspace and browser components replacing the C renderer
- [ ] Milestone C: native snapshot/reopen of multiple tool surfaces
- [ ] Milestone D: regenerated image boots directly into image-owned development tools

## Definition Of Done
- [ ] Text layout and rendering decisions come from Recorz objects in the image
- [ ] Workspace and browser are implemented as image-side tools
- [ ] The VM only provides primitive drawing, input, and persistence support
- [ ] Development can continue from inside the image without depending on C-side UI policy
