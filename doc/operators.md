# Binary Operators

Binary operators operate on two operands.

## Arithmetic

| Operator | Description    |
|----------|----------------|
| `+`      | Addition       |
| `-`      | Subtraction    |
| `*`      | Multiplication |
| `/`      | Division       |

## Bitwise

| Operator | Description |
|----------|-------------|
| `&`      | Bitwise AND |
| `|`      | Bitwise OR  |
| `^`      | Bitwise XOR |
| `>>`     | Right Shift |
| `<<`     | Left Shift  |

## Logical

| Operator | Description |
|----------|-------------|
| `&&`     | Logical AND |
| `||`     | Logical OR  |

### Example

```
i32 a = 10;
i32 b = 3;

bool c = true;
bool d = false;

i32 sum = a + b;
i32 difference = a - b;
i32 product = a * b;
i32 quotient = a / b;

i32 bitwise_and = a & b;
i32 bitwise_or = a | b;
i32 bitwise_xor = a ^ b;
i32 right_shifted = a >> b;
i32 left_shifted = a << b;

bool logical_and = d && c;
bool logical_or = c || d;
```

# Unary Operators

Unary operators operate on a single operand.

## Arithmetic

| Operator | Description         |
|----------|---------------------|
| `-`      | Arithmetic negation |

## Bitwise

| Operator | Description    |
|----------|----------------|
| `~`      | Bitwise NOT    |

### Example

```
i32 x_normal = 10;

i32 x_negative = -x;
i32 x_reversed = ~x;
```