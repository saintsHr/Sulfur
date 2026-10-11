# While Statements

While statements are used to repeatedly execute a block of code, as long as a
boolean expression evaluates to `true`.

## While
A `while` statement executes a block of code repeatedly, checking its
condition before every iteration. The loop stops as soon as the condition
evaluates to `false`.

```
while (condition) {
    // code
}
```

### Examples

```
u64 i = 0;
while (i < 10) {
    i = i + 1;
}
```

```
u64 i = 0;
while (i < 1000000) i = i + 1;
```

The condition must evaluate to a `bool`, and is re-evaluated at the start of
every iteration.

## Loop Body

The body of a `while` can be a single statement or a block, following the
same rule as `if`. If the condition is `false` on the first check, the body
never executes.

```
bool found = false;
u64 i = 0;
while (!found) {
    if (i == 42) {
        found = true;
    }
    i = i + 1;
}
```

## Infinite Loops
Omitting a way to make the condition eventually `false` results in a loop
that never stops.

```
while (true) {
    // runs forever
}
```