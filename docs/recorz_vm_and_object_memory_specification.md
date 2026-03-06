# Recorz VM and Object Memory Specification
## Version 0.1 Draft

## Status of This Document

This document specifies the intended **virtual machine, object memory, execution model, process model, primitive boundary, and persistence substrate** for Recorz. It complements the Recorz Formal Specification and the Recorz Language Specification.

This is a design specification, not a frozen implementation manual. It defines normative architectural intent while leaving room for staged implementation.

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** are to be interpreted as normative requirements.

---

## 1. Scope

This specification defines:

- the responsibilities of the Recorz virtual machine,
- the object memory model,
- execution contexts and method activation,
- message send and method lookup behavior at the VM boundary,
- compiled method representation requirements,
- primitive dispatch,
- process scheduling,
- snapshot and image persistence semantics,
- machine interface boundaries,
- support expectations for low-level code generation and future accelerator integration.

This document does **not** fully define:

- the source-language grammar,
- the user interface framework,
- exact device protocols,
- exact FPGA toolchain formats,
- a specific garbage collector algorithm,
- a fixed bytecode set or machine IR.

---

## 2. Design Objectives

A conforming Recorz VM design SHALL pursue the following objectives:

1. **Smalltalk-style liveness**
   - The VM SHALL support a live, image-based, inspectable system.

2. **Reflective comprehensibility**
   - Runtime state SHALL be observable enough to support browsers, inspectors, and a debugger.

3. **Minimal but scalable core**
   - The design SHALL admit a small educational implementation while scaling to a richer platform.

4. **Classical message-oriented execution**
   - Message sending and method activation SHALL remain central.

5. **Preservation of non-local control semantics**
   - The VM SHALL support general-subset block and return behavior in a Smalltalk-compatible way.

6. **Controlled low-level escape hatches**
   - The primitive boundary SHALL expose machine and VM capabilities without destroying language coherence.

7. **Image continuity with source recoverability**
   - The VM SHALL support snapshot persistence while cooperating with source-level reconstruction.

8. **Future architectural headroom**
   - The VM design SHALL leave room for multiprocessing, accelerator integration, and future capability isolation.

---

## 3. Terminology

For the purposes of this specification:

- **VM**: The execution substrate for Recorz object code.
- **Object memory**: The managed heap and associated metadata used to store objects.
- **Immediate object**: A value represented directly in a machine word or compact tagged form rather than as an allocated heap object.
- **Heap object**: An allocated object residing in managed object memory.
- **Class object**: An object describing instance structure and method behavior.
- **Method dictionary**: A mapping from selectors to compiled methods or method descriptors.
- **Compiled method**: A VM-level executable representation of a source method.
- **Context**: A reified or reifiable activation record.
- **Process**: A schedulable locus of execution.
- **Primitive**: A VM or machine operation exposed through method-level protocol.
- **Snapshot**: A persistent serialization of image state sufficient to resume execution or restore the object graph.
- **Thin machine layer**: The minimal board- and processor-specific substrate beneath the VM.

---

## 4. VM Architectural Role

### 4.1 General Role

The Recorz VM SHALL serve as the runtime substrate between the language semantics and the machine.

It SHALL be responsible for at least:

- object allocation,
- object representation,
- method lookup support,
- method activation,
- block activation,
- process scheduling,
- primitive dispatch,
- garbage collection,
- context creation or reification,
- snapshot save and restore,
- machine interface mediation.

### 4.2 Implementation Freedom

A conforming implementation MAY use:

- bytecode interpretation,
- threaded interpretation,
- direct AST interpretation,
- JIT compilation,
- ahead-of-time compilation for selected subsets,
- mixed execution strategies.

Regardless of implementation strategy, observable language semantics SHALL remain materially consistent with the language specification.

### 4.3 Debuggability Requirement

The VM SHALL preserve enough runtime structure to support live debugging. Internal optimization is permitted, but it SHALL NOT make the system fundamentally opaque.

---

## 5. Object Memory Model

### 5.1 General Requirement

Recorz SHALL provide a managed object memory capable of storing all ordinary language objects, classes, methods, contexts, processes, and persistent tool state.

### 5.2 Object Categories

A conforming implementation SHALL distinguish at least the following categories conceptually:

- immediate values,
- ordinary pointer-based heap objects,
- variable-sized indexable objects,
- byte-oriented objects,
- compiled methods,
- contexts or reifiable activation structures,
- class and metaclass objects.

