# If Statements

If statements are used to execute specific blocks of code, based on a boolean expression.

## If

An `if` statement executes a block of code only when its condition evaluates to
`true`.

```
if (condition) {
    // code
}
```

### Examples

```
bool adult = false;
bool perfect = false;

if (age >= 18) {
    adult = true;
}

if (score == 100) perfect = true;
```

The condition must evaluate to a `bool`.

## Else

An `else` statement can be used after an `if` to execute a block of code when the
condition evaluates to `false`.

```
if (condition) {
    // code
} else {
    // code
}
```

### Examples

```
bool adult;

if (age >= 18) {
    adult = true
} else {
    adult = false;
}
```

Only one of the two blocks is executed.

## Else If

Multiple conditions can be checked by combining `if` and `else`.

```
if (condition) {
    // code
} else if (condition) {
    // code
} else {
    // code
}
```

### Examples

```
i32 payment;

if (score >= 90) {
    payment = 1600;
} else if (score >= 80) {
    payment = 1400;
} else if (score >= 70) {
    payment = 1200;
} else {
    payment = 1000;
}
```
