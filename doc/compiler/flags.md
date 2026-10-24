# Compiler Flags

Flags control most aspects of the compilation process. The following section lists all available flags and describes their functionality.

## `-i <filepath>`

Specifies the input file to be compiled.

## `-o <filepath>`

Specifies the output file where the compilation result will be written.

## `--no-entry`

Instructs the compiler's backend to omit the entry point and the call to the `main` function from the generated output.

## `--dump-stages`

Prints all compilation stages to the terminal, allowing you to inspect the compilation pipeline and debug the compiler's behavior.
