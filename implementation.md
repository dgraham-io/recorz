# Implementation Log

## 2026-03-09 - Add RV32-First Comparisons And Boolean Control Flow
- Extended the live selector/runtime surface in `kernel/mvp/Selector.rz`, `platform/qemu-riscv32/runtime_spec.json`, `platform/qemu-riscv64/runtime_spec.json`, `platform/qemu-riscv32/program.c`, `platform/qemu-riscv64/program.c`, and `tools/build_qemu_riscv_mvp_image.py` with `=`, `<`, `>`, `ifTrue:`, `ifFalse:`, `ifTrue:ifFalse:`, plus shared `jump`, `jump_if_true`, and `jump_if_false` opcodes so the image can express real boolean control flow without changing the tiny object model.
- Implemented the new behavior in `platform/qemu-riscv32/vm.c` first and mirrored it in `platform/qemu-riscv64/vm.c`: the workspace evaluator and live method compiler now parse comparison sends and inline conditional blocks, `perform_send` answers real boolean objects for integer comparisons, and the compiled-method/workspace executors branch with explicit jump bytecodes instead of host-side special cases.
- Kept the change Smalltalk-shaped and inspectable by lowering `ifTrue:` / `ifFalse:` blocks directly into the existing tiny instruction stream, validating those branch targets with a control-flow-aware compiled-method validator, and reusing the already-seeded `True` / `False` singletons as the condition values.
- Added the RV32 proof at `examples/qemu_riscv_in_image_conditional_method_demo.rz`, which files in a tiny stateful class, evaluates comparison-based conditions through `Workspace evaluate:`, and prints branch results from the running image.
- Updated `tests/test_qemu_riscv32_serial_integration.py`, `tests/test_qemu_riscv_mvp_lowering.py`, and `tests/test_qemu_riscv_image_inspector.py` for the enlarged selector/opcode surface and re-verified with `python3 -m unittest tests.test_qemu_riscv32_serial_integration.QemuRiscv32SerialIntegrationTests.test_in_image_source_compiler_supports_comparisons_and_conditional_control_flow -v`, `python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_image_inspector -v`, `make -C platform/qemu-riscv32 clean all inspect-image`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-08 - Compact RV32 Runtime Strings
- Reduced the RV32 `dev` runtime string pool budget in `platform/qemu-riscv32/vm.h` from `32768` to `8192` bytes now that runtime-generated strings are compacted instead of accumulating monotonically.
- Added live runtime-string compaction in `platform/qemu-riscv32/vm.c`: when the pool would overflow, the VM now scans heap fields, the execution stack, and the panic send context for string references into the pool, compacts only the still-live strings, and rewrites those references to the new offsets before retrying the allocation.
- Kept the change RV32-focused and behavior-preserving by leaving the string model intact; the optimization only reclaims dead one-shot strings from repeated `KernelInstaller memoryReport` / `fileOutClassNamed:` style operations instead of inventing a new string object kind.
- Extended `tests/test_qemu_riscv32_serial_integration.py` with a QEMU proof that repeatedly stores `KernelInstaller memoryReport` into `Workspace`, then reads it back without tripping `runtime string pool overflow`, which would fail with the old monotonic pool.
- Verified with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv32_dev_loop_integration tests.test_qemu_riscv32_budget_integration -v` and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Set QEMU Framebuffer To 1024x768
- Changed the seeded framebuffer bitmap in `kernel/mvp/Bitmap.rz` from `1280x1024` to `1024x768`, keeping the image-owned display size aligned with the more practical FPGA-oriented target mode we discussed.
- Updated both QEMU display backends in `platform/qemu-riscv64/display.h` and `platform/qemu-riscv32/display.h` so the host-side framebuffer allocation and `ramfb` registration match the seeded bitmap dimensions on both RV64 and RV32 targets.
- Adjusted the RV32 serial proof in `tests/test_qemu_riscv32_serial_integration.py` and the render/lowering/integration assertions across the QEMU test suite to expect the new framebuffer size and the resulting pixel count.
- Tightened the persisted workspace class-list snapshot assertion in `tests/test_qemu_riscv_snapshot_integration.py` so it validates the still-visible rows at the new resolution instead of depending on the previous taller viewport layout.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_render_integration tests.test_qemu_riscv32_render_integration tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv_dev_loop_integration tests.test_qemu_riscv_method_update_integration -v` and `make -C platform/qemu-riscv64 clean screenshot && make -C platform/qemu-riscv32 clean screenshot`.

## 2026-03-08 - Add QEMU RV32 Framebuffer Target
- Brought the new `platform/qemu-riscv32` target up from serial-only bring-up to a real QEMU `virt` framebuffer target by restoring the proven DTB/`fw_cfg`/`ramfb` machine layer and display pipeline on top of the existing RV32 build flags (`rv32im` / `ilp32`).
- Re-enabled the graphical build and run workflow in `platform/qemu-riscv32/Makefile`, including `run`, `run-headless`, `screenshot`, `save-snapshot`, `continue-snapshot`, and `inspect-image`, while keeping the serial transcript echo in `platform/qemu-riscv32/vm.c` so RV32 still has UART diagnostics during framebuffer bring-up.
- Switched `platform/qemu-riscv32/main.c` back to the normal MVP boot flow with external method-update, file-in, and snapshot payload hooks, so the RV32 target now exercises the same target-side image/runtime seams as the RV64 QEMU platform instead of a reduced serial-only path.
- Added RV32-specific framebuffer coverage in `tests/test_qemu_riscv32_render_integration.py`, updated `tests/test_qemu_riscv32_makefile.py` to lock the new `ramfb` build shape, and fixed `tests/test_qemu_riscv32_serial_integration.py` to capture UART output robustly while the graphical target is running.
- Verified with `make -C platform/qemu-riscv32 clean all inspect-image`, `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv32_makefile tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv32_render_integration -v`, and `PYTHONPATH=src python3 -m unittest discover -s tests -v`.

## 2026-03-08 - Add QEMU RV32 Serial Target
- Added a new `platform/qemu-riscv32` target as the first RV32-clean bring-up slice, reusing the current image/VM core but switching the platform build to `rv32im/ilp32` and `qemu-system-riscv32`.
- Kept the target deliberately serial-first by replacing the copied `machine.c` with a minimal UART-only machine layer and a no-op display layer, so this step tests CPU width and ABI portability without mixing in `ramfb`, `fw_cfg`, or LiteX-specific display work.
- Made the RV32 target visibly useful by echoing framebuffer-form transcript writes and newlines to serial in `platform/qemu-riscv32/vm.c`, which lets the existing default demo prove real execution on RV32 without inventing a separate serial Transcript class.
- Added `tests/test_qemu_riscv32_makefile.py` to lock the new build/run shape (`qemu-system-riscv32`, `rv32im`, no `ramfb`, no `fw_cfg`) and `tests/test_qemu_riscv32_serial_integration.py` to verify that the default Recorz demo boots and prints `HELLO FROM RECORZ` plus the existing runtime metadata over QEMU RV32 serial.
- Verified with `make -C platform/qemu-riscv32 clean all inspect-image` and `PYTHONPATH=src python3 -m unittest discover -s tests -v`.

## 2026-03-07 - Add Persistent Workspace Method Browser
- Expanded the source-declared `Workspace` surface in `kernel/mvp/Workspace.rz` with `browseMethodsForClassNamed:` and `browseMethod:ofClassNamed:`, and added the matching selectors in `kernel/mvp/Selector.rz`, so the live image now has a class/method browser state instead of only class, object, and buffer actions.
- Extended `platform/qemu-riscv64/vm.c` with Transcript-style method-list and method-source browsers, plus persisted view-state reopening for `WORKSPACE_VIEW_METHODS` and `WORKSPACE_VIEW_METHOD`, keeping the session model image-owned and snapshot-friendly.
- Reused the new current-buffer model by making the method browser show the persisted workspace source buffer for the selected method, which keeps the session inspectable and avoids a separate non-image method-source cache.
- Added `examples/qemu_riscv_workspace_method_browser_save_demo.rz` and snapshot integration coverage proving that the image can install a method from the current workspace buffer, switch into a method browser view, save, reboot, and reopen directly into that persisted method-source view without a demo-specific reload script.
- Updated the source-derived lowering and image-inspector expectations for the enlarged kernel surface (`objects=319`, `selectors=51`, `methods=47`) and the new workspace browse primitives.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Workspace Current Buffer Actions
- Expanded the source-declared `Workspace` singleton in `kernel/mvp/Workspace.rz` with a real current buffer (`currentSource`) plus `contents`, `setContents:`, `evaluateCurrent`, and `fileInCurrent`, so the image now has a persistent editable session buffer instead of only a remembered rerun string.
- Extended the selector, method-entry, and primitive-binding surface in `kernel/mvp/Selector.rz`, `platform/qemu-riscv64/program.c`, and `platform/qemu-riscv64/vm.c` so top-level programs and the target VM both accept the new workspace session messages.
- Kept the implementation aligned with the current in-image installer path by making `Workspace>>fileInCurrent` reuse the existing class chunk installer for class chunks and the existing method-source installer for method buffers on the currently browsed class.
- Added `examples/qemu_riscv_workspace_buffer_save_demo.rz` plus `tests/test_qemu_riscv_snapshot_integration.py` coverage proving that a saved snapshot can reboot into `Workspace>>evaluateCurrent` and run the persisted buffer without a demo-specific top-level script.
- Updated `tests/test_qemu_riscv_mvp_lowering.py` and `tests/test_qemu_riscv_image_inspector.py` for the enlarged source-derived kernel surface and the resulting image growth (`objects=313`, `selectors=49`, `methods=45`).
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_image_inspector tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_evaluate_persisted_workspace_buffer_without_demo_specific_program -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Persisted Workspace Source Session Rerun
- Added `examples/qemu_riscv_workspace_session_save_demo.rz`, which proves that a live `Workspace` session can remember source via `evaluate:`, configure itself as the persisted startup hook, and save that state into a snapshot without any host-side method update payload.
- Extended `tests/test_qemu_riscv_snapshot_integration.py` with a QEMU end-to-end case that saves the session-bearing image, reboots from that snapshot with the idle workspace demo, and verifies the restored framebuffer output comes from `Workspace>>rerun` rather than a demo-specific top-level script.
- Fixed `platform/qemu-riscv64/program.c` so the top-level program manifest accepts the expanded selector range up through `#rerun`; without that fix, the new save demo failed during program validation before the runtime could persist or replay the workspace session.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_rerun_workspace_source_session_without_demo_specific_program -v`, `PYTHONPATH=src python3 -m unittest discover -s tests -v`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Workspace Source Session Primitives
- Expanded the source-declared `Workspace` singleton in `kernel/mvp/Workspace.rz` to remember `lastSource` and expose `evaluate:` plus `rerun`, so the live image now has a minimal persistent session surface in addition to the existing browser state.
- Added the matching source-owned selectors in `kernel/mvp/Selector.rz`, which grows the kernel selector and method-entry surface without introducing any new builder-owned semantic tables.
- Extended `platform/qemu-riscv64/vm.c` with a narrow in-image do-it compiler that targets the existing unified executable engine: it supports simple statement streams of global sends with string, small-integer, `nil`, and global operands, then executes them through `execute_executable()` instead of adding a separate evaluator path.
- Kept the new surface inspectable and image-first by storing the remembered source string directly in the `Workspace` object, so snapshots persist the session source automatically through the existing heap serialization path.
- Updated the source-derived lowering and image-inspector expectations in `tests/test_qemu_riscv_mvp_lowering.py` and `tests/test_qemu_riscv_image_inspector.py` for the expanded `Workspace` layout and image growth (`objects=301`, `selectors=45`, `methods=41`).
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_image_inspector -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Workspace Named Object Browser Mode
- Expanded the source-declared `Workspace` singleton in `kernel/mvp/Workspace.rz` with `browseObjectNamed:` and the matching source-owned selector in `kernel/mvp/Selector.rz`, so the live browser surface now covers class lists, individual classes, and remembered named objects through one persisted workspace object.
- Extended `platform/qemu-riscv64/vm.c` so `Workspace>>reopen` now restores object-browser state as well as class and class-list views, and added a Transcript-style object browser that renders object name, class, slot count, and named dynamic instance variables with their current values.
- Kept the object browser image-first by reusing the existing named-object registry and live class metadata instead of inventing a second persistence path or a separate browser model.
- Added `examples/qemu_riscv_workspace_object_browser_save_demo.rz` plus `tests/test_qemu_riscv_snapshot_integration.py` coverage proving that a saved snapshot can reboot directly into a persisted object browser view for a live named instance with state.
- Updated `tests/test_qemu_riscv_mvp_lowering.py` and `tests/test_qemu_riscv_image_inspector.py` for the expanded source-declared browser surface and the resulting image summary (`objects=295`, `selectors=43`, `methods=39`).
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_image_inspector -v`, `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_reopen_workspace_object_browser_state_without_demo_specific_program -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `PYTHONPATH=src python3 -m unittest discover -s tests -v`.

## 2026-03-07 - Add Workspace Class List Browser Mode
- Expanded `kernel/mvp/Workspace.rz` so the source-declared `Workspace` singleton now carries persistent browser state (`currentViewKind currentTargetName`) and exposes `browseClasses` alongside the existing class-browser and file-in primitives.
- Extended the source-declared selector and method-entry surface for the new browser mode in `kernel/mvp/Selector.rz`, then updated the lowering and image-inspector expectations in `tests/test_qemu_riscv_mvp_lowering.py` and `tests/test_qemu_riscv_image_inspector.py` for the resulting image growth (`objects=292`, `selectors=42`, `methods=38`).
- Updated `platform/qemu-riscv64/vm.c` so `Workspace` now remembers either a class-list or class-browser view, restores that view in `Workspace>>reopen`, and renders class names in uppercase for the current framebuffer glyph set.
- Added `examples/qemu_riscv_workspace_class_list_save_demo.rz` plus `tests/test_qemu_riscv_snapshot_integration.py` coverage proving that a saved snapshot can reboot directly into the persisted class-list browser state without a demo-specific top-level script.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_image_inspector -v`, `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_reopen_workspace_class_list_state_without_demo_specific_program -v`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Larger QEMU Framebuffer
- Increased the seeded framebuffer form and target display defaults from `640x480` to `1280x1024` by updating the kernel boot object declaration in `kernel/mvp/Bitmap.rz` and the target display constants in `platform/qemu-riscv64/display.h`.
- Updated the QEMU screenshot-oriented integration tests and lowering expectations so they assert the larger framebuffer dimensions while keeping the existing rendering behavior and boot flow unchanged.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_render_integration tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv_dev_loop_integration tests.test_qemu_riscv_method_update_integration -v` and `make -C platform/qemu-riscv64 clean screenshot`.

## 2026-03-07 - Persisted Startup Behavior Demo
- Added a startup-behavior snapshot proof with `qemu_riscv_startup_hook_save_demo.rz` and `qemu_riscv_startup_hook_idle_demo.rz`: the first configures and saves a startup hook, and the second is a no-op top-level program so the rebooted image’s own startup behavior is solely responsible for the visible framebuffer output.
- Added a QEMU snapshot integration case that saves the configured image, reboots from that snapshot with the no-op program, and verifies the saved startup output renders without any scenario-specific host script.
- Fixed two runtime seams exposed by the new proof: the program loader now accepts the extended selector range, and the compiled-method validator no longer incorrectly rejects nested sends whose arity exceeds the method’s own argument count.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_boot_into_persisted_startup_behavior_without_demo_specific_program -v` and `PYTHONPATH=src python3 -m unittest discover -s tests -v`.

