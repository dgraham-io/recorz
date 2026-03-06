# Recorz Standard Library and Core Class Specification
## Version 0.1 Draft

## Status of This Document

This document specifies the intended **standard library, core object hierarchy, foundational protocols, and essential class families** for Recorz. It complements the Recorz Formal Specification, Recorz Language Specification, Recorz VM and Object Memory Specification, and Recorz Compiler and Bootstrap Specification.

This is a design specification, not a frozen class reference manual. It defines the normative shape and responsibilities of the core library while leaving room for implementation staging and experimentation.

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** are to be interpreted as normative requirements.

---

## 1. Scope

This specification defines:

- the role of the standard library in Recorz,
- the core class hierarchy and foundational object protocols,
- essential collection, numeric, stream, and text abstractions,
- process and synchronization-facing library protocols,
- reflection and compiler-model library objects,
- source organization and protocol conventions,
- minimum class families required for a self-hosting and inspectable system.

This document does **not** fully define:

- the graphical widget framework,
- the final UI look and feel,
- exact networking protocol libraries,
- exact device-driver APIs,
- exact FPGA-control APIs,
- every optional application framework.

---

## 2. Design Objectives

A conforming Recorz core library design SHALL pursue the following objectives:

1. **Smalltalk clarity**
   - The core library SHALL preserve the elegance and coherence associated with Smalltalk class libraries.

2. **Minimal conceptual load**
   - The library SHALL prefer a small number of powerful abstractions over many overlapping facilities.

3. **Explorability**
   - Core classes and protocols SHALL be inspectable, browsable, and understandable by curious humans.

4. **Self-hosting support**
   - The library SHALL contain the class families needed to build browsers, debuggers, compiler tools, and a live environment.

5. **Scalability**
   - The same basic library model SHALL scale from a tiny educational system to a richer desktop-oriented platform.

6. **Protocol coherence**
   - Core behavior SHOULD be expressed through protocol families with regular naming and compositional behavior.

7. **Reflective friendliness**
   - The library SHALL cooperate with the reflective and debugging goals of Recorz.

---

## 3. Terminology

For the purposes of this specification:

- **Core class**: A class fundamental to ordinary language use or system comprehension.
- **Protocol**: A family of related messages expressing a behavior contract.
- **Collection**: An object that groups or organizes elements.
- **Sequenceable collection**: A collection with stable positional access.
- **Stream**: An object supporting sequential reading, writing, or transformation of elements.
- **Magnitude**: A value type that supports ordering.
- **Association**: A key/value pair object.
- **Compiler model object**: An object representing source, syntax, semantics, or compilation artifacts.
- **Tool model object**: An object representing browser, inspector, debugger, or related environment state.

---

## 4. Library Architecture Overview

### 4.1 General Requirement

A conforming Recorz implementation SHALL provide a standard library organized around a coherent class hierarchy and reusable protocols.

### 4.2 Core Library Families

The standard library SHALL include, at minimum, class families for:

- root object behavior,
- class and metaclass behavior,
- booleans and undefined object,
- numbers,
- characters and textual data,
- symbols,
- collections,
- streams,
- exceptions or exceptional conditions,
- processes and synchronization primitives,
- reflection objects,
- compiler and source-model objects,
- time or clock access,
- basic graphics or display-facing abstractions sufficient for a live environment.

### 4.3 Library Philosophy

The library SHOULD be built from ordinary objects and protocols wherever possible. Mechanisms that can be understood as objects SHOULD NOT be hidden in special-case machinery without compelling reason.

---

## 5. Root Object Protocol

### 5.1 Root Class Requirement

Recorz SHALL provide a distinguished root class, typically analogous to `Object`.
All ordinary classes SHALL inherit, directly or indirectly, from this root.

### 5.2 Root Protocol Responsibilities

The root object protocol SHALL include behavior for at least:

- identity and equality participation,
- class access,
- message fallback cooperation,
- printing or display representation,
- copying or copy protocol entry points,
- reflection entry points,
- error signaling entry points,
- inspection support.

### 5.3 Recommended Root Messages

A root-class protocol SHOULD include messages analogous in purpose to:

- `class`
- `==`
- `=`
- `hash`
- `printString`
- `isNil`
- `notNil`
- `yourself`
- `copy`
- `shallowCopy`
- `error:`
- `doesNotUnderstand:`

