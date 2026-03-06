# Recorz Formal Specification
## Version 0.1 Draft

## Status of This Document

This document defines the intended language, runtime, environment, and platform model for **Recorz**, a Smalltalk- and Strongtalk-derived computing system. It is a design specification, not yet a conformance test suite.

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** in this document are to be interpreted as normative requirements.

---

## 1. Introduction

Recorz is a programming language and live computing environment derived from the Smalltalk tradition and informed by Strongtalk’s gradual typing model. Recorz is intended to support a complete graphical system running on bare hardware, without a conventional operating system, while preserving a highly reflective, inspectable, and debuggable programming experience.

Recorz is designed to scale from:

- a minimal educational or experimental virtual machine,
- to a self-hosting image-based development system,
- to a desktop-class environment capable of typical personal computing tasks and games,
- to a platform that can generate low-level processor code and synthesize FPGA-targeted hardware descriptions.

The central design constraint of Recorz is that it SHALL preserve the conceptual simplicity of Smalltalk wherever possible. Extensions for typing, low-level systems work, and hardware description SHALL be introduced conservatively and SHALL avoid unnecessary new abstractions.

---

## 2. Design Objectives

A conforming Recorz design SHALL pursue the following objectives:

1. **Smalltalk continuity**
   - Recorz SHALL remain recognizably compatible with Smalltalk in syntax, object model, and style.
   - A programmer SHOULD be able to write mostly ordinary Smalltalk-style code when beginning to use the system.

2. **Gradual typing**
   - Recorz SHALL support optional typing in the spirit of Strongtalk.
   - Typed and untyped code SHALL interoperate.
   - Unannotated code SHALL remain valid and idiomatic.

3. **Minimal abstraction growth**
   - Recorz SHOULD prefer a small number of orthogonal mechanisms over many specialized features.
   - New facilities for low-level programming and HDL SHALL, where practical, be implemented as restricted subsets of the main language rather than as wholly separate languages.

4. **Live introspection and debugging**
   - The system SHALL support strong reflection over objects, classes, methods, and execution contexts.
   - Debugging SHALL be considered a core use case, not an auxiliary facility.

5. **Malleability and self-hosting**
   - The system SHALL support development from within the system itself.
   - External bootstrap code SHALL be minimized and discarded as soon as practical.

6. **Bare-metal graphical computing**
   - Recorz SHALL support a graphical environment running on bare hardware with only a thin machine layer beneath it.
   - A conventional operating system SHALL NOT be a required long-term dependency.

7. **Low-level and hardware reach**
   - Recorz SHALL support implementation of VM internals, device drivers, machine-code generation, and FPGA-oriented hardware description.

8. **Scalability of architecture**
   - The same conceptual architecture SHALL support both tiny and much larger deployments.

9. **Future isolation**
   - The architecture SHALL leave room for future object-capability or sandboxing mechanisms.

---

## 3. Terminology

For the purposes of this specification:

- **Image**: A persistent snapshot of live system state including objects, classes, methods, and tool state.
- **Environment**: The live graphical user-facing system, including browsers, inspectors, debuggers, and workspaces.
- **Thin machine layer**: The minimal processor- and board-specific code needed to initialize hardware and support the Recorz runtime.
- **VM**: The virtual machine or runtime substrate responsible for object execution, memory, scheduling, and primitive dispatch.
- **General subset**: Ordinary dynamic Recorz code.
- **Typed subset**: General Recorz code enriched with optional type information.
- **Systems subset**: A restricted Recorz subset intended for VM, runtime, and driver implementation.
- **HDL subset**: A restricted Recorz subset intended for hardware elaboration and FPGA synthesis.
- **Capability domain**: A future isolated execution region governed by explicit authority-bearing references.

---

## 4. System Model

Recorz SHALL consist of the following major components:

1. **Language**
   - A Smalltalk-derived syntax and semantics.
   - Optional gradual typing.
   - Restricted subsets for systems programming and HDL.

2. **Runtime / VM**
   - Object memory.
   - Message send and method activation semantics.
   - Process scheduling.
   - Primitive interface.
   - Debugging and persistence hooks.