## 2026-03-07 - Persisted Startup Hook State
- Added a persisted startup hook to the QEMU MVP runtime so the image can remember one receiver object plus selector name across snapshot save/reload and invoke that hook automatically on boot after snapshot load, method updates, and external file-in.
- Added `KernelInstaller>>configureStartup:selectorNamed:` and `KernelInstaller>>clearStartup` in the kernel source tree, extended the selector universe, and wired the new primitive-backed installer entries into the target VM and snapshot format.
- Updated the focused lowering and image-inspector tests for the enlarged selector/method-entry surface and verified the runtime step with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_image_inspector -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Image-First Dev Loop Targets
- Added fixed image-first boot/save workspace examples plus `dev-init`, `dev-boot`, `dev-screenshot`, and `dev-file-in` Makefile targets so the normal workflow can stay on one persisted image instead of swapping scenario-specific demo scripts.
- Added a QEMU integration test that initializes a dev snapshot, boots it with the fixed image-first example, files in an external `.rz` update with the same dev workflow, and reboots the same snapshot again to confirm the filed-in behavior persists without changing the example path.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_dev_loop_integration -v`, `PYTHONPATH=src python3 -m unittest discover -s tests -v`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Multi-File External File-In Streams
- Generalized the external file-in path so the VM can consume a stream of multiple `RecorzKernelClass:` chunk groups in one boot-time payload instead of only a single class chunk blob.
- Extended the QEMU Makefile with `FILE_IN_PAYLOADS`, which concatenates multiple host `.rz` files into one generated payload and feeds that stream through the existing `opt/recorz-file-in` transport.
- Added focused Makefile coverage to lock the generated combined payload path and the resulting `fw_cfg` wiring.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_makefile -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Snapshot-First External File-In Workflow
- Added external `.rz` file-in demos that boot from a saved snapshot, apply a host-provided class chunk over `fw_cfg`, save the evolved image, and then reboot from that persisted result.
- Added a QEMU snapshot integration case that proves the full workflow end to end: save base image, baseline reload, continue from snapshot with `FILE_IN_PAYLOAD`, save again, and reload the evolved snapshot with the filed-in behavior still present.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_snapshot_integration -v`, `PYTHONPATH=src python3 -m unittest discover -s tests -v`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - External File-In Transport For Snapshot Boot
- Added a new `FILE_IN_PAYLOAD` QEMU `fw_cfg` transport so the target can receive an external `.rz` class chunk file at boot without going through the older method-update payload path.
- Extended the target boot path to read that payload in `main.c`, pass it into the VM, and apply it through the existing class-chunk installer before the top-level program runs.
- Added focused Makefile coverage to lock both the new `opt/recorz-file-in` `fw_cfg` wiring and the existing safe same-path `continue-snapshot` replacement flow.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_makefile -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Persisted Image Development Loop
- Added second-generation snapshot evolution demos so a saved live image can be resumed, mutated in-image, saved again, and then rebooted with the evolved behavior and state intact.
- Added a new QEMU snapshot integration case that saves a snapshot, continues from that same snapshot with `continue-snapshot`, replaces a live class method, saves the next generation back to the same path, and then reboots from the evolved snapshot.
- Verified the persisted-image loop with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_snapshot_integration -v`, `PYTHONPATH=src python3 -m unittest discover -s tests -v`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Safe Snapshot Continuation Workflow
- Updated the QEMU Makefile snapshot save path so it always extracts into a temporary file and only replaces the final snapshot path after extraction succeeds, which makes same-path snapshot continuation safe.
- Added a dedicated `continue-snapshot` target that requires `SNAPSHOT_PAYLOAD`, boots from that snapshot, and writes the next generation back to the same path for a simpler persisted-image development loop.
- Added a focused Makefile regression that checks the dry-run output uses a temporary snapshot path and does not delete the input snapshot before the replacement move.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_makefile -v`.

## 2026-03-07 - Existing-Class In-Image Class File-In
- Added `KernelInstaller>>fileInClassChunks:` so the running image can accept a small `RecorzKernelClass:` chunk stream, resolve an existing class from the class header, and file its method chunks in through the existing live install path.
- Added the new selector and primitive-backed entry to the MVP kernel source tree, extended the target VM with class-header parsing and existing-class lookup, and updated the host-side image inspector and structural tests for the enlarged selector/method-entry set.
- Added an end-to-end demo and QEMU integration test proving that a `Display` class chunk can be filed in from inside the image without an external update payload and immediately change framebuffer-visible behavior.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_method_update_integration -v`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Dynamic Class Creation From In-Image Class File-In
- Extended `KernelInstaller>>fileInClassChunks:` so it resolves an existing class or creates a new live `Class` object on demand when the class chunk names a missing class.
- Added a tiny dynamic class registry in the target VM so newly created classes persist for later lookups and file-ins, and emit a serial log when a new class is created.
- Added an end-to-end QEMU demo and integration test proving that a `RecorzKernelClass: #Helper` chunk can create a missing class, install a method onto it, and continue rendering without any host-side update payload.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_method_update_integration -v`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

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

