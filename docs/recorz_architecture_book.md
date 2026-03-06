# Recorz Architecture Book
## Version 0.1 Draft

## Status of This Document

This book is the consolidated architectural reference for **Recorz**, a Smalltalk- and Strongtalk-inspired language and living system intended to scale from a tiny educational implementation to a self-hosting, graphical, bare-metal computing platform with support for low-level programming, hardware description, accelerators, multiprocessing, and future capability-based isolation.

This document unifies the major Recorz specifications into a single architectural narrative. It is intended to serve as the top-level design reference for the project.

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** are to be interpreted as normative requirements.

---

# Table of Contents

1. Purpose of Recorz
2. Core Design Principles
3. System Overview
4. Language Architecture
5. Type System and Gradual Typing
6. Reflective Model and Debugging
7. Restricted Execution Subsets
8. VM and Object Memory Architecture
9. Compiler and Bootstrap Architecture
10. Standard Library and Core Class Model
11. Process, Concurrency, and Multiprocessing Direction
12. Accelerator and FPGA Integration
13. Security, Capabilities, and Isolation
14. Image, Source, and Persistence Model
15. Development Environment and Tools
16. Architectural Through-Lines
17. Recommended Implementation Roadmap
18. Conformance and Staging Strategy
19. Major Open Questions
20. Summary

---

## 1. Purpose of Recorz

Recorz is intended to be a **malleable, inspectable, image-based computing system** in the Smalltalk tradition.

Its primary goal is to support **curious humans**: people who want to explore, understand, debug, extend, and eventually rebuild the system from within the system itself.

Recorz is designed to unify several normally separated concerns:

- a Smalltalk-like programming language,
- a live graphical development environment,
- a self-hosting compiler toolchain,
- a reflective VM and object memory,
- low-level runtime and systems programming,
- hardware-description and FPGA-oriented work,
- a path to multiprocessing and capability-based isolation.

Recorz is therefore not merely a language, and not merely a desktop environment. It is intended as a **living architecture** spanning objects, tools, runtime internals, and hardware-adjacent execution.

---

## 2. Core Design Principles

The whole architecture is governed by a small number of principles.

### 2.1 Smalltalk continuity

Recorz SHALL remain recognizably grounded in Smalltalk and Strongtalk.

- Message sending remains central.
- Blocks remain central.
- Browsers, inspectors, workspaces, and debuggers remain central.
- Classes and metaclasses remain central.
- Live development remains central.

A user SHOULD be able to begin by writing ordinary Smalltalk-like code and later grow into typing, systems work, and hardware work without abandoning the language’s core identity.

### 2.2 Minimal abstraction growth

Recorz SHOULD prefer a few orthogonal mechanisms over many specialized features.

New power SHOULD come from:

- optional typing,
- explicit metadata,
- restricted execution subsets,
- inspectable tool support,

rather than from a large number of unrelated sublanguages.

### 2.3 Reflective comprehensibility

The system MUST explain itself.

Objects, methods, classes, selectors, contexts, processes, compiler objects, compiled methods, and generated artifacts SHOULD be inspectable. Debugging is not auxiliary; it is part of the system’s identity.

### 2.4 Live malleability

Recorz SHOULD evolve from within itself.

External bootstrap code is allowed only to the degree necessary to bring up the first viable system. That code is intended to become unimportant to day-to-day development.

### 2.5 Scalable architecture

The architecture SHALL scale from:

- a tiny VM and image,
- to a self-hosting language system,
- to a richer graphical platform,
- to a system capable of low-level programming and hardware coordination.

### 2.6 Explicit authority and future isolation

Recorz may begin permissively, but the architecture SHALL leave room for restricted domains, capability-based authority, and sandboxing without requiring a complete redesign.

---

## 3. System Overview

At the highest level, Recorz consists of six tightly related parts:

1. **The language**
   - a Smalltalk-derived syntax and semantics,
   - optional Strongtalk-like gradual typing,
   - reflective behavior,
   - restricted systems and HDL subsets.

2. **The VM and object memory**
   - object representation,
   - method execution,
   - contexts,
   - processes,
   - garbage collection,
   - snapshot support,
   - primitive boundary.

3. **The compiler toolchain**
   - parser,
   - AST,
   - semantic analysis,
   - type analysis,
   - subset enforcement,
   - lowering and installation,
   - source-to-image reconstruction.

4. **The standard library**
   - object model classes,
   - collections, numbers, streams, processes,
   - reflection objects,
   - compiler model objects,
   - graphics-adjacent classes.