3. **Live graphical environment**
   - Browser, workspace, inspector, debugger, and related tools.

4. **Low-level toolchain**
   - Facilities for code generation, architecture-specific lowering, and HDL elaboration/synthesis.

These components SHALL be integrated into a coherent environment rather than exposed as unrelated products.

---

## 5. Language Core

### 5.1 Object Model

Recorz SHALL use a class-based object model in the Smalltalk tradition.

A conforming implementation SHALL provide:

- objects as instances of classes,
- class-defined instance variable structure,
- per-class method dictionaries,
- metaclasses for class-side behavior,
- message sending as the primary computation model,
- reflection over objects, classes, methods, and contexts.

Recorz SHALL NOT require a prototype-based model.

### 5.2 Message Semantics

Recorz SHALL preserve Smalltalk message precedence:

1. unary messages,
2. binary messages,
3. keyword messages.

Parentheses SHALL be available for explicit grouping.
Cascades SHALL be supported.
Blocks SHALL be supported.
Returns with `^` SHALL be supported.
Assignment with `:=` SHALL be supported.

### 5.3 Syntax Stability

The language syntax SHOULD remain as close as possible to Smalltalk / Strongtalk syntax.
A conforming design MUST justify any syntax additions against the following rules:

- the addition MUST serve a substantial need,
- the addition MUST preserve readability,
- the addition SHOULD avoid punctuation-heavy forms,
- the addition SHOULD not force ordinary users into a different style from classic Smalltalk.

### 5.4 Example Baseline

The following style SHALL remain valid and idiomatic:

```smalltalk
factorial
    self = 0 ifTrue: [ ^1 ].
    ^self * (self - 1) factorial
```

---

## 6. Type System

### 6.1 General Requirements

Recorz SHALL support gradual typing in the spirit of Strongtalk.

The type system SHALL satisfy the following:

- typing MUST be optional,
- untyped code MUST remain valid,
- typed and untyped code MUST interoperate,
- types SHOULD improve tooling, documentation, optimization, and reliability,
- omission of type annotations MUST NOT create a separate language mode.

### 6.2 Type Annotations

Recorz MAY support annotations on:

- method arguments,
- return types,
- local variables,
- instance variables,
- class variables,
- block parameters,
- generic parameters, if generics are adopted.

The exact surface syntax MAY evolve, but it SHALL remain visually compatible with Smalltalk conventions.

### 6.3 Type Categories

A conforming design SHOULD support at least the following type forms:

- nominal class types,
- protocol or interface-like constraints,
- optional or nilable types,
- union types where useful,
- inferred types where practical.

Parameterized or generic types MAY be supported if they can be expressed without excessive complexity.

### 6.4 Boundary Semantics

When typed code interacts with untyped code, the implementation SHALL define a consistent boundary policy. A conforming implementation MAY use one or more of the following:

- dynamic guards,
- inserted checks,
- inferred trust within proven regions,
- fallback to dynamic behavior.

Boundary rules SHALL be inspectable by tooling.

### 6.5 Types and Optimization

Type information MAY be used for:

- static analysis,
- earlier error reporting,
- specialized method versions,
- primitive selection,
- machine-code eligibility,
- layout and representation decisions in restricted subsets.

Types SHALL NOT be required for ordinary exploratory programming.

---

## 7. Reflection and Introspection

### 7.1 Reflective Requirements

Recorz SHALL support strong runtime reflection sufficient for Smalltalk-style live programming.

A conforming implementation SHALL provide reflective access to at least:

- object identity,
- class identity,
- method dictionaries,
- inheritance structure,
- source or source association,
- compiled method representation,
- active execution contexts,
- senders and implementors relationships,
- process state.

### 7.2 Execution Contexts

Execution contexts SHALL be available in a form sufficient to support:

- stack inspection,
- debugger stepping,
- sender traversal,
- receiver inspection,
- argument inspection,
- temporary variable inspection,
- restart or resume actions where supported.

### 7.3 Reflective Limits in Restricted Subsets

The systems subset and HDL subset MAY restrict reflection.
Any such restrictions MUST be explicit, documented, and visible to tools.

---

