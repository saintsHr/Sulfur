# Binary Operators

Binary operators operate on two operands. Some binary operators can also be combined with assignment to update the value of the left-hand operand.

## Arithmetic Operators

| Operator | Description                   |
| -------- | ----------------------------- |
| `+`      | Addition                      |
| `-`      | Subtraction                   |
| `*`      | Multiplication                |
| `/`      | Division                      |
| `+=`     | Addition and assignment       |
| `-=`     | Subtraction and assignment    |
| `*=`     | Multiplication and assignment |
| `/=`     | Division and assignment       |

Compound assignment operators combine an arithmetic or bitwise operation with assignment.

For example:

```text
a += b;
```

is equivalent to:

```text
a = a + b;
```

### Example

```text
i32 a = 10;
i32 b = 3;

// Arithmetic
i32 sum = a + b;
i32 difference = a - b;
i32 product = a * b;
i32 quotient = a / b;

// Compound assignment
a += b; // a = a + b
a -= b; // a = a - b
a *= b; // a = a * b
a /= b; // a = a / b
```

## Bitwise Operators

Bitwise operators perform operations on the individual bits of integer operands.

| Operator | Description                |
| -------- | -------------------------- |
| `&`      | Bitwise AND                |
| `\|`      | Bitwise OR                 |
| `^`      | Bitwise XOR                |
| `>>`     | Right shift                |
| `<<`     | Left shift                 |
| `&=`     | Bitwise AND and assignment |
| `\|=`    | Bitwise OR and assignment  |
| `^=`     | Bitwise XOR and assignment |
| `>>=`    | Right shift and assignment |
| `<<=`    | Left shift and assignment  |

For example:

```text
a &= b;
```

is equivalent to:

```text
a = a & b;
```

### Example

```text
i32 a = 10;
i32 b = 3;

// Bitwise
i32 bitwise_and = a & b;
i32 bitwise_or = a | b;
i32 bitwise_xor = a ^ b;
i32 right_shifted = a >> b;
i32 left_shifted = a << b;

// Compound assignment
a &= b;  // a = a & b
a |= b;  // a = a | b
a ^= b;  // a = a ^ b
a >>= b; // a = a >> b
a <<= b; // a = a << b
```

## Logical Operators

Logical operators operate on boolean operands and produce a boolean result.

| Operator | Description |
| -------- | ----------- |
| `&&`     | Logical AND |
| `\|\|`   | Logical OR  |

### Example

```text
bool c = true;
bool d = false;

bool logical_and = d && c;
bool logical_or = c || d;
```

# Unary Operators

Unary operators operate on a single operand.

## Arithmetic Operators

| Operator | Description         |
| -------- | ------------------- |
| `-`      | Arithmetic negation |

## Bitwise Operators

| Operator | Description |
| -------- | ----------- |
| `~`      | Bitwise NOT |

### Example

```text
i32 x = 10;

i32 x_negative = -x;
i32 x_reversed = ~x;
```

Here, `-x` negates the value of `x`, while `~x` inverts every bit of its integer representation.