The physical representation MAY vary.

### 5.3 Identity

Ordinary heap objects SHALL possess stable object identity for the purposes of language-level identity comparison.

If an implementation moves objects during garbage collection, identity semantics SHALL remain preserved.

### 5.4 Immediate Representations

An implementation MAY represent selected values as immediates, including but not limited to:

- small integers,
- characters,
- booleans,
- `nil`,
- selected compact tagged values.

If immediates are used, the VM SHALL preserve the illusion that these values are ordinary language objects supporting message sends and class identity.

### 5.5 Heap Object Header Requirements

A heap object representation SHALL include enough metadata to determine at least:

- class or format identity,
- size or layout category,
- GC-relevant information,
- identity preservation requirements where applicable.

The exact bit layout is implementation-defined.

### 5.6 Fixed and Variable Structure

The object memory SHALL support:

- fixed-field instance objects,
- variable-sized indexable objects,
- byte-oriented objects,
- mixed formats if needed for efficiency.

The language-visible class model SHALL remain independent of specific physical layout details except in restricted low-level subsets.

---

## 6. Classes, Metaclasses, and Behavior Objects

### 6.1 Class Responsibilities

A class object SHALL define or reference:

- its superclass,
- instance variable structure,
- method dictionary,
- class-side companion structure through metaclass relationship,
- metadata useful for browsing and debugging.

### 6.2 Metaclass Responsibilities

A metaclass SHALL support class-side method organization consistent with Smalltalk family semantics.

### 6.3 Method Dictionary

A method dictionary or semantically equivalent structure SHALL map selectors to executable behavior representations.

This mapping MUST be inspectable by tools in the general system.

### 6.4 Behavioral Extensibility

The VM SHALL support runtime method replacement and method addition in the general live environment, subject to implementation constraints.

Method replacement SHOULD cooperate with debugging and active execution as safely as possible.

---

## 7. Compiled Method Model

### 7.1 General Requirement

The VM SHALL define a compiled method representation as the executable form of a source-level method.

### 7.2 Required Logical Contents

A compiled method SHALL retain or reference enough information to support:

- selector association,
- defining class association,
- literal references,
- executable body representation,
- temporary and argument count information,
- source mapping or source association for debugging,
- primitive metadata where applicable.

### 7.3 Execution Representation

The executable body MAY be represented as:

- bytecodes,
- threaded instructions,
- native code with recoverable metadata,
- IR-backed executable objects,
- another equivalent representation.

### 7.4 Source Mapping

A conforming implementation SHALL preserve sufficient source association to support browsers and debuggers.
This MAY be direct source storage, source pointer association, or equivalent mapping metadata.

### 7.5 Literal Frame

A compiled method SHALL be able to reference literal objects such as:

- selectors,
- symbols,
- numbers,
- strings,
- class references,
- embedded block metadata,
- primitive descriptors.

---

## 8. Method Activation and Contexts

### 8.1 Activation Model

Method execution SHALL conceptually create an activation containing at least:

- receiver,
- selector or compiled method reference,
- arguments,
- temporaries,
- instruction pointer or equivalent execution state,
- sender relationship.

### 8.2 Context Exposure

The general system SHALL expose contexts sufficiently to support:

- `thisContext`,
- sender traversal,
- stack inspection,
- debugger stepping,
- restart or resume behavior as supported,
- temporary and argument inspection.

### 8.3 Reification Strategy

The implementation MAY choose:

- always-materialized contexts,
- lazily materialized contexts,
- optimized stack frames reified on demand,
- hybrid representations.

Regardless of internal strategy, observable semantics SHALL support reflective tooling.

### 8.4 Sender Chains

The VM SHALL preserve sender relationships in a form adequate for debugger traversal and non-local return semantics.

### 8.5 Block Activations

Block execution SHALL be represented in a manner sufficient to preserve:

- lexical capture,
- argument passing,
- association with its home method or home context as required,
- non-local return behavior in the general subset.

---

## 9. Message Send and Lookup Semantics

### 9.1 Ordinary Send

An ordinary message send SHALL:

1. evaluate the receiver,
2. evaluate arguments according to source-language order,
3. identify the selector,
4. perform method lookup beginning at the receiver’s class,
5. activate the located method or dispatch through an equivalent optimized path.

### 9.2 super Send