## 8. Execution Subsets

Recorz SHALL define a single language family with multiple execution subsets.
These subsets SHALL share the main conceptual model while imposing additional constraints for specialized purposes.

### 8.1 General Dynamic Subset

The general subset SHALL support:

- dynamic message sending,
- closures / blocks,
- reflective access,
- ordinary object allocation,
- debugger-visible execution,
- image-based development.

This SHALL be the default programming mode.

### 8.2 Typed Subset

The typed subset SHALL be the general subset plus optional type information and any checks or optimizations implied by it.
This subset SHALL remain semantically close to ordinary Recorz.

### 8.3 Systems Subset

The systems subset SHALL support implementation of:

- VM internals,
- memory managers,
- schedulers,
- interrupt support,
- device drivers,
- code generators,
- architecture support packages.

The systems subset MAY restrict:

- reflective features,
- unrestricted heap allocation,
- closure capture in critical regions,
- exception use,
- polymorphic dynamic dispatch,
- assumptions about object representation.

Such restrictions MUST be documented as part of the subset definition.

### 8.4 HDL Subset

The HDL subset SHALL support elaboration of hardware descriptions suitable for FPGA synthesis.
The HDL subset MUST be deterministic and statically elaborable.
It MAY restrict or forbid:

- unrestricted message dispatch during elaboration,
- general heap allocation,
- dynamic object graph mutation during synthesis,
- uncontrolled side effects,
- arbitrary runtime reflection.

The HDL subset SHOULD preserve as much of the main language style as practical.

---

## 9. Low-Level Programming Model

### 9.1 Purpose

Recorz SHALL support low-level programming sufficient to implement VM internals and hardware-facing software.

### 9.2 Required Capabilities

A conforming design for the systems subset SHALL support, either directly or through primitives:

- explicit integer widths,
- bit operations,
- address arithmetic,
- layout-sensitive data structures,
- raw memory access,
- calling convention control where required,
- explicit primitive invocation,
- interrupt-safe execution regions,
- predictable translation behavior.

### 9.3 Translation Model

The implementation SHOULD lower systems-subset code through an intermediate representation before emitting target code.
The translation pipeline SHOULD be inspectable.

### 9.4 Processor Targets

Recorz SHALL be designed to target at least one conventional processor architecture such as RISC-V.
The architecture SHALL also leave room for custom processors or soft cores.

---

## 10. HDL Model

### 10.1 Purpose

The HDL subset exists to enable hardware acceleration and hardware exploration from within the Recorz environment.

### 10.2 Required Hardware Concepts

A conforming HDL design SHALL support representation of at least:

- modules or components,
- ports,
- signals,
- registers,
- memories,
- clock domains,
- reset behavior,
- combinational logic,
- synchronous logic,
- finite state machines,
- fixed-width integers and bit vectors,
- composition of subcomponents.

### 10.3 Elaboration Rules

The HDL subset MUST be statically elaborable.
The elaboration process MUST produce a hardware intermediate representation or equivalent structure.
Elaboration SHALL be deterministic for the same source and configuration.

### 10.4 Simulation and Inspection

The environment SHOULD support:

- hardware simulation,
- signal inspection,
- assertions,
- cycle stepping,
- waveform or timeline views,
- cross-linking between source and generated hardware structure.

### 10.5 Synthesis Boundary

The system MAY delegate final synthesis and place-and-route to external toolchains, but the HDL authoring, inspection, and artifact management SHOULD remain integrated into Recorz.

---

## 11. Virtual Machine and Runtime

### 11.1 Minimal VM

Recorz SHALL admit a minimal VM implementation capable of:

- loading core objects and methods,
- allocating objects,
- performing message sends,
- executing methods and blocks,
- supporting a primitive interface,
- saving and loading an image,
- supporting a minimal event loop and display/input substrate.

### 11.2 VM Responsibilities

A conforming VM SHALL define behavior for:

- object memory,
- method activation,
- method lookup,
- process scheduling,
- garbage collection,
- primitive invocation,
- context creation and traversal,
- snapshot save/restore,
- interaction with the thin machine layer.

### 11.3 Garbage Collection

