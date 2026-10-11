# Operator Precedence

Expressions are evaluated according to the precedence rules below.
Higher precedence binds more tightly. All binary operators are left-associative
unless otherwise specified.

| Precedence     | Example / Ops                       |
| -------------- | ----------------------------------- |
| Primary        | literals, identifiers, `(expr)`     |
| Unary          | `-expr`, `~expr`                    |
| Casts          | `expr as type`                      |
| Multiplicative | `* /`                               |
| Additive       | `+ -`                               |
| Shift          | `<< >>`                             |
| Bitwise AND    | `&`                                 |
| Bitwise XOR    | `^`                                 |
| Bitwise OR     | `\|`                                |
| Relational     | `== != < <= > >=` (non-associative) |
| Logical AND    | `&&`                                |
| Logical OR     | `\|\|`                              |