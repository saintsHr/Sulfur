# Literals

Literals are fixed values written directly in the source code.

## Integer Literals

Integer literals represent whole numbers, they always
try to fit the type required by the context, otherwise,
implicitly casts to i64

```
i32 positive = 42;
i16 negative = -15;
u64 zero = 0;
```

## Boolean Literals

Boolean literals represent two possible states, true or false, and it always have the bool type.

```
bool is_walking = true;
bool is_jumping = false;
```