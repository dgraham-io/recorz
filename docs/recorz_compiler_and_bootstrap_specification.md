# Recorz Compiler and Bootstrap Specification
## Version 0.1 Draft

## Status of This Document

This document specifies the intended **compiler architecture, semantic analysis pipeline, subset lowering strategy, image construction flow, and bootstrap process** for Recorz. It complements the Recorz Formal Specification, Recorz Language Specification, and Recorz VM and Object Memory Specification.

This is a design specification, not a fixed implementation manual. It defines normative architectural intent while leaving room for staged implementation and experimentation.

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** are to be interpreted as normative requirements.

---

## 1. Scope

This specification defines:

- the responsibilities of the Recorz compiler toolchain,
- source parsing and front-end requirements,
- semantic analysis and binding rules at compiler level,
- gradual typing integration,
- lowering paths for the general, systems, and HDL subsets,
- compiled artifact expectations,
- image construction and source loading behavior,
- bootstrap phases from seed system to self-hosting environment.

This document does **not** fully define:

- the final bytecode set or machine IR,
- the exact snapshot binary format,
- the exact FPGA synthesis back-end,
- a fixed package manager design,
- final optimizer heuristics.

---

## 2. Design Objectives

A conforming Recorz compiler and bootstrap design SHALL pursue the following objectives:

1. **Preserve Smalltalk continuity**
   - Ordinary Smalltalk-style code SHALL compile without forcing programmers into a different mental model.

2. **Support live development**
   - Compilation SHALL function both incrementally inside a live image and in source-driven rebuild flows.

3. **Keep early implementation small**
   - The architecture SHALL admit a tiny initial compiler and loader.

4. **Scale toward self-hosting**
   - The initial compiler path SHALL evolve into a compiler largely written in Recorz itself.

5. **Integrate gradual typing without fragmentation**
   - Typed and untyped code SHALL pass through a coherent compiler pipeline.

6. **Support restricted subsets**
   - The same language family SHALL feed systems-subset and HDL-subset compilation through explicit subset checks and lowering paths.

7. **Prioritize inspectability**
   - Compiler products and intermediate representations SHOULD be inspectable from within the environment.

8. **Make bootstrap disposable**
   - Non-Recorz bootstrap code SHALL be kept minimal and SHALL be designed to become unnecessary for ongoing development.

---

## 3. Terminology

For the purposes of this specification:

- **Parser**: The component that transforms source text into syntax structures.
- **AST**: An abstract syntax tree representing parsed source.
- **Semantic analysis**: Binding, scope resolution, selector formation, subset checks, and related analysis.
- **Type analysis**: Optional checking or inference related to gradual typing.
- **IR**: Intermediate representation used for optimization or lowering.
- **General subset**: Ordinary dynamic Recorz code.
- **Systems subset**: Restricted code for VM and low-level implementation.
- **HDL subset**: Restricted code for elaboration into hardware representations.
- **Image build**: Construction or reconstruction of a live system image from source and/or seed artifacts.
- **Seed compiler**: The earliest compiler or translator used before self-hosting is achieved.
- **Self-hosting compiler**: A compiler implemented substantially within Recorz itself.

---

## 4. Compiler Architecture Overview

### 4.1 General Architecture

A conforming Recorz implementation SHALL provide a compiler pipeline capable of transforming source code into executable artifacts suitable for the VM.

The architecture SHOULD conceptually include the following stages:

1. lexical analysis,
2. parsing,
3. AST construction,
4. name and scope binding,
5. subset identification and restriction checks,
6. optional type analysis,
7. IR or executable-method generation,
8. compiled-method or equivalent artifact creation,
9. installation into a live image or packaging for image build.

### 4.2 Live and Batch Modes

The compiler SHALL support both:

- **incremental live compilation** inside a running image, and
- **source-driven build or rebuild** flows for bootstrapping or reconstruction.

### 4.3 Compiler Self-Representation

Compiler components SHOULD, when feasible, be represented as inspectable objects in the live system. This includes parsers, AST nodes, analyzers, and code generators where practical.

---

## 5. Source Model and Units of Compilation

### 5.1 Source Units

Recorz SHALL support source as persistent program text independent of any single image instance.

A source unit MAY be:

- a method chunk,
- a class definition,
- an extension definition,
- a package or module file,
- an image bootstrap script,
- another structured unit with equivalent semantics.

