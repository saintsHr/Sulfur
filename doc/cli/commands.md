# CLI Commands

The Sulfur CLI provides commands for creating projects, compiling and running source files, cleaning build artifacts, and retrieving compiler information.

## `sulfur new <name>`

Creates a new project with the specified name, including the initial source directory, entry-point file, and project configuration.

The generated project includes:
- `src/` — Directory for source files.
- `src/main.slfr` — Main source file.
- `sulfur.toml` — Project configuration file.

## `sulfur build`

Compiles all `.slfr` source files found recursively inside the `src/` directory.

Generated assembly files are written to the `build/` directory, preserving the source directory structure.

## `sulfur run`

Runs the executable binary produced by a previous build. It does not compile the project, so `sulfur build` must be run first.

The command fails with an error if no build is found, or if the result of the compilation is not an executable binary.

## `sulfur clean`

Removes the `build/` directory and its contents, cleaning up generated build artifacts.

## `sulfur --version`

Displays the compiler version, copyright information, and license.

## `sulfur --help`

Displays the CLI usage information, including the available commands and their arguments.
