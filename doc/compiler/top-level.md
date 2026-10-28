# Top-Level Declarations

The top-level scope is used to define the global elements of a program. It can contain function definitions, but cannot contain executable statements directly.

## Function Definitions

Functions can be defined at the top level. Their bodies contain the executable code that runs when the function is called.

```
i32 add(i32 a, i32 b) {
    return a + b;
}
```

## Executable Statements

Executable statements cannot appear directly in the top-level scope. They must be placed inside a function body.

### Examples

The following is valid:

```
i32 calculate(i32 value) {
    return value * 2;
}
```

The following is invalid:

```
print("Hello, World!");

i32 result = calculate(10);
```

The statements above are not allowed because they attempt to execute code directly in the top-level scope.

## Execution Context

The top-level scope is reserved for function definitions. Executable statements must be placed inside function bodies and are executed when the corresponding function is called.
