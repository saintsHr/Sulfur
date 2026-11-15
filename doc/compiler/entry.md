## Entry point

Every Sulfur program starts its execution at a special function called `main`. When the program is run, the runtime calls `main`, and the program ends when it returns.

```
// entry point signature:
// pub fn main() i32

pub fn main() i32 {
    return 0;
}
```

The signature of `main` is fixed: it must be `pub`, take no parameters and return an `i32`. Declaring `main` with any other signature is a compile-time error.

A program must have exactly one `main` function. A program with no `main`, or with more than one, fails to compile.

# Exit code

The `i32` returned by `main` is the exit code of the program, which is handed to the operating system when the program ends. By convention, `0` means success and any other value means failure.

```
pub fn main() i32 {
    if (!ok) {
        return 1; // signals failure to the caller
    }

    return 0;
}
```