A `super` send SHALL preserve the ordinary receiver but begin method lookup at the superclass of the method’s defining class.

### 9.3 Lookup Failure

If lookup fails, the VM SHALL trigger a defined fallback behavior semantically equivalent to the language’s message-not-understood protocol.

### 9.4 Inline Caches and Optimization

A conforming implementation MAY employ:

- inline caches,
- polymorphic inline caches,
- monomorphic fast paths,
- selector hashing,
- speculative optimization,
- deoptimization.

Such optimization SHALL preserve debugger-visible semantics or provide a route to recover them.

---

## 10. Blocks and Non-Local Return Support

### 10.1 Block Closures

The VM SHALL support block objects as executable closures with access to captured lexical state.

### 10.2 Home Association

For general-subset semantics, a block SHALL retain enough association with its lexical home to support correct non-local returns.

### 10.3 Non-Local Return

When a block executes a return from within a method body’s lexical scope, the VM SHALL attempt a non-local return to the appropriate home activation.

### 10.4 Invalid Return Target

If the target activation is no longer valid, the VM SHALL signal a defined exceptional condition.

### 10.5 Restricted Subsets

The systems subset MAY weaken or forbid non-local return.
The HDL subset SHALL forbid general non-local return.
The VM SHALL allow restricted-subset compilers to reject unsupported control forms explicitly.

---

## 11. Primitive Interface

### 11.1 Role

The primitive interface is the controlled execution boundary between ordinary Recorz semantics and VM- or machine-level functionality.

### 11.2 Primitive Declaration

Methods MAY declare primitive behavior through pragma, metadata, or an equivalent descriptor mechanism.

### 11.3 Primitive Dispatch

A primitive dispatch mechanism SHALL support:

- identification of the primitive operation,
- marshaling of receiver and arguments,
- return of normal results,
- failure signaling or fallback.

### 11.4 Fallback Behavior

A primitive-backed method SHOULD be able to provide a fallback body executed if the primitive cannot complete successfully.

### 11.5 Primitive Metadata

A conforming implementation SHOULD preserve metadata sufficient for tools to display:

- primitive identity,
- expected argument shape,
- side-effect class,
- failure modes,
- subset or privilege restrictions where relevant.

### 11.6 Restricted Access

Some primitives MAY be reserved for VM, systems-subset, or future capability-governed code.
The architecture SHALL permit such restriction even if early implementations are permissive.

---

## 12. Garbage Collection and Memory Management

### 12.1 General Requirement

The VM SHALL provide automatic memory management for ordinary heap objects.

### 12.2 Algorithm Freedom

A conforming implementation MAY use:

- mark-sweep,
- copying collection,
- generational collection,
- incremental collection,
- concurrent collection,
- hybrids thereof.

### 12.3 Semantic Constraints

Regardless of chosen algorithm, the VM SHALL preserve:

- object identity semantics,
- reachability-based retention of live objects,
- validity of reflective operations,
- consistency of captured state used by active blocks and contexts.

### 12.4 Pinning and External References

If low-level or hardware-facing features require stable addresses, the VM SHALL define a policy for:

- pinning,
- external handles,
- unmanaged buffers,
- raw memory interfaces.

That policy MUST be explicit and MUST NOT silently violate GC invariants.

### 12.5 Finalization and Weak References

A conforming implementation MAY support finalization, weak references, ephemerons, or equivalent facilities. If provided, their ordering and reliability guarantees SHALL be documented.

---

## 13. Process Model and Scheduling

### 13.1 General Requirement

The VM SHALL support schedulable execution units called processes or an equivalent abstraction.

### 13.2 Minimal Capabilities

A minimal conforming implementation SHALL support:

- creation of a new process from an activation or block,
- suspension,
- resumption,
- termination,
- process inspection,
- debugger interaction with active processes.

### 13.3 Scheduling Model

The earliest implementation MAY use cooperative scheduling, priority scheduling, preemptive scheduling, or a hybrid.
The chosen model SHALL be explicit.

### 13.4 Process Introspection

The system SHALL support process inspection sufficient to view:

- active context,
- suspension reason if any,
- priority or scheduling metadata,
- process identity.

### 13.5 Synchronization

The VM SHOULD support synchronization objects such as semaphores, mutex-like constructs, signals, or message-based equivalents.
The exact choice MAY vary, but it SHALL be inspectable.

