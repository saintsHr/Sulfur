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

# Integers

Any explicit cast between integers are allowed, even if it is not safe, while implicit cast is only allowed when the conversion is safe. 