5. **The live environment**
   - browser,
   - workspace,
   - inspector,
   - debugger,
   - process tools,
   - compiler tools,
   - hardware and accelerator tools.

6. **The execution and security architecture**
   - concurrency,
   - future domains/object spaces,
   - message coordination,
   - accelerator integration,
   - capability-based security direction.

These parts are meant to form one coherent system rather than a collection of detached components.

---

## 4. Language Architecture

The Recorz language is class-based, message-oriented, and syntactically close to Smalltalk.

### 4.1 Core language model

Recorz SHALL provide:

- unary messages,
- binary messages,
- keyword messages,
- blocks,
- assignment with `:=`,
- return with `^`,
- cascades,
- single inheritance,
- class and metaclass structure,
- reflective pseudo-variables including `self`, `super`, and `thisContext` in the general subset.

### 4.2 Compatibility goal

Ordinary Smalltalk-style methods SHALL remain valid and idiomatic.

The user’s initial experience SHOULD be as close as practical to traditional Smalltalk.

### 4.3 Message precedence

The language preserves Smalltalk message precedence:

1. unary
2. binary
3. keyword

This is a defining part of the language’s readability and identity.

### 4.4 Control structures

Control remains message-oriented rather than syntax-heavy. Conditionals and iteration SHOULD continue to be expressed through boolean and block protocol rather than through C-like statement syntax.

---

## 5. Type System and Gradual Typing

Recorz adopts gradual typing in the spirit of Strongtalk.

### 5.1 Typing philosophy

Typing SHALL be optional.
Untyped code remains valid.
Typed and untyped code SHALL interoperate.

Typing exists to support:

- better tools,
- earlier error detection,
- optimization,
- clearer intent,
- safer low-level subsets.

Typing does not replace exploratory programming.

### 5.2 Type categories

The architecture SHOULD support:

- nominal class types,
- protocol/interface-like constraints,
- optional or nilable types,
- union forms where useful,
- inferred types where practical,
- parameterized types only if they can be kept conceptually simple.

### 5.3 Boundary behavior

Where typed and untyped code meet, the system MUST define inspectable boundary behavior, which may include guards, inserted checks, specialized paths, or dynamic fallback.

### 5.4 Architectural importance

The type system is not isolated from the rest of the platform. It affects:

- compiler analysis,
- method specialization,
- restricted subset eligibility,
- browser feedback,
- tooling and refactoring.

---

## 6. Reflective Model and Debugging

Reflection is one of the deepest architectural commitments of Recorz.

### 6.1 Required reflective visibility

The general system SHALL expose enough structure to support:

- object inspection,
- class and metaclass inspection,
- method browsing,
- selector inspection,
- senders and implementors queries,
- active context inspection,
- process inspection,
- source association,
- debugging and restart behavior.

### 6.2 Contexts and `thisContext`

The general subset SHALL support `thisContext`.
Implementation strategy may vary, but the user-visible system MUST retain enough execution structure for live debugging.

### 6.3 Debugger identity

The debugger is not a bolt-on tool. It is a central expression of the language and runtime model. Recorz is incomplete without strong debugger semantics.

### 6.4 Reflective boundaries

Systems and HDL subsets MAY restrict reflection, but such restrictions MUST be explicit, tool-visible, and conceptually justified.

---

## 7. Restricted Execution Subsets

One of Recorz’s central architectural moves is to remain one language family while allowing specialized execution subsets.

### 7.1 General subset

This is ordinary dynamic Recorz:

- reflective,
- debugger-friendly,
- block-rich,
- message-oriented,
- suited for most application and tool code.

### 7.2 Typed subset

This is the general language plus type metadata and associated checks or optimizations.

### 7.3 Systems subset

This is a restricted form intended for:

- VM internals,
- memory managers,
- low-level runtime,
- device drivers,
- code generators,
- architecture support.

It MAY restrict:

- reflection,
- general allocation,
- non-local return,
- dynamic dispatch,
- arbitrary block capture,
- other features incompatible with predictable low-level translation.

### 7.4 HDL subset

This is a restricted form intended for hardware elaboration and FPGA work.

It MUST be deterministic and statically elaborable.
It SHALL reject language constructs with no coherent hardware meaning.

### 7.5 Architectural benefit

This subset approach preserves a unified mental model. Users do not “leave Recorz” to do systems or hardware work; they move into a more constrained region of the same language world.

---

## 8. VM and Object Memory Architecture

The VM is the runtime substrate beneath the live system.

### 8.1 VM responsibilities

The VM SHALL provide:

- object allocation,
- object memory management,
- method execution,
- message lookup support,
- blocks and contexts,
- process scheduling,
- primitive dispatch,
- snapshot save/restore,
- machine boundary mediation.

### 8.2 Object memory

The architecture supports:

- immediate values,
- heap objects,
- variable-sized objects,
- byte-oriented objects,
- compiled methods,
- contexts,
- class and metaclass objects.

Representation is implementation-defined, but object identity and language semantics MUST be preserved.

### 8.3 Compiled methods

Compiled methods SHALL retain or reference:

- selector identity,
- defining class association,
- executable body,
- literal references,
- source association,
- metadata relevant to primitives and tooling.

### 8.4 Contexts and non-local return

The VM SHALL preserve sufficient structure for:

- sender chains,
- block home association,
- debugger stepping,
- non-local return behavior in the general subset.

### 8.5 Execution strategy freedom

The VM MAY use interpreters, bytecodes, JITs, native code, or hybrids. Whatever strategy is used, the system MUST preserve observable Smalltalk-like semantics and debugger friendliness.

---

## 9. Compiler and Bootstrap Architecture

The compiler connects source to runtime and eventually becomes part of the live system itself.

### 9.1 Compiler pipeline

The conceptual compiler pipeline includes:

1. parsing,
2. AST construction,
3. semantic binding,
4. optional type analysis,
5. subset classification,
6. IR or executable lowering,
7. compiled artifact installation or packaging.

### 9.2 Live and source-driven compilation

The compiler SHALL support both:

- incremental live compilation inside a running image,
- source-driven reconstruction of an image from outside or from a fresh seed state.

### 9.3 Compiler introspection

Compiler objects SHOULD themselves be inspectable, including parsers, AST nodes, analyzers, diagnostics, and lowerings.

### 9.4 Bootstrap philosophy

Some external seed code is allowed, but it MUST be minimal and intentionally temporary.

### 9.5 Self-hosting goal

The long-term goal is that the primary compiler and most tool evolution occur inside Recorz itself.

---

## 10. Standard Library and Core Class Model

The standard library is where the language becomes usable and understandable.

### 10.1 Role of the library

The library SHALL provide the object families needed for:

- ordinary programming,
- reflective inspection,
- compiler construction,
- debugger construction,
- process management,
- a live graphical environment.

### 10.2 Core class families

The architecture includes class families for:

- root object behavior,
- classes and metaclasses,
- booleans and nil,
- numbers and magnitudes,
- characters, strings, and symbols,
- collections,
- streams,
- exceptions,
- processes and synchronization,
- reflection objects,
- compiler model objects,
- basic graphics-adjacent objects.

### 10.3 Library philosophy

The library SHOULD remain elegant and coherent in the Smalltalk tradition. It SHOULD avoid becoming a large accidental pile of unrelated utilities.

### 10.4 Tool support from the library

The library is not just for application programmers. It is also the substrate out of which browsers, inspectors, debuggers, parsers, and build tools are made.

---

## 11. Process, Concurrency, and Multiprocessing Direction

Concurrency begins with familiar live processes but is intended to grow beyond a permanently shared global heap model.

### 11.1 Early process model

Early Recorz MAY use a Smalltalk-style process model with:

- inspectable processes,
- a scheduler,
- semaphores or equivalent,
- debugger integration.

### 11.2 Long-term direction

The preferred long-term architecture SHOULD move toward:

- explicit coordination,
- ownership boundaries,
- isolated domains or object spaces,
- message-passing between concurrency regions,
- multiple processors and accelerator-aware execution.

### 11.3 Why this direction matters

Classic shared-memory concurrency is easy to start with but hard to scale for comprehension, debugging, and security. Recorz therefore treats the early process model as a starting point, not a permanent ceiling.

---

## 12. Accelerator and FPGA Integration

Recorz is intended to reach beyond CPUs alone.

### 12.1 HDL integration

The HDL subset allows hardware descriptions to be authored in the Recorz environment while remaining part of the same conceptual world as ordinary code.

### 12.2 Accelerator object model

Accelerators and FPGA cores SHOULD be represented by inspectable objects or descriptors exposing:

- identity,
- configuration state,
- bound software endpoints,
- lifecycle state,
- performance and health metadata,
- authority/ownership context.

### 12.3 Coordination model

Accelerator use SHOULD be modeled through explicit submission, completion, error, and quiescence mechanisms rather than hidden side effects.

### 12.4 Dynamic reconfiguration

