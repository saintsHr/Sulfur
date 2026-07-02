## Cast

Sulfur supports both implicit and explicit casts.

Implicit casts happen automatically whenever they are guaranteed to be safe, so you usually don't need to think about them.

Explicit casts require the ```as``` keyword. They are useful when you intentionally want to convert a value, but they are not always safe, as some conversions may lose information or change the value.

```
// cast syntax:
// [value] as [type]

u64 x = 4837;
i16 y = x as i16;
```

The table below lists all supported casts.

| From | To  | Implicit | Explicit | Safe |       Notes        |
|------|-----|----------|----------|------|--------------------|
| u8   | i8  | No       | Yes      | No   | May overflow       |
| u16  | i16 | No       | Yes      | No   | May overflow       |
| u32  | i32 | No       | Yes      | No   | May overflow       |
| u64  | i64 | No       | Yes      | No   | May overflow       |
| i8   | u8  | No       | Yes      | No   | May change value   |
| i16  | u16 | No       | Yes      | No   | May change value   |
| i32  | u32 | No       | Yes      | No   | May change value   |
| i64  | u64 | No       | Yes      | No   | May change value   |