## 2026-03-07 - Extract Dynamic Seed Section Builder
- Pulled class, selector, compiled-method, method-entry, and method-descriptor section assembly out of `build_seed_manifest()` into `build_dynamic_seed_sections()` and a small `DynamicSeedSections` carrier.
- Kept the same seed layout and count assertions inside the extracted helper, so `build_seed_manifest()` now mostly orchestrates fixed boot objects, dynamic sections, and final bindings.
- Added a focused regression covering the extracted dynamic seed sections, including fixed-object class links, dynamic section counts, and the first class descriptor layout.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Extract Seed Binding And Encoding Stages
- Pulled global/root binding derivation into `build_seed_bindings()` and final byte encoding into `encode_seed_manifest()`, leaving `build_seed_manifest()` as a simple orchestration over fixed boot objects, dynamic sections, bindings, and manifest encoding.
- Added a small `SeedBindings` carrier so the final manifest stage takes an explicit binding payload rather than separate parallel lists.
- Added a focused regression covering the extracted binding builder and manifest encoder, including the first global/root bindings and the encoded seed header tuple.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Boot Object Export Maps From Specs
- Added `global_exports` and `root_exports` to `BootObjectSpec` so the fixed boot object declarations now own their exported global/root binding semantics.
- Replaced the parallel `GLOBAL_NAME_TO_BOOT_OBJECT_NAME` and `SEED_ROOT_NAME_TO_BOOT_OBJECT_NAME` tables with `build_boot_object_export_map()`, derived directly from the declared boot object family specs.
- Extended the focused binding-map regression to assert the boot object export declarations that now drive the derived global/root maps.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Kernel Class Orders From Specs
- Added a small `KernelClassSpec` layer so the builder now declares kernel class descriptor order and source boot order in one place instead of keeping separate standalone class-order lists.
- Derived `KERNEL_CLASS_BOOT_ORDER`, `CLASS_DESCRIPTOR_KIND_NAMES`, and `KERNEL_CLASS_NAME_TO_OBJECT_KIND` from those declared class specs without changing the generated seed image or target VM behavior.
- Extended the focused lowering regressions to lock both the new class spec layer and the derived boot-order / descriptor-order lists.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Boot Object Family Views From Family Specs
- Inlined the fixed pre-glyph, glyph, and post-glyph object declarations directly into `BOOT_OBJECT_FAMILY_SPECS` so the family layout itself is now the source of truth for the fixed boot graph.
- Added `BOOT_OBJECT_FAMILY_SPECS_BY_NAME` and derived the existing before/after/glyph family views from that map instead of keeping separate top-level boot-object lists in parallel.
- Extended the focused lowering regressions to lock both the named family map and the derived family views without changing the generated seed image or target VM behavior.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Fixed Boot Payload Fields From Named Specs
- Replaced the remaining raw inline field tuples for the fixed text/framebuffer boot objects with named payload value specs and a small `build_small_integer_boot_field_specs()` helper.
- Derived the transcript layout, style, framebuffer bitmap, transcript metrics, and glyph bitmap field tuples from those named payload specs, while keeping object refs and glyph refs explicit where they matter semantically.
- Extended the focused lowering regressions to lock the new named payload specs and the helper-driven field materialization without changing the generated seed image or target VM behavior.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Flat Boot Object Views From Family Specs
- Added a derived flat ordered boot-object view and a derived boot-object name map so the fixed boot graph can now be addressed either by family or by object identity from the same declared family specs.
- Changed the boot-object export-map builder and fixed boot object count to use that flat ordered view instead of separate nested family-only traversal and count arithmetic.
- Extended the focused lowering regressions to lock the new flat ordered view, the name map, and the unchanged family-derived aliases without changing the generated seed image or target VM behavior.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Dynamic Seed Layout From Section Specs
- Added a small `SeedLayoutSectionSpec` layer so the builder now declares dynamic seed section order and count-source metadata in one place instead of keeping a standalone section-name list plus inline count wiring.
- Updated `build_seed_layout()` to derive section counts from the named count sources declared in those section specs without changing the generated seed layout or target VM behavior.
- Extended the focused lowering regressions to lock both the new section-spec layer and the unchanged derived section layout.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Dynamic Object Section Sequence From Specs
- Added a small `DynamicSeedObjectSectionSpec` layer so the builder now declares the object-bearing dynamic section sequence in one place instead of repeating the same section names in validation and final manifest assembly.
- Updated `build_dynamic_seed_sections()` and `build_seed_manifest()` to use that section sequence for dynamic section count validation and final seed-object extension order without changing the generated seed image or target VM behavior.
- Extended the focused lowering regressions to lock the new dynamic object section spec list and the helper-driven validation/assembly path.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Dynamic Builder Step Order From Specs
- Added a small `DynamicSeedBuildStepSpec` layer plus a `DynamicSeedBuildState` carrier so the dynamic seed builder now declares its dependency-ordered helper sequence instead of hand-wiring the same helper calls inline.
- Updated `build_dynamic_seed_sections()` to run selector, compiled-method, method-entry, method-descriptor, and class seed object builders through that explicit step sequence while keeping the generated seed image and target VM behavior unchanged.
- Extended the focused lowering regressions to lock the new dynamic build-step spec list alongside the existing dynamic section specs.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Replace String-Based Dynamic Builder Dispatch
- Replaced the string-based `globals()` dispatch inside the dynamic seed build-step specs with direct callable metadata, so the staged builder sequence is now explicit without another name-lookup indirection layer.
- Kept the same dependency order and result attributes, but moved `DynamicSeedBuildStepSpec` onto direct builder callables and extended the focused regressions to lock both the callable sequence and the builder names.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Key Dynamic Seed Sections By Layout Name
- Removed the remaining parallel `result_attribute` strings from the dynamic seed section specs and step specs, so both the build sequence and the final section assembly now key directly off declared layout section names.
- Collapsed `DynamicSeedSections` onto a section-keyed object map with a small `seed_objects_for_layout_section()` helper, while keeping convenience accessors for the existing class/selector/method section tests.
- Extended the focused lowering regressions to lock the new section-keyed structure, the build-step section order, and the unchanged seed manifest assembly path.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Dynamic Seed Build Dependencies
- Added explicit `required_layout_sections` metadata to the dynamic seed build-step specs so the staged builder now declares which prior sections each step depends on instead of relying on sequence alone.
- Added validation for both the declared build-step coverage and the runtime build order, catching duplicate, missing, or out-of-order dynamic seed section dependencies before the seed manifest is assembled.
- Extended the focused lowering regressions to lock the new dependency tuples and the validation path without changing the generated seed image or target VM behavior.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Centralize Dynamic Seed State Updates
- Added `DynamicSeedBuildStepResult` plus per-step `state_update_fields` metadata so the dynamic seed builder now applies shared-state updates through one explicit result path instead of mutating `DynamicSeedBuildState` ad hoc inside each step.
- Kept the same dynamic section order and dependency contract, but moved selector, compiled-method, method-entry, and method-descriptor state propagation behind `apply_dynamic_seed_build_step_result()` and made the class section assert that it does not unexpectedly rewrite the class index map.
- Extended the focused lowering regressions to lock the declared state-update fields alongside the existing section order and dependency metadata.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Dynamic Seed State Dependencies
- Added explicit `required_state_fields` metadata and an `INITIAL_DYNAMIC_SEED_STATE_FIELDS` declaration so the dynamic seed builder now states which shared state each step consumes, not just which sections it depends on and which fields it produces.
- Extended both build-step validation and runtime execution checks to reject missing, invalid, or out-of-order required state fields before any dynamic seed section is assembled.
- Extended the focused lowering regressions to lock the required-state-field tuples and the declared initial dynamic seed state alongside the existing section and state-update contracts.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Unify Dynamic Seed Section Specs
- Added one `DynamicSeedSectionSpec` layer as the source of truth for dynamic seed layout count sources, build order, section dependencies, and state contracts, instead of keeping separate hand-authored layout/build/object spec lists in parallel.
- Derived `SEED_LAYOUT_SECTION_SPECS`, `DYNAMIC_SEED_OBJECT_SECTION_SPECS`, and `DYNAMIC_SEED_BUILD_STEP_SPECS` from that unified section spec layer, including a duplicate-build-order check in the builder helper.
- Extended the focused lowering regressions to lock the unified dynamic section spec list and the unchanged derived layout/build/object views.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Introduce Fixed Boot Graph Spec
- Added `FixedBootGraphSpec` as the top-level source of truth for the fixed boot graph so family order, flattened boot object order, and export maps are now derived from one declared fixed graph instead of only from adjacent helper lists.
- Switched the fixed boot seed builder to materialize directly from `FIXED_BOOT_GRAPH_SPEC.family_specs` while preserving the existing derived `BOOT_OBJECT_FAMILY_*` and `BOOT_OBJECT_SPECS_*` views for readability and stable test coverage.
- Extended the focused lowering regressions to lock the new fixed boot graph source-of-truth alongside the unchanged derived family views and boot object order.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Introduce Top-Level Boot Image Spec
- Added `BootImageSpec` as a top-level declaration over the fixed boot graph and dynamic seed section specs, so the builder now has one explicit boot-model object spanning both halves of the seed image.
- Switched the derived dynamic section views and fixed boot materialization path to consume `BOOT_IMAGE_SPEC`, while preserving the existing `FIXED_BOOT_GRAPH_SPEC`, `DYNAMIC_SEED_SECTION_SPECS`, and helper-derived views for stability.
- Extended the focused lowering regressions to lock the new `BOOT_IMAGE_SPEC` and the fact that the existing fixed/dynamic spec views remain derived from it.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Fixed Boot Views From Boot Image Spec
- Moved the fixed boot family/order/export derived views to initialize from `BOOT_IMAGE_SPEC.fixed_boot_graph_spec` instead of directly from the standalone fixed graph constant, so the top-level boot image spec now owns more of the active bootstrap surface.
- Kept `FIXED_BOOT_GRAPH_SPEC` as the stable underlying declaration, but made the boot object family maps, flattened object order, fixed count, and export maps flow through the top-level boot image model.
- Extended the focused lowering regressions to lock that those fixed boot views are now derived from `BOOT_IMAGE_SPEC.fixed_boot_graph_spec` while keeping the generated seed image unchanged.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Thread Seed Assembly Through Boot Image Context
- Added `BootImageSeedBuildContext` so the seed-manifest assembly path has one boot-image-shaped context carrying the fixed object count, glyph bitmap specs, dynamic section/build-step views, and export maps it needs.
- Threaded that context through fixed boot materialization, dynamic seed section validation/build, binding generation, seed encoding, and final seed-manifest assembly so those helpers stop reaching into adjacent globals directly.
- Extended the focused lowering regressions to lock the new `BOOT_IMAGE_SEED_BUILD_CONTEXT` and its derived dynamic section/build-step views while keeping the generated seed image unchanged.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Move Seed Layout Inputs Into Build Context
- Added `class_kind_order` and `seed_layout_section_specs` to `BootImageSeedBuildContext`, so the remaining seed layout inputs now travel with the boot-image-shaped context instead of being read from adjacent globals during dynamic section assembly.
- Updated `build_seed_layout()` and `build_dynamic_seed_sections()` to consume those context-owned values while preserving the existing derived `CLASS_DESCRIPTOR_KIND_ORDER` and `SEED_LAYOUT_SECTION_SPECS` views for compatibility and focused tests.
- Extended the focused lowering regressions to lock the new context-owned class order and seed layout spec alongside the existing boot image context coverage.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Move Dynamic Ordering Inputs Into Build Context
- Added `selector_value_order`, `compiled_method_entry_order`, and `method_entry_order` to `BootImageSeedBuildContext` and to the initial dynamic seed build state, so dynamic section construction stops reaching into adjacent ordering globals during selector, compiled-method, and method-entry assembly.
- Updated the dynamic build-step contracts to declare those ordering values as required initial state for the relevant sections, and switched `build_dynamic_seed_sections()` to pass the context-owned ordering inputs into both seed layout computation and per-section materialization.
- Extended the focused lowering regressions to lock the new context-owned ordering tuples and the stronger dynamic build-step dependency declarations alongside the existing boot image context coverage.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Move Ordering Into Boot Image Spec
- Added `BootImageOrderingSpec` and made `BootImageSpec` own the class, selector, compiled-method, and method-entry ordering tuples instead of treating them as adjacent standalone policy inputs.
- Updated the boot image seed build context to derive its ordering fields from `boot_image_spec.ordering_spec`, so the context builder no longer takes those ordering inputs separately.
- Extended the focused lowering regressions to lock the new boot-image-owned ordering declaration and the fact that the active build context now reads those tuples from `BOOT_IMAGE_SPEC`.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Seed Build Context Views From Boot Image Spec
- Simplified `build_boot_image_seed_build_context()` so it now derives its seed layout section specs, dynamic build-step views, glyph bitmap family, fixed boot object count, and boot object export maps directly from `BootImageSpec` instead of taking those as separate adjacent arguments.
- Left the existing derived globals in place for readability and compatibility, but made the active seed build context consume the same data through one boot-image-owned derivation path.
- Extended the focused lowering regressions to lock the context-owned fixed count, glyph specs, and export maps alongside the existing fixed/dynamic view coverage.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Move Dynamic State Contract Into Boot Context
- Added `initial_dynamic_seed_state_fields` to `BootImageSeedBuildContext` and derived it from the boot image model plus the dynamic build state shape, so the dynamic section validator and builder no longer rely on a standalone neighboring global for their initial state contract.
- Updated the dynamic seed validation and execution paths to consume that context-owned initial state field set, while keeping `INITIAL_DYNAMIC_SEED_STATE_FIELDS` as a derived compatibility alias for focused tests and readability.
- Extended the focused lowering regressions to lock the fact that the active initial dynamic state contract now lives on `BOOT_IMAGE_SEED_BUILD_CONTEXT`.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Move Source Class Boot Order Into .rz Headers
- Extended the `RecorzKernelClass:` header syntax so kernel source classes now declare `descriptorOrder:` and `sourceBootOrder:` in their `.rz` files instead of keeping source boot order only in Python-side class specs.
- Updated the builder to parse those header fields into `KernelClassHeader`, derive the active source class boot order from parsed headers, and keep the full class descriptor name list as a separate explicit compatibility list for now.
- Extended the focused lowering regressions to lock the richer header parsing and the fact that the active source class boot order now comes from parsed `.rz` headers.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Support Classes In Kernel Source Tree
- Added header-only `.rz` class declarations for the remaining support/kernel-internal classes (`TextLayout`, `TextStyle`, `TextMetrics`, `TextBehavior`, `MethodDescriptor`, `MethodEntry`, `Selector`, and `CompiledMethod`) so the full kernel class universe now lives under `kernel/mvp/`.
- Replaced the Python-side full class list with parsed class headers, derived `CLASS_DESCRIPTOR_KIND_NAMES` from `descriptorOrder`, and continued deriving the source boot subset from `sourceBootOrder`, making the class descriptor order and source class set both source-owned.
- Updated the method-source loader to accept header-only support classes in the kernel source tree and extended the focused lowering regressions to lock the full descriptor-order header set.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Source Boot Object Declarations
- Added `RecorzKernelBootObject:` chunk syntax to `.rz` files and extended the builder to parse source-owned boot object declarations, including family/order metadata, object/glyph/small-integer field specs, and global/root exports.
- Declared the fixed singleton/root objects alongside their owning classes in `kernel/mvp/*.rz`, while keeping the live fixed boot graph builder unchanged in this step so the new declaration path could land independently.
- Updated the method-source loader to skip boot object declaration chunks and extended the focused lowering regressions to lock parsed boot object declarations sourced from `.rz` files.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Fixed Boot Objects From Source Declarations
- Replaced the Python-authored before/after fixed boot object spec tuples with a source-derived path: `FIXED_BOOT_GRAPH_SPEC` now materializes those singleton/root families from parsed `RecorzKernelBootObject:` declarations while leaving the glyph bitmap family on the existing generated path for now.
- Added explicit builder validation for per-family contiguous `order:` ranges and extended the focused lowering regressions to lock that the active before/after boot object specs are derived from `KERNEL_BOOT_OBJECT_DECLARATIONS_BY_NAME`.
- Kept the generated image shape unchanged while moving the live fixed singleton/root object graph and export semantics onto kernel-owned declarations.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Move Glyph Family Metadata Into Kernel Source
- Added `RecorzKernelGlyphBitmapFamily:` declaration syntax and declared the glyph bitmap family in [kernel/mvp/Bitmap.rz](/Users/david/repos/recorz/kernel/mvp/Bitmap.rz), so the glyph name prefix, class, dimensions, storage kind, and count are now kernel-owned metadata.
- Updated the builder to parse that glyph family declaration, derive the `GLYPH_BITMAP_*` compatibility views from it, and materialize the glyph bitmap boot family in `FIXED_BOOT_GRAPH_SPEC` from the parsed source declaration instead of a Python-authored family spec.
- Extended the method-source loader to skip glyph family declaration chunks and added focused lowering regressions for the parsed glyph family declaration and the source-derived glyph metadata.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Global Specs From Boot Object Exports
- Removed the hand-kept `GLOBAL_SPECS` list from the builder and now derive the active global constant table from the source-declared fixed boot graph's `globalExports:` metadata.
- Added a small `kernel_global_constant_name()` helper plus `build_global_specs_from_boot_object_exports()` so the generated global ids and enum definitions flow from the same source-owned boot object declarations that already define the fixed singleton/root objects.
- Extended the focused lowering regressions to lock both the expected global table contents and the fact that the live `GLOBAL_SPECS` list is derived from `FIXED_BOOT_GRAPH_SPEC`.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Target Selectors In Kernel Source
- Added `RecorzKernelSelector:` declaration syntax and declared the full MVP target selector universe in [kernel/mvp/Selector.rz](/Users/david/repos/recorz/kernel/mvp/Selector.rz), including selectors used only by the do-it interpreter such as arithmetic and `printString`.
- Updated the builder to parse and validate source-declared selector order, derive the active `SELECTOR_SPECS` table and generated selector constants from those declarations, and skip selector declaration chunks when loading kernel methods.
- Added validation that every source-defined kernel method selector is declared in the selector source file and extended the focused lowering regressions to lock parsed selector declarations and the source-derived selector map.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Declare Seed Roots In Kernel Source
- Added `RecorzKernelRoot:` declaration syntax and declared the full seed-root order in the owning `.rz` class files, so root ids are no longer authored by the builder-side `SEED_ROOT_SPECS` list.
- Updated the builder to parse and validate source-declared roots, derive the active `SEED_ROOT_SPECS` table and root constant ids from those declarations, and skip root declaration chunks during kernel method loading.
- Switched the active root-object binding map to derive from root declarations rather than from the boot-object export helper alone, while still validating that each declared root points at a boot object that exports it.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Object Kinds From Class Headers
- Extended `RecorzKernelClass:` headers with `objectKindOrder:` and declared the live runtime object-kind order in the kernel source files themselves, so the builder no longer owns the active object-kind list.
- Replaced the hand-kept `OBJECT_KIND_SPECS` table with a source-derived path built from parsed class headers, and dropped the dead historical method-shape kinds from the live runtime object-kind universe.
- Updated the target VM's object-kind name switch and the focused lowering/inspector expectations to match the smaller source-declared object-kind set while keeping the boot image behavior unchanged.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Field Indices From Class Headers
- Replaced the live `Class`, `MethodDescriptor`, and `MethodEntry` field index constants in the builder with a source-derived path based on parsed `instanceVariableNames:` from `RecorzKernelClass:` headers.
- Added a small `kernel_instance_variable_index()` helper so builder-side seed/class assembly now follows the same source-owned slot order already declared in the kernel `.rz` files.
- Extended the focused lowering regressions to lock the new source-derived field indices for the support/runtime classes that the seed builder actively materializes.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Materialize Dynamic Seed Objects By Field Name
- Replaced the live positional field-list assembly for `Class`, `Selector`, `MethodEntry`, `MethodDescriptor`, and `CompiledMethod` seed objects with a named-field materializer that follows the source-declared `instanceVariableNames:` order.
- Added `materialize_named_seed_fields()` so the dynamic seed builder now describes those runtime objects in terms of field names rather than builder-authored slot positions.
- Extended the focused lowering regressions to lock the new named-field materialization behavior, including sparse compiled-method word materialization with explicit `nil` padding for skipped slots.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Derive Fixed Boot Payload Constants From Source Declarations
- Removed the remaining hand-authored fixed boot payload compatibility constants for transcript layout/style/metrics, the framebuffer bitmap, the default form, and transcript behavior from the builder.
- Added small helpers that derive boot object field specs, small-integer payload tuples, and glyph references directly from parsed `RecorzKernelBootObject:` declarations, then rebuilt the compatibility constants from those source-owned declarations.
- Also switched the live `BITMAP_STORAGE_FRAMEBUFFER`, `BITMAP_STORAGE_GLYPH_MONO`, and `GLYPH_FALLBACK_CODE` compatibility values to derive from the source-declared framebuffer/glyph/behavior metadata instead of hard-coded Python integers.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Remove Dead Historical Method-Shape Constants
- Removed the remaining dead builder-side constants and tables for historical accessor/field-send/root-send/root-value/interpreted method shapes, which are no longer part of the active MVP image path.
- Simplified the host-side image inspector so method-entry validation now recognizes only the two live implementation cases: primitive-backed entries and compiled-method entries.
- Kept the boot image summary unchanged, with the dead method-shape object counts still reporting as zero through compatibility paths while removing stale semantic truth from the builder and inspector.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Externalize Runtime Format Constants
- Added `/Users/david/repos/recorz/platform/qemu-riscv64/runtime_spec.json` as the repo-owned source of truth for the live MVP program/image/seed format constants, do-it opcodes, literal kinds, seed field kinds, and compiled-method opcode set.
- Updated the builder to load that runtime spec and derive the active opcode/literal/format/seed compatibility constants from it instead of authoring those values directly in Python.
- Extended the focused lowering regressions to lock that the live builder constants now derive from `runtime_spec.json`, not from builder-side Python literals.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Move Wire-Format Constants Into Generated Target Header
- Extended the generated runtime header so it now exports the shared program/image/seed magic bytes, versions, profile bytes, section ids, feature bits, and format-derived structure sizes.
- Removed the duplicated wire-format `#define`s from `program.c`, `seed.c`, and `image.c`, so those target loaders now consume the same generated host/target contract instead of maintaining their own copies.
- Extended the focused header-generation regression to lock the newly generated wire-format defines.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Move VM Runtime Layout Constants Into Generated Header
- Extended `runtime_spec.json` and the generated runtime header so the target now gets shared compiled-method opcode ids, method implementation ids, the seed invalid-object marker, source-derived field index macros, and shared bitmap storage ids from one generated contract.
- Removed the last duplicated numeric runtime/layout truth from `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv64/vm.h` by switching the VM to those generated constants, which also brought the `TextLayout` (`left top scale lineSpacing`) and `TextStyle` (`foregroundColor backgroundColor`) slot usage back into sync with the kernel source declarations.
- Added `runtime_spec.json` as an explicit dependency for generated-image/generated-header rebuilds and extended the focused lowering regression to lock the new generated VM/runtime constants.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Route Target Dispatch Through Image-Owned Method Entries
- Removed the target-side generated `method_entry_specs` table from the active VM path and switched `platform/qemu-riscv64/vm.c` to validate and dispatch sends directly from the seeded `MethodDescriptor` and `MethodEntry` objects.
- The target now decides between primitive-backed and compiled-backed methods from the `MethodEntry` implementation field itself, while still validating selector/arity/primitive-kind through the image-owned method descriptors and compiled method objects.
- Stopped emitting the dead `RECORZ_MVP_GENERATED_METHOD_ENTRY_SPECS` macro from the generated runtime header and extended the focused lowering regression to lock that removal.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Unify Do-It And Method Execution Paths
- Extended the shared MVP instruction set so the compiled-method opcode family now covers both the seeded kernel-method subset and the current do-it subset (`pushLiteral`, `pushLexical`, `storeLexical`, `dup`, `pop`, `pushRoot`, `pushArgument`, `pushField`, `returnTop`, and `returnReceiver` all share one numeric execution model).
- Replaced the separate top-level do-it interpreter in `platform/qemu-riscv64/vm.c` with one `execute_executable()` activation engine that now runs both top-level programs and seeded compiled methods, using the same local stack/lexical model and the same send/return behavior.
- Kept the program section wrapper intact for now, but made it feed that same activation engine instead of a bespoke bytecode loop, which removes the remaining runtime split between do-it execution and kernel method execution.
- Updated the runtime spec/header generation and focused lowering regressions to lock the unified opcode numbering and the absence of the old split execution path.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Method Update Payload Builder
- Added a shared `method_update` wire format to `platform/qemu-riscv64/runtime_spec.json` for the smallest live-install path: replacing one existing compiled method without rebuilding the whole boot image.
- Extended `tools/build_qemu_riscv_mvp_image.py` and the generated runtime header so both host and target can share that payload contract, including magic, version, header size, and the `fw_cfg` file name used to transport updates into QEMU.
- Added `tools/build_qemu_riscv_method_update.py` plus focused lowering tests so a single method chunk can now be compiled into a validated method-update payload, while rejecting primitive-backed updates for this first narrow slice.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Tiny In-Image Method Install Path
- Added an optional `fw_cfg` file-read seam to `platform/qemu-riscv64/machine.c` and threaded an optional method-update payload through `platform/qemu-riscv64/main.c` into the VM, using the shared `method_update` wire format.
- Extended `platform/qemu-riscv64/vm.c` so, after seed bootstrap and before running the top-level do-it, it can locate an existing compiled method by class kind and selector, allocate a replacement `CompiledMethod` object in the live heap, validate it, and swap the owning `MethodEntry` to the new implementation.
- Added optional `UPDATE_PAYLOAD=...` support to `platform/qemu-riscv64/Makefile`, a small live-update demo do-it plus a replacement `Transcript>>cr` method chunk under `examples/`, and a QEMU framebuffer integration test that proves behavior changes without rebuilding the embedded boot image.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Grow Live Class Method Tables
- Extended the live method installer in `platform/qemu-riscv64/vm.c` so method-update payloads can now either replace an existing method entry or append a new `MethodDescriptor` and `MethodEntry` to a class at runtime by cloning the current method table range to a new heap block and retargeting the class's `methodStart` and `methodCount`.
- Relaxed the runtime send path so dynamically installed method entries can use VM-assigned execution ids outside the original seeded boot-image range, while keeping seeded boot-image validation unchanged.
- Added a new QEMU demo and update chunk under `examples/` that installs a previously missing `Display>>cr` method over `fw_cfg`, plus an integration test that proves the demo panics without the update and renders successfully once the new selector is appended to `Display`'s live method table.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add In-Image Installer Surface
- Added a new source-declared `KernelInstaller` class/global under `kernel/mvp/KernelInstaller.rz`, two installer selectors in `kernel/mvp/Selector.rz`, and the matching primitive-backed method entries so the running image now owns a tiny object-level install surface instead of relying only on external `fw_cfg` payloads.
- Extended the target VM in `platform/qemu-riscv64/vm.c` so `KernelInstaller` can allocate a live `CompiledMethod` object from up to four instruction words and install it onto a live class by selector id and argument count, reusing the same append-capable method-table growth path already added for host-driven updates.
- Added `examples/qemu_riscv_in_image_install_demo.rz` plus a QEMU integration test that proves the running image can create and install `Display>>cr` for itself and render successfully without any host `UPDATE_PAYLOAD`, then updated the host inspector/runtime-lowering tests for the new class/global/selector/method counts.
- Fixed the target seed/global array range in `platform/qemu-riscv64/vm.h` so the new `KernelInstaller` global no longer relied on undefined behavior.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add In-Image Source Method Installer
- Extended `kernel/mvp/KernelInstaller.rz` and `kernel/mvp/Selector.rz` with `installMethodSource:onClass:` so the running image can install a tiny source-defined method without rebuilding the boot image and without supplying raw compiled-method words.
- Added a narrow target-side method-source compiler in `platform/qemu-riscv64/vm.c` that accepts one Smalltalk-shaped method chunk in the current MVP subset, lowers it to the existing compiled-method instruction format, allocates a live `CompiledMethod`, and reuses the append/replace installer path already used by the raw-word installer.
- Added `examples/qemu_riscv_in_image_source_install_demo.rz` plus an end-to-end QEMU integration test that proves the running image can compile and install `Display>>cr` from source text and immediately render the changed behavior.
- Updated `platform/qemu-riscv64/program.c`, `platform/qemu-riscv64/vm.c`, `tools/inspect_qemu_riscv_mvp_image.py`, and the focused image/lowering tests so the expanded selector/method-entry universe for the new installer remains consistent across host and target, including the target-side selector-range validation paths.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Tiny In-Image Chunk File-In Path
- Extended `kernel/mvp/KernelInstaller.rz` and `kernel/mvp/Selector.rz` with `fileInMethodChunks:onClass:` so the running image can consume a small `!`-separated chunk stream for one class, rather than only installing one isolated method source string at a time.
- Added chunk splitting and multi-method installation to `platform/qemu-riscv64/vm.c` on top of the existing tiny source compiler: the new path skips a leading `RecorzKernelClass:` chunk, compiles each remaining method chunk in the current MVP subset, and reuses the same append/replace installer path already used by the raw-word and single-method source installers.
- Added `examples/qemu_riscv_in_image_chunk_file_in_demo.rz` plus a QEMU integration test that proves the running image can file in two `Display` methods in one chunk stream and immediately use both of them to render three visible lines without any host update payload.
- Updated `platform/qemu-riscv64/program.c`, `platform/qemu-riscv64/vm.c`, `tools/inspect_qemu_riscv_mvp_image.py`, and the focused image/lowering tests so the expanded selector/method-entry/primitive counts for the chunk installer remain consistent across host and target.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Generic Object Instances And Class>>new
- Added a new source-declared `Object` class under `kernel/mvp/Object.rz`, extended `kernel/mvp/Class.rz` with `new`, and declared the new selector in `kernel/mvp/Selector.rz` so the MVP kernel can represent ordinary instance objects instead of only the fixed built-in roots and service objects.
- Extended `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv64/program.c` so the target now knows the new `Object` kind, caches the `Object` class descriptor for dynamic class creation, and executes `Class>>new` as a primitive that allocates a generic heap object and stamps it with the receiver's class link.
- Increased the target heap headroom in `platform/qemu-riscv64/vm.h` so the newer live-install, class-file-in, and generic-instance paths can coexist without exhausting the object heap during QEMU runs.
- Added `examples/qemu_riscv_in_image_instance_demo.rz` plus integration coverage in `tests/test_qemu_riscv_method_update_integration.py`, and updated the host inspector/lowering tests for the expanded class, selector, method-entry, primitive, and seed-object counts.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 EXAMPLE=/Users/david/repos/recorz/examples/qemu_riscv_in_image_instance_demo.rz screenshot`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Named Class Lookup For Live Class File-In
- Extended `kernel/mvp/KernelInstaller.rz` and `kernel/mvp/Selector.rz` with `classNamed:` so the running image can resolve a class later by name after it has been filed in, instead of only using the class object returned directly from `fileInClassChunks:`.
- Added the matching primitive-backed runtime path in `platform/qemu-riscv64/vm.c` and updated `platform/qemu-riscv64/program.c` so the target accepts the expanded selector universe and `KernelInstaller classNamed:` resolves both seeded classes and dynamically created live classes through the existing class-name registry.
- Added `examples/qemu_riscv_in_image_named_class_instance_demo.rz` plus integration coverage in `tests/test_qemu_riscv_method_update_integration.py` proving the running image can file in `Helper`, resolve it later by name, create an instance with `new`, and execute its installed instance method to affect framebuffer output.
- Updated `tools/inspect_qemu_riscv_mvp_image.py`, `tests/test_qemu_riscv_mvp_lowering.py`, and `tests/test_qemu_riscv_image_inspector.py` for the new selector, method-entry, primitive, and seed-object counts (`objects=254`, `selectors=24`, `method_entries=29`, `method_descriptors=29`).
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 EXAMPLE=/Users/david/repos/recorz/examples/qemu_riscv_in_image_named_class_instance_demo.rz screenshot`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Persisted Workspace Browser Startup
- Added `examples/qemu_riscv_workspace_startup_save_demo.rz` and `examples/qemu_riscv_workspace_startup_idle_demo.rz`, which use the new `Workspace` singleton to file in a tiny live class, browse it, persist `Workspace>>reopen` as the startup hook, save a snapshot, and boot back into that browser state without a demo-specific top-level script.
- Updated `platform/qemu-riscv64/vm.c` so the workspace browser uppercases displayed class names and superclass names before drawing them, which keeps the Transcript-style browser readable on the current uppercase-only framebuffer glyph set.
- Fixed `platform/qemu-riscv64/vm.c` so the seeded `Workspace` singleton uses the generated `RECORZ_MVP_WORKSPACE_FIELD_CURRENT_CLASS_NAME` field index instead of the dynamic-class ivar lookup path, which previously caused `Workspace currentClassName field is not declared` at runtime.
- Fixed `platform/qemu-riscv64/seed.c` so seed global binding validation and required-global checks include the new `Workspace` global instead of stopping at `KernelInstaller`.
- Added end-to-end coverage in `tests/test_qemu_riscv_snapshot_integration.py` proving that a snapshot can boot directly into the persisted workspace/browser state.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_reopen_workspace_browser_state_without_demo_specific_program -v`, `PYTHONPATH=src python3 -m unittest discover -s tests -v`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Live Class Header Metadata
- Extended the target-side class-chunk parser in `platform/qemu-riscv64/vm.c` from a bare `RecorzKernelClass: #Name` reader to a real live class definition parser that understands `superclass:` and `instanceVariableNames:` for in-image class file-in, while also tolerating the numeric kernel header fields already used by source-owned kernel class files.
- Replaced the dynamic class registry’s parallel name/handle arrays with stored live class definitions, so dynamically created classes now retain their declared superclass handle and named instance variables inside the running image instead of being treated as anonymous empty `Object` subclasses.
- Updated `Class>>new` in `platform/qemu-riscv64/vm.c` to size freshly allocated generic `Object` instances from that live class metadata, which is the first runtime step toward real stateful dynamically filed-in classes.
- Switched the existing in-image class creation and instance demos under `examples/` to use explicit `superclass: #Object instanceVariableNames: 'value'` headers so the richer live class definition path is exercised by the normal QEMU integration suite.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Live Instance Field Bytecodes
- Extended the shared runtime contract in `platform/qemu-riscv64/runtime_spec.json` with a new `store_field` opcode for both the top-level program stream and seeded/live compiled methods, keeping field mutation on the same unified execution model rather than inventing a target-only special case.
- Threaded that new opcode through the target runtime in `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv64/program.c`: compiled-method validation, program validation, opcode naming, and the unified `execute_executable()` activation engine now all understand instance field writes alongside the existing field reads.
- Widened the tiny in-image source compiler in `platform/qemu-riscv64/vm.c` so live class methods can now resolve instance-variable names from the class metadata added in the previous step, compile `field := argument.` assignments to `storeField`, and return `^field` or `^argument` through the same compiled-method path already used for other live-installed methods.
- Updated `tests/test_qemu_riscv_mvp_lowering.py` to lock the generated runtime header for the new shared opcode values and reverified the normal QEMU/image path with the expanded execution contract.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Workspace Singleton And Transcript Browser Primitives
- Added `kernel/mvp/Workspace.rz` as a source-declared boot class and singleton global with `currentClassName` state plus the first three workspace/browser messages: `fileIn:`, `browseClassNamed:`, and `reopen`.
- Extended the source-declared selector universe in `kernel/mvp/Selector.rz` with `fileIn:`, `browseClassNamed:`, and `reopen`, which keeps the new workspace surface aligned with the same image-owned selector objects and method metadata as the rest of the MVP kernel.
- Extended the target runtime in `platform/qemu-riscv64/vm.c`, `platform/qemu-riscv64/program.c`, and `platform/qemu-riscv64/vm.h` so the VM now understands the larger global/object/selector ranges, exposes the `Workspace` global to source, and routes the new workspace primitives through a tiny Transcript-style browser that clears the default form and prints class name, superclass, slot count, and local method count.
- Updated `tools/inspect_qemu_riscv_mvp_image.py`, `tests/test_qemu_riscv_mvp_lowering.py`, and `tests/test_qemu_riscv_image_inspector.py` for the enlarged boot image summary (`objects=289`, `classes=20`, `selectors=41`, `methods=37`, `globals=8`) now that the image seeds `Workspace` alongside the existing kernel singletons.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_image_inspector -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Add Stateful Live Class Demo Path
- Extended the source-declared selector universe in `kernel/mvp/Selector.rz` with `value` and `setValue:` so the live class demo can use normal selector objects instead of falling back to a one-off runtime escape hatch, and updated the target/runtime selector-range checks in `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv64/program.c` to follow the expanded source-owned selector set.
- Changed `tools/build_qemu_riscv_mvp_image.py` to materialize selector seed objects for the full declared selector universe rather than only the selectors referenced by the seeded built-in method table, which keeps live-installed methods aligned with the source-owned runtime symbol contract.
- Added `examples/qemu_riscv_in_image_stateful_class_demo.rz` plus integration coverage in `tests/test_qemu_riscv_method_update_integration.py` proving the running image can file in `Helper` with `instanceVariableNames: 'value'`, allocate an instance, run a compiled setter that stores a string into that named field, run a compiled getter that reads it back, and pass the result into `Transcript show:` to render on the framebuffer.
- Updated `tools/inspect_qemu_riscv_mvp_image.py`, `tests/test_qemu_riscv_mvp_lowering.py`, and `tests/test_qemu_riscv_image_inspector.py` for the enlarged seeded selector table and the new image summary (`objects=261`, `selector_objects=31`) now that the boot image carries the full source-declared selector universe.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-07 - Improve Framebuffer Demo Readability
- Changed the seeded transcript palette in `kernel/mvp/TextStyle.rz` from green-on-light-gray to a darker slate foreground on a warm light background, so the default text output reads clearly without changing the object/message boundary or text rendering model.
- Updated `examples/qemu_riscv_fb_demo.rz` to clear the default form through `Form>>clear` instead of explicitly painting the whole framebuffer black, which keeps the default demo aligned with the seeded transcript style instead of fighting it.
- Matched the pre-VM fallback framebuffer clear color in `platform/qemu-riscv64/display.c` to the same warm background so boot and cleared-form output stay visually consistent.
- Updated the QEMU render and method-update integration tests in `tests/test_qemu_riscv_render_integration.py`, `tests/test_qemu_riscv_method_update_integration.py`, and `tests/test_qemu_riscv_mvp_lowering.py` to lock the new seeded palette and the lighter default demo output.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image screenshot`.

## 2026-03-07 - Rebuild QEMU Image When EXAMPLE Changes
- Fixed `platform/qemu-riscv64/Makefile` so changing `EXAMPLE=...` always takes effect even when reusing the same `BUILD_DIR` and not running `clean`; the generated embedded image is now rebuilt on each QEMU image build invocation instead of relying on stale timestamp comparisons against whichever example happened to be built last.
- Switched the default `EXAMPLE` assignment in the QEMU Makefile to `?=` so command-line example selection remains explicit and conventional.
- Added `tests/test_qemu_riscv_makefile.py` to lock the regression: it runs `make inspect-image` twice against the same build directory, first with the framebuffer demo and then with the stateful-class demo, and asserts that the generated image checksum and program manifest change without a clean build in between.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 EXAMPLE=/Users/david/repos/recorz/examples/qemu_riscv_in_image_stateful_class_demo.rz inspect-image`.

