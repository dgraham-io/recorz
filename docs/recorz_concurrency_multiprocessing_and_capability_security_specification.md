# Recorz Concurrency, Multiprocessing, and Capability Security Specification
## Version 0.1 Draft

## Status of This Document

This document specifies the intended **process model, synchronization model, multiprocessing direction, accelerator coordination model, isolation architecture, and capability-security design** for Recorz. It complements the Recorz Formal Specification, Recorz Language Specification, Recorz VM and Object Memory Specification, Recorz Compiler and Bootstrap Specification, and Recorz Standard Library and Core Class Specification.

This is a design specification, not a finalized kernel or runtime manual. It defines normative architectural intent while leaving room for staged implementation and experimentation.

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** are to be interpreted as normative requirements.

---

## 1. Scope

This specification defines:

- the general concurrency model of Recorz,
- the role and semantics of processes,
- synchronization and coordination mechanisms,
- the architectural direction for multiprocessing,
- the treatment of accelerators and FPGA resources as execution resources,
- isolation boundaries for future execution domains,
- an object-capability-oriented security direction,
- privilege and authority boundaries relevant to primitives, devices, and system mutation.

This document does **not** fully define:

- the final scheduler algorithm,
- the final low-level IPC transport,
- the exact memory-consistency model for multi-core execution,
- the exact privilege UI shown to users,
- the final formal semantics of capability delegation,
- the final sandbox serialization format.

---

## 2. Design Objectives

A conforming Recorz concurrency and security design SHALL pursue the following objectives:

1. **Preserve a Smalltalk-like live process experience early**
   - Early Recorz SHALL support inspectable live processes and debugger-friendly control flow.

2. **Avoid architectural dead ends**
   - The earliest process model SHALL NOT force the system into permanent single-core global-heap assumptions.

3. **Favor comprehensibility**
   - Concurrency mechanisms SHOULD be visible, inspectable, and explainable to humans.

4. **Prefer explicit coordination over hidden shared mutation**
   - The long-term architecture SHOULD emphasize message-oriented coordination and ownership boundaries.

5. **Support heterogeneous execution**
   - CPUs, co-processors, and FPGA accelerators SHOULD be modelable as coordinated compute resources.

6. **Permit future isolation without redesigning the entire platform**
   - Capability and sandbox boundaries SHALL be anticipated in primitive APIs, process APIs, and device ownership models.

7. **Retain liveness where possible**
   - Even when isolation grows stronger, the system SHOULD remain inspectable and debuggable through authorized tools.

---

## 3. Terminology

For the purposes of this specification:

- **Process**: A schedulable locus of execution.
- **Scheduler**: The runtime component that determines which process executes.
- **Synchronization object**: An object used to coordinate ordering, waiting, wakeup, or ownership.
- **Domain**: An execution region with a defined authority and resource boundary.
- **Object space**: A region of objects with controlled reference and mutation semantics.
- **Capability**: An authority-bearing reference that both names an object/service and grants permission to use it.
- **Ambient authority**: Permission obtained implicitly from global reachability rather than explicit capability possession.
- **Trusted core**: The privileged portion of the system allowed to perform sensitive runtime or device operations.
- **Sandboxed code**: Code running with restricted authority.
- **Endpoint**: A message, queue, mailbox, or channel object used for communication across concurrency or domain boundaries.
- **Accelerator**: A non-CPU execution resource such as an FPGA core or co-processor.
- **Quiescence**: A state in which a process group or accelerator binding is paused or drained to permit safe transition.

---

## 4. Concurrency Model Overview

### 4.1 General Requirement

Recorz SHALL provide a concurrency model centered on inspectable processes.

### 4.2 Early System Model

An early implementation MAY begin with a classic Smalltalk-like process model using a shared object memory and a scheduler.

### 4.3 Long-Term Direction

The long-term architecture SHOULD evolve toward:

- message-oriented coordination,
- explicit ownership boundaries,
- isolated execution domains or object spaces,
- controlled sharing rather than unrestricted shared mutable access.

### 4.4 Compatibility Principle

Early simple process support SHALL remain a valid subset of the broader architecture. Later isolation and multiprocessing SHOULD extend the model rather than discard it outright.

---

## 5. Process Semantics

### 5.1 Process as Inspectable Object

A process SHALL be represented as an inspectable object or an object with equivalent reflective access.

### 5.2 Required Process State

A process representation SHALL expose or make available through authorized reflection at least:

