## Qualifiers

Qualifiers modify how a declaration behaves. They are placed before the type.

```
// qualifier syntax:
// [qualifier] [type] [name]

mut u32 counter = 0;
```

# mut

`mut` applies only to variables.

By default, variables in Sulfur are immutable: once a value is assigned, it
cannot change. The `mut` qualifier makes a variable mutable, allowing it to be
reassigned after its declaration.

```
u32 a = 10;
a = 20; // error: 'a' is not mutable

mut u32 b = 10;
b = 20; // ok
```

Mutability is part of the variable, not the type. Copying a value into a new
variable does not carry `mut` over, so the new variable is immutable unless
declared otherwise.

# pub

`pub` applies only to functions.

By default, every function is private to the file (or module) where it is
defined. The `pub` qualifier makes a function globally linked and visible
to other modules, so they can call it.

```
// visible to other modules
pub fn add(i64 a, i64 b) i64 {
    return a + b;
}

// private to this module
fn helper(u32 a) u32 {
    return a * 2;
}
```

Only functions marked `pub` are part of a module's public interface.

# extern

`extern` applies only to functions.

The `extern` qualifier declares a function that is defined elsewhere, such as
in a C library or another object file. It tells the compiler that the function
exists and what its signature is, but does not define it. The linker resolves
it later.

```extern fn abs(i32 x) i64;```

Since the compiler cannot see the definition, an ```extern``` function cannot
have a body.

# Summary

| Qualifier | Applies to | Effect                                                  |
|-----------|------------|---------------------------------------------------------|
| `mut`     | variables  | Allows reassignment after declaration.                  |
| `pub`     | functions  | Makes the function global and visible to other modules. |
| `extern`  | functions  | Declares a function defined elsewhere.                  |
