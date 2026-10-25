# Operator Precedence

Expressions are parsed according to the precedence rules below.
Higher precedence binds more tightly.

| Precedence     | Ops                                                      | Associativity |
| -------------- | -------------------------------------------------------- | ------------- |
| Primary        | integer literals, `true`, `false`, identifiers, `(expr)` | n/a           |
| Postfix        | `expr++`, `expr--`                                       | left          |
| Unary (prefix) | `-expr`, `~expr`, `!expr`, `++expr`, `--expr`            | right         |
| Cast           | `expr as type`                                           | left          |
| Multiplicative | `*` `/`                                                  | left          |
| Additive       | `+` `-`                                                  | left          |
| Shift          | `<<` `>>`                                                | left          |
| Bitwise AND    | `&`                                                      | left          |
| Bitwise XOR    | `^`                                                      | left          |
| Bitwise OR     | `\|`                                                     | left          |
| Relational     | `==` `!=` `<` `<=` `>` `>=`                              | non-chainable |
| Logical AND    | `&&`                                                     | left          |
| Logical OR     | `\|\|`                                                   | left          |

## Associativity

Binary operators are left-associative, except relational operators.

Prefix unary operators are right-associative: the operand of a prefix
operator is itself parsed as a prefix expression, so `-~x` is `-(~x)`
and `!!x` is `!(!x)`.
Note that `--x` is a prefix decrement, not a double negation.
Write `- -x` or `-(-x)` for that.

Relational operators are not chainable. After one relational operator is
parsed, the expression ends at that level. In `a < b < c` the second `<`
is not part of the expression, and the rest is left to the enclosing syntax.
The parser has no dedicated diagnostic for this. Use parentheses or
`&&` instead: `(a < b) && (b < c)`.
This also applies to mixed relational operators such as `a < b == c`.

## Postfix

Postfix `++` and `--` apply to a primary expression (a literal, an
identifier, or a parenthesized expression), and can be repeated.
Postfix binds tighter than prefix operators, so `-x++` is `-(x++)`.

## Casts

The operand of `as` is a prefix-unary-level expression, and the target
must be a type keyword. Casts chain left to right.

- `a + b as i64` is `a + (b as i64)`.
  To convert the result of an operation, write `(a + b) as i64`.
- `-x as u32` is `(-x) as u32`.
- `x as i64 as u8` is `(x as i64) as u8`.
- `x as i64 * 2` is `(x as i64) * 2`.
- The cast loop does not consume `++` or `--`, so they cannot directly
  follow `as type`. `(x as i64)++` is syntactically a postfix applied to a
  parenthesized expression; whether it is valid is decided by later stages,
  not by the parser.

## Relational vs. bitwise

Bitwise operators bind tighter than relational ones, so
`a & b == c` is `(a & b) == c`.

## Assignments are statements

Assignments are not expressions. A statement that starts with an identifier
followed by an assignment operator is parsed as an assignment. Compound
assignments (`op=`) are rewritten to `name = name op rhs`.

## Not defined by the parser

- Whether `++`/`--` (prefix or postfix) require an assignable operand is
  checked after parsing. The parser accepts, for example, `++-x` and `5++`.
