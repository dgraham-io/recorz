# Implementation Log

## 2026-03-06 - Boot Image Metadata
- Added image-level evolution metadata to the QEMU MVP `RCZI` boot image: a feature bitmap plus an FNV-1a checksum.
- Updated the target image loader to reject unknown feature flags and verify the checksum before handing control to the program and seed loaders.
- Extended the lowering tests to validate the new image header fields and checksum behavior.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all`.

## 2026-03-06 - Boot Image Inspector
- Added a host-side `RCZI` inspector in `tools/inspect_qemu_riscv_mvp_image.py` so the generated boot image can be decoded without running QEMU.
- Wired the inspector into `make -C platform/qemu-riscv64 inspect-image` for fast inspection of the generated demo image.
- Added focused tests for successful inspection and checksum-failure rejection.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Boot Image Profile
- Added a fixed `RV64MVP1` profile identifier to the `RCZI` boot image header so incompatible images can be rejected before section loading.
- Updated the target image loader and the host-side inspector to enforce that profile match.
- Extended the image manifest and inspector tests to cover the new profile field and mismatch rejection.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Boot Entry Metadata
- Added a dedicated `entry` section to the `RCZI` image that describes the current top-level boot contract: a zero-argument do-it targeting the program section.
- Updated the target image loader to validate that entry metadata before it touches the program or seed loaders.
- Extended the host-side inspector to display the entry contract and reject malformed entry metadata.
- Extended the manifest and inspector tests to cover the new entry section and malformed-entry rejection.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Seed Binding Tables
- Replaced the fixed seed-header slots for globals, roots, and glyph offsets with explicit seed binding tables and a glyph object-index table.
- Updated the target seed loader and VM root initialization to resolve globals and roots from those image-provided tables instead of C-implied header layout.
- Updated the host-side inspector and tests to decode and validate the new seed format.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Seeded Transcript Layout
- Added a seeded transcript-layout object so text margins, line spacing, and pixel scale come from image data instead of fixed constants in `vm.c`.
- Updated the VM text-rendering path to resolve cursor origin, wrapping, and scaling from that seeded layout root without changing the target message surface.
- Updated seed generation, inspection, and manifest tests to cover the new layout root and shifted object indices.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Seeded Transcript Colors
- Added a seeded transcript-style object so default foreground and background colors come from the boot image instead of fixed constants in `vm.c`.
- Widened seed field payloads to 32-bit values so the seed can carry full color integers directly without special encoding.
- Updated the VM bootstrap, seed loader, image inspector, and manifest tests to cover the new style root and shifted seed object indices.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Seeded Transcript Metrics
- Added a seeded transcript-metrics object so transcript cursor advance and wrapping come from image data instead of hardcoded cell-size constants in `vm.c`.
- Updated the VM to resolve text cell width and height from that seeded root, leaving glyph bitmap assets and the runtime message surface unchanged.
- Bumped the seed version and updated the seed generator, loader, inspector, and manifest tests to cover the new metrics root.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Seeded Transcript Behavior
- Replaced the raw fallback-glyph root with a seeded transcript-behavior object that carries fallback glyph selection and clear-on-overflow policy together.
- Updated the VM to derive fallback bitmap selection and overflow clearing from that seeded behavior object instead of keeping those policies implicit in `vm.c`.
- Bumped the seed version and updated the seed generator, loader, inspector, and manifest tests to cover the new behavior root and shifted seed object count.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Seeded Built-In Class Descriptors
- Added explicit seeded class-descriptor objects and per-object class links for the built-in target objects so the boot image carries more of the kernel object/class structure directly.
- Reused the seed object header to carry class indices, updated the target seed loader and heap bootstrap to install those class links, and kept the runtime message surface unchanged.
- Extended the image inspector and tests to validate class links and report the number of seeded class descriptors.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Boot-Time Class Graph Validation
- Made the seeded built-in class layer active at boot by validating that every seeded object has a class link, that class links point to class descriptors, and that each class descriptor's instance-kind field matches the object's runtime kind.
- Extended the host-side image inspector to validate the same class graph and report both class-descriptor count and total class-link count for the seed image.
- Added targeted manifest assertions for the first built-in object/class link and the class-class descriptor shape.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Minimal Class Introspection
- Added the smallest runtime-facing use of the seeded class layer: heap objects now answer `class`, and class objects answer `instanceKind`.
- Updated the target lowerer and demo program to exercise that path, so the framebuffer output now proves class introspection works end to end on the MVP target.
- Fixed runtime allocation so newly created `Form` and `Bitmap` objects inherit the correct seeded class links instead of only the pre-seeded roots having valid class identity.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Class-Based Primitive Routing
- Refactored target-side heap-object send dispatch to derive each receiver's primitive family from its seeded class descriptor instead of branching directly on raw object-kind tags.
- Kept the selector surface and behavior unchanged, so this is a runtime cleanup that makes the seeded class layer part of ordinary dispatch without attempting a full method lookup system.
- Applied the same class-based check to the `BitBlt fillForm:color:` operand validation path so newly allocated forms and pre-seeded forms are treated uniformly through class identity.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all`, and `make -C platform/qemu-riscv64 inspect-image`.

