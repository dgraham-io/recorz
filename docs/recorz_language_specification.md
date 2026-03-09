# Recorz Language Specification
## Version 0.1 Draft

## Status of This Document

This document specifies the **Recorz programming language** independent of broader platform concerns such as graphics, device management, or bare-metal boot flow. It defines the surface language, core semantics, gradual typing model, reflective model, and the restricted language subsets used for systems programming and hardware description.

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** are to be interpreted as normative requirements.

---

## 1. Scope

Recorz is a class-based, message-oriented programming language in the Smalltalk and Strongtalk tradition.

This specification defines:

- lexical structure,
- syntactic structure,
- evaluation order,
- object and method semantics,
- blocks and returns,
- variable binding rules,
- gradual typing,
- reflective requirements,
- primitive interface boundary,
- restricted language subsets for systems programming and HDL.

This document does **not** define:

- the object memory layout in a particular VM,
- garbage collector algorithms,
- image file formats,
- UI framework details,
- hardware-driver APIs,
- FPGA synthesis file formats.

---

## 2. Design Principles

A conforming Recorz language design SHALL satisfy the following principles:

1. **Smalltalk continuity**
   - Ordinary Smalltalk-style code SHALL remain the baseline programming style.

2. **Minimal syntax growth**
   - New syntax SHALL be introduced only where justified by typing, low-level subsets, or hardware description.

3. **Message-centric semantics**
   - Message sending SHALL remain the primary computational mechanism.

4. **Gradual adoption of advanced features**
   - A programmer SHOULD be able to begin with untyped code and later adopt types and restricted subsets incrementally.

5. **Reflective visibility**
   - The language SHALL support strong introspection and debugger-friendly semantics in the general subset.

6. **Subset discipline rather than language fragmentation**
   - Systems programming and HDL work SHOULD be expressed as restricted subsets of Recorz rather than unrelated languages.

---

## 3. Terminology

For the purposes of this specification:

- **Object**: A runtime entity receiving messages and possessing class identity.
- **Class**: A behavior-and-structure descriptor defining instance variables and methods.
- **Metaclass**: The class of a class object, used for class-side behavior.
- **Selector**: The symbolic name of a message.
- **Method**: A behavior definition associated with a class and selector.
- **Message send**: Application of a selector to a receiver with zero or more arguments.
- **Block**: A closure-like object representing deferred executable code.
- **Context**: An activation record or execution frame.
- **Primitive**: A VM- or runtime-provided operation invoked through a language-level method boundary.
- **Typed region**: Code whose declarations or methods include explicit or inferred type information.
- **General subset**: Full ordinary Recorz semantics.
- **Systems subset**: Restricted subset for low-level implementation.
- **HDL subset**: Restricted subset for hardware elaboration.

---

## 4. Lexical Structure

### 4.1 Character Set

A conforming implementation SHALL support at least a Unicode-capable source representation, but it MAY restrict the set of identifier characters in early implementations.

### 4.2 Whitespace

Whitespace MAY appear between tokens except where it would split a token.
Whitespace has no semantic significance except as token separator.

### 4.3 Comments

Recorz SHALL support Smalltalk-style string comments using double quotes.
A comment begins with `"` and ends at the next unescaped `"`.
Comment contents are ignored by evaluation.

Example:

```smalltalk
"This is a comment"
```

### 4.4 Identifiers

Identifiers denote variable names, class names, pool names, and similar symbols.

A conforming implementation SHALL distinguish the following identifier categories syntactically:

- **lowercase-initial identifiers** for variables and ordinary names,
- **uppercase-initial identifiers** for globals and class names,
- **keyword fragments** ending in `:`,
- **binary selectors** made of operator characters.

### 4.5 Literals

The language SHALL support at least the following literal forms:

- integers,
- scaled or floating-point numbers,
- characters,
- strings,
- symbols,
- arrays of literals,
- byte arrays or equivalent literal binary forms, optionally.

Example forms:

```smalltalk
42
3.14
$a
'hello'
#size
#('a' 'b' 'c')
```

### 4.6 Reserved Pseudo-Variables

The following pseudo-variables SHALL be reserved:

- `self`
- `super`
- `nil`
- `true`
- `false`
- `thisContext`

A conforming implementation MUST define their meaning and MUST NOT allow them to be rebound as ordinary variables.