- identity,
- scheduling status,
- active context or equivalent execution state,
- priority or scheduling metadata,
- owning domain or authority context if applicable,
- waiting reason if suspended,
- termination state if completed.

### 5.3 Minimal Process Operations

The runtime and library SHALL support operations equivalent in role to:

- create or fork,
- resume,
- suspend,
- yield,
- terminate,
- inspect,
- debug.

### 5.4 Process Creation

A process MAY be created from a block, activation, or equivalent executable object. The creation API SHALL define:

- captured lexical state behavior,
- initial priority or scheduling class,
- initial domain membership,
- inherited or assigned capability context where applicable.

### 5.5 Termination Semantics

The runtime SHALL define what becomes of:

- waiting dependents,
- owned synchronization objects,
- outstanding communication endpoints,
- accelerator requests,
- exceptional state,

when a process terminates normally or abnormally.

---

## 6. Scheduling Model

### 6.1 General Requirement

The runtime SHALL define a scheduler model for processes.

### 6.2 Allowed Early Strategies

A conforming implementation MAY use:

- cooperative scheduling,
- priority-based scheduling,
- preemptive scheduling,
- hybrid scheduling.

The chosen model MUST be explicit.

### 6.3 Introspection Requirement

Scheduler-visible metadata SHOULD be inspectable so that the environment can show:

- runnable processes,
- waiting processes,
- priorities,
- wakeup reasons,
- starvation or blockage indicators if available.

### 6.4 Fairness and Responsiveness

A mature implementation SHOULD aim to balance:

- UI responsiveness,
- debugger control,
- background task progress,
- fairness among runnable processes.

### 6.5 Scheduler Evolution

The scheduler architecture SHOULD permit later scaling to multiple cores or domains without requiring all user-visible process abstractions to change radically.

---

## 7. Synchronization and Coordination

### 7.1 General Requirement

Recorz SHALL provide synchronization mechanisms suitable for live interactive programming and later multiprocessing evolution.

### 7.2 Minimum Mechanisms

An early implementation SHALL provide at least one waiting/wakeup coordination object, such as a semaphore or equivalent.

### 7.3 Recommended Coordination Objects

A mature implementation SHOULD include objects analogous in role to:

- semaphores,
- mutexes or mutual exclusion objects,
- condition variables or signals,
- queues,
- mailboxes,
- channels or endpoints.

### 7.4 Inspectability Requirement

Synchronization objects SHOULD be inspectable so the system can explain:

- current owner,
- waiting processes,
- queue length,
- wakeup state,
- blocked communication paths.

### 7.5 Avoiding Hidden Coupling

The library and runtime SHOULD discourage hidden shared-state coordination patterns that are difficult to inspect or reason about.

---

## 8. Shared Memory and Ownership Direction

### 8.1 Early Shared Heap Allowance

Early implementations MAY use a shared object heap for all processes.

### 8.2 Architectural Warning

The architecture SHALL NOT assume that unrestricted shared mutable access is the permanent model for all future scaling.

### 8.3 Long-Term Ownership Direction

The preferred long-term direction SHOULD include at least some notion of:

- domain-local ownership,
- controlled cross-domain references,
- explicit transfer or sharing rules,
- message-passing boundaries.

### 8.4 Mutable Sharing Policy

A future implementation MAY allow shared immutable objects broadly while restricting mutable object sharing across domain boundaries.

### 8.5 Tool Support

Tools SHOULD be able to explain where an object resides, who owns it, and what sharing or transfer rules apply to it.

---

## 9. Multiprocessing Architecture Direction

### 9.1 General Requirement

Recorz SHALL be architected with a path to multiple-processor execution.

### 9.2 Preferred Model

The preferred long-term model SHOULD be based on one or more of:

- isolated domains,
- actors or actor-like regions,
- vats or event-loop domains,
- object spaces with explicit communication,
- controlled shared-memory regions for special cases.

### 9.3 Reason for Preference

This preference exists because classic unrestricted shared-memory concurrency scales poorly for comprehension, debugging, and security.

### 9.4 Transitional Strategy

A conforming implementation MAY move in stages:

1. single scheduler on one processor,
2. multiple schedulers or domain-local run queues,
3. CPU-core-aware placement,
4. accelerator-aware scheduling,
5. capability-governed cross-domain coordination.

### 9.5 Stability Principle

User-facing process abstractions SHOULD remain recognizable even as the runtime evolves from single-core to multi-core operation.