### 5.2 Stable Semantic Unit

The most stable semantic compilation unit SHALL be the **method**, together with the class or extension context in which it is defined.

### 5.3 Class and Package Metadata

Source representation SHOULD preserve enough metadata to reconstruct:

- class name,
- superclass,
- instance variable declarations,
- class variable declarations if used,
- pool or shared binding references if used,
- method protocol/category organization,
- package or module provenance.

### 5.4 Round-Trip Expectation

A conforming design SHOULD permit source to be loaded into a fresh image and reconstructed into semantically equivalent classes and methods.

---

## 6. Front-End Parsing Requirements

### 6.1 Lexical and Grammar Conformance

The compiler front end SHALL accept source conforming to the Recorz Language Specification.

### 6.2 AST Requirement

The parser SHALL produce a structured representation sufficient to preserve:

- source locations,
- selector structure,
- literal values,
- block nesting,
- return forms,
- assignment structure,
- method metadata,
- optional type annotations,
- optional pragmas or subset declarations.

### 6.3 Source Location Mapping

AST nodes SHOULD retain source spans or equivalent location metadata for error reporting, browsers, refactoring tools, and debugger/source mapping.

### 6.4 Error Recovery

Interactive compiler tools SHOULD provide partial parse recovery or useful syntax diagnostics suitable for a live programming environment.

---

## 7. Semantic Analysis

### 7.1 Binding

Semantic analysis SHALL resolve at least:

- method selector identity,
- argument bindings,
- temporary variable bindings,
- block parameter bindings,
- block-local temporary bindings,
- instance variable references,
- pseudo-variable usage,
- global or namespace bindings,
- `super` send meaning relative to defining class.

### 7.2 Scope Rules

The compiler SHALL enforce lexical scope rules consistent with the language specification.
Name resolution SHALL distinguish:

- lexical variables,
- instance variables,
- pseudo-variables,
- globals or namespace entries.

### 7.3 Selector Formation

The compiler SHALL normalize selector identity from unary, binary, and keyword method headers and sends.

### 7.4 Method Shape Validation

The compiler SHALL validate:

- selector and argument count agreement,
- temporary declaration legality,
- return placement rules,
- block parameter structure,
- subset-specific forbidden constructs where applicable.

### 7.5 Pragmas and Metadata

If pragmas or equivalent metadata are supported, the compiler SHALL preserve and expose them to later phases and tools.

---

## 8. Gradual Typing Integration

### 8.1 General Requirement

The compiler SHALL admit both typed and untyped programs through a unified compilation pipeline.

### 8.2 Type Information Sources

Type analysis MAY use:

- explicit annotations,
- local inference,
- class metadata,
- message protocol inference,
- flow-sensitive reasoning where practical,
- subset requirements.

### 8.3 Compiler Obligations

When type annotations are present, the compiler SHALL:

- parse and preserve them,
- attach them to relevant semantic entities,
- use them for checking or inference as defined by the implementation,
- make results visible to tools.

### 8.4 Typed and Untyped Interoperation

The compiler SHALL define how calls across typed and untyped boundaries are lowered. A conforming implementation MAY use:

- inserted guards,
- trust boundaries,
- wrapper generation,
- specialized versions with dynamic fallback.

### 8.5 Modes of Reporting

An implementation MUST define whether type findings are treated as:

- errors,
- warnings,
- advisory information,
- optimization hints,
- or combinations depending on build mode.

### 8.6 Type Information Persistence

Type information SHOULD be preserved in compiled artifacts or associated metadata sufficiently for tools, browsers, and later recompilation.

---

## 9. Subset Identification and Enforcement

### 9.1 Subset Model

The compiler SHALL recognize at least the following semantic classes of code:

- general dynamic subset,
- typed subset,
- systems subset,
- HDL subset.

### 9.2 Declaration of Intent

Subset membership MAY be inferred, declared explicitly through metadata, or both. The chosen mechanism SHALL be visible to tools.

### 9.3 Enforcement Requirement

If code is compiled under systems-subset or HDL-subset rules, the compiler SHALL reject constructs forbidden by that subset.

### 9.4 Mixed Systems

A conforming implementation MAY allow a general-subset method to call a systems-subset or primitive-backed method, subject to explicit boundary rules.

### 9.5 Tool Visibility