---

## 5. Concrete Syntax Overview

Recorz preserves Smalltalk precedence and surface style.

### 5.1 Expression Categories

Expressions are composed from:

- primary expressions,
- unary messages,
- binary messages,
- keyword messages,
- cascades,
- assignments,
- blocks,
- returns,
- parenthesized expressions.

### 5.2 Precedence Rules

A conforming implementation SHALL evaluate message syntax in this order:

1. primary expressions,
2. unary messages,
3. binary messages,
4. keyword messages,
5. cascades.

Assignment binds less tightly than message syntax.
Return applies to an expression within method or block context subject to return semantics defined below.

### 5.3 Informal Examples

```smalltalk
object size
3 + 4 * 5
array at: 1 put: 42
stream nextPutAll: 'abc'; cr
x := y + 1
```

Under Smalltalk precedence, `3 + 4 * 5` parses as `(3 + 4) * 5` unless parentheses indicate otherwise.

---

## 6. Grammar

The following grammar is an intentionally conservative EBNF-style approximation of the surface language. An implementation MAY refactor it internally, but accepted programs SHALL conform to materially equivalent syntax.

```ebnf
CompilationUnit  = { Definition } ;

Definition       = ClassDefinition | ExtensionDefinition | TopLevelChunk ;

TopLevelChunk    = Expression [ "." ] ;

ClassDefinition  = ClassHeader ClassBody ;
ClassHeader      = Identifier "subclass:" Identifier ClassOptions ;
ClassOptions     = { ClassOption } ;
ClassOption      = Identifier ":" Literal ;
ClassBody        = "[" { MethodDefinition | ClassSideMarker } "]" ;
ClassSideMarker  = "class" ;

MethodDefinition = MethodHeader MethodBody ;
MethodHeader     = UnaryHeader | BinaryHeader | KeywordHeader ;
UnaryHeader      = Identifier [ TypeAnnotation ] ;
BinaryHeader     = BinarySelector Parameter [ ReturnAnnotation ] ;
KeywordHeader    = KeywordPart { KeywordPart } [ ReturnAnnotation ] ;
KeywordPart      = Keyword Parameter ;
Parameter        = Identifier [ TypeAnnotation ] ;
ReturnAnnotation = "^" TypeExpression ;
TypeAnnotation   = "<" TypeExpression ">" ;

MethodBody       = [ PragmaList ] [ TempDecls ] StatementSequence ;
PragmaList       = "<" Pragma { Pragma } ">" ;
Pragma           = Identifier [ ":" Literal ] ;
TempDecls        = "|" { TempDecl } "|" ;
TempDecl         = Identifier [ TypeAnnotation ] ;

StatementSequence = { Statement "." } [ FinalStatement ] ;
Statement         = Expression | Assignment | Return ;
FinalStatement    = Expression | Assignment | Return ;
Assignment        = Identifier ":=" Expression ;
Return            = "^" Expression ;

Expression       = Cascade ;
Cascade          = KeywordExpr [ ";" MessageContinuation { ";" MessageContinuation } ] ;
MessageContinuation = UnaryMessage | BinaryMessage | KeywordMessage ;

KeywordExpr      = BinaryExpr [ KeywordMessage ] ;
BinaryExpr       = UnaryExpr { BinaryMessage } ;
UnaryExpr        = Primary { UnaryMessage } ;

UnaryMessage     = Identifier ;
BinaryMessage    = BinarySelector UnaryExpr ;
KeywordMessage   = KeywordPartExpr { KeywordPartExpr } ;
KeywordPartExpr  = Keyword BinaryExpr ;

Primary          = Literal
                 | Identifier
                 | PseudoVariable
                 | Block
                 | Parenthesized ;

Parenthesized    = "(" Expression ")" ;

Block            = "[" [ BlockParams "|" ] [ TempDecls ] StatementSequence "]" ;
BlockParams      = BlockParam { BlockParam } ;
BlockParam       = ":" Identifier [ TypeAnnotation ] ;

PseudoVariable   = "self" | "super" | "nil" | "true" | "false" | "thisContext" ;

Keyword          = Identifier ":" ;
BinarySelector   = BinaryChar { BinaryChar } ;
BinaryChar       = "+" | "-" | "*" | "/" | "\\" | "~" | "<" | ">" | "=" | "@" | "%" | "|" | "&" | "?" | "," ;
Identifier       = Letter { Letter | Digit | "_" } ;
Literal          = Number | String | Symbol | Character | ArrayLiteral ;
```