## 2026-03-07 - Add Superclass-Aware Method Lookup
- Extended `platform/qemu-riscv64/vm.c` so method lookup now walks the `Class>>superclass` chain instead of only scanning the receiver class’s local method range, which is the first real runtime step toward Smalltalk-shaped inheritance for live classes.
- Added `examples/qemu_riscv_in_image_inherited_method_demo.rz`, which files in `Parent`, files in `Child superclass: #Parent`, instantiates `Child`, and proves that a method defined only on `Parent` is still understood by the `Child` instance.
- Added integration coverage in `tests/test_qemu_riscv_method_update_integration.py` to lock that inherited-method path at the QEMU framebuffer level.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 EXAMPLE=/Users/david/repos/recorz/examples/qemu_riscv_in_image_inherited_method_demo.rz screenshot`.

## 2026-03-07 - Add Inherited Instance Layout For Live Classes
- Extended `platform/qemu-riscv64/vm.c` so live instance sizing now counts inherited instance variables across the superclass chain, instead of only looking at the current class’s local ivars.
- Changed live field-name resolution in the tiny in-image compiler to resolve both inherited and local instance variables, while also offsetting child-local slots after inherited slots so parent and child methods can agree on the same object layout.
- Added validation for live class headers so dynamic classes cannot silently rename, resize, or duplicate inherited instance-variable names, which keeps the inherited layout stable enough for later image persistence work.
- Added `kernel/mvp/Selector.rz` entries for `detail` and `setDetail:`, plus `examples/qemu_riscv_in_image_inherited_state_demo.rz` and integration coverage in `tests/test_qemu_riscv_method_update_integration.py` proving that a `Child` instance can hold both inherited `value` state from `Parent` and its own local `detail` state at the same time.
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 EXAMPLE=/Users/david/repos/recorz/examples/qemu_riscv_in_image_inherited_state_demo.rz screenshot`.

