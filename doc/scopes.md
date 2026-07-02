# Scope

A scope defines where a symbol is visible and can be accessed.

Every pair of braces (`{}`) creates a new scope. Symbols declared inside a scope are only accessible within that scope and are destroyed when the scope ends.

## Example

```
i32 x = 10;

{
    i32 y = 20;

    // Both x and y are visible here.
}

// x is still accessible.
// y is no longer accessible.
```

## Shadowing

A variable declared in an inner scope may have the same name as one in an outer scope. In that case, the inner variable shadows the outer one until the scope ends.

```
i32 value = 10;

{
    i32 value = 20;

    // value == 20
}

// value == 10
```