Subset classification and rejections SHOULD be directly visible in browsers and compiler diagnostics.

---

## 10. Intermediate Representations

### 10.1 IR Freedom

A conforming implementation MAY use one or more intermediate representations between AST and final executable artifacts.

### 10.2 Recommended IR Layers

A mature implementation SHOULD conceptually support:

1. a high-level semantic IR preserving message and block structure,
2. an optimization IR suitable for analysis and specialization,
3. a low-level IR or bytecode/native form suitable for VM execution,
4. specialized IR branches for systems-subset and HDL-subset lowering.

### 10.3 Inspectability

IR forms SHOULD be inspectable in the live environment, at least in developer tooling, to support comprehension and debugging.

### 10.4 Stability Rule

The language specification SHALL NOT depend on exposing any specific IR to ordinary programmers.

---

## 11. General-Subset Lowering

### 11.1 Goal

General-subset lowering SHALL preserve dynamic message-oriented semantics.

### 11.2 Required Semantic Preservation

Lowering SHALL preserve:

- message send order,
- block creation and capture,
- non-local return semantics,
- `super` send semantics,
- reflective hooks required by the language,
- fallback behavior for message lookup and primitive failure.

### 11.3 Target Form

General-subset code MAY be lowered to:

- bytecodes,
- interpreter-oriented instruction forms,
- native code with deoptimization metadata,
- equivalent execution artifacts.

### 11.4 Incremental Recompilation

The compiler SHOULD support recompiling and reinstalling individual methods without requiring whole-program rebuild.

---

## 12. Systems-Subset Lowering

### 12.1 Goal

Systems-subset lowering SHALL generate artifacts predictable enough for low-level runtime, VM, and driver use.

### 12.2 Compiler Checks

The systems-subset compiler SHALL validate constraints such as:

- forbidden reflective access,
- forbidden non-local return,
- controlled allocation,
- explicit-width arithmetic requirements,
- layout-sensitive representation constraints,
- restricted dynamic dispatch where required.

### 12.3 Translation Targets

Systems-subset code MAY lower to:

- specialized bytecodes,
- VM-recognized primitive forms,
- C-like intermediate forms,
- machine-oriented IR,
- direct target machine code,
- another predictable low-level form.

### 12.4 Representation Metadata

If systems-subset code depends on layout or width guarantees, the compiler SHALL make those guarantees explicit in metadata or diagnostics.

### 12.5 Debug Boundary

Even where systems-subset lowering sacrifices some reflective power, the compiler SHOULD preserve enough metadata for source correlation and developer diagnostics.

---

## 13. HDL-Subset Lowering

### 13.1 Goal

HDL-subset lowering SHALL translate eligible Recorz source into a deterministic hardware elaboration representation.

### 13.2 Compiler Checks

The HDL compiler or elaborator SHALL reject constructs without coherent hardware meaning, including where applicable:

- unrestricted allocation,
- general runtime reflection,
- non-local returns,
- unconstrained recursion,
- elaboration-time dependence on opaque dynamic dispatch,
- side effects lacking a hardware interpretation.

### 13.3 Elaboration Output

HDL-subset compilation SHALL produce at least one of the following:

- a hardware graph,
- a register-transfer representation,
- a netlist-like intermediate form,
- a simulation model tied to source structure,
- or another equivalent hardware IR.

### 13.4 Metadata Preservation

The HDL pipeline SHOULD preserve mappings between source constructs and elaborated signals, registers, ports, states, and component instances.

### 13.5 Simulation Integration

The compiler SHOULD support or cooperate with a simulation mode suitable for signal inspection and source-level debugging.

---

## 14. Compiled Artifact Installation

### 14.1 Method Installation

The compiler SHALL support installation of compiled methods into live classes or extension contexts.

### 14.2 Atomicity Expectation

Method installation SHOULD be atomic at the granularity needed to avoid leaving a class in an inconsistent partially updated state.

### 14.3 Replacement Behavior

When a method is replaced, the runtime and tools SHALL observe well-defined behavior. Active frames MAY continue using the old version or transition by implementation policy, but the policy MUST be explicit.

### 14.4 Metadata Association

Installed artifacts SHOULD retain or reference:

- source association,
- protocol/category,
- package/module provenance,
- subset classification,
- type metadata,
- primitive metadata where applicable.

