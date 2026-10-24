# CLI Commands

The Sulfur CLI provides commands for creating projects, compiling source files, cleaning build artifacts, and retrieving compiler information.

## `sulfur new <name>`

Creates a new project with the specified name, including the initial source directory, entry-point file, and project configuration.

The generated project includes:
- `src/` — Directory for source files.
- `src/main.slfr` — Main source file.
- `sulfur.toml` — Project configuration file.

## `sulfur build`

Compiles all `.slfr` source files found recursively inside the `src/` directory.

Generated assembly files are written to the `build/` directory, preserving the source directory structure.

## `sulfur clean`

Removes the `build/` directory and its contents, cleaning up generated build artifacts.

## `sulfur --version`

Displays the compiler version, copyright information, and license.

## `sulfur --help`

Displays the CLI usage information, including the available commands and their arguments.