The implementation MAY choose its collection strategy, but the strategy SHOULD preserve the usability of a live, interactive, image-based environment.
Any low-level code generation or hardware interface that depends on object movement or pinning MUST define its memory interaction rules explicitly.

### 11.4 Primitive Interface

The primitive interface SHALL provide the controlled boundary between ordinary Recorz code and VM or hardware functionality.
Primitive failure behavior MUST be defined and debugger-visible.

---

## 12. Bare-Metal Platform Model

### 12.1 Platform Structure

The target platform structure SHALL be:

1. hardware,
2. thin machine layer,
3. Recorz VM/runtime,
4. Recorz image and graphical environment.

A conventional operating system SHALL NOT be required in the intended long-term deployment model.

### 12.2 Thin Machine Layer

The thin machine layer MAY include:

- processor and memory initialization,
- interrupt vector setup,
- display and input initialization,
- storage transport setup,
- timer setup,
- network device initialization,
- FPGA configuration entry points.

This layer SHOULD remain as small and replaceable as practical.

### 12.3 Device Exposure

Hardware services SHALL be surfaced into Recorz through explicit and inspectable objects or primitives.
Opaque or hidden global mechanisms SHOULD be avoided.

---

## 13. Image-Based Persistence

### 13.1 Primary Persistence Model

Recorz SHALL support image-based persistence.
The image SHALL be capable of storing:

- objects,
- classes,
- methods,
- compiler state where appropriate,
- user interface state,
- project state,
- development tools and metadata.

### 13.2 Source Interchange

Recorz SHALL also support source-based interchange and reconstruction.
A conforming design SHALL define a source representation capable of:

- describing classes and methods,
- reconstructing code into a fresh image,
- supporting archival and review,
- participating in bootstrap.

### 13.3 Dual Artifact Requirement

The system SHALL treat both image and source as important artifacts:

- the image for liveness and continuity,
- source for interchange, review, reproducibility, and bootstrap.

The architecture SHALL NOT depend exclusively on one to the exclusion of the other.

---

## 14. Bootstrapping and Self-Hosting

### 14.1 Bootstrap Constraint

Recorz MAY use external bootstrap code only to the extent necessary to create an initial working system.
That code SHALL be:

- minimal,
- clearly delimited,
- intended for removal or obsolescence.

### 14.2 Bootstrap Phases

A conforming project plan SHOULD proceed through stages such as:

1. minimal loader and VM seed,
2. core object and class system,
3. compiler and image services inside Recorz,
4. graphical tools inside Recorz,
5. migration of VM-supporting tools into Recorz,
6. minimization of external bootstrap dependencies.

### 14.3 Self-Hosting Goal

The long-term goal SHALL be that most ongoing development, including language evolution, occurs from within Recorz itself.

---

## 15. Development Environment

### 15.1 Required Tools

A conforming Recorz environment SHALL provide a graphical tool set including at least:

- system browser,
- workspace,
- inspector,
- debugger,
- transcript or log,
- senders/implementors tools,
- process browser,
- class and hierarchy browser.

### 15.2 Recommended Tools

The environment SHOULD also provide:

- object explorer,
- change browser or history tools,
- package/module browser,
- source/image synchronization tools,
- machine-code inspection tools,
- HDL browsing and simulation tools,
- processor and accelerator monitors.

### 15.3 Tool Principles

Tooling SHALL prioritize:

- introspection,
- explainability,
- immediate feedback,
- live modification,
- debugger integration,
- cross-linking between source, runtime state, and generated artifacts.

---

## 16. Multiprocessing and Accelerators

### 16.1 Architectural Requirement

Recorz SHALL be architected from the beginning with a path to multi-processor execution, even if the earliest implementation is single-processor.

### 16.2 Concurrency Direction

The preferred long-term direction SHOULD emphasize:

- explicit message-oriented concurrency,
- ownership boundaries,
- isolated active regions or domains,
- controlled sharing rather than unconstrained global mutation.

### 16.3 First Implementations

An initial implementation MAY use a simpler process model, but its APIs SHOULD avoid assumptions that would prevent later multi-processor evolution.

### 16.4 FPGA and Accelerator Integration

