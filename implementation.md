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