---

## 10. Domains and Object Spaces

### 10.1 Domain Concept

A domain is an execution region with a defined authority context, process set, and object-access rules.

### 10.2 Domain Responsibilities

A mature implementation SHOULD allow a domain to define or reference:

- its scheduler context,
- its object space or accessible object graph,
- its granted capabilities,
- its communication endpoints,
- its resource quotas or limits,
- its debugging permissions.

### 10.3 Domain Isolation Levels

A conforming architecture MAY support multiple domain isolation levels, including:

- no isolation for trusted core regions,
- soft isolation with policy checks,
- strong logical isolation with controlled endpoints,
- stronger hardware-assisted isolation if future platforms support it.

### 10.4 Cross-Domain References

A future implementation SHOULD define whether cross-domain object references are:

- forbidden,
- proxied,
- capability-wrapped,
- serializable by copy,
- shared only if immutable or specially marked.

### 10.5 Domain Lifecycle

The runtime SHOULD define lifecycle operations for domains, including:

- creation,
- start,
- pause,
- quiesce,
- inspect,
- terminate,
- snapshot or migration if supported.

---

## 11. Message Passing and Endpoints

### 11.1 General Requirement

The long-term concurrency architecture SHOULD provide explicit message-passing facilities for cross-process or cross-domain coordination.

### 11.2 Endpoint Objects

Communication SHOULD be represented by inspectable endpoint objects such as:

- mailboxes,
- channels,
- request/reply ports,
- event queues,
- stream-like message conduits.

### 11.3 Message Semantics

A mature implementation SHOULD define whether messages are transferred by:

- reference,
- copy,
- proxy,
- move/ownership transfer,
- shared immutable reference,
- or combinations depending on object kind and domain relationship.

### 11.4 Backpressure and Blocking

The communication model SHOULD define:

- bounded versus unbounded queues,
- blocking versus non-blocking send,
- timeouts,
- cancellation,
- backpressure signaling.

### 11.5 Tool Visibility

The environment SHOULD be able to show communication topology, queued messages, waiting endpoints, and deadlock or backlog indicators where practical.

---

## 12. Accelerator and FPGA Coordination Model

### 12.1 General Requirement

Recorz SHALL treat accelerators, including FPGA cores, as coordinated compute resources rather than opaque side effects.

### 12.2 Accelerator Object Model

A mature implementation SHOULD represent each accelerator resource using ordinary inspectable objects or descriptors exposing at least:

- identity,
- type,
- configuration/bitstream metadata,
- current lifecycle state,
- owning domain or authority,
- bound software interfaces,
- health and error status,
- performance or utilization metadata if available.

### 12.3 Request Submission

Accelerator work SHOULD be modeled through explicit submission and completion objects or message protocols.

### 12.4 Synchronization with Software

The runtime SHALL define how software processes synchronize with accelerator tasks, including where applicable:

- submission,
- completion notification,
- timeout,
- cancellation,
- failure reporting,
- result retrieval,
- quiescence.

### 12.5 Shared Accelerator Safety

If an accelerator is shared across domains or processes, the runtime SHALL define ownership and arbitration rules explicitly.

### 12.6 Dynamic Reconfiguration

If FPGA reconfiguration is supported, the runtime SHOULD require safe transition steps such as:

- quiescing dependent processes,
- draining or invalidating in-flight requests,
- compatibility verification,
- rebinding endpoints,
- explicit resume or failure reporting.

---

## 13. Quiescence, Pausing, and System Transition Safety

### 13.1 Need for Quiescence

Recorz SHALL support the concept of quiescence for operations that require a stable transition boundary.

### 13.2 Applicable Operations

Quiescence MAY be required for:

- snapshotting,
- live class layout migration,
- compiler/runtime upgrades,
- domain migration,
- accelerator reconfiguration,
- certain privileged device operations.

### 13.3 Observable State

The environment SHOULD be able to show which processes, domains, or accelerators are preventing quiescence.

### 13.4 Policy Flexibility

A conforming implementation MAY choose stop-the-world, cooperative quiescence, per-domain quiescence, or other strategies, but the policy MUST be explicit.

---

## 14. Security Model Overview

### 14.1 Early Permissiveness

Early Recorz systems MAY be permissive and highly reflective in the trusted core environment.

### 14.2 Long-Term Requirement

The architecture SHALL preserve a path toward capability-oriented security and sandboxing.

### 14.3 Central Security Principle