Exact selector spelling MAY vary only with strong justification. Smalltalk-compatible spellings are RECOMMENDED.

### 5.4 Minimality Rule

The root class SHOULD remain conceptually small. Higher-level convenience behavior SHOULD be introduced in subclasses or traits/mechanisms with comparable clarity if such mechanisms are adopted.

---

## 6. Nil, Booleans, and Fundamental Singletons

### 6.1 Undefined Object

The library SHALL provide a distinguished singleton object corresponding to `nil`.
Its class SHOULD be analogous to `UndefinedObject`.

### 6.2 Required Nil Behavior

`nil` SHALL support at least:

- identity as a singleton,
- default object absence semantics,
- printing support,
- participation in `ifNil:` / `ifNotNil:`-style protocols if adopted.

### 6.3 Boolean Objects

The library SHALL provide distinguished boolean singleton objects corresponding to `true` and `false`.
Their behavior SHALL support message-oriented control flow.

### 6.4 Boolean Protocols

Boolean protocol SHOULD include messages analogous to:

- `ifTrue:`
- `ifFalse:`
- `ifTrue:ifFalse:`
- `and:`
- `or:`
- `not`
- `xor:`

These protocols SHALL preserve lazy block-based semantics where applicable.

---

## 7. Class, Metaclass, and Behavioral Reflection

### 7.1 Class Objects

The library SHALL expose classes as ordinary inspectable objects.
Class protocol SHALL support at least:

- superclass access,
- class name access,
- instance variable metadata access,
- method dictionary access or equivalent behavior lookup inspection,
- subclass enumeration,
- instance creation entry points,
- protocol/category browsing metadata.

### 7.2 Metaclasses

Metaclasses SHALL be present and inspectable.
The class-side protocol model SHALL remain consistent with Smalltalk family expectations.

### 7.3 Behavior Family

A conforming design SHOULD include a behavior-related class family analogous in role to:

- `Behavior`
- `ClassDescription`
- `Class`
- `Metaclass`

Exact hierarchy details MAY vary, but the responsibilities MUST remain clear and inspectable.

### 7.4 Method and Selector Reflection

The library SHALL provide inspectable objects or equivalent access paths for:

- selectors,
- compiled methods,
- method source association,
- senders and implementors queries,
- protocol/category membership.

---

## 8. Equality, Hashing, and Copying Protocols

### 8.1 Identity and Equality

The library SHALL distinguish:

- identity equality,
- structural or semantic equality.

Identity semantics SHALL cooperate with the VM definition of object identity.

### 8.2 Hashing

Objects participating in hashed collections SHALL provide a `hash`-like protocol consistent with their equality semantics.

### 8.3 Copying

The library SHOULD define coherent copying protocols, including roles analogous to:

- shallow copy,
- deep or post-copy customization,
- immutable sharing where appropriate.

### 8.4 Collection Compatibility

Collection classes relying on equality or hashing SHALL document which comparison protocol they use.

---

## 9. Numeric Tower

### 9.1 General Requirement

The core library SHALL include a numeric class family sufficient for ordinary computation, compiler implementation, graphics support, and low-level reasoning.

### 9.2 Minimum Numeric Families

A conforming implementation SHALL support classes or equivalent numeric categories for:

- integers,
- small integers or immediate integers,
- large integers or arbitrary-precision integers if feasible,
- floating-point numbers,
- fractions or rational numbers, if adopted,
- scaled decimals if adopted.

### 9.3 Numeric Hierarchy

The library SHOULD include a hierarchy analogous in spirit to:

- `Number`
- `Magnitude`
- `Integer`
- `SmallInteger`
- `LargeInteger`
- `Float`
- optional `Fraction`

### 9.4 Numeric Protocols

Numeric protocol SHALL support at least:

- arithmetic,
- comparison,
- conversion,
- printing,
- iteration helpers where appropriate,
- bit operations for integer-like classes.

### 9.5 Magnitude Protocol

Orderable values SHOULD participate in a magnitude-style protocol including behavior analogous to:

- `<`
- `<=`
- `>`
- `>=`
- `between:and:`
- `min:`
- `max:`

### 9.6 Integer Bit Protocol

