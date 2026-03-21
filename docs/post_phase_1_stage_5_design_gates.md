# Stage 5 Design Gates

This file defines the explicit gates for the deferred directions named in Stage 5 of [docs/post_phase_1_execution_plan.md](/Users/david/repos/recorz/docs/post_phase_1_execution_plan.md).

The goal is to keep the topics separate so they do not collapse into one backlog.

## Gradual Typing

Entry criteria:

- the compiler/bootstrap path is stable enough to accept a new compatibility policy
- the image can still regenerate and reopen the current tool stack predictably
- the team is ready to define typing as a language feature, not a bootstrap patch

Non-goals:

- full whole-program inference on day one
- breaking untyped image code to force type adoption
- moving typing policy into the VM

Proof artifacts:

- a syntax note for annotations
- a lowering or IR note that explains how typed and untyped code coexist
- tests for typed compilation and compatibility with existing untyped source

## Capability Hooks

Entry criteria:

- object/process inspection is already stable enough to expose authority boundaries clearly
- the runtime has a documented process and debugger model
- snapshot behavior is understood well enough to preserve capability state safely

Non-goals:

- a full sandboxed-application platform in the first step
- policy hidden in ad hoc host scripts
- capability semantics that cannot be inspected from tools

Proof artifacts:

- a capability-object model note
- explicit revocation or attenuation tests
- a documented boundary for what lives in VM substrate versus image policy

## HDL / Systems-Subset Work

Entry criteria:

- source-to-image regeneration is reproducible enough to serve as a proving ground
- the compiler and bootstrap path have a stable self-hosting story
- the project can define a minimal subset without surprising the normal RV32 path

Non-goals:

- full hardware-design toolchain replacement
- pulling accelerator work into the critical path too early
- introducing a subset syntax before the language/compiler gates are clear

Proof artifacts:

- a subset declaration note
- a lowering/provenance note
- tests that prove the subset boundaries are enforced

## Broader Multiprocessing

Entry criteria:

- the single-core scheduler/process model is stable and inspectable
- process browser/debugger flows already explain live state well
- snapshot and restore semantics for the current process model are boring

Non-goals:

- multi-core scheduling optimization as the first goal
- exposing unconstrained shared mutable state across domains
- hiding ownership and transfer rules inside the runtime

Proof artifacts:

- a domain or object-space model note
- message/transfer rules
- tests for quiescence, snapshot recovery, and cross-boundary visibility

## Landing Note

This file is ready to land as a docs-only design-gate record.