### 6.1 Notes on the Grammar

1. This grammar is normative in structure but not intended to freeze class-definition file-out syntax permanently.
2. Method syntax and expression syntax are more stable than package or class declaration syntax.
3. Early Recorz systems MAY inherit a chunk-file format close to Smalltalk file-out conventions.

---

## 7. Binding and Names

### 7.1 Lexical Bindings

The language SHALL support lexical bindings for:

- method arguments,
- temporary variables,
- block parameters,
- block-local temporaries.

### 7.2 Instance Variables

Instance variables are bound relative to the method receiver’s class definition.
Name lookup for an unqualified identifier inside a method SHALL prefer lexical bindings before instance variables.

### 7.3 Global Names

Uppercase-initial identifiers MAY denote globals, classes, or namespace entries depending on the environment model.
The exact global environment object MAY vary by implementation, but lookup semantics SHALL be defined and inspectable.

### 7.4 Pseudo-Variables

- `self` denotes the current receiver.
- `super` denotes a lexical form of message send beginning lookup in the superclass of the method’s defining class while preserving the same receiver.
- `nil`, `true`, and `false` denote distinguished singleton objects.
- `thisContext` denotes the current execution context in the general subset.

A conforming implementation MAY restrict `thisContext` in systems or HDL subsets.

---

## 8. Objects, Classes, and Methods

### 8.1 Object Model

Recorz SHALL be class-based.
Every ordinary object SHALL have a class.
Classes SHALL define:

- instance variable layout,
- instance-side methods,
- inheritance relationship.

Metaclasses SHALL define class-side methods.

### 8.2 Inheritance

Single inheritance SHALL be supported.
Multiple inheritance is not required and SHOULD NOT be introduced without compelling justification.

### 8.3 Method Definition Categories

Methods SHALL be defined in three selector families:

- unary selectors,
- binary selectors,
- keyword selectors.

Examples:

```smalltalk
size
+ aNumber
at: index put: value
```

### 8.4 Method Activation

A method activation SHALL create a context containing at least:

- receiver,
- selector,
- arguments,
- temporaries,
- instruction state sufficient for stepping or resumption.

### 8.5 Method Lookup

Ordinary sends SHALL perform dynamic lookup beginning at the receiver’s class.
`super` sends SHALL begin lookup at the superclass of the method’s defining class.

If a selector is not found, the implementation SHALL define a well-specified fallback, typically an equivalent of `doesNotUnderstand:`.

---

## 9. Expression Semantics

### 9.1 Primary Evaluation

Primary expressions evaluate as follows:

- literals evaluate to their literal object values,
- identifiers evaluate by lexical / instance / global lookup rules,
- pseudo-variables evaluate to their distinguished meaning,
- blocks evaluate to block objects,
- parenthesized expressions evaluate to the value of the enclosed expression.

### 9.2 Message Sending

Message sending is left-associative within each precedence level.
A unary message takes no argument.
A binary message takes one argument.
A keyword message takes one argument per keyword fragment.

Examples:

```smalltalk
receiver doThing
3 + 4
dictionary at: key ifAbsent: [ nil ]
```

### 9.3 Cascades

A cascade SHALL evaluate its receiver once and then apply multiple messages to that same receiver.
The value of a cascade SHALL be the value of its final message send unless otherwise specified.

Example:

```smalltalk
stream nextPutAll: 'abc'; cr; flush
```

### 9.4 Assignment

`:=` assigns the value of its right-hand expression to the named variable binding.
Assignment is an expression and evaluates to the assigned value unless the implementation specifies otherwise. A Smalltalk-compatible implementation SHOULD return the assigned value.

### 9.5 Statement Sequences

In a statement sequence separated by periods, statements are evaluated in order. The value of the final statement is the value of the sequence unless terminated by return.

---

## 10. Blocks and Return Semantics

### 10.1 Block Nature

A block is a closure-like object representing executable code with captured lexical bindings.
A conforming implementation SHALL support zero or more parameters and zero or more local temporaries.

Examples:

```smalltalk
[ 1 + 2 ]
[ :x | x + 1 ]
[ :x :y | | z | z := x + y. z * 2 ]
```

### 10.2 Block Invocation

Blocks are invoked by message sends such as `value`, `value:`, `value:value:`, and related protocol.
A conforming implementation MAY optimize invocation but SHALL preserve observable semantics.

### 10.3 Lexical Capture

Blocks SHALL capture lexical bindings from surrounding scopes according to the language’s closure model.
Captured variables SHALL remain available for the lifetime required by block reachability.

### 10.4 Non-Local Return

A return expression `^expr` appearing inside a block in the general subset SHALL perform a **non-local return** from the lexically enclosing method activation, unless that activation is no longer valid and the implementation defines an exceptional condition.

This behavior is intentionally Smalltalk-like and is REQUIRED for the general subset.

### 10.5 Restricted Subset Variation

The systems subset MAY restrict non-local returns.
The HDL subset SHALL NOT permit general non-local returns.
Any restriction MUST be explicit and tool-visible.

---

## 11. Control Structures

### 11.1 Control as Message Protocol

Recorz SHALL preserve the Smalltalk style in which common control forms are expressed through message sends and block arguments rather than special syntax.

Examples include protocols equivalent to:

- `ifTrue:`
- `ifFalse:`
- `ifTrue:ifFalse:`
- `whileTrue:`
- `whileFalse:`
- collection iteration protocols

### 11.2 Special Forms

The language core SHOULD minimize built-in special forms.
Method return, block syntax, assignment, and `super` semantics are special syntactic forms.
Ordinary conditionals and iteration SHOULD remain library-level message protocols.

---

## 12. Equality, Identity, and Truth

### 12.1 Object Identity

The language SHALL provide an identity predicate equivalent in meaning to Smalltalk `==`.
Identity compares whether two references denote the same object.

### 12.2 Equality

The language SHALL provide equality as ordinary message protocol, typically analogous to `=`.
Classes MAY refine equality semantics.

### 12.3 Booleans

`true` and `false` SHALL be distinguished boolean objects.
Conditional protocols SHALL operate through their methods and block evaluation protocol.

### 12.4 Nil

`nil` SHALL denote a distinguished object representing absence or undefined object reference.
Its semantics MAY include default initialization and nil-aware protocols.

---

## 13. Type System

### 13.1 General Model

Recorz SHALL support **gradual typing**.
Typing SHALL be optional.
Untyped code SHALL remain valid.
Typed and untyped code SHALL interoperate.

### 13.2 Annotation Positions

A conforming implementation MAY permit type annotations on:

- method parameters,
- return values,
- temporary variables,
- instance variables,
- block parameters,
- class declarations or generic parameters.

The specification RECOMMENDS a visually light annotation style, such as angle-bracket forms, but implementations MAY choose another Smalltalk-compatible notation.

Example:

```smalltalk
sum: a <Integer> with: b <Integer> ^ <Integer>
    ^a + b
```

### 13.3 Type Categories

A mature Recorz type system SHOULD support:

- nominal class types,
- protocol/interface-style constraints,
- optional/nilable types,
- union types,
- inferred types,
- parametric types if they can be kept simple.

### 13.4 Untyped Regions

An untyped declaration or method SHALL behave according to ordinary dynamic semantics.
No program SHALL be invalid solely because it omits type annotations.

### 13.5 Typed Regions

Typed regions MAY be used for:

- static checking,
- dynamic boundary checks,
- optimization,
- specialization,
- restricted-subset eligibility.

### 13.6 Typed/Untyped Interoperation

When a value flows between typed and untyped regions, the implementation SHALL define a consistent boundary behavior. The implementation MAY use:

- dynamic checks,
- inferred trust in proven regions,
- wrapper or guard insertion,
- fallback to dynamic dispatch.

These rules MUST be inspectable in tools.

### 13.7 Type Errors

A conforming implementation MUST define whether a type violation is:

- compile-time only,
- runtime check failure,
- warning-level only,
- or some combination depending on mode.

The preferred design is that type errors are surfaced as early as practical without eliminating dynamic execution entirely.

---

## 14. Reflective Model

### 14.1 General Requirement

The general subset SHALL support strong reflection consistent with live Smalltalk-style programming.