Integer classes SHOULD support bit operations needed for systems work, including behavior analogous to:

- bit and/or/xor,
- shifts,
- bit testing,
- explicit width conversion where supported.

---

## 10. Characters, Strings, and Symbols

### 10.1 Character Support

The library SHALL include a character class suitable for textual and lexical work.

### 10.2 String Support

The library SHALL include a string class supporting:

- indexed character access,
- concatenation,
- slicing or copying,
- search and matching basics,
- printing and conversion,
- stream interaction.

### 10.3 Symbol Support

The library SHALL include interned symbolic identifiers suitable for:

- selectors,
- global names,
- protocol names,
- metadata keys.

### 10.4 Textual Distinction

Strings and symbols SHALL remain distinct concepts even if internally optimized.
Selectors SHOULD be representable as symbols or equivalent symbolic identifiers.

### 10.5 Unicode and Encoding Policy

A conforming implementation SHOULD define a clear policy for Unicode support, string encoding, and normalization. Early implementations MAY be simpler, but the policy MUST be explicit.

---

## 11. Collection Framework

### 11.1 General Requirement

The standard library SHALL provide a coherent collection framework.

### 11.2 Minimum Collection Families

A conforming implementation SHALL include classes or equivalent abstractions for:

- general collections,
- sequenceable collections,
- arrays,
- ordered mutable collections,
- sets,
- dictionaries/maps,
- associations,
- strings as text-oriented sequenceable collections,
- byte arrays or binary collections if binary data support is included.

### 11.3 Recommended Hierarchy

A hierarchy analogous in spirit to the following is RECOMMENDED:

- `Collection`
- `SequenceableCollection`
- `Array`
- `OrderedCollection`
- `Set`
- `Dictionary`
- `Association`
- `ByteArray`
- `String`

### 11.4 Core Collection Protocols

Collections SHALL support protocol families for:

- enumeration,
- querying emptiness and size,
- membership testing,
- accumulation or transformation,
- conversion between collection forms where practical.

### 11.5 Enumeration Protocols

A collection protocol SHOULD include messages analogous to:

- `do:`
- `collect:`
- `select:`
- `reject:`
- `detect:ifNone:`
- `inject:into:`
- `anySatisfy:`
- `allSatisfy:`
- `size`
- `isEmpty`

### 11.6 Mutation Protocols

Mutable collections SHOULD support messages analogous to:

- `add:`
- `remove:`
- `remove:ifAbsent:`
- `at:put:`
- `addAll:`

### 11.7 Ordered Access

Sequenceable collections SHOULD support messages analogous to:

- `at:`
- `at:put:`
- `first`
- `last`
- `copyFrom:to:`
- `indexOf:ifAbsent:`

### 11.8 Dictionary Protocol

Dictionary-like collections SHALL support key/value access protocols analogous to:

- `at:`
- `at:ifAbsent:`
- `at:put:`
- `includesKey:`
- `keys`
- `values`
- `associations`

### 11.9 Collection Semantics

Collection classes SHALL document whether they rely on identity, equality, hashing, ordering, or insertion order.

---

## 12. Streams

### 12.1 General Requirement

The library SHALL provide stream abstractions for sequential access and transformation.

### 12.2 Stream Families

A conforming implementation SHOULD support:

- read streams,
- write streams,
- read/write streams,
- text streams,
- binary streams,
- collection-backed streams.

### 12.3 Stream Protocols

Stream protocol SHOULD include behavior analogous to:

- `next`
- `next:`
- `nextPut:`
- `nextPutAll:`
- `peek`
- `atEnd`
- `contents`
- `reset`
- `position`
- `position:`

### 12.4 Compiler and Tool Importance

Streams are REQUIRED not only for user code but also for compiler, source, serializer, and debugger tooling.

---

## 13. Exceptions and Error Signaling

### 13.1 Exceptional Condition Requirement

The library SHALL provide a coherent mechanism for signaling and handling exceptional conditions.

### 13.2 Design Flexibility

A conforming implementation MAY use an exception hierarchy, resumable conditions, or a similar mechanism, but it MUST integrate with the debugger and reflective environment.

### 13.3 Minimum Responsibilities

The error system SHALL support:

- signaling an error condition,
- attaching descriptive information,
- debugger entry,
- distinguishing ordinary errors from warnings or notifications if such categories are adopted.

### 13.4 Recommended Hierarchy

A class family analogous to the following is RECOMMENDED:

- `Exception`
- `Error`
- `Warning`
- `Notification`
- subset-specific exceptional conditions such as primitive failure or invalid non-local return.

### 13.5 Debugger Cooperation

Exceptional conditions SHALL be representable and inspectable as ordinary objects to the extent practical.

---

## 14. Processes, Scheduling, and Synchronization Protocols

### 14.1 Process Objects

The library SHALL expose process abstractions as ordinary inspectable objects.

### 14.2 Minimum Process Protocol

A process protocol SHOULD include behavior analogous to:

- `resume`
- `suspend`
- `terminate`
- `priority`
- `priority:`
- `isSuspended`
- access to active context or execution state where policy allows.

### 14.3 Synchronization Objects

The library SHOULD include synchronization abstractions such as:

- semaphores,
- mutual exclusion objects,
- signals or conditions,
- queues/mailboxes if the concurrency model evolves in that direction.

### 14.4 Future-Proofing Rule

Even if the first implementation is simple, the library SHOULD avoid locking the platform into assumptions that prevent later isolated domains or message-oriented multiprocessing.

---

## 15. Time, Clocks, and Scheduling Support

### 15.1 Time Access

The standard library SHOULD include objects or protocols for:

- time-of-day access,
- monotonic timing,
- delays or timers,
- timestamp formatting.

### 15.2 Delay/Timer Objects

A live environment SHOULD provide delay or timer abstractions usable by processes and UI/event systems.

### 15.3 Determinism Note

Libraries touching wall-clock time SHOULD make the distinction between real time and logical/simulation time explicit where relevant.

---

## 16. Reflection and Context Objects

### 16.1 Reflective Object Families

The library SHALL expose objects or equivalent protocols for reflective access to:

- classes,
- metaclasses,
- compiled methods,
- selectors,
- contexts,
- processes,
- source references.

### 16.2 Context Protocol

Context-like objects SHOULD support at least:

- sender access,
- receiver access,
- method access,
- temporary and argument inspection,
- program counter or source-location correlation where practical.

### 16.3 Introspection Protocols

The environment SHOULD provide library support for operations analogous to:

- implementors of a selector,
- senders of a selector,
- subclass enumeration,
- instance variable metadata queries,
- source lookup.

### 16.4 Safety Boundary

Restricted subsets MAY reduce reflective availability, but the general library model SHALL assume strong reflection.

---

## 17. Compiler and Source Model Classes

### 17.1 Requirement

A self-hosting Recorz system SHALL include classes or equivalent object families representing compiler and source artifacts.

### 17.2 Recommended Compiler Model Families

The core library SHOULD include objects for:

- source files or source chunks,
- tokens,
- AST nodes,
- semantic bindings,
- type descriptors,
- compiler diagnostics,
- IR nodes,
- build manifests.

### 17.3 Tooling Use

These classes SHALL support browsers, inspectors, compiler tools, and self-understanding of the language implementation.

---

## 18. Source and Code Organization Support

### 18.1 Protocol Categories

The library and environment SHOULD support organizing methods into protocols or categories for browsing and comprehension.

### 18.2 Package or Module Metadata

A conforming implementation SHOULD support source provenance through packages, modules, or equivalent organizational objects.

### 18.3 Extension Methods

If extension methods are supported, their provenance MUST remain visible in library metadata and browsers.

---

## 19. Printing, Inspection, and Serialization Support

### 19.1 Printing Protocol

Core objects SHOULD provide readable printing behavior suitable for workspaces, inspectors, logs, and debuggers.

### 19.2 Inspection Protocol

Objects SHOULD cooperate with generic inspectors by exposing names and values of relevant slots, elements, or derived properties.

### 19.3 Serialization Support

The core library MAY provide object-level hooks for snapshot, source, or interchange-oriented serialization, but such hooks MUST remain compatible with VM-level image persistence rules.

---

## 20. Minimal Graphics-Adjacent Library Support

### 20.1 Requirement

Because Recorz aims to host a graphical live environment, the core library SHALL include basic abstractions sufficient for early graphics and display work.