The runtime SHOULD treat FPGA cores and other accelerators as first-class compute resources.
A conforming design SHOULD support:

- loading a core,
- binding software objects to hardware interfaces,
- dispatching work,
- collecting results,
- observing health and status,
- unloading or reloading where safe.

### 16.5 Dynamic Reloading

If the target hardware permits reconfiguration, the system SHOULD support dynamic FPGA reload subject to explicit safety rules. These SHOULD include:

- quiescing dependent paths,
- compatibility checks,
- controlled rebind of software endpoints,
- rollback or failure reporting.

---

## 17. Security and Isolation

### 17.1 Near-Term Assumption

Early implementations MAY prioritize openness, malleability, and reflection over hard isolation.

### 17.2 Long-Term Requirement

The architecture SHALL preserve a path toward object-capability or sandbox-based protection so that user code need not possess unrestricted authority over the core system.

### 17.3 Capability-Oriented Direction

A future capability-based design SHOULD include:

- explicit authority-bearing references,
- restricted primitive access,
- isolated execution domains,
- limited reflective powers in sandboxed regions,
- controlled access to devices, storage, and accelerators.

### 17.4 Architectural Hooks

Early design work SHOULD avoid preventing future isolation. In particular, primitive APIs, scheduler boundaries, and device ownership models SHOULD be designed with future restriction in mind.

---

## 18. Non-Goals and Exclusions

A conforming Recorz design SHOULD avoid the following unless a compelling case is made:

- radical syntax departure from Smalltalk,
- mandatory static typing for ordinary code,
- opaque systems that prevent inspection,
- a separate unrelated systems language for VM work,
- a separate unrelated HDL language for hardware work,
- dependence on a conventional host OS as the end-state architecture.

---

## 19. Conformance Profiles

This specification MAY be implemented in stages. The following conformance profiles are RECOMMENDED.

### 19.1 Core Profile

A Core Profile implementation SHALL provide:

- the general dynamic subset,
- Smalltalk-like syntax and object model,
- image load/save,
- a minimal VM,
- browser, workspace, inspector, and debugger.

### 19.2 Typed Profile

A Typed Profile implementation SHALL provide Core Profile plus:

- gradual typing support,
- typed/untyped interoperation,
- type-aware tooling.

### 19.3 Systems Profile

A Systems Profile implementation SHALL provide Typed Profile or Core Profile plus:

- a defined systems subset,
- primitive and low-level facilities,
- target code generation path.

### 19.4 Hardware Profile

A Hardware Profile implementation SHALL provide Systems Profile plus:

- HDL subset,
- hardware elaboration model,
- simulation or inspection support,
- hardware artifact management.

### 19.5 Platform Profile

A Platform Profile implementation SHALL provide Hardware Profile plus:

- bare-metal deployment model,
- integrated graphics/input stack,
- accelerator lifecycle support,
- foundations for multiprocessing and future isolation.

---

## 20. Open Design Questions

The following questions are intentionally left open for later refinement, but any implementation SHALL answer them explicitly:

1. precise type annotation syntax,
2. exact type soundness goals at typed/untyped boundaries,
3. garbage collector strategy,
4. code generation intermediate representation,
5. systems-subset restrictions,
6. HDL elaboration object model,
7. concurrency domain model,
8. format and determinism requirements for source-based rebuild,
9. image packaging and module boundaries,
10. capability and sandbox authority semantics.

---

## 21. Summary of Normative Intent

Recorz is specified as a live, image-based, Smalltalk-descended system that:

- SHALL preserve the clarity and style of Smalltalk,
- SHALL support optional Strongtalk-like gradual typing,
- SHALL support deep reflection and debugging,
- SHALL support development from within itself,
- SHALL target bare-metal graphical computing,
- SHALL support low-level systems programming,
- SHALL support an HDL-oriented restricted subset for FPGA work,
- SHALL leave room for multiprocessing and future capability-based isolation,
- SHALL scale from a small VM to a richer personal computing platform.

Its unifying principle is that one coherent, inspectable system SHOULD span the full stack from ordinary objects to runtime internals to hardware-oriented artifacts.

