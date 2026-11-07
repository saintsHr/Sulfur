# Literals

Literals are fixed values written directly in the source code.

## Integer Literals

Integer literals represent whole numbers, they always
try to fit the type required by the context, otherwise,
implicitly casts to i64

```
i32 num1 = 421;
i16 num2 = -15;
u64 num3 = 0xDEADBEEF;
u16 num4 = 0b01101001;
```

## Boolean Literals

Boolean literals represent two possible states, true or false, and it always have
the bool type.

```
bool is_walking = true;
bool is_jumping = false;
```
