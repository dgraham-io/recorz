# Recorz UI Primitive Contract

## Purpose
This note completes Pass 1 from [smalltalk80_ui_extraction_plan.md](/Users/david/repos/recorz/docs/smalltalk80_ui_extraction_plan.md): it summarizes the Smalltalk-80 display/text primitives we want to emulate in spirit, maps them onto the current RV32 runtime, and fixes the initial boundary between the VM/platform and the image-owned UI.

This is a contract for the first native Recorz UI, not a promise to reproduce Smalltalk-80 exactly.

## Smalltalk-80 Pass 1 Summary
These are the key lessons from the first extraction set in [Smalltalk-80.sources](/Users/david/repos/recorz/misc/Smalltalk-80.sources).

### `BitBlt`
- Owns low-level bitmap transfer, clipping, fill, and transfer rules.
- Is the central graphics primitive, not a full UI policy object.
- Suggests Recorz should keep bitmap movement and rectangular fill below the image, but not text/editor policy.

### `Form`
- Is the image-owned bitmap/display abstraction.
- Can represent off-screen and on-screen drawing surfaces.
- Suggests Recorz should let the image own forms, while the VM only knows how to bind one to the real framebuffer and move pixels to it.

### `Cursor`
- Is just another form-like object with cursor-specific binding semantics.
- Suggests Recorz should keep cursor image/hotspot in image objects and expose only a tiny binding primitive.

### `DisplayScreen`
- Is a screen-bound form, not a separate UI framework.
- Suggests Recorz should add a `beDisplay`-style path instead of hard-wiring all output to the current global display form forever.

### `CharacterScanner`
- Sits close to `BitBlt`, but still exists to support image-owned text behavior.
- Suggests Recorz should expose a low-level scan primitive that returns placement/stop information, then let image code own composition, wrapping, cursor placement, and editor behavior.

## Exact Initial Primitive Surface
The first Recorz native UI should keep only the following behavior in C.

### 1. Bitmap Transfer And Fill
- `BitBlt>>copyBits`-class primitive path
- rectangular fill path
- clipping support
- monochrome-to-form and form-to-form transfer support as needed
- transfer/composition rule selection

Current related runtime/platform code:
- [display.c](/Users/david/repos/recorz/platform/qemu-riscv32/display.c)
- [BitBlt.rz](/Users/david/repos/recorz/kernel/mvp/BitBlt.rz)
- [vm.c](/Users/david/repos/recorz/platform/qemu-riscv32/vm.c)

### 2. Display Binding
- `Form>>beDisplay`-class primitive path
- bind an image-side `Form` as the active screen surface
- keep framebuffer ownership and raw scanout in the platform

### 3. Cursor Binding
- `Cursor>>beCursor`-class primitive path
- install cursor bitmap and hotspot from image-side objects
- allow show/hide and position updates without encoding editor policy in C

### 4. Text Scan Primitive
- `CharacterScanner`-class primitive
- accepts source text/run state and stop conditions
- returns next placement/advance/stop state
- does not decide editor behavior, tool layout, or UI structure

### 5. Input Delivery
- keyboard and serial delivery as raw events/bytes
- no editor or browser policy in the delivery path

### 6. Persistence, Recovery, And Debug
- snapshot save/load transport
- boot glue
- panic/debug output
- later GC support and root tracing

## What Moves Into The Image
The image should own:
- font choice and metrics policy
- glyph use above the bitmap transfer primitive
- text composition and wrapping
- line breaking, status/feedback composition, cursor placement, and selection behavior
- workspace/browser rendering
- tool commands and UI structure

## Current RV32 Helper Audit
This classifies the major C-side helper families in [vm.c](/Users/david/repos/recorz/platform/qemu-riscv32/vm.c) and the low-level framebuffer helpers in [display.c](/Users/david/repos/recorz/platform/qemu-riscv32/display.c).

### Keep As Long-Lived Primitive / Platform Support
- `display_form_blit_mono_bitmap(...)`
- `display_form_fill_color(...)`
- `display_form_fill_rect(...)`
- `bitblt_copy_mono_bitmap_to_form(...)`
- `bitblt_copy_mono_bitmap_to_mono_bitmap(...)`
- raw keyboard/serial delivery in the machine layer
- snapshot/boot/panic/debug transport

These are the right low-level responsibilities to keep below the image.

### Temporary Bridge / Transitional Runtime Support
- `form_write_string(...)`
- `form_write_string_with_colors(...)`
- `form_write_code_point_with_colors(...)`
- `form_newline(...)`
- `form_clear(...)`
- the current shared bridge objects used to delegate `writeStyledText:` into image-side code

These are useful now, but they are not the long-term UI boundary. They exist to keep the current transcript/workspace working while text policy moves upward.

### Behavior That Should Move Upward Into The Image
- `text_left_margin()`
- `text_right_margin()`
- `text_wrap_width()`
- `text_tab_width()`
- `text_line_height()`
- `text_line_break_mode()`
- status/feedback composition helpers
- cursor-line/column calculation helpers
- all `workspace_render_*` functions
- most `workspace_input_monitor_*` presentation logic

These helpers are not low-level primitives. They are image-level text/editor/tool behavior still living in C.

## Clipping And Transfer-Rule Decisions
These decisions are now explicit.

### Clipping
- Clipping stays below the image as part of the bitmap transfer primitive.
- Image code should be able to request drawing without manually clipping each glyph or rectangle.
- The first UI only needs rectangular clipping.

### Transfer Rules
- The primitive surface should support a small rule set first:
  copy, paint/over, erase/under, and monochrome transparent-zero handling.
- We do not need every historical BitBlt combination rule before the first native workspace/browser.
- The rule set should be documented and stable before image-owned UI classes depend on it.

## Glyph Blitting Decision
Glyph drawing should not remain a special VM text service.

The intended long-term shape is:
- glyphs stay image-visible bitmap/form objects
- image-side scanner/composer decides what glyph to draw and where
- `BitBlt`-class primitives perform the actual transfer to the destination form

Implication:
- `Form>>writeString:` and `Form>>writeStyledText:` are transitional convenience surfaces
- the real native UI should eventually render text through image-owned scanner/composer objects using the same underlying copy primitive

## RV32 Performance Assumptions
These assumptions constrain the primitive boundary.

- Recorz should not send per-pixel messages from image code.
- Bitmap transfer, fill, scan, clipping, and cursor install must remain primitive operations.
- Image-side composition may walk characters/runs, but should hand off spans/glyphs to primitives.
- The first native tool set should optimize for the 12-point primary development font on 1024x768-class displays.
- A 16-point comfort mode is acceptable later if it reuses the same primitive boundary.
- Off-screen intermediate forms should stay simple at first; monochrome scratch forms are acceptable during early bring-up.
- The boundary must remain workable on RV32 with a future 32 MB target platform, even if the current dev profile is roomier.

## Immediate Consequences For Implementation
This contract means the next concrete work should be:

1. Add `beDisplay`-style form binding.
2. Add `beCursor`-style cursor binding.
3. Add the first `CharacterScanner`-class primitive.
4. Replace the remaining VM-owned text layout rules with image-side scanner/composer objects.
5. Retire the C workspace/browser renderers once image-side tools reach parity.

## Completion Criteria For This Note
Pass 1 is complete if this note remains true:
- the long-lived primitive set is small and explicit
- text/editor/tool policy is clearly above that boundary
- the current C helpers are classified rather than implicitly permanent
- RV32 constraints are part of the contract instead of an afterthought