### 13.6 Future Multiprocessing Direction

The process model SHALL not preclude future evolution toward multiple processors, isolated domains, or accelerator-backed execution.
The architecture SHOULD minimize hard dependence on a single unrestricted global mutator model.

---

## 14. Snapshot and Image Persistence

### 14.1 General Requirement

The VM SHALL support image-based persistence through snapshots sufficient to restore system state.

### 14.2 Snapshot Contents

A snapshot SHALL preserve, subject to implementation policy:

- reachable object graph,
- classes and metaclasses,
- compiled methods or loadable method representations,
- process state sufficient for resumption or controlled restart,
- tool state,
- global bindings,
- other image-level metadata needed for a coherent restore.

### 14.3 Restore Semantics

A restored image SHALL resume with object identity relationships preserved within the restored graph.
The restore mechanism MAY resume active processes directly or MAY restart them in a controlled way if the policy is explicit.

### 14.4 External Resource Boundaries

The snapshot format SHALL define a policy for external resources such as:

- device handles,
- network connections,
- framebuffer mappings,
- accelerator bindings,
- file streams.

Such resources MAY require rebinding on restore, but the policy MUST be explicit and tool-visible.

### 14.5 Determinism and Source Reconstruction

The snapshot system SHOULD cooperate with a source-based reconstruction path. The VM need not guarantee bitwise-deterministic snapshots, but it SHOULD preserve enough metadata to support meaningful source/image round-tripping.

---

## 15. Machine Interface and Thin Machine Layer

### 15.1 General Role

The VM SHALL execute above a thin machine layer providing the minimum platform-specific services needed to initialize hardware and support the runtime.

### 15.2 Responsibilities of the Thin Machine Layer

The thin machine layer MAY include:

- processor initialization,
- memory map discovery,
- interrupt vector setup,
- timer initialization,
- display bootstrap,
- keyboard/pointer initialization,
- storage transport initialization,
- network transport initialization,
- FPGA configuration entry points.

### 15.3 Boundary Principle

Machine-specific logic SHOULD remain below the VM or behind explicit primitives and objects. Platform-dependent code SHOULD be narrow, replaceable, and inspectable.

### 15.4 No Conventional OS Requirement

The long-term Recorz platform SHALL NOT require a conventional general-purpose host operating system beneath the VM.

---

## 16. Bytecode, IR, and Native Code Strategy

### 16.1 Representation Freedom

A conforming implementation MAY use bytecode, IR, or native code execution.
No single execution format is required by this specification.

### 16.2 Minimal Recommendation

A small initial implementation SHOULD favor a compact bytecode or similarly simple executable representation to reduce bootstrap complexity.

### 16.3 Optimization Recommendation

More advanced implementations MAY add:

- JIT compilation,
- adaptive optimization,
- native code caches,
- AOT compilation for systems-subset artifacts,
- hardware-targeted lowering passes.

### 16.4 Debug Mapping Requirement

If optimized or native execution is used, the implementation SHALL preserve or reconstruct enough metadata to support:

- stack inspection,
- source association,
- breakpoint placement,
- stepping,
- deoptimization or equivalent debugger handoff where necessary.

---

## 17. Systems Subset Support in the VM

### 17.1 General Requirement

The VM SHALL provide a path for restricted low-level code to interact with machine representations more directly than ordinary dynamic code.

### 17.2 Required Support Areas

The architecture SHOULD support the systems subset in areas including:

- explicit-width integers,
- layout-sensitive structures,
- raw memory access,
- controlled pointer-like references,
- predictable calling conventions,
- target-specific code emission,
- interrupt-safe regions.

### 17.3 Subset Separation

The VM SHOULD make clear which compiled artifacts belong to:

- the general dynamic subset,
- typed optimized subset,
- systems subset,
- HDL elaboration tools.

### 17.4 Reflective Limits

When systems-subset execution omits general reflective features for performance or predictability, the VM SHALL make those omissions explicit in metadata and tools.

---

## 18. Accelerator and FPGA Integration Hooks

### 18.1 Architectural Goal

The VM and runtime SHALL leave room for FPGA accelerators and multiple processors as first-class execution resources.

### 18.2 Runtime Object Model for Accelerators

The system SHOULD represent accelerator resources through ordinary inspectable objects or descriptors that expose at least:

- accelerator identity,
- configuration state,
- version or bitstream metadata,
- software binding points,
- execution status,
- lifecycle operations.

