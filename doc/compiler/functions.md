## Functions

Functions are declared with the `fn` keyword, followed by the name, the
parameter list and the return type.

```
// function syntax:
// fn [name]([type] [param], ...) [return type] { [body] }

fn some_function(i64 a, i64 b) i64 {
    return a + b;
}
```

# Parameters

Each parameter is declared as a type followed by a name, separated by commas.
A function can have any number of parameters, including none.

```
fn add(i64 a, i64 b) i64 {
    return a + b;
}

fn answer() i64 {
    return 42;
}
```

# Return type

The return type comes after the parameter list. Every path of a function that
has a return type must return a value of that type. Implicit casts apply to
returned values, so returning a value that can be safely converted to the
return type is allowed.

```
fn widen(i32 x) i64 {
    return x; // ok: implicit i32 -> i64 is safe
}

fn narrow(i64 x) i32 {
    return x; // error: implicit i64 -> i32 is not safe
}

fn narrow_explicit(i64 x) i32 {
    return x as i32; // ok
}
```

# void

`void` is the return type of functions that do not return a value. It can be
written explicitly or omitted, in which case it is implied.

```
fn log_value(i64 x) void {
    // ...
}

fn log_value_implicit(i64 x) {
    // same as above, return type is implicitly void
}
```

# Calling functions

A function is called by writing its name followed by the arguments in
parentheses. The same implicit cast rules apply to arguments.

```
i64 sum = add(2, 3);

i32 small = 10;
i64 result = add(small, 5);
```
