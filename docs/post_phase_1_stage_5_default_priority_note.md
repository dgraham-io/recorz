# Stage 5 Default Priority Note

When there is no human override, the next major implementation program should default to deeper self-hosting.

## Rationale

This is the least disruptive continuation of the current architecture:

- The image already owns the practical tool surface, so the next gain is to move more compiler and regeneration responsibility upward instead of widening the VM.
- The host builders are still bootstrap machinery, which means the next meaningful reduction is to make them thinner and more mechanical.
- Deeper self-hosting preserves the current RV32-first, `32M`-bounded development path and does not require a new runtime contract.
- It builds directly on the existing regeneration, runtime-binding, and source-ownership work rather than asking for a separate architecture track.

## Default Rule

Use deeper self-hosting unless a human explicitly selects one of the other deferred directions.

If a different direction is chosen later, it should only become active after its own design-gate note exists and the choice is recorded as an intentional override.

## Preserved Goals

This default does not change the project goals:

- RV32 remains the primary path.
- The `32M` target remains in force.
- RV64 remains a validation target.
- Host builders remain bootstrap-only.

## Landing Note

This note is ready to land as a docs-only decision record.
