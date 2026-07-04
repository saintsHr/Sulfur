# Manifest

---

**Sulfur** is a low-level, systems-oriented programming language designed to provide performance, safety, and predictability while maintaining practical ergonomics.

## Why Sulfur Exists

Sulfur exists to provide a systems programming language where performance, explicitness, and predictability are first-class design goals. Rather than hiding complexity behind language features or runtime behavior, Sulfur exposes it in a way that is understandable, controllable and verifiable by both the programmer and the compiler.

---

## Core Principles

- **Transparent execution**
  The compiler may optimize generated code, but such optimizations must never alter observable semantics or introduce hidden behavior visible to the programmer.

- **Deterministic semantics**
  Language semantics are deterministic. Observable nondeterminism only arises from explicitly nondeterministic operations (I/O, concurrency, randomness, etc.).

- **No implicit data mutation**
  Data is immutable by default. Any mutation must be explicitly declared and visible in the code.

- **Explicit control over abstraction**
  Abstractions are allowed only when they are visible, predictable, and do not hide performance or memory behavior.

- **Clarity over cleverness**
  Code should prioritize readability and directness over implicit optimizations or syntactic sugar.

- **Minimal but explicit boilerplate**
  The language avoids unnecessary verbosity, but never at the cost of hiding behavior or intent.

---

## Goals

- Systems-level performance comparable to C/C++
- Predictable memory and execution model
- Ergonomic enough to reduce common low-level errors
- Suitable for OS, drivers, and performance-critical applications
- Excellent interoperability with existing systems software

## Non-Goals

- Automatic memory management
- Hidden runtime behavior or “magic”
- Dynamic typing or runtime type inference
- Overly abstracted high-level paradigms

---

## Feature Philosophy

Every language feature must solve a real problem while remaining consistent with the language's existing design. Features should be added only when their long-term benefits clearly outweigh their complexity.

## Safety Philosophy

Safety is achieved through **explicitness and compiler guarantees**, not runtime checks or hidden mechanisms.

## Error Philosophy

Compiler diagnostics are part of the language design. Error messages should explain why code is invalid and, whenever possible, how to fix it.

## Stability Philosophy

Breaking changes should be rare and only introduced when they significantly improve the language.

## Simplicity Philosophy

Features must always justify their complexity.

## Orthogonality Philosophy

Features should compose naturally rather than requiring special rules.

## Consistency Philosophy

Similar problems should have similar solutions. The language should avoid special cases whenever possible.

## Memory Philosophy

Sulfur gives programmers explicit control over memory allocation and lifetime. The compiler does not manage memory on behalf of the programmer. Instead, it performs static analysis to detect mistakes, highlight inefficiencies, and prevent memory-unsafe behavior whenever it can be proven. When safety cannot be proven, the compiler should clearly communicate the associated risks. The programmer retains control over memory decisions, while the compiler acts as a safeguard rather than an owner.

## Interoperability Philosophy

Sulfur is designed to coexist with existing software ecosystems. Interoperability with established systems languages should be straightforward whenever practical, allowing incremental adoption rather than requiring complete rewrites. Existing ecosystems should be considered first-class citizens rather than obstacles to adoption.

## Portability Philosophy

The language should remain portable across operating systems, architectures, and execution environments. Platform-specific behavior should be explicit rather than embedded into the language itself.

---

## Language Identity

Sulfur prioritizes:

- control over convenience
- explicitness over brevity
- predictability over flexibility
- interoperability over isolation

---