Authority SHOULD flow through explicit object references rather than ambient global reachability.

### 14.4 Compatibility Goal

The security model SHOULD be additive. It SHOULD become possible to run restricted applications without requiring the trusted core architecture to be discarded.

---

## 15. Object Capability Direction

### 15.1 Capability Definition

A capability is an authority-bearing reference that grants the ability to invoke a service or affect a resource.

### 15.2 General Rule

A sandboxed component SHOULD only be able to affect objects and services reachable through capabilities explicitly provided to it.

### 15.3 Capability-Carrying Objects

Capabilities MAY be embodied as:

- ordinary service objects,
- endpoint objects,
- proxy objects,
- wrapped references with attenuated authority,
- device handles,
- domain management tokens.

### 15.4 Attenuation

The design SHOULD support attenuated capabilities that grant less authority than a more privileged reference.

### 15.5 Delegation

The design SHOULD define capability delegation explicitly. Delegation MAY be direct, proxied, revocable, time-limited, or quota-bound.

### 15.6 Revocation

A mature implementation SHOULD support revocable capabilities or revocation indirection for selected resource classes.

---

## 16. Ambient Authority Reduction

### 16.1 Risk Statement

Classic image-based systems often rely heavily on global reachability and reflective mutation. This is convenient for trusted development but unsafe for sandboxed code.

### 16.2 Architectural Requirement

Future Recorz sandboxing SHALL reduce dependence on ambient authority in at least the following areas:

- file/storage access,
- device access,
- network access,
- primitive invocation,
- class and method mutation,
- domain/process management,
- snapshot authority,
- accelerator control.

### 16.3 Trusted Core Exception

The trusted core MAY continue to possess broad authority, but its APIs SHOULD make authority boundaries explicit so that reduced-authority environments can be introduced later.

---

## 17. Primitive and Device Authority

### 17.1 Primitive Restriction Requirement

The runtime architecture SHALL support restrictions on primitive use based on domain, privilege level, or capability possession.

### 17.2 Device Ownership

Devices and hardware resources SHOULD be owned or mediated by explicit authority-bearing objects.

### 17.3 Raw Memory and Unsafe Operations

Operations involving raw memory, interrupt control, direct hardware register access, or unsafe accelerator control SHALL be treated as privileged.

### 17.4 Systems Subset Distinction

Compilation in the systems subset SHALL NOT automatically imply execution authority. Authorization and subset eligibility are separate concepts.

### 17.5 Tooling

Tools SHOULD show when a primitive or device operation is:

- unrestricted in the trusted core,
- denied,
- capability-gated,
- sandbox-safe,
- audit-relevant.

---

## 18. Reflective Authority and Debugging Boundaries

### 18.1 Reflection as Authority

In a reflective system, the ability to inspect or mutate classes, contexts, methods, or process state may itself confer significant authority.

### 18.2 General Requirement

The architecture SHOULD permit reflection to be selectively reduced or proxied in sandboxed domains.

### 18.3 Trusted Debugging

Authorized debugging tools SHOULD be able to inspect processes and domains deeply.

### 18.4 Sandboxed Debugging

A sandboxed component MAY be allowed introspection over its own objects and processes while being denied arbitrary reflection over the trusted core or peer domains.

### 18.5 Context Access Policy

Access to contexts, senders, and process internals SHOULD be policy-aware in future isolated environments.

---

## 19. Class Mutation and System Evolution Authority

### 19.1 Live Mutation as Power

Live method installation, class reshaping, and compiler replacement are central to Recorz, but they are also highly privileged operations.

### 19.2 Trusted Core Requirement

The trusted development environment SHALL be able to evolve itself live.

### 19.3 Restricted Environments

Sandboxed or application domains SHOULD NOT automatically gain authority to mutate core classes, install privileged methods, or alter system compiler objects.

### 19.4 Scoped Mutation

A future implementation MAY support scoped code mutation within a restricted package, module, or domain while still protecting core system structures.

---

## 20. Resource Limits and Quotas

### 20.1 Need for Limits

A future secure or multi-tenant implementation SHOULD support resource accounting and limits.

### 20.2 Possible Controlled Resources

Limits MAY apply to:

- process count,
- memory consumption,
- mailbox or queue growth,
- timer creation,
- accelerator usage,
- storage allocation,
- network operations,
- CPU time or scheduling share.

### 20.3 Object Representation

Quotas and limits SHOULD be represented through inspectable policy or domain objects rather than hidden global tables where practical.

