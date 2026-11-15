## Arithmetic

The arithmetic part of the `std::math` module provides common arithmetic functions.

Each function is documented with its signature, so check the parameter and return types to see which numeric types it supports.

```
// usage:
// std::math::[function]([args])

i64 x = std::math::pow(2, 10); // 1024
u64 y = std::math::sqrt(144);  // 12
```

# Available functions

| Function                         | Returns | Description                      |
|----------------------------------|---------|----------------------------------|
| `pow(i64 base, u64 exp)`         | `i64`   | Raises `base` to the power `exp` |
| `sqrt(u64 n)`                    | `u64`   | Square root of `n`               |
| `abs(i64 n)`                     | `u64`   | Absolute value of `n`            |
| `max(i64 a, i64 b)`              | `i64`   | Larger of two values             |
| `min(i64 a, i64 b)`              | `i64`   | Smaller of two values            |
| `clamp(i64 n, i64 min, i64 max)` | `i64`   | Restricts `n` to a range         |
| `gcd(i64 a, i64 b)`              | `i64`   | Greatest common divisor          |
| `lcm(i64 a, i64 b)`              | `i64`   | Least common multiple            |

# `pow`

Returns `base` raised to the power `exp`. Any value raised to `0` is `1`, including `0`.

```
std::math::pow(2, 10); // 1024
std::math::pow(-3, 3); // -27
std::math::pow(7, 0);  // 1
```

The exponent is unsigned, so negative exponents are not supported. The result must fit in the return type.

# `sqrt`

Returns the square root of `n`. For integers, the result is rounded down: it is the largest `x` such that `x * x <= n`.

```
std::math::sqrt(144); // 12
std::math::sqrt(10);  // 3
std::math::sqrt(0);   // 0
```

# `abs`

Returns the absolute value of `n`. The return type is unsigned, so `abs` is defined for every input, including the minimum value of the signed type, whose absolute value does not fit in that same type.

```
std::math::abs(-42); // 42
std::math::abs(42);  // 42
```

# `max` and `min`

`max` returns the larger of `a` and `b`, and `min` returns the smaller. If both are equal, that value is returned.

```
std::math::max(3, 8); // 8
std::math::min(3, 8); // 3
```

# `clamp`

Restricts `n` to the range from `min` to `max` (both inclusive). If `n` is above `max`, `max` is returned. If `n` is below `min`, `min` is returned. Otherwise `n` is returned unchanged.

```
std::math::clamp(15, 0, 10); // 10
std::math::clamp(-5, 0, 10); // 0
std::math::clamp(7, 0, 10);  // 7
```

`min` must be less than or equal to `max`. Otherwise the result is not meaningful.

# `gcd`

Returns the greatest common divisor of `a` and `b`. The result is always non-negative, regardless of the signs of the arguments. `gcd(a, 0)` is the absolute value of `a`, and `gcd(0, 0)` is `0`.

```
std::math::gcd(12, 18);  // 6
std::math::gcd(-12, 18); // 6
std::math::gcd(5, 0);    // 5
```

# `lcm`

Returns the least common multiple of `a` and `b`. The result is always non-negative. If either argument is `0`, the result is `0`.

```
std::math::lcm(4, 6);  // 12
std::math::lcm(-4, 6); // 12
std::math::lcm(0, 6);  // 0
```

The result must fit in the return type.
