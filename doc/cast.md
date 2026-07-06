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

Explicit casts between integers are always allowed, even when unsafe. Implicit casts are only allowed when the conversion is guaranteed to be safe (i.e. no loss of information).

# Booleans

Booleans (bool) represent one of two values: true or false.
Unlike integers, bools are never convertible to or from any other type — neither implicitly nor explicitly. There is no cast, safe or unsafe, between bool and any numeric type. A bool is always a bool.