Where the hardware platform allows it, Recorz SHOULD support dynamic FPGA reconfiguration with safe transition rules.

---

## 13. Security, Capabilities, and Isolation

Security is a long-term architectural concern rather than a purely later add-on.

### 13.1 Early permissiveness

Early systems MAY be highly reflective and permissive in the trusted core.

### 13.2 Future capability direction

The architecture SHALL preserve a path toward object-capability security, where authority flows through explicit references rather than ambient global access.

### 13.3 Sensitive boundaries

The following areas SHOULD be capable of later restriction:

- primitive use,
- raw memory access,
- device access,
- network/storage access,
- class and method mutation,
- process/domain control,
- snapshot authority,
- accelerator control,
- deep reflection over peer or core state.

### 13.4 Trusted core vs restricted domains

The trusted core retains the power to evolve the system. Restricted domains SHOULD later be able to run applications without uncontrolled authority over the rest of the platform.

---

## 14. Image, Source, and Persistence Model

Recorz lives through both image continuity and source reconstruction.

### 14.1 Image as living state

The image contains:

- objects,
- classes,
- methods,
- tool state,
- process state as policy allows,
- project and environment continuity.

### 14.2 Source as reconstructible truth

Source exists for:

- interchange,
- versioning,
- review,
- archival,
- source-driven rebuild,
- bootstrap.

### 14.3 Dual artifact model

The architecture SHALL treat both image and source as first-class artifacts.
Neither replaces the other.

### 14.4 Transition safety

Some transitions—such as snapshotting, class layout change, compiler replacement, or accelerator reconfiguration—may require quiescence or other coordinated runtime policies.

---

## 15. Development Environment and Tools

Recorz aspires to the spirit of the Xerox Alto and Dorado: a graphical environment where programming, browsing, inspection, and debugging are native activities.

### 15.1 Required tools

The environment SHALL provide or evolve toward:

- system browser,
- workspace,
- inspector,
- debugger,
- transcript/log,
- senders/implementors queries,
- process browser,
- hierarchy browser,
- compiler tooling,
- hardware and accelerator tooling as the system matures.

### 15.2 Tool design values

Tools SHOULD emphasize:

- immediate feedback,
- inspectability,
- clarity,
- recovery from mistakes,
- source/runtime/generated-artifact cross-linking.

### 15.3 Educational importance

The tools are not just for productivity. They are also the system’s main mechanism for teaching users how it works.

---

## 16. Architectural Through-Lines

Several ideas recur across every part of the system.

### 16.1 One living system

The language, compiler, runtime, tools, and low-level layers are all intended to become part of the same image-based world.

### 16.2 One conceptual language family

General code, typed code, systems code, and HDL code are related through restriction and metadata, not through unrelated language identities.

### 16.3 Tools are part of the architecture

The browser, inspector, debugger, process tools, and compiler tools are not accessories. They are architectural commitments.

### 16.4 Reflection with disciplined boundaries

Reflection is central, but the architecture also anticipates that later restricted domains may need attenuated reflection.

### 16.5 Simple now, extensible later

The early system may be small and permissive. The architecture must nevertheless avoid choices that make self-hosting, multiprocessing, accelerators, or sandboxing impossible later.

---

## 17. Recommended Implementation Roadmap

The following roadmap is recommended for practical execution.

### Phase 1: Tiny seed system

Build:

- minimal object representation,
- method execution,
- simple parser/compiler,
- image load/save,
- basic classes,
- simple inspector/debugger entry,
- a tiny graphical or textual workspace environment.

Goal: prove the language, object model, and liveness.

### Phase 2: Small live Smalltalk-like system

Add:

- stronger class library,
- browser,
- inspector,
- debugger,
- streams,
- collections,
- processes,
- source association.

Goal: produce a usable live programming system.

### Phase 3: Self-describing compiler and tools

Add:

- AST and compiler model objects,
- richer diagnostics,
- source-to-image rebuild path,
- compiler introspection,
- improved change and recovery tools.

Goal: move language implementation inward.

### Phase 4: Gradual typing and optimization

Add:

- optional type annotations,
- type-aware tools,
- specialized compilation paths,
- explicit typed/untyped boundary behavior.

Goal: strengthen reliability and enable better code generation without abandoning dynamic development.

### Phase 5: Systems subset and low-level path

Add:

- systems-subset validation,
- width-sensitive and layout-sensitive representations,
- primitive metadata,
- machine-oriented lowering path,
- low-level driver/runtime facilities.

Goal: bring VM and runtime implementation closer into Recorz.

### Phase 6: HDL subset and accelerator pipeline

