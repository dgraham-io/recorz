# Stage 3 Manifest Boundary Note

## Scope Completed

- `DS3.1` extracted derivable selector/source manifest data into explicit builder-side registry surfaces instead of leaving it only as implicit global tables.
- `DS3.2` made the runtime-bindings header and image-manifest emission consume explicit boundary structures:
  - generated seed-class source records
  - generated selector records
  - generated primitive-binding records
  - generated primitive-binding owner records
  - explicit image section layouts
- `DS3.3` tightened regeneration authority checks so the tests can distinguish between:
  - bundle-driven registry/source authority
  - retained directory-backed method-source authority

## Selector/Source Facts That Remain Bootstrap-Only

- Binary image emission and packing for entry/program/seed sections.
- Runtime-bindings C header emission and the native-handler table.
- Contiguous numeric ids for selectors, object kinds, roots, and primitive bindings.
- Directory-backed method-source compilation used for built-in method lowering.

## Selector/Source Facts That Can Become Image-Authored

- Human-readable registry surfaces for:
  - seed class sources
  - selectors
  - primitive bindings
  - primitive-binding owners
- Manifest-section role summaries and section ordering.
- Explanatory ownership metadata around bundle-vs-directory authority.

## Why Each Retained Host Responsibility Is Still Necessary

- The native RV32/RV64 targets compile against generated ids and handler symbols before the image is alive.
- Image emission still needs a host-side packer for the bootable binary payload.
- Method-entry lowering remains bootstrap-only because the current image does not yet author compiled-method words or native binding tables directly.
- The current bundle path does not fully replace directory-backed method lowering, so Stage 3 keeps that boundary explicit instead of claiming full bundle authority.

## Deferred Items Kept Out Of Scope

- No attempt to move compiled-method lowering or native handler-table generation into the image.
- No claim that regenerated bundles are already the sole authority for built-in method bodies.
- No widening of RV64 beyond its validation role.

## Verification Commands Run

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`
- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv32_regeneration_integration`
- `python3 -m py_compile /Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py /Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py /Users/david/repos/recorz/tests/test_qemu_riscv32_regeneration_integration.py /Users/david/repos/recorz/tests/test_qemu_riscv_mvp_lowering.py`

## Remaining Risks Or Non-Goals

- Bundle authority is now explicit, but method-source authority is still split. That is documented rather than hidden.
- The host builder is thinner and more explicit, but it still owns honest bootstrap work.
- Further self-hosting would require a later program to move compiled-method and runtime-binding authorship upward without weakening the RV32 `32M` contract.