### 18.3 Dynamic Reload Support

If the platform supports dynamic FPGA reconfiguration, the VM/runtime architecture SHOULD support:

- quiescing dependent software regions,
- compatibility checking,
- rebinding endpoints,
- failure reporting,
- controlled resume.

### 18.4 Snapshot Interaction

Snapshot policy SHALL define how accelerator state is treated. In general, live device state MAY require explicit reattachment rather than raw persistence.

---

## 19. Capability and Isolation Hooks

### 19.1 Future Requirement

Although early systems MAY be permissive, the VM architecture SHALL leave room for future isolation or capability-based control.

### 19.2 Relevant VM Boundaries

The following areas SHOULD be designed to admit future restriction:

- primitive access,
- process creation and authority,
- raw memory access,
- device ownership,
- snapshot authority,
- method installation and mutation privileges,
- cross-domain object references.

### 19.3 Administrative / Trusted Execution Distinction

A future implementation MAY distinguish trusted core execution from less-trusted application execution. The VM SHALL avoid assumptions that make such separation impossible.

---

## 20. Error and Exceptional Conditions

### 20.1 Lookup Failure

Message lookup failure SHALL trigger the language-defined missing-message path.

### 20.2 Primitive Failure

Primitive failure SHALL be explicit and diagnosable.

### 20.3 Invalid Non-Local Return

An attempt to return to an invalid home activation SHALL raise a defined exceptional condition.

### 20.4 Snapshot Failure

Snapshot save or restore failure SHALL produce diagnostics sufficient for operator understanding.

### 20.5 Subset Misuse

If a restricted subset attempts to use disallowed runtime facilities, the relevant compiler, loader, or runtime boundary SHALL reject or trap the operation clearly.

---

## 21. Minimal VM Conformance Profile

A minimal conforming Recorz VM SHALL provide:

- managed object memory,
- class-based object representation,
- method dictionaries or equivalent behavior lookup structure,
- compiled method execution,
- contexts sufficient for debugging,
- support for `thisContext` in the general subset,
- blocks and non-local return in the general subset,
- primitive dispatch,
- automatic memory management,
- process creation and inspection,
- snapshot save/restore,
- a thin machine interface for display/input/event bootstrap.

---

## 22. Recommended Staging

### 22.1 Stage 1: Educational Core

An initial VM SHOULD implement:

- simple object headers,
- small integer immediates,
- bytecode or equivalent interpreter,
- materialized contexts,
- basic process scheduler,
- simple snapshot support.

### 22.2 Stage 2: Self-Hosting Runtime

A second stage SHOULD add:

- stronger method metadata,
- better source association,
- improved process tools,
- generational or otherwise more efficient GC,
- richer primitive surface.

### 22.3 Stage 3: Systems and Optimization

A later stage SHOULD add:

- systems-subset support,
- target lowering infrastructure,
- inline caches and/or JIT,
- pinning or external buffer policy,
- device and interrupt framework.

### 22.4 Stage 4: Multiprocessor and Accelerator Evolution

A later stage SHOULD add:

- isolated execution domains or equivalent concurrency boundaries,
- accelerator resource model,
- controlled FPGA lifecycle integration,
- early capability-aware privilege boundaries.

---

## 23. Open VM Questions

The following questions remain open and SHALL be resolved by later revisions or implementation notes:

1. exact object header format,
2. exact immediate tagging strategy,
3. compiled method binary layout,
4. bytecode set or IR choice,
5. context reification strategy,
6. process scheduling policy,
7. garbage collector algorithm and barriers,
8. snapshot binary format,
9. stable external handle design,
10. privilege model for primitives and mutation.

---

## 24. Summary of Normative Intent

The Recorz VM and object memory are specified as a reflective, image-oriented runtime that:

- SHALL support classical Smalltalk-style message execution,
- SHALL preserve contexts and debugger-visible control flow,
- SHALL support blocks and non-local returns in the general subset,
- SHALL provide automatic memory management and snapshot persistence,
- SHALL expose a controlled primitive boundary for low-level work,
- SHALL allow a small initial implementation,
- SHALL leave architectural room for systems programming, accelerators, multiprocessing, and future capability isolation.

Its purpose is to provide a simple but durable execution core for a living system that spans ordinary object programming, runtime implementation, and eventually hardware-adjacent computation.

