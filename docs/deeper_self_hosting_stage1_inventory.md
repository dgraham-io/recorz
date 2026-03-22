# Stage 1 Inventory

This note records the completed Stage 1 bootstrap-ownership inventory for the deeper self-hosting program.

## Scope Completed

- Audited `/Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py` as the active source-discovery, lowering, and image-packing authority.
- Audited `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py` as the runtime-binding wrapper entry point.
- Classified discovery, registry, and manifest work into bootstrap-only host responsibilities, image-owned metadata candidates, and purely mechanical marshalling helpers.

## Builder-Owned Discovery Inventory

- Kernel source-unit discovery:
  - `load_kernel_source_units_from_directory`
  - `load_kernel_source_units_from_bundle`
  - `load_kernel_source_units`
- Kernel class/header discovery:
  - `parse_kernel_class_header`
  - `load_kernel_method_sources`
  - `load_kernel_canonical_class_sources`
  - `load_kernel_builder_class_sources`
- Boot-object discovery:
  - `parse_kernel_boot_object_chunk`
  - `load_kernel_boot_object_declarations`
  - `build_boot_object_specs_from_declarations`
  - `build_fixed_boot_graph_spec`
- Root and glyph-family discovery:
  - `parse_kernel_root_chunk`
  - `load_kernel_root_declarations`
  - `parse_kernel_glyph_bitmap_family_chunk`
  - `load_kernel_glyph_bitmap_family_declaration`
- Selector discovery:
  - `parse_kernel_selector_chunk`
  - `load_kernel_selector_declarations`
  - `build_selector_specs_from_declarations`
- Method-entry and primitive-binding discovery:
  - `parse_kernel_method_chunk`
  - `validate_kernel_method_selectors`
  - `validate_kernel_primitive_binding_symbol_names`
  - `compile_kernel_method_program`
- Seed/image layout derivation:
  - `build_boot_image_spec`
  - `build_boot_image_seed_build_context`
  - `build_seed_layout`
  - `build_dynamic_seed_sections`
  - `build_seed_manifest`
  - `build_entry_manifest`
  - `describe_image_manifest`
  - `build_image_manifest`

## Bootstrap-Only Responsibilities To Keep

- Parsing the host-side runtime spec from `/Users/david/repos/recorz/platform/qemu-riscv64/runtime_spec.json`.
- Lowering source into the binary image/seed/program payloads that QEMU consumes before the image is alive.
- Compiling canonical kernel methods into tiny compiled-method instruction words.
- Packing the final seed, entry, and program sections into the image manifest.
- Generating the C-facing runtime-bindings header consumed by the RV32 and RV64 native targets.
- Enforcing contiguous declaration orders and binary-layout invariants that the boot image cannot repair after emission.

## Image-Owned Metadata Candidates

- Human-readable ownership summaries currently implicit in the builder:
  - source roots
  - registry counts
  - manifest section roles
  - primitive-binding ownership summaries
- Selector, class, boot-object, and root declaration summaries that are already derivable from generated registries and regenerated kernel source.
- Runtime-binding surface summaries:
  - primitive binding counts
  - binding owner class/selector pairs
  - generated selector/count summaries
- Regeneration-authority explanations:
  - which source is canonical
  - which source is builder-inclusive
  - which regenerated artifact is intended to be fed back into the builder

## Mechanical Marshalling Helpers

- `build_named_constant_maps*` helpers
- `build_constant_value_map_from_explicit_specs`
- `build_constant_definitions_from_explicit_specs`
- `build_named_value_map_from_explicit_specs`
- `encode_compiled_method_instruction`
- `append_enum_definition`
- `append_macro_definition`
- `append_magic_byte_definitions`
- `write_output_bytes_if_changed`
- `write_output_text_if_changed`
- the wrapper script `/Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`

## Host-Generated Runtime-Binding Facts That Must Remain

- C enum/macro emission for:
  - opcode values
  - selector ids
  - object kinds
  - seed field kinds
  - method entry ids
  - primitive binding ids
- Handler symbol names and the generated dispatch table consumed by native VM code.
- Struct/array layouts for:
  - generated seed class source records
  - selector records
  - primitive binding records
  - primitive binding owner records

## Selector/Source Metadata Candidates For Image Authorship

- Human-readable summaries of:
  - generated selector counts and ranges
  - primitive binding counts and ownership
  - seed class source counts and class/source authority roles
  - manifest section roles and counts
- Browsable anchors that point from runtime metadata to:
  - regenerated kernel source
  - regenerated boot source
  - regenerated file-in source
- Explanatory labels for builder-owned surfaces currently visible only through tests or Python helpers.

## Blockers Or Reasons To Defer Specific Candidates

- Selector ids, object-kind values, and binary section packing still must stay host-generated because the native targets compile against them before the image runs.
- Method-entry order and compiled-method lowering remain bootstrap-only until the image can author and serialize those structures without delegating policy back to C.
- Runtime-binding handler tables remain host-generated because they connect image-authored selector metadata to native C entry points.

## Recommended Stage 2 Scope

- Extend the existing `Runtime Metadata` browser with concise compiler/bootstrap summaries instead of raw dumps.
- Surface primitive-binding ownership and generated-registry summaries in the image using the data the host already emits.
- Keep regenerated-source browse/return/save flows coherent while those summaries grow.

## Explicit Non-Goals To Keep Out Of Stage 2

- Replacing the host builder as the binary emitter.
- Moving method-entry packing or runtime-binding header emission into image code.
- Reintroducing tool policy or compiler ownership into C.

## Verification Commands Run

- `PYTHONPATH=src:tools python3 -m unittest -q tests.test_qemu_riscv_mvp_lowering`
- `python3 -m py_compile /Users/david/repos/recorz/tools/build_qemu_riscv_mvp_image.py /Users/david/repos/recorz/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py`

## Remaining Risks Or Non-Goals

- The current image can browse regenerated sources and high-level runtime counts, but it still cannot explain the builder/runtime-binding boundary in a richer way without Stage 2 work.
- The host builder continues to derive declaration order and binary-layout policy at import time. Stage 3 should thin that policy boundary, but it should not attempt to erase the honest bootstrap-only role identified here.
