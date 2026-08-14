<p align="center">
  <img src="assets/sulfur-banner.png">
</p>

<p align="center">
  <a href="https://github.com/saintsHr/Sulfur/actions">
    <img src="https://img.shields.io/github/actions/workflow/status/saintsHr/Sulfur/ci.yml?branch=main&style=flat-square&label=CI" alt="CI">
  </a>
  <a href="https://github.com/saintsHr/Sulfur/stargazers">
    <img src="https://img.shields.io/github/stars/saintsHr/Sulfur?style=flat-square" alt="Stars">
  </a>
  <a href="https://github.com/saintsHr/Sulfur/network/members">
    <img src="https://img.shields.io/github/forks/saintsHr/Sulfur?style=flat-square" alt="Forks">
  </a>
  <a href="https://github.com/saintsHr/Sulfur/issues">
    <img src="https://img.shields.io/github/issues/saintsHr/Sulfur?style=flat-square" alt="Issues">
  </a>
  <a href="https://github.com/saintsHr/Sulfur/commits/main">
    <img src="https://img.shields.io/github/last-commit/saintsHr/Sulfur?style=flat-square" alt="Last Commit">
  </a>
  <a href="https://github.com/saintsHr/Sulfur/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/saintsHr/Sulfur?style=flat-square" alt="License">
  </a>
</p>

<h1 align="center">⟨ Sulfur ⟩</h1>

<p align="center">
  A low-level, systems-oriented programming language focused on
  <strong>explicit behavior, predictable execution, and direct control over memory.</strong>
</p>

<p align="center">
  Performance · Safety · Predictability · Explicitness · Determinism · Control
</p>

## 🧭 Design

Sulfur favors:

* **Explicitness over convenience**
* **Predictability over flexibility**
* **Control over abstraction**
* **Clarity over cleverness**
* **Interoperability over isolation**

Data is immutable by default. Mutation is explicit. Memory allocation and
lifetime are controlled by the programmer rather than a garbage collector or
automatic memory manager.

The compiler is responsible for enforcing what can be proven statically.
When it cannot prove that an operation is safe, it should make that limitation
visible rather than hiding it behind runtime behavior.

For the full design rationale, see [`MANIFEST.md`](MANIFEST.md).

## 🎯 Goals

* Performance suitable for systems software
* A predictable memory and execution model
* Explicit control over allocation, mutation, and lifetime
* Static detection of memory-safety problems where possible
* Straightforward interoperability with existing systems software
* Portability across operating systems and architectures
* A small language with composable rules

## 🚫 Non-Goals

Sulfur intentionally does not aim to provide:

* Garbage collection or automatic memory management
* Hidden runtime behavior
* Dynamic typing
* Implicit mutation
* Abstractions that conceal performance or memory costs
* Language features that exist primarily for syntactic convenience

## 🛠️ Current Status

Sulfur is **experimental and under active development**.
The compiler, language specification, standard library, syntax, and
semantics are still evolving. Breaking changes are expected,
and current code should not be assumed to remain compatible
with future versions.

## 📚 Documentation

Project documentation is available in [`doc/`](doc/).

The [`MANIFEST.md`](MANIFEST.md) describes the principles and constraints
that guide the language's design.

## 🤝 Contributing

Sulfur is still being shaped.

Contributions, experiments, bug reports, and technical discussion are welcome.
Keep in mind that parts of the language and compiler are not yet stable,
and proposed changes may affect the design itself.

## 📄 License

Sulfur is released under the **MIT License**.