---

## 15. Compiler Objects in the Live System

### 15.1 Introspective Tooling Goal

The Recorz environment SHOULD expose compiler entities as ordinary inspectable objects where practical.

### 15.2 Candidate Objects

Such objects MAY include:

- parser instances,
- token streams,
- AST nodes,
- semantic binding objects,
- type descriptors,
- IR nodes,
- code generator objects,
- diagnostics objects,
- build manifests.

### 15.3 Educational Value

Compiler introspection is RECOMMENDED as a first-class feature because Recorz aims to support learning and comprehension, not merely code production.

---

## 16. Image Construction from Source

### 16.1 Source-to-Image Requirement

The system SHALL support constructing or reconstructing an image from source-level artifacts plus whatever minimal seed is required.

### 16.2 Build Order Requirements

An image build process SHALL define ordering or dependency rules sufficient to establish:

- core object and class hierarchy,
- essential globals,
- compiler availability where needed,
- standard library load order,
- tool installation,
- package/module installation.

### 16.3 Bootstrap Class Availability

A bootstrap image builder MAY create a minimal pre-class or seed object structure using external code, but SHALL converge toward source-defined classes as early as practical.

### 16.4 Rebuild Integrity

The build process SHOULD detect or report:

- missing superclass definitions,
- cyclic dependency issues beyond what the object model permits,
- missing globals,
- invalid extension targets,
- incompatible subset declarations,
- primitive declarations lacking backing support.

---

## 17. Bootstrap Architecture

### 17.1 Bootstrap Principle

Recorz MAY begin with non-Recorz seed code, but that code SHALL be:

- minimal,
- deliberately temporary,
- constrained to what is necessary to load the first useful Recorz system.

### 17.2 Bootstrap Goal

The long-term goal SHALL be a system in which ongoing language, library, and tool evolution occurs predominantly from within Recorz.

### 17.3 Bootstrap Non-Goal

The project SHALL NOT depend permanently on a large external implementation if an in-system implementation can replace it.

---

## 18. Bootstrap Phases

### 18.1 Phase 0: Seed Loader

The first stage MAY consist of a tiny external runtime and loader capable of:

- creating a minimal object representation,
- loading a bootstrap image or object graph,
- installing a minimal compiler or parser core,
- entering a simple execution loop.

### 18.2 Phase 1: Minimal Language Core

The next stage SHOULD establish inside Recorz:

- basic classes and metaclasses,
- numbers, collections, strings, symbols,
- block execution semantics,
- method compilation for a core subset,
- primitive-backed essential operations.

### 18.3 Phase 2: Self-Describing Compiler Core

A subsequent stage SHOULD implement in Recorz itself:

- parser components,
- AST structures,
- semantic analyzers,
- compiled-method builders,
- source browsers.

### 18.4 Phase 3: Live Development Environment

The next stage SHOULD establish:

- browser,
- workspace,
- inspector,
- debugger,
- change tracking,
- source loading and filing mechanisms.

### 18.5 Phase 4: Self-Hosting Compiler

At this stage, the primary compiler used for ordinary development SHOULD be written substantially in Recorz.
External tools MAY remain for seed rebuild or validation, but SHALL not be the center of ongoing development.

### 18.6 Phase 5: Systems and HDL Expansion

Later bootstrap maturity SHOULD add:

- systems-subset compiler path,
- target lowering infrastructure,
- HDL elaboration path,
- image reconstruction from richer source manifests,
- accelerator-aware build tooling.

### 18.7 Phase 6: Bootstrap Minimization

The external bootstrap substrate SHOULD eventually shrink to:

- minimal hardware bring-up,
- minimal image loader,
- minimal recovery and rebuild tooling.

---

## 19. Self-Hosting Requirements

### 19.1 Compiler-in-Recorz Goal

A self-hosting implementation SHALL provide a compiler largely implemented in Recorz source.

### 19.2 Trust Chain

The build chain SHOULD define how the system trusts or validates:

- the seed compiler,
- the self-hosted compiler,
- compiler regeneration,
- image rebuild consistency.

### 19.3 Regeneration Path

The project SHOULD preserve a documented path by which a fresh system can be built from source plus the minimal seed substrate.

---

## 20. Incremental Development and Live Patching

### 20.1 Incremental Compilation

