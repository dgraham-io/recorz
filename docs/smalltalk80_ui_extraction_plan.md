# Smalltalk-80 UI Extraction Plan For Recorz

## Purpose
Use the original Smalltalk-80 source at [Smalltalk-80.sources](/Users/david/repos/recorz/misc/Smalltalk-80.sources) as a design reference for Recorz's first native UI, without treating it as a porting target.

The point of this plan is to extract:
- the smallest useful primitive boundary
- the image-side responsibilities that belong above that boundary
- the tool patterns that matter for a first Recorz workspace and browser

The point is not to reproduce the full Smalltalk-80 MVC stack, object memory, or exact tool set.

## Recorz Constraints
- RV32 remains the primary development target.
- The first native UI should help us continue development inside the image as soon as possible.
- Recorz stays Smalltalk/Strongtalk-shaped, but should keep room for later capability-oriented isolation, gradual typing, systems work, and declarative UI experiments.
- The VM should keep low-level drawing, input delivery, persistence, panic/debug output, and later GC support.
- UI policy, text composition, and tool behavior should move into image-side objects.

## How To Use Smalltalk-80 Here
- Treat Smalltalk-80 as a protocol and architecture reference.
- Reuse class responsibilities and primitive boundaries where they fit.
- Do not copy the whole MVC framework blindly.
- Do not import original object-memory assumptions into Recorz.
- Prefer smaller, cleaner Recorz equivalents over historical completeness.

## First Extraction Set
These are the classes and categories in [Smalltalk-80.sources](/Users/david/repos/recorz/misc/Smalltalk-80.sources) that matter most for the initial Recorz UI.

### 1. Primitive Boundary And Display Surface
- `BitBlt`
  Source role: `Object subclass: #BitBlt`
  Category: `Graphics-Support`
  Use in Recorz: core bitmap transfer primitive surface, clipping, fill, transfer rules.
- `Form`
  Source role: `DisplayMedium subclass: #Form`
  Category: `Graphics-Display Objects`
  Use in Recorz: image-owned bitmap/display abstraction, screen target, offscreen target.
- `Cursor`
  Source role: `Form subclass: #Cursor`
  Category: `Graphics-Display Objects`
  Use in Recorz: cursor image and hotspot owned by the image, with a tiny VM binding primitive.
- `DisplayScreen`
  Source role: `Form subclass: #DisplayScreen`
  Category: `Graphics-Display Objects`
  Use in Recorz: reference model for a real screen-bound form and `beDisplay`-style behavior.

### 2. Text Composition And Rendering
- `CharacterScanner`
  Source role: `BitBlt subclass: #CharacterScanner`
  Category: `Graphics-Support`
  Use in Recorz: model for the text-scan primitive boundary and image-side scanner state.
- `TextStyle`
  Source role: `Object subclass: #TextStyle`
  Category: `Graphics-Support`
  Use in Recorz: confirms that style should stay image-owned, not VM-owned.
- `DisplayText`
  Source role: `DisplayObject subclass: #DisplayText`
  Category: `Graphics-Display Objects`
  Use in Recorz: model for a rendered text object layered on top of forms/scanners.
- `Paragraph`
  Source role: `DisplayText subclass: #Paragraph`
  Category: `Graphics-Display Objects`
  Use in Recorz: composition and layout reference for the first image-owned text body/editor surface.

### 3. Editor And Tool Models
- `ParagraphEditor`
  Source role: `ScrollController subclass: #ParagraphEditor`
  Category: `Interface-Text`
  Use in Recorz: reference for keyboard editing behavior, selection, scrolling, and editor command surface.
- `StringHolder`
  Source role: `Object subclass: #StringHolder`
  Category: `Interface-Text`
  Use in Recorz: strong reference point for a workspace/source-holder model.
- `Browser`
  Source role: `Object subclass: #Browser`
  Category: `Interface-Browser`
  Use in Recorz: model for the browser state object separate from the rendered surface.
- `PopUpMenu`
  Source role: `Object subclass: #PopUpMenu`
  Category: `Interface-Menus`
  Use in Recorz: later command/menu pattern reference, not a first priority.

### 4. UI Framework Reference Only
- `View`
  Source role: `Object subclass: #View`
  Category: `Interface-Framework`
- `Controller`
  Source role: `Object subclass: #Controller`
  Category: `Interface-Framework`

These are useful as reference points for where policy lives, but should not be treated as mandatory architecture for Recorz. We can still choose a simpler pane model or later declarative layer.