### 20.2 Minimum Concepts

A minimal but conforming system SHOULD include object families for:

- points,
- rectangles or bounds,
- colors or pixel values,
- forms/bitmaps or display surfaces,
- display or canvas abstraction,
- input events at a basic level.

### 20.3 Early Simplicity

The first implementation MAY keep graphics primitives extremely small, but the object model SHOULD still be coherent and inspectable.

---

## 21. Binary Data and Low-Level Support Classes

### 21.1 Requirement

The library SHOULD include classes supporting low-level and systems-adjacent work, such as:

- byte arrays,
- word arrays or fixed-width storage objects where appropriate,
- external buffer handles,
- address-like or memory-view abstractions where safe.

### 21.2 Boundary Rule

These classes SHALL cooperate with the VM’s memory and primitive rules. They MUST NOT silently subvert garbage collection or object identity semantics.

### 21.3 Systems Subset Cooperation

Where the systems subset requires explicit-width representations, the library SHOULD provide suitable classes or metadata-bearing abstractions visible to tools.

---

## 22. Library Conformance Profiles

### 22.1 Minimal Core Profile

A minimal conforming standard library SHALL provide:

- root object behavior,
- class and metaclass access,
- nil and booleans,
- integers and basic numbers,
- characters, strings, and symbols,
- arrays and at least one general mutable collection,
- dictionaries or key/value storage,
- streams,
- exceptional condition signaling,
- processes and at least one synchronization object,
- reflection access sufficient for inspection and debugging.

### 22.2 Self-Hosting Profile

A self-hosting-capable library SHALL provide Minimal Core plus:

- compiler model classes,
- source organization metadata,
- richer collections and streams,
- printing and inspection protocols suitable for tools,
- enough graphics-adjacent support for browsers and debuggers.

### 22.3 Platform Profile

A broader platform-oriented library SHOULD add:

- richer numeric support,
- text and encoding tools,
- time and timer objects,
- binary and external-buffer support,
- graphics and event abstractions,
- accelerator and device descriptor classes where appropriate.

---

## 23. Recommended Initial Class Families

The following class names or close analogues are RECOMMENDED as an initial conceptual map:

- `Object`
- `Behavior`
- `ClassDescription`
- `Class`
- `Metaclass`
- `UndefinedObject`
- `Boolean`, `True`, `False`
- `Magnitude`
- `Number`
- `Integer`, `SmallInteger`, `LargeInteger`, `Float`
- `Character`
- `String`
- `Symbol`
- `Collection`
- `SequenceableCollection`
- `Array`
- `OrderedCollection`
- `Set`
- `Dictionary`
- `Association`
- `ByteArray`
- `Stream`, `ReadStream`, `WriteStream`, `ReadWriteStream`
- `Exception`, `Error`, `Warning`
- `Process`
- `Semaphore`
- `CompiledMethod`
- `Context`
- `Point`
- `Rectangle`
- `Form` or equivalent display-surface class

This list is RECOMMENDED for continuity and clarity, but exact names MAY vary if the same conceptual roles remain clearly defined.

---

## 24. Open Questions

The following questions remain open and SHALL be resolved by later revisions or implementation notes:

1. exact numeric tower scope in the first implementation,
2. whether fractions and scaled decimals are in the initial core,
3. exact Unicode and text model,
4. exact exception versus condition semantics,
5. precise process and synchronization protocol set,
6. live class-layout migration support in library tools,
7. exact graphics object hierarchy,
8. binary/external buffer API design,
9. packaging/module organization model,
10. which reflective queries belong in core classes versus tooling layers.

---

## 25. Summary of Normative Intent

The Recorz standard library and core class model are specified as a coherent, Smalltalk-like object system that:

- SHALL provide a clear root object protocol,
- SHALL expose classes, metaclasses, methods, contexts, and processes as inspectable concepts,
- SHALL include numbers, text, symbols, collections, streams, and errors as foundational class families,
- SHALL support compiler, debugger, and browser construction from within the system,
- SHALL scale from a tiny environment to a richer personal computing platform,
- SHALL prioritize elegance, regular protocols, and human comprehension.

Its purpose is to make the living Recorz system feel intellectually continuous from ordinary application code through compiler tools, debugger tools, and eventually low-level and graphical facilities.