The compiler SHALL support method-at-a-time or similarly fine-grained compilation suitable for a live environment.

### 20.2 Live Object Evolution

The architecture SHOULD support controlled changes to:

- methods,
- classes,
- protocols/categories,
- selected class layouts,
- compiler components themselves.

### 20.3 Class Layout Evolution

If instance variable layout changes are supported live, the system SHALL define a migration policy for existing instances. If not yet supported, the system SHALL diagnose the limitation clearly.

### 20.4 Recovery from Compiler Failure

Because the compiler lives within the environment, the system SHOULD support recovering from broken compiler changes through:

- source history,
- alternate compiler entry points,
- safe-mode startup,
- recovery images,
- or equivalent fallback tools.

---

## 21. Diagnostics and Explainability

### 21.1 General Requirement

Compiler diagnostics SHALL prioritize clarity for humans.

### 21.2 Required Diagnostic Capabilities

The compiler SHOULD report:

- syntax errors,
- binding errors,
- selector mismatches,
- subset violations,
- primitive declaration issues,
- type findings,
- image build dependency failures,
- bootstrap phase failures.

### 21.3 Source Mapping

Diagnostics SHOULD include source locations and, where practical, related semantic context such as defining class and selector.

### 21.4 Introspection of Compiler Decisions

The system SHOULD allow developers to inspect important compiler decisions such as:

- inferred types,
- inserted guards,
- subset classification,
- lowered forms,
- method installation metadata.

---

## 22. Minimal Conformance Profile

A minimal conforming Recorz compiler and bootstrap implementation SHALL provide:

- parsing of the core language,
- semantic binding for methods and blocks,
- compiled-method generation for the general subset,
- installation into a live image or seed image,
- source-to-image loading for core classes,
- a documented seed bootstrap path,
- a plan for eventual self-hosting.

A fuller implementation SHOULD also provide:

- gradual typing analysis,
- subset enforcement,
- systems-subset lowering,
- HDL-subset lowering,
- inspectable AST/IR objects,
- compiler tools inside the live environment,
- documented image reconstruction from source.

---

## 23. Recommended Staging

### 23.1 Stage 1: Tiny Front End

The project SHOULD begin with:

- a simple parser,
- direct compiled-method generation,
- minimal source loading,
- bootstrap image creation.

### 23.2 Stage 2: Semantic and Tooling Growth

The next stage SHOULD add:

- richer diagnostics,
- source positions,
- compiler model objects,
- browser integration,
- more robust rebuild order.

### 23.3 Stage 3: Typing and IR

The next stage SHOULD add:

- optional type annotations,
- type analysis,
- high-level IR,
- specialization paths.

### 23.4 Stage 4: Low-Level and HDL Paths

The next stage SHOULD add:

- systems-subset checks,
- low-level lowering,
- HDL elaboration,
- simulation mappings,
- artifact provenance tracking.

### 23.5 Stage 5: Self-Hosted Maturity

The project SHOULD then emphasize:

- compiler self-hosting,
- recovery tooling,
- reproducible source-to-image workflows,
- minimal external dependencies.

---

## 24. Open Questions

The following questions remain open and SHALL be resolved by later revisions or implementation notes:

1. exact source package/file format,
2. exact AST class hierarchy,
3. exact type inference scope,
4. whether one IR or multiple IR layers are best,
5. degree of optimizer aggressiveness compatible with debugging,
6. exact subset declaration syntax,
7. exact image manifest format,
8. trust and validation strategy for self-host regeneration,
9. class layout migration mechanics,
10. failure recovery path when compiler objects are themselves broken.

---

## 25. Summary of Normative Intent

The Recorz compiler and bootstrap architecture are specified as a live, inspectable, gradually self-hosting system that:

- SHALL compile ordinary Smalltalk-style source without abandoning the language’s core identity,
- SHALL support incremental live compilation and source-driven image construction,
- SHALL integrate gradual typing without fragmenting the language,
- SHALL enforce systems and HDL subset restrictions where applicable,
- SHALL preserve compiler artifacts and decisions in forms visible to tools,
- SHALL evolve from a minimal seed into a substantially self-hosting implementation,
- SHALL keep non-Recorz bootstrap code minimal and intentionally temporary.

Its purpose is to let Recorz explain and rebuild itself, so that language, runtime, and tools become part of the same living system rather than externally maintained machinery.