### 14.2 Required Reflective Access

A conforming general-subset implementation SHALL make available, directly or indirectly:

- object class,
- class superclass,
- instance variable metadata,
- method dictionaries,
- method source or source association,
- selector references,
- active contexts,
- sender chain traversal,
- receiver and temporary inspection,
- message send fallback behavior.

### 14.3 thisContext

`thisContext` SHALL be supported in the general subset.
It denotes the current activation context.
A conforming implementation MAY represent contexts lazily or optimize them internally, but observable semantics SHALL support debugging and inspection.

### 14.4 Reflective Boundaries

The systems and HDL subsets MAY restrict reflection. If so:

- the restriction MUST be declared,
- tooling MUST explain it,
- code depending on unavailable reflection MUST be rejected or downgraded clearly.

---

## 15. Primitives

### 15.1 Purpose

Primitives provide the boundary between Recorz methods and VM or machine-supported operations.

### 15.2 Invocation Model

A method MAY be designated as primitive-backed through pragma or equivalent metadata.
The surface syntax is implementation-defined, but SHOULD remain Smalltalk-compatible.

Example conceptual form:

```smalltalk
integerAdd: a to: b
    <primitive: 'integerAdd'>
    self primitiveFailed
```

### 15.3 Failure Semantics

If a primitive cannot complete normally, the language SHALL define a well-specified failure path.
A fallback method body SHOULD be permitted.
Primitive failure MUST be debugger-visible in the general subset.

### 15.4 Primitive Safety

Primitive boundaries used by systems or hardware-facing code SHOULD expose enough metadata for tools to explain calling conventions, side effects, and failure behavior.

---

## 16. Systems Subset

### 16.1 Purpose

The systems subset is a restricted form of Recorz intended for:

- VM implementation,
- runtime internals,
- memory managers,
- device drivers,
- interrupt-related code,
- machine-code emitters,
- architecture support code.

### 16.2 General Rule

The systems subset SHOULD remain recognizably Recorz, but it MAY impose restrictions required for predictability and translation.

### 16.3 Required Capabilities

A systems-subset design SHALL support, either directly or through primitives:

- explicit integer widths,
- fixed-layout records or equivalent layout control,
- bit operations,
- raw memory access,
- address arithmetic,
- explicit representation constraints,
- predictable code generation.

### 16.4 Permitted Restrictions

The systems subset MAY restrict or prohibit:

- `thisContext`,
- unrestricted block capture,
- non-local returns,
- arbitrary heap allocation,
- dynamic selector construction,
- unrestricted reflection,
- late-bound polymorphism in critical regions.

### 16.5 Determinacy

A systems-subset compiler SHOULD reject programs whose semantics cannot be translated predictably to the target execution model.

---

## 17. HDL Subset

### 17.1 Purpose

The HDL subset is a restricted Recorz form intended to describe synchronous digital hardware for simulation and FPGA synthesis.

### 17.2 General Rule

The HDL subset SHALL be statically elaborable and deterministic.
It SHALL preserve as much of Recorz’s surface style as practical while rejecting constructs that do not map coherently to hardware.

### 17.3 Required Concepts

The HDL subset SHALL support representation of:

- modules/components,
- ports,
- bit vectors and fixed-width integers,
- registers,
- combinational logic,
- synchronous logic,
- reset behavior,
- finite-state machines,
- composition of subcomponents,
- explicit clocking relationships.

### 17.4 Forbidden or Restricted Constructs

The HDL subset SHALL forbid or tightly restrict:

- unrestricted heap allocation,
- general-purpose dynamic dispatch during elaboration,
- `thisContext`,
- non-local returns,
- unconstrained recursion unless elaboration-safe,
- side effects without hardware meaning,
- runtime object creation during synthesized execution.

### 17.5 Elaboration

HDL source SHALL elaborate to a hardware intermediate representation.
The same source and configuration SHALL produce equivalent elaboration results.

### 17.6 Simulation Semantics

An implementation SHOULD define simulation-time behavior for HDL constructs in a way that allows source-level debugging and signal inspection.

---

## 18. Errors and Exceptional Conditions

### 18.1 Message Lookup Failure

If a message send cannot be resolved, the implementation SHALL invoke a defined failure protocol equivalent in spirit to `doesNotUnderstand:` unless the active subset forbids it.