Add:

- HDL subset,
- hardware elaboration objects,
- simulation hooks,
- FPGA artifact management,
- accelerator submission/completion protocol.

Goal: allow Recorz to describe and coordinate hardware accelerators from within the system.

### Phase 7: Concurrency evolution and domain model

Add:

- richer synchronization visibility,
- mailboxes/channels/endpoints,
- ownership metadata,
- initial domain/object-space abstractions,
- quiescence-aware runtime transitions.

Goal: move beyond a permanently global shared-heap model.

### Phase 8: Capability hooks and restricted domains

Add:

- explicit authority-bearing service objects,
- primitive/device gating,
- reflective restriction points,
- resource policy objects,
- sandboxed application domains.

Goal: allow trusted core evolution alongside safer application execution.

### Phase 9: Mature platform integration

Add:

- richer graphics and input stack,
- desktop-oriented facilities,
- better device mediation,
- improved multiprocessing and accelerator scheduling,
- better source/image reproducibility.

Goal: arrive at a platform-class Recorz system.

---

## 18. Conformance and Staging Strategy

A Recorz implementation does not need to provide every feature immediately.

### 18.1 Core conformance

A minimal but real Recorz system SHOULD provide:

- Smalltalk-like language core,
- image support,
- reflective objects and contexts,
- browser/inspector/debugger basics,
- root classes and collections,
- simple process support.

### 18.2 Self-hosting-oriented conformance

A more mature system SHOULD add:

- compiler objects,
- source rebuild path,
- better class library,
- typing support,
- stronger diagnostics.

### 18.3 Platform-oriented conformance

A fuller system SHOULD add:

- systems subset,
- HDL subset,
- richer graphics,
- accelerator coordination,
- domain model,
- capability hooks.

This staged approach allows meaningful progress without forcing premature complexity.

---

## 19. Major Open Questions

The architecture is intentionally strong in direction while leaving some implementation details open.

Key open questions include:

1. exact type annotation syntax,
2. exact IR strategy,
3. exact bytecode or execution representation,
4. exact garbage collector strategy,
5. exact numeric tower for the first implementation,
6. exact HDL declaration syntax,
7. exact domain/object-space design,
8. exact capability representation and revocation mechanism,
9. exact image manifest and source package format,
10. exact live class-layout migration strategy.

These questions should be answered incrementally as the implementation matures, without violating the architectural through-lines described here.

---

## 20. Summary

Recorz is specified as a **Smalltalk-descended living system** whose central commitments are:

- preserve the clarity and feel of Smalltalk,
- adopt Strongtalk-like gradual typing conservatively,
- make reflection and debugging central,
- support self-hosting and live evolution,
- scale from tiny VM to broader platform,
- reach downward into low-level code and hardware description,
- reach outward toward multiprocessing and accelerator coordination,
- preserve a long-term path toward capability-based isolation.

The most important architectural idea is this:

**Recorz should remain one coherent, inspectable system from browser to compiler to VM to hardware-facing artifacts.**

That unity is what gives the platform its educational value, its malleability, and its long-term technical character.

---

## Appendix A. Recommended Companion Specifications

This architecture book is intended to sit above the following more detailed specifications:

1. **Recorz Formal Specification**
   - top-level normative platform scope and goals.

2. **Recorz Language Specification**
   - syntax, semantics, typing, reflection, primitives, and restricted subsets.

3. **Recorz VM and Object Memory Specification**
   - runtime, objects, methods, contexts, processes, snapshots, and primitive boundary.

4. **Recorz Compiler and Bootstrap Specification**
   - compiler pipeline, source-to-image flow, subset lowering, and self-hosting path.

5. **Recorz Standard Library and Core Class Specification**
   - library families, protocols, reflective classes, compiler model objects, and graphics-adjacent basics.

6. **Recorz Concurrency, Multiprocessing, and Capability Security Specification**
   - processes, schedulers, domains, endpoints, accelerators, capability direction, and isolation.

Together, these documents form the first complete architectural suite for Recorz.

---

## Appendix B. Recommended Reading Order

For a new contributor, the recommended reading order is:

1. this Architecture Book,
2. the Language Specification,
3. the VM and Object Memory Specification,
4. the Compiler and Bootstrap Specification,
5. the Standard Library and Core Class Specification,
6. the Concurrency, Multiprocessing, and Capability Security Specification,
7. the Formal Specification as a top-level normative cross-check.

This order moves from architectural intention to language and runtime mechanics, then outward into tools, libraries, and long-term platform concerns.

