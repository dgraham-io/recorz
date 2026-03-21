# Stage 5 Readiness Audit

This note audits the deferred directions named in [docs/post_phase_1_execution_plan.md](/Users/david/repos/recorz/docs/post_phase_1_execution_plan.md) Stage 5 and records which ones are ready to become the next program.

## Existing Readiness Material

The repo already has enough design-gate material to make a decision without re-litigating the whole architecture:

- [docs/post_phase_1_roadmap.md](/Users/david/repos/recorz/docs/post_phase_1_roadmap.md) names the deferred directions and says the next planning layer begins after the Bluebook-parity tranche.
- [docs/recorz_architecture_book.md](/Users/david/repos/recorz/docs/recorz_architecture_book.md) gives the staged conformance model for self-hosting, typing, and platform-oriented features.
- [docs/recorz_compiler_and_bootstrap_specification.md](/Users/david/repos/recorz/docs/recorz_compiler_and_bootstrap_specification.md) already names compiler self-hosting, reproducible source-to-image workflows, and minimal external dependencies as the Stage 5 maturity direction.
- [docs/recorz_concurrency_multiprocessing_and_capability_security_specification.md](/Users/david/repos/recorz/docs/recorz_concurrency_multiprocessing_and_capability_security_specification.md) already defines later-stage process, multiprocessing, and capability expectations.
- [docs/recorz_language_specification.md](/Users/david/repos/recorz/docs/recorz_language_specification.md) already treats gradual typing as a long-term language feature, not a current implementation requirement.

## Readiness Matrix

### Deeper Self-Hosting

- Status: ready to become the default next program.
- Why: the current image already owns the browser/debugger/process tool model, regeneration exists, and the host builders are still explicitly mechanical.
- Remaining work is implementation planning, not feature invention.
- Landing decision: yes, as the next execution plan.

### Richer Process Semantics

- Status: partially ready, but not the default next program.
- Why: the runtime now has inspectable process and debugger state, but the next step would change semantics rather than thin the bootstrap path.
- Missing gate: a dedicated process-semantic design note with proof artifacts and explicit snapshot/quiescence expectations.
- Landing decision: defer until after the self-hosting tranche is planned.

### Capability Hooks

- Status: not ready.
- Why: the specs name the topic, but the repo does not yet have a capability model, revocation story, or authority proof artifacts.
- Missing gate: capability-object model, primitive boundary rules, and an explicit threat model.
- Landing decision: keep deferred.

### Gradual Typing

- Status: not ready.
- Why: the language/spec documents describe typing as a future direction, but the current compiler/bootstrap surface is still a seed path.
- Missing gate: syntax, analysis model, IR/proof artifacts, and a compatibility policy for untyped image code.
- Landing decision: keep deferred.

### HDL / Systems-Subset Work

- Status: not ready.
- Why: the repo still lacks the self-hosting maturity and subset-proof machinery that would make this productive.
- Missing gate: a stable subset declaration format plus lowering and validation artifacts.
- Landing decision: keep deferred.

### Broader Multiprocessing

- Status: not ready.
- Why: the current scheduler/process model is useful, but it is still a single-primary-path system with no explicit multi-domain architecture.
- Missing gate: domain or object-space design, cross-boundary communication rules, and snapshot semantics for isolated domains.
- Landing decision: keep deferred.

## Audit Conclusion

The next program should default to deeper self-hosting.
The other deferred topics are valid and documented, but they need their own design-gate notes before they can become the next execution lane.

This file is ready to land as a docs-only artifact.