## 2026-03-07 - Add Persistent Live Snapshot Save And Reload
- Extended the source-declared live kernel surface in `kernel/mvp/Selector.rz` and `kernel/mvp/KernelInstaller.rz` with `rememberObject:named:`, `objectNamed:`, and `saveSnapshot`, so persistence is exposed as ordinary image-level messages instead of a host-only escape hatch.
- Added a full live snapshot path in `platform/qemu-riscv64/vm.c`: the VM can now serialize the current heap, globals, roots, dynamic class registry, named object registry, mono bitmap pool, cursor state, and live string data into a standalone snapshot blob, dump it over serial in a host-extractable format, and later reload that blob as the running image state before executing the top-level do-it.
- Added host-side persistence plumbing with `tools/extract_qemu_riscv_snapshot.py`, `SNAPSHOT_PAYLOAD`/`save-snapshot` support in `platform/qemu-riscv64/Makefile`, and boot-time snapshot loading in `platform/qemu-riscv64/main.c`, so the QEMU target can save a live image artifact and boot back into it without rebuilding the kernel image.
- Added `examples/qemu_riscv_snapshot_save_demo.rz` and `examples/qemu_riscv_snapshot_reload_demo.rz`, plus integration coverage in `tests/test_qemu_riscv_snapshot_integration.py`, proving that a dynamically filed-in subclass instance with inherited and local state can be named, saved, rebooted, resolved from the live registry, and rendered again after reload.
- Updated the seeded selector/method image expectations in `tests/test_qemu_riscv_mvp_lowering.py` and `tests/test_qemu_riscv_image_inspector.py` for the enlarged `KernelInstaller` surface (`objects=272`, `selector_objects=36`, `method_entries=32`).
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-08 - Add Metaclasses And Class-Side Methods
- Extended the source-declared selector and workspace surface in `kernel/mvp/Selector.rz` and `kernel/mvp/Workspace.rz` with `browseClassMethodsForClassNamed:` and `browseClassMethod:ofClassNamed:`, so class-side browsing is expressed through the same kernel-owned selector set as the rest of the live environment.
- Added explicit live metaclass support in both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c`: dynamic class creation now materializes a dedicated metaclass per live class, class-side chunk file-in routes methods onto that metaclass, sends to class objects resolve through metaclass lookup, and the same paths persist cleanly through snapshots.
- Extended the in-image file-in/compiler flow to recognize `RecorzKernelClassSide:` chunks, so a single class chunk stream can install class-side behavior without introducing a separate ad hoc update path. `Workspace fileInCurrent` now routes either instance-side or class-side class chunks through that same live installer.
- Added Transcript-based class-side browser support to the `Workspace` runtime in both QEMU targets, including persistent reopen behavior for class-side method lists and method source views after snapshot reload.
- Added end-to-end proof programs: `examples/qemu_riscv_in_image_class_side_demo.rz`, `examples/qemu_riscv_class_side_snapshot_save_demo.rz`, `examples/qemu_riscv_class_side_snapshot_reload_demo.rz`, and `examples/qemu_riscv_workspace_class_side_browser_save_demo.rz`, plus snapshot coverage in `tests/test_qemu_riscv_snapshot_integration.py`.
- Updated `tests/test_qemu_riscv_mvp_lowering.py` and `tests/test_qemu_riscv_image_inspector.py` for the enlarged seeded method/selector surface (`objects=325`, `selector_objects=53`, `method_entries=49`).
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add Live Class File-Out Source Export
- Extended the source-declared kernel surface in `kernel/mvp/Selector.rz`, `kernel/mvp/KernelInstaller.rz`, and `kernel/mvp/Workspace.rz` with `fileOutClassNamed:`, keeping class export on the same image-owned selector/method model as live file-in and workspace browsing.
- Added live method-source retention and export support in both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c`: methods installed from source are now remembered alongside their class/selector/arity, raw compiled-method replacement forgets any stale remembered source, and `KernelInstaller>>fileOutClassNamed:` can reconstruct a chunk stream for dynamic live classes with both instance-side and class-side methods.
- Persisted that live source registry through snapshots by extending the VM snapshot format and loader in both QEMU targets, so a saved image can keep a workspace export buffer and later re-file it into the running image after reboot.
- Added `Workspace>>fileOutClassNamed:` as a Transcript/workspace-facing primitive that loads the exported class chunk stream into the current workspace buffer, which lets the existing `Workspace fileInCurrent` path round-trip a live class without any host-side code generation step.
- Added proof demos: `examples/qemu_riscv_in_image_class_file_out_demo.rz`, `examples/qemu_riscv_class_file_out_snapshot_save_demo.rz`, and `examples/qemu_riscv_class_file_out_snapshot_reload_demo.rz`. These prove that a live class can be filed out into the workspace buffer, re-filed into the running image, persisted in a snapshot, and re-filed again after reload.
- Added end-to-end coverage in `tests/test_qemu_riscv_method_update_integration.py` and `tests/test_qemu_riscv_snapshot_integration.py`, and updated `tests/test_qemu_riscv_mvp_lowering.py` and `tests/test_qemu_riscv_image_inspector.py` for the enlarged seed surface (`objects=330`, `selector_objects=54`, `method_entries=51`).
- Verified with `PYTHONPATH=src python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add RV32 Dev-Loop Makefile Parity
- Added `dev-init`, `dev-boot`, `dev-screenshot`, and `dev-file-in` to `platform/qemu-riscv32/Makefile`, matching the existing RV64 image-first workflow so RV32 can iterate on one persisted snapshot instead of only running cold-boot examples.
- Reused the same `DEV_SNAPSHOT`, `DEV_BOOT_EXAMPLE`, and `DEV_SAVE_EXAMPLE` shape as the RV64 target, so the host-side transport stays uniform while RV32 becomes the first-class development path.
- Added `tests/test_qemu_riscv32_makefile.py` coverage for the new RV32 snapshot/dev targets, including temporary-output safety for `continue-snapshot` and proof that `dev-file-in` routes through the snapshot continuation flow with the RV32 target.
- Verified with `PYTHONPATH=src python3 -m unittest tests.test_qemu_riscv32_makefile -v` and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add RV32 Snapshot And Live-Image Parity Coverage
- Added RV32-specific live-image integration coverage in `tests/test_qemu_riscv32_dev_loop_integration.py` and `tests/test_qemu_riscv32_snapshot_integration.py`, so the same persisted-image workflow already proven on RV64 is now exercised directly on the RV32 target: snapshot save/reload, snapshot-first file-in, workspace/browser reopen, class-side persistence, and class file-out round-trip.
- Hardened `tools/extract_qemu_riscv_snapshot.py` for the actual RV32 QEMU serial log shape by accepting both CRLF line endings and prefixed `recorz-snapshot-begin` markers, then locked those cases in `tests/test_qemu_riscv_snapshot_extractor.py`.
- Changed `platform/qemu-riscv32/Makefile` to write screenshot and snapshot serial output directly to `qemu.log` with `-serial file:...` and to reuse the longer snapshot extraction timeout in the RV32 save/continue flows, removing host stdout buffering from the persisted-image path.
- Updated `tests/test_qemu_riscv32_makefile.py` to match the timeout-aware RV32 snapshot extraction command.
- Verified with `PYTHONPATH=src:. python3 -m unittest discover -s tests -v` and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Reduce RV32 QEMU Memory Budget To 32MB
- Added `QEMU_MEMORY ?= 32M` to `platform/qemu-riscv32/Makefile` and switched all RV32 run, screenshot, save-snapshot, and continue-snapshot flows to use that shared budget instead of a hardcoded `128M`, so the RV32 target now runs against a tighter memory profile closer to the eventual hardware constraints.
- Updated `tests/test_qemu_riscv32_makefile.py` to lock the new `-m 32M` runtime invocation in the RV32 Makefile surface.
- Verified that the reduced budget still supports the full RV32 live-image workflow with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv32_makefile tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv32_render_integration tests.test_qemu_riscv32_dev_loop_integration tests.test_qemu_riscv32_snapshot_integration -v` and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add Explicit RV32 Runtime Budget Profile
- Moved the main RV32 runtime capacities into `platform/qemu-riscv32/vm.h` as explicit target-budget macros: heap, mono bitmap pool, dynamic classes, named objects, live method sources, runtime string pool, snapshot string pool, and snapshot buffer.
- Switched `platform/qemu-riscv32/vm.c` to consume those profile macros instead of carrying duplicate local numeric limits, which makes the target budget surface explicit and easier to reason about before tightening it further.
- Verified with `make -C platform/qemu-riscv32 clean all inspect-image` and `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv32_makefile tests.test_qemu_riscv32_serial_integration -v`.