## What To Extract First
The extraction order should follow Recorz's current needs, not Smalltalk-80's historical layering.

### Pass 1. Primitive Contract
Goal: settle the first Recorz primitive boundary.

Extract from Smalltalk-80:
- what `BitBlt` is responsible for
- what `Form` owns versus what the display owns
- what `Cursor` requires from the VM
- what a `DisplayScreen`-style bound screen surface implies
- what `CharacterScanner` expects from a text-scan primitive

Deliverables:
- a short Recorz primitive list derived from these classes
- a mapping from current Recorz primitives to missing Smalltalk-shaped primitives
- a list of current C helpers that should become either primitives or image code

### Pass 2. Text Stack
Goal: get Recorz text composition and scanning out of ad hoc C policy.

Extract from Smalltalk-80:
- scanner state and stop conditions from `CharacterScanner`
- style ownership from `TextStyle`
- composition responsibilities from `DisplayText` and `Paragraph`

Deliverables:
- a Recorz `CharacterScanner` plan
- a Recorz text run/composition model
- a plan for 12-point primary rendering and an optional 16-point comfort mode on RV32

### Pass 3. Editor Model
Goal: define the first native workspace editor behavior.

Extract from Smalltalk-80:
- source-holder role from `StringHolder`
- editing responsibilities from `ParagraphEditor`
- how editor state is separate from display primitives

Deliverables:
- a Recorz workspace model object plan
- a Recorz text-editor component plan
- the command model for do it, print it, accept, revert, test, save, regenerate

### Pass 4. Browser Model
Goal: define the first native browser on top of the same text/editor components.

Extract from Smalltalk-80:
- browser state model from `Browser`
- separation between browser state and rendered views

Deliverables:
- a Recorz browser model object
- class/package/method browsing state structure
- a plan for browser/editor integration without duplicating source widgets

### Pass 5. UI Structure Decision
Goal: choose the thinnest image-side structure that supports the first real tools.

Extract from Smalltalk-80:
- how `View` and `Controller` separate concerns
- how much of that separation is worth keeping in Recorz

Deliverables:
- decision note: minimal pane system or minimal declarative view tree
- focus and input-routing design
- redraw/invalidation model

## Recommended Recorz Mapping
This is the recommended first mapping from Smalltalk-80 ideas to Recorz work.

### Keep In The VM / Platform
- framebuffer access
- bitmap copy/fill primitives
- display binding
- cursor binding
- text scan primitive
- keyboard and serial event delivery
- snapshot/load/save
- panic/debug output
- later GC support

### Move Into The Image
- font choice and text metrics policy
- text composition and wrapping
- glyph placement policy above the scan primitive
- cursor and selection behavior
- workspace rendering
- browser rendering
- editor/browser command routing
- menus or command surfaces

## What Not To Copy Directly
- the exact Smalltalk-80 MVC structure
- the full menu/controller framework
- original object memory details
- original scheduler/process assumptions
- global-system assumptions that conflict with Recorz's long-term capability direction

## Immediate Extraction Tasks
These are the next concrete tasks that should come out of this plan.

1. Read and summarize `BitBlt`, `Form`, `Cursor`, `DisplayScreen`, and `CharacterScanner` into a Recorz primitive note.
2. Define the first Recorz primitive surface in docs using those classes as reference.
3. Introduce `beDisplay`-style and `beCursor`-style primitives in Recorz.
4. Design and implement the first `CharacterScanner`-class primitive for Recorz.
5. Build the first image-side scanner/composer on top of that primitive.
6. Replace remaining C text layout policy in the workspace with image-owned scanner/composer code.
7. Define `StringHolder`-like workspace model objects and `Browser`-like model objects in Recorz.
8. Build the first native workspace and browser surfaces on top of those image-owned models.

## GC Implications
Smalltalk-80 is also a reminder that once tools live in the image, object memory quality matters.

For Recorz, GC becomes important when:
- editor/view trees become normal image objects
- text composition objects are transient and numerous
- display/cursor objects and tool state need stable roots

This does not mean Recorz must copy Smalltalk-80's collector. It means the collector eventually needs to support the same style of image-owned tools cleanly.

## Definition Of Success
This extraction plan succeeds when it gives Recorz:
- a clearly defined Smalltalk-style primitive boundary
- a practical plan for image-owned text composition and rendering
- a workspace/editor/browser model that is native to the image
- guidance from Smalltalk-80 without turning Recorz into a historical clone
