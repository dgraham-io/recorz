# Recorz Potential Newspeak Direction
## Status: Exploratory, Undecided

This document records a possible future direction for Recorz inspired by parts of
the Newspeak language family. It does not change the current architectural source
of truth in the existing Recorz specifications, and it does not change the
project's present implementation priorities.

The current implementation priority remains:

- bring up a minimal Smalltalk/Strongtalk-like Recorz VM as soon as possible,
- keep RV32 as the primary active target,
- preserve the existing path toward image-based live development,
- avoid delaying core VM/runtime milestones in pursuit of a broader language redesign.

## 1. Purpose

Recorz is already committed to:

- Smalltalk continuity,
- Strongtalk-inspired gradual typing,
- live image-based development,
- future capability-oriented isolation,
- source and image as dual first-class artifacts.

Some Newspeak ideas may eventually fit well with those goals, especially around:

- stronger lexical structure,
- reduced dependence on ambient globals,
- module or namespace objects,
- capability-friendly program structure,
- nested definitions and explicit outer references.

This document records that such a direction may be worth exploring later.

## 2. What Is Potentially Attractive

If Recorz later adopts selected Newspeak-like ideas, the most promising areas are:

1. Lexically grounded module and namespace structure.
2. Reduced ambient global reachability in less-trusted code.
3. More explicit authority passing through ordinary object references.
4. Nested classes or module-local definitions where they improve encapsulation.
5. Application-facing environments that are more capability-friendly than a flat global image.

These are interesting because they align with the existing Recorz direction toward:

- explicit authority,
- future restricted domains,
- inspectable object-based structure,
- a unified language family rather than unrelated sublanguages.

## 3. Current Constraints

The current Recorz implementation is still in an MVP bootstrap stage. It is not
yet the right time to redesign the language around a Newspeak-like model.

In particular, the project should first complete a minimal but coherent
Smalltalk/Strongtalk-like runtime with:

- real block and closure semantics,
- contexts and `thisContext`,
- process objects and basic scheduling,
- automatic memory management,
- a stronger in-image compiler path,
- a cleaner self-hosted development loop.

Until that foundation exists, a broader semantic shift would create more churn
than value.

## 4. Explicit Non-Decisions

The following are intentionally left undecided:

1. Whether Recorz should ever adopt full Newspeak semantics.
2. Whether only selected ideas should be adopted rather than the whole model.
3. Whether nested classes/modules should become part of the general subset.
4. Whether application code and trusted core code should eventually use different
   environment models.
5. Whether global names remain first-class in the trusted image while restricted
   domains move toward explicit imports/capabilities.
6. Whether any Newspeak-inspired syntax changes are needed at all.

## 5. Syntax Preference If Explored Later

If Recorz later explores this direction, the preferred constraint is:

- preserve a Smalltalk-like surface style as much as possible,
- avoid punctuation-heavy forms,
- avoid introducing `{`, `}`, or `::=` unless there is a compelling reason,
- prefer chunk-style, message-oriented, and inspectable source forms,
- keep ordinary methods and expressions recognizably Smalltalk-like.

This means any future module/nesting/import model should, if practical, be
expressed with minimal syntax growth and in a way that does not disrupt the
current Smalltalk/Strongtalk feel.

## 6. Architectural Guardrails For Now

To preserve the option of a future Newspeak-like direction without committing to
it now, current implementation work should continue to follow these guardrails:

1. Do not make flat global lookup more central than necessary.
2. Treat current ambient globals as an MVP convenience, not a permanent semantic bedrock.
3. Keep lexical binding and closure semantics real, not ad hoc.
4. Keep package/source organization flexible enough to admit richer namespace or
   module structure later.
5. Preserve a path toward capability-oriented execution domains.
6. Keep trusted-core reflection and future restricted execution as separable concerns.

## 7. Near-Term Implementation Priority

For the near term, Recorz remains on the current path:

1. minimal Smalltalk/Strongtalk-like VM,
2. RV32-first execution and development,
3. image persistence and live source development,
4. progressively moving compiler/runtime semantics into Recorz,
5. only after that, broader language-structure experiments.

This document should therefore be read as a recorded possibility, not as an
instruction to redirect current implementation work.