## 2026-03-08 - Add RV32 Memory Report And Budget Accounting
- Extended the source-declared kernel surface in `kernel/mvp/Selector.rz` and `kernel/mvp/KernelInstaller.rz` with `memoryReport`, keeping runtime budget inspection on the same image-owned selector/primitive model as the rest of the live system.
- Added `KernelInstaller>>memoryReport` in both `platform/qemu-riscv32/vm.c` and `platform/qemu-riscv64/vm.c`. It now reports live heap usage, dynamic-class count, named-object count, live method source count, runtime string usage, snapshot string usage, snapshot size, and mono bitmap usage against the current target limits.
- Refactored the snapshot encoder in both QEMU targets to reuse one `current_snapshot_string_byte_count()` helper, so the memory report and snapshot save path are measuring the same live string payload instead of duplicating that accounting logic.
- Added `examples/qemu_riscv_memory_report_demo.rz` and RV32 serial coverage in `tests/test_qemu_riscv32_serial_integration.py`, proving that the report is emitted through the normal `Transcript` path and that the current RV32 system stays within the explicit `32MB`-oriented runtime budgets.
- Updated the shared generated-image/header expectations in `tests/test_qemu_riscv_mvp_lowering.py` and `tests/test_qemu_riscv_image_inspector.py` for the enlarged kernel surface (`objects=333`, `selector_objects=55`, `method_entries=52`), and widened selector validation in `platform/qemu-riscv32/program.c`, `platform/qemu-riscv64/program.c`, `platform/qemu-riscv32/vm.c`, and `platform/qemu-riscv64/vm.c` so the new selector is accepted everywhere the boot image and runtime lookup need it.
- Verified with `PYTHONPATH=src:. python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv32 clean all inspect-image`, and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-08 - Trim RV32 Runtime Budgets To Measured Headroom
- Reduced the explicit RV32 profile in `platform/qemu-riscv32/vm.h` from the earlier broad safety margins to a tighter target-oriented budget: heap `384`, mono bitmaps `8`, dynamic classes `8`, named objects `8`, live method sources `16`, runtime string pool `8192`, and snapshot buffer `24576`, while keeping the snapshot string section at `8192` so larger workspace/file-out buffers can still persist cleanly.
- Updated `tests/test_qemu_riscv32_serial_integration.py` so the `KernelInstaller memoryReport` check now locks those tighter RV32 limits at runtime instead of just checking the older overprovisioned profile.
- Re-verified the existing RV32 live-image workflow against the trimmed profile with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv32_makefile tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv32_dev_loop_integration -v` and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add RV32 Budget Overflow Integration Coverage
- Added `tests/test_qemu_riscv32_budget_integration.py`, which drives the RV32 QEMU target with generated `.rz` sources and locks the explicit overflow paths for the trimmed RV32 budget profile: dynamic class registry overflow, named object registry overflow, object heap overflow, and snapshot buffer overflow.
- Each new integration case asserts the concrete panic text and send selector reported by the target VM, so the RV32 memory limits now fail in targeted, inspectable ways instead of only being inferred from the generic runtime panic path.
- Re-verified the RV32 budget surface with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv32_budget_integration tests.test_qemu_riscv32_makefile tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv32_dev_loop_integration -v` and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Split RV32 Into Dev And Target Profiles
- Added explicit `dev` and `target` RV32 runtime profiles in `platform/qemu-riscv32/vm.h`. The `dev` profile is now the default QEMU path and uses roomier live-runtime budgets (`heap 768`, `dynamic classes 24`, `named objects 24`, `live method sources 64`, `runtime strings 32768`, `snapshot strings 16384`, `snapshot buffer 65536`, `mono bitmaps 16`), while the existing tighter limits remain available as the `target` profile for future hardware-oriented validation.
- Updated `platform/qemu-riscv32/Makefile` so `RV32_PROFILE ?= dev` selects the profile at compile time, bakes the profile into the default build directory (`misc/qemu-riscv32-dev-mvp` / `misc/qemu-riscv32-target-mvp`), and prevents stale object reuse when switching between development and constrained-target builds.
- Extended the RV32 `KernelInstaller memoryReport` output to include the active profile name, and updated the RV32 test surface so the normal dev workflow expects the larger `dev` budgets while the explicit overflow integration tests build with `RV32_PROFILE=target`.
- Verified with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv32_makefile tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv32_budget_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv32_dev_loop_integration -v`, `make -C platform/qemu-riscv32 clean all inspect-image`, and `make -C platform/qemu-riscv32 RV32_PROFILE=target clean all inspect-image`.

## 2026-03-08 - Reuse RV32 Source I/O Scratch Buffer
- Reduced RV32 static runtime storage in `platform/qemu-riscv32/vm.c` by collapsing the separate `class_file_out_buffer` and `file_in_source_buffer` allocations into one shared `kernel_source_io_buffer`. Those paths are never live at the same time, so this removes roughly 8KB of duplicate BSS without changing the live image/file-in/file-out behavior.
- Kept the optimization local to the RV32 target so the current memory-trimming work stays focused on the first real architecture, while preserving the existing semantics of `KernelInstaller fileIn:`, external file-in payloads, and `fileOutClassNamed:`.
- Re-verified the affected RV32 paths with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv32_dev_loop_integration -v` and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Pack RV32 Live Method Source Storage
- Reworked the RV32 live method source registry in `platform/qemu-riscv32/vm.c` and `platform/qemu-riscv32/vm.h` so each entry now stores compact metadata (`class_handle`, selector, arity, offset, length) and uses a dedicated packed source-text pool instead of reserving a full `512`-byte source buffer in every record.
- Added `RECORZ_MVP_LIVE_METHOD_SOURCE_POOL_LIMIT` to the RV32 dev/target profiles and surfaced that usage in `KernelInstaller memoryReport` as `MSRP`, so the dev profile now keeps live method source bytes inspectable and bounded without burning a full `LIVE_METHOD_SOURCE_LIMIT * METHOD_SOURCE_CHUNK_LIMIT` block of static memory.
- Updated the RV32 snapshot format and loader to persist the packed live method source pool and per-entry offsets/lengths, then re-verified that external file-in, live class file-out, class-side persistence, workspace reopen, and the target-profile overflow checks still work with the new representation.
- Verified with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv32_budget_integration tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv32_dev_loop_integration -v` and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Reopen Live Workspace Source Views From The Image
- Extended the shared workspace/browser runtime in `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` so `Workspace fileOutClassNamed:` now becomes a real `CLASS SOURCE` view: it remembers that view kind, renders the filed-out class source immediately, and can reopen that source browser after snapshot boot through the existing startup-hook path.
- Taught `Workspace browseMethod:ofClassNamed:` and `Workspace browseClassMethod:ofClassNamed:` to pull the current source buffer from the live method-source registry before rendering, so workspace method browsers now show live installed source from the image instead of depending on whatever text happened to remain in the buffer.
- Added the new workspace class-source snapshot coverage to `tests/test_qemu_riscv_snapshot_integration.py` and `tests/test_qemu_riscv32_snapshot_integration.py`, and updated the browser save demos in `examples/qemu_riscv_workspace_method_browser_save_demo.rz`, `examples/qemu_riscv_workspace_class_side_browser_save_demo.rz`, and `examples/qemu_riscv_class_file_out_snapshot_save_demo.rz` so they rely on live source rehydration and persistent source-view reopen behavior.
- Hardened the workspace renderer on both QEMU targets so long source lines are clipped with `...` instead of panicking, which keeps class file-out browsing usable for longer class headers and method chunks on both RV64 and RV32.
- Updated `tests/test_qemu_riscv32_render_integration.py` to follow the RV32 `dev` profile build directory introduced by the profile split.
- Verified with `PYTHONPATH=src:. python3 -m unittest discover -s tests -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Canonicalize Live Class File-Out Chunk Streams
- Tightened `fileOutClassNamed:` on both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` so live class source export now emits a deterministic chunk stream: the class header is explicitly bang-terminated, instance-side and class-side method chunks are bang-terminated too, and method chunks are emitted in a stable selector/arity order instead of depending on live installation order.
- Kept the change strictly on the live source export side, so the existing in-image file-in path and snapshot/workspace round-trips continue to consume the same chunk grammar while the exported source becomes a more canonical Smalltalk-style stream for later file-in/file-out evolution.
- Re-verified the affected round-trip surface with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv_method_update_integration tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv_mvp_lowering -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add Explicit Do-It Chunks To Live File-In Streams
- Extended the shared live file-in path in `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` so chunk streams can now contain `RecorzKernelDoIt:` sections alongside class and class-side chunks. Those do-it sections are parsed, their executable body is extracted, and they run through the existing workspace evaluator inside the image instead of requiring a separate top-level demo flow.
- Updated `Workspace fileInCurrent` to route `RecorzKernelDoIt:` buffers through the same chunk-stream loader, which keeps class chunks, class-side chunks, and do-it chunks on one file-in protocol surface and moves the live source workflow closer to a fuller Smalltalk-style chunk model.
- Added `examples/qemu_riscv_in_image_chunk_doit_file_in_demo.rz` as a concrete proof that a class chunk and a do-it chunk can live in one source buffer and execute in one pass, and added integration coverage in `tests/test_qemu_riscv_method_update_integration.py`.
- Verified with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv_method_update_integration tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv32_snapshot_integration -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Use Canonical Chunk Buffers In Workspace Method Browsers
- Changed `Workspace browseMethod:ofClassNamed:` and `Workspace browseClassMethod:ofClassNamed:` in both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` so they now synthesize canonical chunk-stream buffers instead of loading only the raw method body into the current source. Instance-side method views now carry a minimal `RecorzKernelClass:` header plus the method chunk, and class-side views carry a `RecorzKernelClass:` header, a `RecorzKernelClassSide:` section, and the selected method chunk.
- Kept the method-source export path compatible with both live and seeded classes by generating a safe minimal class header for seeded kernel classes like `Display`, while still preserving full instance-variable metadata for live classes. That keeps the normal workspace browse/edit/refile loop on the same chunk grammar as class file-out, without losing the ability to inspect kernel methods already in the boot image.
- Updated `tests/test_qemu_riscv_snapshot_integration.py` to treat method-browser source as a larger canonical source region rather than hard-coding the older short method-body line layout, and re-verified the workspace/browser snapshot path on both RV64 and RV32.
- Verified with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_reopen_workspace_method_browser_state_without_demo_specific_program tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_reopen_workspace_class_side_method_browser_state_without_demo_specific_program tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_browser_state_without_demo_specific_program -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Normalize Workspace Do-Its To Explicit Chunk Source
- Changed `Workspace evaluate:`, `Workspace evaluateCurrent`, and `Workspace rerun` in both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` so the workspace now remembers do-it source as explicit `RecorzKernelDoIt:` chunks instead of raw ad hoc strings. The stored source is normalized once, and execution then evaluates the extracted do-it body through the existing workspace evaluator.
- Kept the user-facing workflow compatible with the current demos by accepting raw source on input and normalizing it behind the scenes, which means existing `Workspace setContents:` / `evaluateCurrent` and `Workspace evaluate:` paths still work while the persisted image now stores the canonical chunk form internally.
- Re-verified the persisted workspace buffer/session behavior with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_evaluate_persisted_workspace_buffer_without_demo_specific_program tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_rerun_workspace_source_session_without_demo_specific_program tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_live_snapshot_round_trips_dynamic_class_and_inherited_state -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add Class Comments To Live Class Source Round-Trip
- Extended live class definition metadata in both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` with a bounded `comment:` field, taught the class-header parser to accept `comment: '...'`, and made dynamic class updates preserve comment-only edits instead of discarding them as empty redefinitions.
- Persisted that comment metadata through live class file-out and snapshot save/load on both targets by expanding the dynamic class snapshot records, including comments in canonical `RecorzKernelClass:` headers, and showing them in the Transcript-based workspace class browser when present.
- Updated the class file-out demos in `examples/qemu_riscv_in_image_class_file_out_demo.rz` and `examples/qemu_riscv_class_file_out_snapshot_save_demo.rz` so the live class/file-out/snapshot round-trip now exercises `comment:` metadata instead of only superclass and ivar data.
- Verified with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv_method_update_integration tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv32_snapshot_integration -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add Protocol-Aware Canonical Source Round-Trip
- Extended the live method-source registry on both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` so each remembered method now persists an optional protocol/category name through live install, snapshot save/load, and file-out. `fileIn` now accepts explicit `RecorzKernelProtocol: '...'` chunks and associates subsequent methods with that protocol until the next class/class-side boundary or protocol chunk.
- Canonical class file-out now groups instance-side and class-side methods by protocol and emits explicit `RecorzKernelProtocol:` chunks in a stable source order. The workspace method list browser now shows `protocol :: selector` entries when protocol metadata exists, and selecting a method or class source round-trips back through the canonical chunk model instead of ad hoc source fragments.
- Updated the live class/file-out examples in `examples/qemu_riscv_in_image_class_file_out_demo.rz`, `examples/qemu_riscv_class_file_out_snapshot_save_demo.rz`, and `examples/qemu_riscv_workspace_class_side_browser_save_demo.rz` so the round-trip surface now exercises protocol metadata on both instance-side and class-side methods.
- Hardened the Transcript-based workspace/source renderer on both QEMU targets so browser lines are clipped to the visible width and workspace views stop at the bottom of the form instead of triggering transcript clear-on-overflow. This fixed the class-source browser snapshot reopen path and kept canonical class file-out browsing readable on the 1024x768 framebuffer.
- Updated `tools/qemu_hmp_screendump.py` plus the RV64/RV32 screenshot make targets to wait for the `rendered` serial log marker before issuing the HMP screendump, which removed the remaining race between workspace/browser rendering and screenshot capture during integration tests.
- Verified with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv_method_update_integration tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv_mvp_lowering -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add Workspace Protocol Browsers
- Extended `kernel/mvp/Selector.rz` and `kernel/mvp/Workspace.rz` with `browseProtocolsForClassNamed:`, `browseProtocol:ofClassNamed:`, `browseClassProtocolsForClassNamed:`, and `browseClassProtocol:ofClassNamed:`, keeping protocol navigation on the same image-owned workspace/browser surface as class, method, object, and source browsing.
- Added instance-side and class-side protocol list browsers plus protocol-specific method-list browsers in both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c`. These views derive protocol names from the live method-source registry, render protocol counts and selectors through the existing Transcript browser, and persist their targets so `Workspace reopen` can restore them after snapshot boot.
- Broadened selector validation in `platform/qemu-riscv64/program.c`, `platform/qemu-riscv32/program.c`, `platform/qemu-riscv64/vm.c`, and `platform/qemu-riscv32/vm.c` so the new workspace selectors are accepted by both the top-level program loader and the runtime selector cache/lookup path.
- Added protocol-browser snapshot proofs in `examples/qemu_riscv_workspace_protocol_browser_save_demo.rz` and `examples/qemu_riscv_workspace_class_protocol_browser_save_demo.rz`, plus RV64/RV32 snapshot coverage in `tests/test_qemu_riscv_snapshot_integration.py` and `tests/test_qemu_riscv32_snapshot_integration.py`.
- Updated `tests/test_qemu_riscv_image_inspector.py` for the enlarged kernel surface (`objects=345`, `selector_objects=59`, `method_entries=56`) and re-verified with `PYTHONPATH=src:. python3 -m unittest tests.test_qemu_riscv_image_inspector tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_reopen_workspace_protocol_browser_state_without_demo_specific_program tests.test_qemu_riscv_snapshot_integration.QemuRiscvSnapshotIntegrationTests.test_snapshot_can_reopen_workspace_class_protocol_browser_state_without_demo_specific_program tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_protocol_browser_state_without_demo_specific_program tests.test_qemu_riscv32_snapshot_integration.QemuRiscv32SnapshotIntegrationTests.test_snapshot_can_reopen_workspace_class_protocol_browser_state_without_demo_specific_program -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add Package Source File-In And File-Out
- Extended the canonical live chunk protocol on both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` with explicit `RecorzKernelPackage:` chunks plus `package: '...'` class-header metadata. Live class definitions now persist a package name through file-in, snapshot save/load, and file-out, which moves source organization beyond isolated class streams toward package-level source ownership.
- Added `fileOutPackageNamed:` to `kernel/mvp/KernelInstaller.rz`, `kernel/mvp/Workspace.rz`, and `kernel/mvp/Selector.rz`, then implemented package source export and a persisted workspace package-source browser on both targets. `Workspace reopen` can now restore a package source view from the image, and `Workspace fileInCurrent` accepts package buffers directly, so package source round-trips through the same image-owned workflow as classes, class-side methods, do-its, and file-out buffers.
- Added snapshot proofs in `examples/qemu_riscv_package_file_out_snapshot_save_demo.rz` and `examples/qemu_riscv_package_file_out_snapshot_reload_demo.rz`, plus RV64/RV32 snapshot coverage in `tests/test_qemu_riscv_snapshot_integration.py` and `tests/test_qemu_riscv32_snapshot_integration.py`, proving that a package can be filed out into `Workspace`, saved in a snapshot, reopened, re-filed, and executed again after reboot on both targets.
- Hardened the package-source path while landing the feature: increased the in-image source line limit to tolerate canonical class headers with `package:` and `comment:`, fixed RV32 package export corruption by separating the package file-out scratch buffer from the shared source I/O buffer, made both QEMU makefiles create snapshot-output directories automatically, and relaxed `tools/qemu_hmp_screendump.py` so delayed render markers do not fail snapshot/browser screenshot capture.
- Updated `tests/test_qemu_riscv_image_inspector.py` and `tests/test_qemu_riscv_mvp_lowering.py` for the enlarged kernel surface (`objects=350`, `selector_objects=60`, `method_entries=58`) and re-verified with `PYTHONPATH=src:tools:. python3 -m unittest tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv_image_inspector tests.test_qemu_riscv_mvp_lowering -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add Package-Aware Workspace Browsers
- Extended `kernel/mvp/Workspace.rz` and `kernel/mvp/Selector.rz` with `browsePackages` and `browsePackageNamed:` so package navigation now sits beside the existing class, protocol, method, object, and package-source browser surface inside the image.
- Added package list and package class-list workspace views on both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c`. The package list browser renders the unique live package names plus class counts, the package browser renders the classes within a package, and `Workspace reopen` now restores either view directly from snapshot state.
- Added snapshot proofs in `examples/qemu_riscv_workspace_package_list_save_demo.rz` and `examples/qemu_riscv_workspace_package_browser_save_demo.rz`, plus matching RV64/RV32 coverage in `tests/test_qemu_riscv_snapshot_integration.py` and `tests/test_qemu_riscv32_snapshot_integration.py`, so package navigation survives snapshot save/reload on both targets.
- Updated the generated surface expectations in `tests/test_qemu_riscv_image_inspector.py` and `tests/test_qemu_riscv_mvp_lowering.py` for the enlarged selector and method-entry sets (`objects=356`, `selector_objects=62`, `method_entries=60`) and re-verified the package browser step together with the RV32 target-profile budget suite after raising the target heap limit to `512` and the target live method source registry to `64` so those overflow tests still hit their intended boundaries instead of newer bookkeeping limits.
- Verified with `PYTHONPATH=src:tools:. python3 -m unittest tests.test_qemu_riscv_image_inspector tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv32_budget_integration -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Broaden In-Image Source Methods To Real Expression Chains
- Extended the shared in-image source compiler in `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` so installed source methods now support chained unary expressions and real multi-statement bodies instead of only the earlier single-statement, identifier-only special cases. Assignment right-hand sides, keyword-send arguments, and return expressions now compile through one expression path, so live source like `^Display defaultForm width` and `Transcript show: detail printString.` works on both targets.
- Added `COMPILED_METHOD_OP_POP` validation for dynamically compiled methods and changed the statement compiler to discard intermediate statement results only when needed. That keeps the richer multi-statement path correct while preserving the old four-instruction MVP budget for compact methods of the form `statement. ^self`, which keeps existing startup-hook and live-install demos working under the same tiny compiled-method limit.
- Added the RV32-first proof at `examples/qemu_riscv_in_image_multistatement_source_demo.rz` and covered it in `tests/test_qemu_riscv32_serial_integration.py`, demonstrating that a live class can compile and run both a multi-statement method (`Display clear. ^self`) and a chained unary return (`^Display defaultForm width`) entirely from in-image source.
- Re-verified the whole shared runtime path with `PYTHONPATH=src:tools:. python3 -m unittest discover -s tests -v`, which finished green across 134 tests, alongside clean RV64/RV32 image rebuilds through the snapshot and serial integration suites.

## 2026-03-08 - Add First-Class Package Comments To Canonical Source Round-Trip
- Extended the live package model on both `platform/qemu-riscv64/vm.c` and `platform/qemu-riscv32/vm.c` from an inferred class grouping to a first-class package registry with bounded `comment:` metadata. `RecorzKernelPackage:` chunks now accept an optional `comment: '...'`, package browsers render that comment, and package identity no longer depends on scanning dynamic classes for unique package names.
- Persisted package metadata through snapshot save/load on both targets by expanding the live snapshot header to version `4`, adding package records to the serialized runtime state, and reporting package usage in `KernelInstaller memoryReport`. The shared snapshot/browser workflows stayed green on both QEMU targets after the format change.
- Canonical package file-out now emits `RecorzKernelPackage: 'Name' comment: '...'` when a comment exists, and header-only live classes can now be exported cleanly instead of failing file-out when they have no remembered methods yet. That keeps package round-trip aligned with the canonical chunk model instead of requiring every exported class to carry installed method source.
- Updated the package browser/file-out demos in `examples/qemu_riscv_workspace_package_browser_save_demo.rz`, `examples/qemu_riscv_workspace_package_list_save_demo.rz`, and `examples/qemu_riscv_package_file_out_snapshot_save_demo.rz` to exercise package comments, and added the RV32 serial proof `examples/qemu_riscv_package_comment_roundtrip_demo.rz` plus coverage in `tests/test_qemu_riscv32_serial_integration.py` so the canonical package source is validated directly through live file-out text.
- Verified with `PYTHONPATH=src:tools:. python3 -m unittest tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv32_snapshot_integration -v`, `make -C platform/qemu-riscv64 clean all inspect-image`, and `make -C platform/qemu-riscv32 clean all inspect-image`.

## 2026-03-08 - Add RV32-First Binary Source Expressions
- Broadened the in-image source compiler on both `platform/qemu-riscv32/vm.c` and `platform/qemu-riscv64/vm.c` so live source expressions can now parse binary sends with the existing MVP arithmetic selectors `+`, `-`, and `*`. The compiler still keeps the tiny compiled-method budget and execution model, but it no longer limits live source to unary and single-keyword shapes.
- Applied the same binary-expression support to the lightweight workspace evaluator path, so the image-owned source model stays coherent between `Workspace evaluate:` and live method installation even though RV32 is the primary proof target for this step.
- Added the RV32-first proof at `examples/qemu_riscv_in_image_binary_method_demo.rz`, which files in a live class, installs a binary-return getter `^value + value` from source, and prints the computed result over serial after calling that method from the running image.
- Extended `tests/test_qemu_riscv32_serial_integration.py` with `test_in_image_source_compiler_supports_binary_method_expressions`, keeping RV32 as the first-class validation target while the shared RV64 compiler/runtime path stays aligned.
- Verified with `PYTHONPATH=src:tools:. python3 -m unittest tests.test_qemu_riscv32_serial_integration -v`, plus clean RV32/RV64 target rebuilds through `make -C platform/qemu-riscv32 clean all inspect-image` and `make -C platform/qemu-riscv64 clean all inspect-image`.

## 2026-03-08 - Add RV32-First Parenthesized And Multi-Keyword Source Methods
- Extended the shared in-image source compiler in `platform/qemu-riscv32/vm.c` and `platform/qemu-riscv64/vm.c` so live methods now accept parenthesized expressions and multi-keyword method headers with more than one argument name. Argument resolution now maps named method arguments onto the existing `pushArgument` bytecode path instead of assuming a single keyword argument.
- Kept the proof RV32-first with `examples/qemu_riscv_in_image_parenthesized_method_demo.rz` and `examples/qemu_riscv_in_image_multikeyword_method_demo.rz`, demonstrating grouped binary expressions and a real two-keyword method header (`setLeft:right:`) compiled and run entirely from in-image source.
- Added the new selectors `#sum` and `#setLeft:right:` to `kernel/mvp/Selector.rz`, raised the shared program/VM selector ceilings in both QEMU targets, and synchronized `selector_name()` on RV32/RV64 so the live source compiler can resolve the enlarged selector universe by text at runtime.
- Updated RV32 serial integration coverage in `tests/test_qemu_riscv32_serial_integration.py` and refreshed the shared image/header expectations in `tests/test_qemu_riscv_image_inspector.py` and `tests/test_qemu_riscv_mvp_lowering.py` for the larger selector set (`selector_objects=64`, `objects=358`).
- Verified with `PYTHONPATH=src:tools:. python3 -m unittest discover -s tests -v`, plus clean RV32/RV64 target rebuilds through `make -C platform/qemu-riscv32 clean all inspect-image` and `make -C platform/qemu-riscv64 clean all inspect-image`.
- Added keyword-send expressions to the live source compiler and workspace evaluator on both QEMU targets, so installed methods can now compile returns like `^form writeString: left` instead of being limited to statement-level keyword sends.
- Added the RV32-first proof `/Users/david/repos/recorz/examples/qemu_riscv_in_image_expression_keyword_send_demo.rz` and serial integration coverage for keyword sends inside live method expressions.
- Added `nil` expression support to the live source compiler on both QEMU targets by wiring `COMPILED_METHOD_OP_PUSH_NIL` into compiled-method validation and code generation, so installed methods can assign or return `nil` without falling back to host-built code.
- Added the RV32-first proof `/Users/david/repos/recorz/examples/qemu_riscv_in_image_nil_method_demo.rz` and serial integration coverage for `nil` inside live method bodies.
- Added `self` expression support to the live source compiler on both QEMU targets by extending the shared execution opcode set with `PUSH_SELF`, teaching compiled-method validation/execution about it, and allowing installed methods to send messages to the receiver from inside the running image.
- Added the RV32-first proof `/Users/david/repos/recorz/examples/qemu_riscv_in_image_self_method_demo.rz` and serial integration coverage for `self` inside live method bodies.
- Added small-integer literal support to the live source compiler on both QEMU targets by extending the shared execution opcode set with `PUSH_SMALL_INTEGER`, validating and executing that immediate opcode in compiled methods, and allowing installed methods to use literal values like `1` inside live source bodies.
- Added the RV32-first proof `/Users/david/repos/recorz/examples/qemu_riscv_in_image_small_integer_literal_demo.rz` and serial integration coverage for small-integer literals inside live method bodies.
- Added live string literal support on both QEMU targets with a method-keyed string literal registry, a new shared `PUSH_STRING_LITERAL` compiled-method opcode, and snapshot persistence for those literal slots so installed methods can return strings like `''OK''` from inside the running image without falling back to host-built code.
- Added the RV32-first proof `/Users/david/repos/recorz/examples/qemu_riscv_in_image_string_literal_demo.rz` and refreshed the generated runtime/header expectations so the shared image contract includes the new string-literal opcode on both targets.
- Added real `true` and `false` literal support on both QEMU targets by seeding singleton `True` and `False` boot objects as globals, extending the live compiler and top-level program/runtime selector/global ceilings to include the new literal and proof selectors, and teaching the workspace/object browser to report those singleton classes by name instead of as unknown objects.
- Added the RV32-first proof `/Users/david/repos/recorz/examples/qemu_riscv_in_image_boolean_literal_demo.rz` and updated the seed/image expectations to the enlarged literal-aware boot graph (`objects=364`, `selector_objects=66`, `globals=10`).