### 18.2 Non-Local Return Failure

If a block attempts a non-local return to an activation that is no longer valid, the implementation SHALL raise a defined exceptional condition.

### 18.3 Type Violations

Type violations SHALL be surfaced according to the implementation’s typed/untyped boundary policy and type-checking mode.

### 18.4 Primitive Failure

Primitive failure SHALL be observable and diagnosable.

### 18.5 Subset Rejection

If code uses a construct forbidden by the systems or HDL subset, the compiler or elaborator SHALL reject the construct with an explanatory diagnostic.

---

## 19. Source Organization

### 19.1 Methods and Classes

The language SHALL support persistent source representation of classes and methods.
A conforming implementation MAY use chunk-file syntax, package syntax, or image-associated source metadata, provided the represented language semantics are equivalent.

### 19.2 Protocol Organization

Methods SHOULD be groupable into protocols or categories for browsing and comprehension.
This is a tooling-facing organizational concept and need not affect runtime semantics.

### 19.3 Extension Methods

An implementation MAY support methods added to classes outside the class’s original defining package or module. If supported, the source representation MUST preserve provenance clearly.

---

## 20. Minimal Semantic Profile

A minimal conforming Recorz language implementation SHALL provide:

- class-based objects,
- unary/binary/keyword message syntax,
- blocks,
- assignment and return,
- single inheritance,
- dynamic message lookup,
- `self`, `super`, `nil`, `true`, `false`, and `thisContext` in the general subset,
- reflective access sufficient for debugging,
- primitive-backed methods,
- untyped execution.

A fuller implementation SHOULD also provide:

- gradual typing,
- systems subset,
- HDL subset,
- source-level metadata suitable for browsing and round-trip editing.

---

## 21. Worked Examples

### 21.1 Ordinary Smalltalk-Style Method

```smalltalk
factorial
    self = 0 ifTrue: [ ^1 ].
    ^self * (self - 1) factorial
```

### 21.2 Typed Method

```smalltalk
sum: a <Integer> with: b <Integer> ^ <Integer>
    ^a + b
```

### 21.3 Block and Collection Protocol

```smalltalk
numbers do: [ :each | Transcript show: each printString; cr ]
```

### 21.4 Primitive-Backed Method

```smalltalk
basicAt: index
    <primitive: 'basicAt'>
    self primitiveFailed
```

### 21.5 Systems-Subset Style Intent

```smalltalk
wordAt: address <UInt32> ^ <UInt32>
    <subset: #systems>
    ^Memory at32: address
```

### 21.6 HDL-Subset Style Intent

```smalltalk
tick
    <subset: #hdl>
    enable ifTrue: [ counter := counter + 1 ]
```

The last two examples are illustrative. Concrete subset declaration syntax MAY vary.

---

## 22. Open Language Questions

The following issues remain open and SHALL be resolved by later revisions or implementation notes:

1. precise concrete syntax for type annotations,
2. protocol/interface type notation,
3. generic type syntax if adopted,
4. namespace and module naming rules,
5. exact pragma syntax and semantics,
6. numeric tower details and overflow rules,
7. literal binary data syntax,
8. system-subset representation declarations,
9. HDL elaboration declaration syntax,
10. whether some subset restrictions are static-only or also runtime-enforced.

Namespace and module structure remain intentionally open. A future implementation
may explore selected Newspeak-like ideas there, but no such direction is
currently decided, and this does not change the near-term priority of a minimal
Smalltalk/Strongtalk-like Recorz system. See
`docs/recorz_potential_newspeak_direction.md`.

---

## 23. Summary of Normative Intent

Recorz is a Smalltalk-descended language that:

- SHALL preserve unary, binary, and keyword message syntax,
- SHALL preserve blocks, non-local returns, and message-oriented control in the general subset,
- SHALL remain class-based and reflective,
- SHALL support optional Strongtalk-like gradual typing,
- SHALL support primitive-backed low-level operations,
- SHALL define restricted systems and HDL subsets without abandoning the main language identity,
- SHALL prioritize comprehension, debugging, and inspectability.

Its primary goal is to let programmers begin with ordinary Smalltalk-style code and grow naturally into typed, low-level, and hardware-oriented work without leaving the conceptual world of the language.