## 2026-03-06 - Extracted Class-Based Primitive Helpers
- Split the `Form`, `Bitmap`, and `Class` primitive selector handling out of the monolithic heap-object send switch into dedicated helper routines in `vm.c`.
- Kept the runtime behavior and selector surface unchanged, but narrowed the main dispatch path so future primitive-family cleanup can proceed one class-shaped slice at a time.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Extracted Transcript And Display Helpers
- Split the `Transcript` and `Display` primitive selector handling out of the monolithic heap-object send switch into dedicated helper routines in `vm.c`.
- Kept the runtime behavior unchanged and left both classes routed through the same default-form services, but narrowed the dispatch path so more primitive families can be cleaned up independently.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Extracted BitBlt Helper
- Split the `BitBlt` primitive selector handling out of the monolithic heap-object send switch into a dedicated helper routine in `vm.c`.
- Kept the existing operand validation and copy/fill behavior unchanged, so this remains a structural cleanup that leaves the MVP graphics surface stable while shrinking the central dispatch path.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Extracted Glyph And Factory Helpers
- Split the remaining `Glyphs`, `FormFactory`, and `BitmapFactory` primitive selector handling out of the monolithic heap-object send switch into dedicated helper routines in `vm.c`.
- Kept allocation and glyph lookup behavior unchanged, leaving the heap-object send path as a narrow family dispatcher rather than a large inline selector switch.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Added Heap-Object Send Router
- Replaced the remaining inline heap-object family chain in `dispatch_send` with an explicit router layer driven by a small family-handler table.
- Kept the existing class-derived primitive kind logic and per-family behavior unchanged, but moved the VM one step closer to a tiny lookup-shaped runtime instead of a handcrafted send switch.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Added Target Panic Context Reporting
- Added a machine-level panic hook seam and used it from the target VM to print current phase, instruction, last send selector/operands, and a short value stack dump on VM panics.
- Kept the runtime behavior unchanged on the success path, but made target failures much more inspectable over serial without introducing a debugger protocol or widening the language surface.
- Added a failing QEMU example and a headless panic integration test that asserts the serial panic output includes the new VM context report.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Built-In Method Descriptors
- Extended the target seed/image format with a `MethodDescriptor` object kind and two extra class-descriptor fields so each seeded built-in class now owns an explicit contiguous range of built-in method descriptors.
- Kept execution primitive-based for now, but added matching VM boot validation and host-side image inspection so the target image carries a real class-to-method layer instead of only class links plus ad hoc primitive families.
- Updated the manifest tests and inspector summary to cover the new method-descriptor counts and reject malformed method metadata.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Routed Sends Through Seeded Method Lookup
- Changed target heap-object sends to scan the receiver class's seeded built-in method descriptors first, then route the matched send through the existing primitive-family handlers.
- Kept execution primitive-based after lookup, but made the seeded method layer runtime-active instead of boot-only metadata, which is the first real step toward lookup-shaped execution.
- Added a headless QEMU integration case using a `.rz` example file to prove that a supported selector sent to the wrong receiver now fails through class lookup with a targeted panic.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Dispatched Through Method Entry Ids
- Extended seeded built-in method descriptors with an explicit entry id and bumped the target seed format so method lookup now resolves to a concrete execution entry instead of only a primitive family.
- Replaced the heap-object family router with a method-entry handler table in `vm.c`, while keeping execution primitive-backed and validating selector, arity, and receiver kind against the declared entry metadata at boot.
- Extended the host-side image inspector and manifest tests to validate method-entry metadata, report the number of unique seeded method entries, and reject entry/selector mismatches.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Method Entry Objects
- Added a new seeded `MethodEntry` object kind so built-in method descriptors now point to first-class entry objects in the image instead of storing raw execution ids directly.
- Kept execution handler behavior unchanged, but updated the target VM to fetch and validate the `MethodEntry` object before dispatch, which makes the kernel image more method-shaped without widening the language surface.
- Bumped the seed format again, increased the tiny target heap to fit the extra seeded objects, and extended the host-side inspector and manifest tests to validate and report `MethodEntry` objects alongside method descriptors.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Selector Objects
- Added a new seeded `Selector` object kind so built-in method descriptors now point to selector objects instead of carrying raw selector integers directly.
- Kept send bytecodes and runtime behavior unchanged, but updated the target VM and host-side inspector to resolve and validate selector objects as part of method lookup and boot-image checking.
- Bumped the seed format again, increased the tiny target heap to absorb the extra selector objects, and extended the manifest tests and inspector summary to cover selector-object counts and the shifted seed layout.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Activated Selector Objects In Lookup
- Made the seeded selector objects runtime-active by adding a selector-handle cache at boot and resolving object sends through selector-object identity during method lookup instead of comparing raw selector integers.
- Kept the language surface and successful send behavior unchanged, but aligned the runtime more closely with the image-owned selector layer and added host-side duplicate-selector validation to match the target cache assumptions.
- Added a focused inspector regression for duplicate selector objects and kept the QEMU boot/render path green with the new lookup seam.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Accessor Method Bodies
- Added a new seeded `AccessorMethod` object kind so a small set of trivial built-in methods can execute through image-owned method-body objects instead of only through C-side primitive handler entries.
- Moved `Bitmap>>width`, `Bitmap>>height`, `Form>>bits`, and `Class>>instanceKind` onto this accessor-method path, while keeping the rest of the built-in method table primitive-backed and preserving the existing runtime behavior.
- Extended the target VM, seed loader, host-side image inspector, and manifest tests to validate `MethodEntry` implementation objects, report accessor-method counts, and reject malformed accessor-backed entries.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Field-Send Method Bodies
- Added a new seeded `FieldSendMethod` object kind so the target kernel image can represent a slightly richer built-in method body than a direct slot read while still staying entirely inside the current built-in method-entry model.
- Moved `Form>>width` and `Form>>height` off the primitive handler table and onto this field-send path, where the method body reads `bits` from the receiver and then sends `width` or `height` to that bitmap value through the ordinary send machinery.
- Refactored the target VM’s send core just enough to support nested value sends from method-body objects, and extended the seed/image inspector plus manifest tests to validate and report the new `FieldSendMethod` objects.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Root-Send Method Bodies
- Added a new seeded `RootSendMethod` object kind so the kernel image can represent zero-argument methods that delegate to a seeded root object and then choose whether to return the nested result or the original receiver.
- Moved `Transcript>>cr`, `Display>>clear`, and `Display>>newline` off the primitive handler table and onto this root-send path, with each method now delegating to the seeded default form through the same send machinery used by ordinary lookup.
- Extended target-side boot validation and the host-side image inspector to validate root ids, delegated selectors, and return modes for `RootSendMethod` objects, and updated the manifest tests to cover the shifted seed layout and the new object counts.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Forwarded One-Argument Root Sends
- Extended the existing seeded `RootSendMethod` execution path so it can forward the caller's arguments unchanged into its delegated send instead of only supporting zero-argument delegation.
- Moved `Transcript>>show:` and `Display>>writeString:` off the primitive handler table and onto that forwarded root-send path, with both methods now delegating to `writeString:` on the seeded default form and returning the original receiver.
- Kept the seed schema unchanged, but increased the number of seeded `RootSendMethod` objects, updated the manifest expectations to match the shifted method-entry layout, and removed the now-dead default-form helper from `vm.c`.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Root-Value Method Bodies
- Added a new seeded `RootValueMethod` object kind so the kernel image can represent methods that simply answer one of the seeded root objects without falling back to a primitive handler.
- Moved `Display>>defaultForm` off the primitive table and onto that root-value path, which completes the current `Display` family as image-owned method bodies rather than C-only entry handlers.
- Extended target-side boot validation and the host-side image inspector to validate `RootValueMethod` objects and method-entry links, updated the manifest tests for the shifted seed layout and new object counts, and removed the now-dead `form_value` helper from `vm.c`.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Interpreted Display Methods
- Added a new seeded `InterpretedMethod` object kind so the target kernel image can carry its first generic interpreted built-in method bodies instead of only specialized method-body descriptors.
- Moved `Display>>defaultForm`, `Display>>clear`, `Display>>writeString:`, and `Display>>newline` onto that interpreted path, using a tiny fixed instruction set with `pushRoot`, `pushArgument`, `send`, `returnTop`, and `returnReceiver`.
- Updated target-side seed validation, runtime execution, the host-side image inspector, and the manifest tests to validate and report interpreted method objects in the shifted seed layout.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Seeded Interpreted Transcript Methods
- Moved `Transcript>>show:` and `Transcript>>cr` off the specialized root-send descriptor path and onto the generic seeded `InterpretedMethod` path already used by `Display`.
- Kept the tiny instruction set unchanged, but expanded the interpreted seed data so transcript delegation to the default form now runs through the same image-owned execution model as the display methods.
- Updated the host-side image inspector and manifest tests to reflect the new seed mix: zero `RootSendMethod` objects and six interpreted method objects.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Source-Built Compiled Kernel Methods
- Added a new seeded `CompiledMethod` object kind and a tiny compiled-method bytecode path on the target, with support for root/global fetch, argument reads, field reads, sends, and return-top/return-receiver.
- Added a host-side lowerer that uses `recorz.compiler.compile_method` for a very small method-source subset, then lowers that host instruction stream into the target compiled-method format.
- Moved the source-compatible built-ins onto that common format: `Transcript>>show:`, `Transcript>>cr`, `Display>>clear`, `Display>>writeString:`, `Display>>newline`, `Bitmap>>width`, `Bitmap>>height`, `Form>>bits`, `Form>>width`, `Form>>height`, and `Class>>instanceKind`. `Display>>defaultForm` remains on the older interpreted/root-backed path for now.
- Updated the seed/image inspector and manifest tests to reflect the new image shape: zero accessor/field-send objects, one interpreted method object, and eleven compiled method objects.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Kernel MVP Source Tree
- Moved the source-built kernel methods out of the Python image builder and into a checked-in `.rz` source tree under `kernel/mvp/`, driven by a small JSON manifest.
- Kept the source layout intentionally narrow for now: one method per `.rz` file, grouped by class directory, so the builder can stay source-owned without adding a multi-method class-file parser yet.
- Updated the builder to load method sources from `kernel/mvp/manifest.json`, compile them with the existing host compiler, and lower them into the existing target `CompiledMethod` objects.
- Added a regression that proves the builder is loading the kernel methods from disk rather than from embedded method-source strings.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Class-Per-File MVP Kernel Sources
- Collapsed the transitional one-method-per-file MVP kernel source layout into one `.rz` file per class under `kernel/mvp/`, which is a better organizational step toward an eventual Smalltalk-style file-in format.
- Switched the class files to a minimal chunk-like layout where methods are separated by `!` lines, and updated the builder to split and compile those per-class method chunks.
- Simplified `kernel/mvp/manifest.json` so it now carries class metadata, one class-file path, and the explicit method-entry list expected from that file, instead of mapping every entry to its own file.
- Added regressions for both manifest-driven class-file loading and `!`-based chunk splitting.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Compiled Display Default Form Method
- Moved `Display>>defaultForm` out of the last active interpreted-method path and into the same source-built compiled-method path already used by the rest of the MVP kernel methods.
- Added `defaultForm` to `kernel/mvp/Display.rz` and the class manifest, relying on the existing `Display defaultForm` lowering rewrite to compile that source into a `pushRoot default_form` method body.
- Updated the target method-entry spec and the fixed seed-layout tests so the boot image now carries zero interpreted method objects and twelve compiled method objects without changing the overall seed size.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Default QEMU Examples Use .rz
- Renamed the default QEMU framebuffer and panic demo source files from `.rec` to `.rz` so the user-facing MVP flow matches the direction already established by the kernel source tree.
- Updated the QEMU Makefile default `EXAMPLE` path and the panic integration test to point at the new `.rz` example files, without changing the build script or VM behavior.
- Left the lowerer extension-agnostic, so existing explicit `EXAMPLE=...` paths continue to work regardless of filename suffix.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Source-Declare Primitive Kernel Methods
- Extended the class-per-file `kernel/mvp` source tree so it now declares the full current built-in method surface, not just the methods that already lower to target compiled methods.
- Added a minimal primitive declaration chunk form using an indented `<primitive>` body line, then used it in `Form.rz`, `BitBlt.rz`, `Glyphs.rz`, `FormFactory.rz`, and `BitmapFactory.rz` while keeping execution behavior unchanged.
- Updated the image builder to parse and validate both compiled method chunks and primitive declaration chunks from the class files, ensuring each manifest entry has the expected source-side implementation kind.
- Added the `kernel/mvp` manifest and `.rz` files to the QEMU image build dependencies so source changes in the kernel tree now rebuild the boot image reliably.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Remove Dead Method-Shape Machinery
- Removed the unused accessor, field-send, root-send, root-value, and interpreted method classes from the generated target seed image now that the live kernel path uses only primitive-backed and compiled method entries.
- Simplified the target VM's method-entry validation and execution paths to the two active cases, which removes dead code without changing the current runtime surface or QEMU behavior.
- Kept the reserved kind ids stable, but shrank the active seed image from `235` objects / `22` classes to `230` objects / `17` classes by dropping the empty compatibility class descriptors.
- Updated the fixed seed-layout tests and image-summary expectations to the smaller boot image.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Self-Describing Class Headers In .rz Files
- Added a minimal chunk-like class header to every `kernel/mvp/*.rz` file using `RecorzKernelClass: #Name instanceVariableNames: '...'`, so class name and instance-variable shape now live with the source methods instead of in parallel manifest metadata.
- Reduced `kernel/mvp/manifest.json` to file-to-entry mapping only; it no longer carries class names or instance-variable lists.
- Taught the image builder to parse and validate the class header chunk before compiling the remaining method chunks, including duplicate-class detection and header-shape validation.
- Kept the runtime and boot image behavior unchanged while moving one more slice of semantic truth from Python tables into the `.rz` kernel files.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Named Primitive Bindings In Kernel Source
- Replaced the generic `<primitive>` marker in the primitive-backed `.rz` method chunks with explicit named bindings such as `<primitive: #formClear>` and `<primitive: #bitbltFillFormColor>`.
- Taught the image builder to parse those primitive binding names and validate them against the expected method-entry wiring, so primitive-backed methods are now self-describing at the source level instead of relying on a generic primitive marker plus Python-side entry tables.
- Extended the lowering tests to assert both the parsed primitive binding names and the updated source text loaded from disk.
- Kept the target VM and boot image unchanged; this step only moved more semantic truth into the source files.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Discover Kernel Sources Without A Manifest
- Removed `kernel/mvp/manifest.json` and changed the MVP image builder to discover all `kernel/mvp/*.rz` class files directly.
- Kept method-entry ordering stable by rebuilding the loaded source table in `METHOD_ENTRY_DEFINITIONS` order after validating each class file's complete selector set against the expected built-in signatures.
- Tightened the source-loading test to assert the full class-file set now comes from the `.rz` tree itself, and updated the QEMU build dependency list to track only the class source files.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Derive Method Metadata From Kernel Sources
- Removed the duplicated Python table of built-in class/selector/arity definitions and now derive that method metadata directly from the `kernel/mvp/*.rz` class files.
- Kept boot-image layout stable by preserving a small explicit method-entry ordering list, while generating each entry name from the source class name and selector and rebuilding the method table from the parsed source tree.
- Tightened the lowering tests to cover source-derived entry names and the richer loaded source metadata, without changing the target VM surface or the current QEMU boot path.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Route Primitive Entries Through Source Binding Ids
- Changed primitive-backed method entries so their seed `implementation` field now carries a source-derived primitive binding id instead of an inert `nil` value.
- Kept compiled methods unchanged, but updated the target VM to validate and dispatch primitive sends through binding ids that come from the `.rz` primitive declarations, rather than routing them indirectly through method-entry ids.
- Extended the host-side inspector and focused tests to validate primitive binding ids in the seed image and reject mismatched primitive-entry wiring.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Generate Primitive Binding Header From Kernel Sources
- Added a generated QEMU target header for primitive binding constants so the checked-in C headers no longer duplicate the primitive-binding enum values by hand.
- Reused the source-derived kernel metadata in `build_qemu_riscv_mvp_image.py` to render that header, added a tiny wrapper tool to emit it, and updated the QEMU Makefile so `vm.c` rebuilds after the generated header changes.
- Removed the hand-maintained primitive-binding enum from `vm.h`, switched `vm.c` to include the generated header directly, and added a focused test for the generated header text.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Generate Method Entry Constants From Kernel Metadata
- Extended the generated QEMU runtime header so it now carries both method-entry constants and primitive-binding constants derived from the source-built kernel metadata.
- Removed the hand-maintained method-entry enum from `vm.h`, renamed the generator script/header to match their broader role, and kept `vm.c` consuming the generated constants directly.
- Updated the focused header-generation test to assert both the source-derived method-entry enum and the primitive-binding enum in one generated runtime header.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Generate Primitive Handler Table From Kernel Metadata
- Extended the generated QEMU runtime header so it now also emits the primitive-binding handler table initializer, derived from the same `.rz` primitive declarations that already define the binding ids.
- Removed the hand-maintained primitive handler slot mapping from `platform/qemu-riscv64/vm.c`, leaving the target runtime responsible only for the handler functions themselves.
- Renamed the one region-copy primitive handler function so its C name now follows the same binding-derived naming convention as the rest of the generated table.
- Updated the focused header-generation test to assert the generated primitive-handler macro alongside the existing method-entry and primitive-binding enums.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Generate Core Runtime Constants Header
- Extended the generated QEMU runtime header so it now also defines the target opcode, global, selector, literal-kind, object-kind, seed-field-kind, and seed-root enums from the host builder metadata.
- Reduced `platform/qemu-riscv64/vm.h` back to limits and data-structure definitions by removing the duplicated checked-in runtime enums and including the generated header instead.
- Updated the QEMU Makefile so every target C object rebuilds after the generated runtime header changes, keeping `program.c`, `seed.c`, and the VM consistently aligned with the same generated constants.
- Expanded the focused header-generation test to assert the new generated runtime enums alongside the existing method-entry and primitive-binding content.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Derive Builder Selector And Global Maps From Shared Specs
- Replaced the parallel `GLOBAL_IDS`/`GLOBAL_VALUES` and `SELECTOR_IDS`/`SELECTOR_VALUES` tables in `build_qemu_riscv_mvp_image.py` with one declarative spec list per domain and a small helper that derives the lookup maps and generated enum definitions.
- Kept the host compiler/lowerer behavior and the generated target header unchanged, but removed another duplicated host-side contract layer from the builder.
- Added a focused regression that asserts the source-of-truth spec lists and the derived global/selector lookup tables used by the lowerer and image builder.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-06 - Derive Builder Seed Metadata From Shared Specs
- Replaced the parallel builder-side seed field kind, object kind, and seed root constant tables with shared declarative spec lists and derived maps, preserving the existing zero-based seed field numbering and one-based object/root numbering.
- Derived `KERNEL_CLASS_NAME_TO_OBJECT_KIND` from the same object-kind spec layer so the source loader, seed builder, and generated target header all read from one host-side definition path.
- Added a focused regression covering the new seed field/object/root spec lists and their derived maps without changing the target VM or seed image layout.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Method Entry Order From Boot Class Order
- Replaced the full hand-kept `METHOD_ENTRY_ORDER` constant-name list in the builder with a smaller `KERNEL_CLASS_BOOT_ORDER` list and now derive method entry ordering from that class order plus the chunk order inside each `.rz` class file.
- Kept the generated method-entry ids and target image layout stable by matching the existing source order, while moving more ordering truth into the kernel source tree itself.
- Added a focused regression that asserts the boot class order and the resulting derived method-entry order seen by the generated header and seed builder.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Seed Bindings From Named Boot Objects
- Reworked `build_seed_manifest()` so the initial boot object graph is assembled with named objects, then derived the global and root binding tables from those names instead of hand-written numeric index lists.
- Removed the most obvious fixed index assumptions in the seed builder, including the default form/framebuffer bitmap links and the transcript fallback glyph reference, while preserving the existing seed layout and object ordering.
- Added a focused regression covering the boot-object binding maps and the helper that derives global/root bindings from named boot objects.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Fixed Boot Object Specs
- Replaced the remaining inline fixed boot-object construction block in `build_seed_manifest()` with declarative boot object specs plus a small field-materialization helper.
- Kept glyph bitmap generation separate, but now derive the fixed pre-glyph and post-glyph boot objects from one named spec layer instead of a long inline sequence of `add_seed_object(...)` calls.
- Added a focused regression covering the declared boot object spec lists and the helper that materializes object and glyph references into seed fields.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Class Descriptor Order And Builder Helper
- Moved the class descriptor ordering in `build_seed_manifest()` onto an explicit `CLASS_DESCRIPTOR_KIND_NAMES` spec and derived the numeric kind order from the shared object-kind metadata.
- Added small builder helpers for the class index map and class seed object materialization, replacing the remaining inline class-order loop while preserving the existing class descriptor layout.
- Added a focused regression covering the declared class descriptor order and the helper that materializes class seed objects and class indices.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Method Seed Assembly Helpers
- Moved the remaining selector, compiled-method, method-entry, and method-descriptor seed-object assembly in `build_seed_manifest()` behind small helpers and explicit derived order lists.
- Added `SELECTOR_VALUE_ORDER` and `COMPILED_METHOD_ENTRY_ORDER` so the seed builder no longer hides those ordering rules inside inline loops, while preserving the current image layout and object indices.
- Added a focused regression covering selector seed object ordering, compiled-method and method-entry materialization, and the first method descriptor layout.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Seed Layout Sections
- Added an explicit `SEED_LAYOUT_SECTION_NAMES` spec plus a `build_seed_layout()` helper so the class, selector, compiled-method, method-entry, and method-descriptor sections are positioned from one declared layout rule instead of chained inline offset math.
- Updated `build_seed_manifest()` to consume the declared layout starts and added internal count checks so each generated section still matches the declared image layout.
- Added a focused regression covering the declared seed layout section order and the derived start/count boundaries for each dynamic section.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Glyph Bitmap Boot Specs
- Moved the remaining inline glyph bitmap family in `build_seed_manifest()` onto a declared `GLYPH_BITMAP_BOOT_SPECS` list generated from explicit glyph bitmap constants.
- Reused the same boot-spec materialization path already used for other seed objects and derived the seed header glyph count from the declared glyph bitmap specs instead of a hardcoded literal.
- Added a focused regression covering the declared glyph bitmap spec family and updated the seed-header test to derive the glyph count from the declared glyph bitmap specs.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Boot Object Families
- Added an explicit `BOOT_OBJECT_FAMILY_SPECS` layout for the fixed pre-glyph objects, glyph bitmaps, and post-glyph objects, plus derived family names and a fixed boot object count from that layout.
- Replaced the three separate boot-object passes in `build_seed_manifest()` with one family-driven materialization loop and added internal checks that the boot object count and collected glyph indices still match the declared family specs.
- Added a focused regression covering the declared boot object family order, the glyph-index-collecting family, and the derived fixed boot object count.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Extract Fixed Boot Seed Object Builder
- Pulled fixed boot-object materialization out of `build_seed_manifest()` into `build_fixed_boot_seed_objects()` plus a reusable `add_named_seed_object()` helper, so the seed builder now consumes an explicit fixed boot graph instead of constructing it inline.
- Kept the boot object family assertions in the extracted helper, preserving the same fixed boot object count and glyph index collection behavior while simplifying `build_seed_manifest()`.
- Added a focused regression covering the extracted fixed boot seed objects, named object indices, and glyph bitmap object range.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.