---

## 21. Failure, Deadlock, and Diagnostic Visibility

### 21.1 General Requirement

Because Recorz prioritizes comprehension, the runtime and library SHOULD expose concurrency and security failures in diagnosable ways.

### 21.2 Observable Failure Classes

The system SHOULD be able to surface:

- process crashes,
- unhandled exceptions in background work,
- blocked or deadlocked waits,
- starvation indicators,
- mailbox/queue overload,
- domain termination reasons,
- denied capability use,
- revoked capability access,
- accelerator timeout or failure,
- quiescence blockers.

### 21.3 Tool Support

The environment SHOULD provide inspectors or browsers for:

- live process trees,
- waiting graphs,
- domain boundaries,
- capability provenance where practical,
- endpoint connections,
- accelerator bindings.

---

## 22. Conformance Profiles

### 22.1 Minimal Concurrency Profile

A minimal conforming implementation SHALL provide:

- inspectable processes,
- a defined scheduler model,
- suspension/resumption support,
- at least one synchronization primitive,
- debugger visibility into active processes.

### 22.2 Transitional Multiprocessing-Ready Profile

A broader implementation SHOULD provide Minimal Concurrency plus:

- endpoint-based coordination objects,
- object or domain ownership metadata,
- quiescence support for selected transitions,
- accelerator request/completion objects if accelerators exist.

### 22.3 Capability-Ready Profile

A capability-ready implementation SHOULD provide Transitional Profile plus:

- explicit authority-bearing service objects,
- primitive and device gating hooks,
- domain-local capability sets,
- reflective restriction points,
- policy objects for limits and ownership.

### 22.4 Sandboxed Platform Profile

A sandboxed platform SHOULD provide Capability-Ready Profile plus:

- isolated domains or object spaces,
- controlled cross-domain communication,
- reduced ambient authority,
- audit-friendly denial and failure reporting,
- revocable or attenuated capabilities for sensitive services.

---

## 23. Recommended Staging

### 23.1 Stage 1: Live Smalltalk-Style Processes

The project SHOULD begin with:

- simple processes,
- a clear scheduler,
- semaphores or equivalent,
- good debugger visibility.

### 23.2 Stage 2: Better Coordination Visibility

The next stage SHOULD add:

- inspectable queues/mailboxes,
- better blockage diagnostics,
- clearer process ownership metadata,
- quiescence support for snapshots and upgrades.

### 23.3 Stage 3: Multiprocessing-Oriented Architecture

The next stage SHOULD add:

- domain or object-space abstractions,
- explicit cross-boundary communication,
- ownership and transfer rules,
- accelerator coordination objects.

### 23.4 Stage 4: Capability Hooks

The next stage SHOULD add:

- capability-bearing service objects,
- primitive/device gating,
- reflective restriction points,
- policy objects for limits and authority.

### 23.5 Stage 5: Sandboxed Applications

The platform SHOULD later support:

- restricted application domains,
- safe device mediation,
- limited reflective power,
- revocable and attenuated capabilities,
- controlled core mutation boundaries.

---

## 24. Open Questions

The following questions remain open and SHALL be resolved by later revisions or implementation notes:

1. exact scheduler strategy for the first real implementation,
2. exact domain/object-space representation,
3. memory model for cross-core execution,
4. reference/copy/proxy rules for cross-domain messages,
5. capability representation and revocation mechanics,
6. exact reflective restrictions in sandboxed domains,
7. device and accelerator arbitration policies,
8. audit/logging requirements for privileged operations,
9. migration or snapshot semantics for isolated domains,
10. whether some capability enforcement lives in VM, library, compiler metadata, or all three.

---

## 25. Summary of Normative Intent

The Recorz concurrency, multiprocessing, and capability-security architecture is specified as a system that:

- SHALL support inspectable live processes from the beginning,
- SHALL preserve a path from simple shared-heap concurrency toward explicit multi-domain execution,
- SHOULD favor message-oriented coordination and ownership boundaries over unrestricted mutable sharing,
- SHALL treat accelerators and FPGA resources as first-class coordinated execution resources,
- SHALL preserve a path toward capability-based security and sandboxing,
- SHOULD make authority, blockage, ownership, and failure visible to humans through tools.

Its purpose is to let Recorz remain live and understandable while gaining the ability to scale across processors, coordinate with hardware accelerators, and eventually host restricted user applications without surrendering control of the core system.