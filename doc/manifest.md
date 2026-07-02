# Manifest

**Sulfur** is a low-level, systems-oriented programming language designed to provide performance, safety, and predictability while maintaining practical ergonomics.

## Core Principles

- **Transparent execution**
  All behavior must be explicitly defined in source code. No hidden control flow or implicit runtime behavior is allowed.

- **Deterministic semantics**
  Given the same input, programs must always produce the same behavior, unless explicitly stated otherwise.

- **No implicit data mutation**
  Data is immutable by default. Any mutation must be explicitly declared and visible in the code.

- **Explicit control over abstraction**
  Abstractions are allowed only when they are visible, predictable, and do not hide performance or memory behavior.

- **Clarity over cleverness**
  Code should prioritize readability and directness over implicit optimizations or syntactic sugar.

- **Minimal but explicit boilerplate**
  The language avoids unnecessary verbosity, but never at the cost of hiding behavior or intent.

## Design Goals

- Systems-level performance comparable to C/C++
- Predictable memory and execution model
- Ergonomic enough to reduce common low-level errors
- Suitable for OS, drivers, and performance-critical applications

## Non-Goals

- Automatic memory management (unless explicitly introduced later)
- Hidden runtime behavior or “magic”
- Dynamic typing or runtime type inference
- Overly abstracted high-level paradigms

## Safety Philosophy

Safety is achieved through **explicitness and compiler guarantees**, not runtime checks or hidden mechanisms.

## Language Identity

Sulfur prioritizes:
- control over convenience
- explicitness over brevity
- predictability over flexibility