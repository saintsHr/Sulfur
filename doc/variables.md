# Variables

Variables are used to store values in memory. Every variable must have a type, which is specified when it is declared, and a identifier that must start with an letter or underscore.

## Declaration

A variable can be declared with or without an initial value.

```
// With initialization
[type] [identifier] = [value];

// Without initialization
[type] [identifier];
```

### Examples

```
i32 age = 18;
u64 score = 1000;

i16 counter;
bool flag;
```

## Initialization

Variables declared without an initial value are **uninitialized**. You must assign a value before reading from them.

```
i32 x;
x = 42;
```

Reading an uninitialized variable results in undefined behavior.