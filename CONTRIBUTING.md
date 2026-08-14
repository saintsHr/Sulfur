# Contributing to Sulfur

Thanks for your interest in contributing to Sulfur.

Sulfur is an experimental systems programming language, and both the compiler
and the language itself are still evolving. Contributions are welcome,
but changes should remain consistent with the project's design principles.

## Before Contributing

Before making a significant change, especially one involving the language,
compiler architecture, or semantics, read [`MANIFEST.md`](MANIFEST.md).

The Manifest describes the principles that guide Sulfur, including:

* Explicit behavior
* Deterministic semantics
* Explicit mutation
* Predictable memory behavior
* Minimal and composable language features
* Interoperability
* Portability

A feature should have a clear reason to exist and should fit these principles.

## Contribution Workflow

All changes must be submitted through a **Pull Request**.

Do not make changes directly on the `main` branch.

Create a separate branch for your work:

```bash
git checkout -b feature/my-change
```

or:

```bash
git checkout -b fix/my-fix
```

Make your changes, run the relevant tests, commit your work, and push the branch.

Then open a Pull Request targeting `main`.

The `main` branch should contain reviewed and accepted changes.

### Branches

Use a separate branch for each change.

Examples:

```text
feature/parser-improvements
feature/pointer-types
fix/parser-crash
fix/type-checking
docs/memory-model
```

Keep branches focused on a single change or a closely related group of changes.

## Development Setup

Sulfur uses CMake for its build system.

Clone the repository:

```bash
git clone https://github.com/saintsHr/Sulfur.git
cd Sulfur
```

Configure the project:

```bash
cmake -B build
```

Build:

```bash
cmake --build build
```

The repository also contains development scripts in [`scripts/`](scripts/),
including build, formatting, cleaning, and testcase helpers.

## Project Structure

The main project structure is:

```
Sulfur/
├── .github/        	# GitHub configuration and workflows
├── assets/         	# Project assets
├── doc/            	# Language documentation
├── include/        	# Public headers
├── scripts/        	# Development scripts
├── src/            	# Compiler implementation
├── testcases/      	# Language test cases
├── .clang-format   	# Format rules
├── .gitignore        	# Ignored files/folders
├── CMakeLists.txt  	# Build configuration
├── CODE_OF_CONDUCT.md  # Code of conduct
├── CONTRIBUTING.md 	# Contributing guide
├── LICENSE             # License
└── README.md           # Read me
```

The structure may change as the project develops.

## Compiler Development

The compiler is organized around a frontend/backend pipeline.

The frontend currently contains components such as:

* Preprocessor
* Lexer
* Parser
* AST
* Semantic analysis
* Scope and symbol handling

The backend currently contains components such as:

* Intermediate representation
* IR optimization
* Code generation
* Stack handling

Shared functionality is located under `utils/`.

When modifying the compiler, keep responsibilities separated and avoid introducing unnecessary coupling between pipeline stages.

## Language Changes

Changes to the language itself should be treated carefully.

Before proposing a new feature, consider:

1. What problem does it solve?
2. Can the problem be solved without adding a new language feature?
3. Does the feature introduce implicit behavior?
4. Does it make memory or execution behavior harder to understand?
5. Does it introduce unnecessary special cases?
6. Can the compiler reason about its behavior statically?
7. Does it fit the principles described in `MANIFEST.md`?

Language features should not be added simply for convenience.

If a proposed change affects the language's semantics or design, discuss it through an Issue before implementing it.

## Tests

Sulfur has a collection of language test cases in [`testcases/`](testcases/).

Test cases cover areas including:

* Arithmetic
* Operators
* Literals
* Casting
* Scopes
* Control flow
* Semantic errors
* Syntax errors
* Type checking
* Invalid programs

When fixing a bug, add a test that reproduces the problem whenever possible.

When adding a language feature, add tests for both valid and invalid behavior where applicable.

Run the relevant tests before opening a Pull Request.

## Documentation

Documentation is part of the project and is located in [`doc/`](doc/).

If a change affects:

* Language syntax
* Semantics
* Types
* Memory behavior
* Compiler behavior
* Supported platforms
* User-facing functionality

update the relevant documentation as part of the same change.

Documentation should describe actual language behavior and compiler guarantees rather than intended behavior that has not been implemented yet.

## Issues and Project Planning

GitHub Issues are used not only for bug reports and feature discussions, but also as part of Sulfur's project planning.

Issues may represent:

* Bugs
* Feature proposals
* Language design decisions
* Compiler tasks
* Documentation work
* Future improvements
* Technical debt
* Planned milestones

The project board is used as a **Kanban-style backlog and planning space**. Issues may represent work that is not immediately being implemented.

An issue in the backlog does not necessarily mean that the work is currently being developed.

Before starting work on a planned task, check the issue and its current status. For larger changes, discussing the implementation in the issue first can prevent duplicated or conflicting work.

## Issue and Pull Request Labels

Issues and Pull Requests should use **consistent labels**.

Labels should be applied according to the project's established conventions and should be used consistently across Issues and Pull Requests.

When a Pull Request implements or fixes an Issue, keep the labels relevant to the work consistent between them.

Avoid creating or using multiple labels with overlapping meanings.

Labels are also used by the project board to organize the backlog and track planned work.

## Pull Requests

Pull Requests should be focused and should describe the change clearly.

A Pull Request should explain:

* What changed
* Why it changed
* How it was tested
* Any relevant design decisions
* Whether the change affects language semantics

For language changes, explain how the proposal fits the principles defined in `MANIFEST.md`.

Pull Requests must target `main` and must come from a separate branch.

Avoid combining unrelated changes into the same Pull Request.

## Commit Messages

Sulfur follows the **Conventional Commits** specification.

Commit messages should use the following format:

```text
<type>(<scope>): <description>
```

The scope is optional when it does not provide useful information.

Examples:

```text
feat(parser): add support for while statements
fix(lexer): reject invalid hexadecimal literals
fix(semantic): detect use of uninitialized variables
refactor(ast): simplify expression nodes
test(casts): add invalid conversion cases
docs(types): document integer types
build(cmake): update compiler configuration
chore(scripts): improve testcase runner
```

Common commit types include:

* `feat` — new functionality
* `fix` — bug fix
* `refactor` — code changes that do not alter behavior
* `test` — tests
* `docs` — documentation
* `build` — build system or dependency changes
* `chore` — maintenance work

Keep commit messages short, specific, and written in the imperative form.

## Code Style

Follow the existing style of the surrounding code.

Avoid unrelated formatting or refactoring in the same change. Focused diffs make compiler changes easier to review.

Use the existing formatting tools when appropriate.

## Design Discussions

Not every change needs to become a Pull Request immediately.

For substantial language or compiler changes, open an Issue first and discuss the design before implementation.

This is especially recommended for changes that:

* Introduce new syntax
* Change existing semantics
* Modify memory or lifetime rules
* Introduce new runtime behavior
* Require significant compiler restructuring
* Break existing programs

## Scope

Sulfur is still experimental. Some decisions are intentionally unresolved, and implementation details may change.

Contributions do not need to preserve every existing implementation detail, but proposed changes should explain their trade-offs and remain consistent with the project's overall direction.

## License

By contributing to Sulfur, you agree that your contributions are made under the project's [MIT License](LICENSE).
