## Attributes

Attributes attach extra information to a statement, changing how the compiler treats it. They don't change what the code does on its own terms: they tell the compiler how to handle the item they are applied to (for example, how its symbol is named in the output).

An attribute is written with `@` followed by its name, placed directly before the statement it applies to.

```
// attribute syntax:
// @[name]
// [statement]

@no_mangle
void my_function() {
    // ...
}
```

Each attribute only applies to specific kinds of declarations. Applying an attribute to a declaration it doesn't support is a compile-time error.

Multiple attributes can be applied to the same declaration by stacking them. The order does not matter.

```
@attribute_a
@attribute_b
void my_function() {
    // ...
}
```

# Available attributes

| Attribute    | Applies to |
|--------------|------------|
| `@no_mangle` | Functions  |

# `@no_mangle`

By default, the compiler mangles the names of functions, changing the symbol name in the compiled output so that it is unique and encodes extra information (such as the module it belongs to).

`@no_mangle` disables this for the function it is applied to, so the symbol keeps exactly the name written in the source. This is useful when the function must be callable by name from outside Sulfur, such as from C code or a linker script.

```
@no_mangle
void add(i32 a, i32 b) {
    // exported with the symbol name "add"
}
```

`@no_mangle` can only be applied to functions. Since the symbol name is used as-is, two `@no_mangle` functions with the same name will conflict at link time, even if they live in different modules.