## 2026-03-08 - Add Live Temporaries And Local Assignment
- Extended the live source compiler on both `platform/qemu-riscv32/vm.c` and `platform/qemu-riscv64/vm.c` so installed methods can now declare a temporary line (`| current |`), read temporaries, and assign to them with `:=`. The compiler resolves temporaries ahead of instance variables, rejects duplicate argument/temp/ivar names, and emits the existing shared lexical opcodes instead of inventing a second local-storage model.
- Taught the compiled-method validator and executor on both targets to accept lexical push/store opcodes inside installed methods by deriving the lexical slot count from the instruction stream at runtime. That keeps the compiled-method object format unchanged and stays within the same tiny heap/object model, which is important for RV32 and the eventual smaller hardware target.
- Fixed the older workspace/do-it evaluator path to support unary sends consistently with the live method compiler, which removes a real inconsistency in the canonical chunk flow and keeps `RecorzKernelDoIt:` chunks like `Display newline.` working through `Workspace fileInCurrent`.
- Added the RV32-first proof `/Users/david/repos/recorz/examples/qemu_riscv_in_image_temps_method_demo.rz`, which files in a class with a real temporary declaration and local assignment, then runs that method from the live image and prints the result.
- Extended RV32 serial coverage in `tests/test_qemu_riscv32_serial_integration.py` with `test_in_image_source_compiler_supports_temporaries_and_local_assignment`, and re-verified the affected live image/file-in/snapshot/compiler flows with `PYTHONPATH=src:tools:. python3 -m unittest tests.test_qemu_riscv32_serial_integration tests.test_qemu_riscv_method_update_integration tests.test_qemu_riscv_snapshot_integration tests.test_qemu_riscv32_snapshot_integration tests.test_qemu_riscv_mvp_lowering tests.test_qemu_riscv_image_inspector -v`, plus clean RV32/RV64 image rebuilds through `make -C platform/qemu-riscv32 clean all inspect-image` and `make -C platform/qemu-riscv64 clean all inspect-image`.
