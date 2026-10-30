#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/type_utils.h"

static bool analyze_binary(
    sf_semantic *s, sf_binary_expr_node *bin, sf_value_type expected
);
static bool analyze_binary_operands(
    sf_semantic *s, sf_binary_expr_node *bin, sf_value_type operand_expected
);
static bool analyze_logical(
    sf_semantic *s, sf_binary_expr_node *bin,
    sf_value_type ltype, sf_value_type rtype
);
static bool analyze_shift(
    sf_semantic *s, sf_binary_expr_node *bin,
    sf_value_type ltype, sf_value_type rtype
);
static bool analyze_relational(
    sf_semantic *s, sf_binary_expr_node *bin,
    sf_value_type ltype, sf_value_type rtype
);
static bool analyze_arithmetic(
    sf_semantic *s, sf_binary_expr_node *bin,
    sf_value_type ltype, sf_value_type rtype
);

static bool analyze_unary(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
);
static bool analyze_negate(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
);
static bool analyze_negated_literal(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
);
static bool analyze_bitwise_not(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
);
static bool analyze_increment(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
);
static bool analyze_logical_not(sf_semantic *s, sf_unary_expr_node *un);

static bool analyze_cast(sf_semantic *s, sf_cast_expr_node *cast);
static bool analyze_literal(
    sf_semantic *s, sf_literal_node *lit, sf_value_type expected
);
static bool analyze_identifier(sf_semantic *s, sf_identifier_node *id);
static bool analyze_func_call(sf_semantic *s, sf_func_call_node *call);

static bool check_argument_type(
    sf_value_type resolved, sf_value_type param_type,
    const char *func_name, size_t index, sf_span span,
    const char *filename
);

bool sf_semantic_expr(sf_semantic *s, sf_ast_node *node, sf_value_type expected) {
    switch (node->type) {
        case SF_NODE_BINARY_EXPR: {
            return analyze_binary(s, (sf_binary_expr_node *)node, expected);
        }
        case SF_NODE_UNARY_EXPR: {
            return analyze_unary(s, (sf_unary_expr_node *)node, expected);
        }
        case SF_NODE_CAST_EXPR: {
            return analyze_cast(s, (sf_cast_expr_node *)node);
        }
        case SF_NODE_LITERAL: {
            return analyze_literal(s, (sf_literal_node *)node, expected);
        }
        case SF_NODE_IDENTIFIER: {
            return analyze_identifier(s, (sf_identifier_node *)node);
        }
        case SF_NODE_FUNC_CALL: {
            return analyze_func_call(s, (sf_func_call_node *)node);
        }
        default: {
            return true;
        }
    }
}

static bool analyze_binary(
    sf_semantic *s, sf_binary_expr_node *bin, sf_value_type expected
) {
    sf_value_type operand_expected = expected;

    if (sf_op_is_logical(bin->op)) {
        operand_expected = SF_VAL_TYPE_BOOL;
    } else if (sf_op_is_relational(bin->op)) {
        operand_expected = SF_VAL_TYPE_UNRESOLVED;
    } else if (!sf_type_value_is_integer(operand_expected)) {
        operand_expected = SF_VAL_TYPE_UNRESOLVED;
    }

    if (!analyze_binary_operands(s, bin, operand_expected)) {
        return false;
    }

    sf_value_type ltype = bin->left->resolved;
    sf_value_type rtype = bin->right->resolved;

    if (ltype == SF_VAL_TYPE_UNRESOLVED || rtype == SF_VAL_TYPE_UNRESOLVED) {
        return false;
    }

    if (sf_op_is_logical(bin->op)) {
        return analyze_logical(s, bin, ltype, rtype);
    }

    if (sf_op_is_shift(bin->op)) {
        return analyze_shift(s, bin, ltype, rtype);
    }

    if (sf_op_is_relational(bin->op)) {
        return analyze_relational(s, bin, ltype, rtype);
    }

    return analyze_arithmetic(s, bin, ltype, rtype);
}

static bool analyze_binary_operands(
    sf_semantic *s, sf_binary_expr_node *bin, sf_value_type operand_expected
) {
    bool is_relational = sf_op_is_relational(bin->op);
    bool is_shift = sf_op_is_shift(bin->op);

    bool left_is_literal = bin->left->type == SF_NODE_LITERAL;
    bool right_is_literal = bin->right->type == SF_NODE_LITERAL;

    bool swap_order = is_relational && left_is_literal && !right_is_literal;

    if (swap_order) {
        if (!sf_semantic_expr(s, bin->right, operand_expected)) {
            return false;
        }

        sf_value_type left_expected = operand_expected;

        if (bin->right->resolved != SF_VAL_TYPE_UNRESOLVED) {
            left_expected = bin->right->resolved;
        }

        return sf_semantic_expr(s, bin->left, left_expected);
    }

    if (!sf_semantic_expr(s, bin->left, operand_expected)) {
        return false;
    }

    sf_value_type right_expected = operand_expected;

    if (!is_shift && bin->left->resolved != SF_VAL_TYPE_UNRESOLVED) {
        right_expected = bin->left->resolved;
    }

    return sf_semantic_expr(s, bin->right, right_expected);
}

static bool analyze_logical(
    sf_semantic *s, sf_binary_expr_node *bin,
    sf_value_type ltype, sf_value_type rtype
) {
    if (ltype != SF_VAL_TYPE_BOOL || rtype != SF_VAL_TYPE_BOOL) {
        sf_log(
            "type mismatch",
            "cannot perform a logical operation between %s and %s",
            "only use bool type literals/identifiers on logical operations",
            s->filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, bin->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(ltype),
            sf_type_value_name(rtype)
        );

        return false;
    }

    bin->base.resolved = SF_VAL_TYPE_BOOL;

    sf_const_warn(
        s, &bin->base, "this logical expression always evaluates to '%s'"
    );

    return true;
}

static bool analyze_shift(
    sf_semantic *s, sf_binary_expr_node *bin,
    sf_value_type ltype, sf_value_type rtype
) {
    bin->base.resolved = ltype;

    int64_t width = sf_type_value_width_bits(ltype);
    int64_t shift_amount;

    if (!sf_const_eval_int(bin->right, &shift_amount)) {
        return true;
    }

    if (shift_amount < 0 || shift_amount >= width) {
        sf_log(
            "shift amount out of range",
            "shift amount '%lld' is out of range for type '%s' (width %lld)",
            "use a shift amount between 0 and the type's bit width minus one",
            s->filename, SF_LOG_SEMANTIC_LITERAL_OVERFLOW, bin->base.span,
            SF_LOG_SEVERITY_ERROR, (long long)shift_amount,
            sf_type_value_name(ltype), (long long)width
        );

        return false;
    }

    return true;
}

static bool analyze_relational(
    sf_semantic *s, sf_binary_expr_node *bin,
    sf_value_type ltype, sf_value_type rtype
) {
    if (!sf_type_value_is_same_group(ltype, rtype)) {
        sf_log(
            "type mismatch", "cannot compare '%s' and '%s'",
            "cast one of the operands to match the other's type", s->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, bin->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(ltype),
            sf_type_value_name(rtype)
        );

        return false;
    }

    if (sf_type_value_width_bits(ltype) != sf_type_value_width_bits(rtype)) {
        sf_log(
            "type mismatch",
            "cannot compare '%s' and '%s' due to differing widths",
            "cast one of the operands to match the other's width", s->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, bin->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(ltype),
            sf_type_value_name(rtype)
        );

        return false;
    }

    bin->base.resolved = SF_VAL_TYPE_BOOL;

    sf_const_warn(s, &bin->base, "this comparison always evaluates to '%s'");

    return true;
}

static bool analyze_arithmetic(
    sf_semantic *s, sf_binary_expr_node *bin,
    sf_value_type ltype, sf_value_type rtype
) {
    if (!sf_type_value_is_same_group(ltype, rtype)) {
        sf_log(
            "type mismatch", "cannot mix '%s' and '%s' in the same expression",
            "cast one of the operands to match the other's type", s->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, bin->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(ltype),
            sf_type_value_name(rtype)
        );

        return false;
    }

    if (!sf_type_value_is_integer(ltype)) {
        sf_log(
            "invalid operand type",
            "operator '%s' requires integer operands, got '%s'", NULL,
            s->filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, bin->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_operation_name(bin->op),
            sf_type_value_name(ltype)
        );

        return false;
    }

    bin->base.resolved = sf_type_value_promote(ltype, rtype);

    if (
        bin->op == SF_OP_TYPE_DIV &&
        sf_const_is_zero(bin->right, sf_type_value_is_signed(bin->base.resolved))
    ) {
        sf_log(
            "division by zero", "constant expression divides by zero",
            "ensure the divisor is non-zero", s->filename,
            SF_LOG_SEMANTIC_DIVISION_BY_ZERO, bin->base.span,
            SF_LOG_SEVERITY_ERROR
        );

        return false;
    }

    return sf_const_check_overflow(&bin->base, bin->base.resolved, s->filename);
}

static bool analyze_unary(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
) {
    switch (un->op) {
        case SF_OP_TYPE_NEGATE: {
            return analyze_negate(s, un, expected);
        }
        case SF_OP_TYPE_BITWISE_NOT: {
            return analyze_bitwise_not(s, un, expected);
        }
        case SF_OP_TYPE_POSTFIX_DECREMENT:
        case SF_OP_TYPE_POSTFIX_INCREMENT:
        case SF_OP_TYPE_PREFIX_DECREMENT:
        case SF_OP_TYPE_PREFIX_INCREMENT: {
            return analyze_increment(s, un, expected);
        }
        case SF_OP_TYPE_LOGICAL_NOT: {
            return analyze_logical_not(s, un);
        }
        default: {
            return false;
        }
    }
}

static bool analyze_negate(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
) {
    sf_ast_node *operand = un->operand;

    if (
        operand->type == SF_NODE_LITERAL &&
        ((sf_literal_node *)operand)->token_type == SF_TOKEN_TYPE_INTEGER
    ) {
        return analyze_negated_literal(s, un, expected);
    }

    sf_value_type pass_down = expected;

    if (!sf_type_value_is_signed(expected)) {
        pass_down = SF_VAL_TYPE_I64;
    }

    if (!sf_semantic_expr(s, operand, pass_down)) {
        return false;
    }

    sf_value_type child_type = operand->resolved;

    if (child_type == SF_VAL_TYPE_UNRESOLVED) {
        return false;
    }

    if (!sf_type_value_is_signed(child_type)) {
        sf_log(
            "invalid operand to unary '-'",
            "operator '-' requires a signed integer operand, got '%s'", NULL,
            s->filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, un->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(child_type)
        );

        return false;
    }

    un->base.resolved = child_type;

    return true;
}

static bool analyze_negated_literal(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
) {
    sf_literal_node *lit = (sf_literal_node *)un->operand;
    sf_value_type target = expected;

    if (
        target != SF_VAL_TYPE_UNRESOLVED &&
        sf_type_value_is_integer(target) && !sf_type_value_is_signed(target)
    ) {
        sf_log(
            "negative literal in unsigned context",
            "cannot assign a negative literal to a variable of unsigned type "
            "'%s'",
            "use a signed type, or remove the negation", s->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, un->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(target)
        );

        return false;
    }

    if (
        target == SF_VAL_TYPE_UNRESOLVED || !sf_type_value_is_integer(target) ||
        !sf_type_value_is_signed(target)
    ) {
        target = SF_VAL_TYPE_I64;
    }

    errno = 0;
    uint64_t magnitude = strtoull(lit->value, NULL, 10);

    if (
        errno == ERANGE ||
        !sf_type_value_signed_literal_fits_negated(target, magnitude)
    ) {
        sf_log(
            "integer literal out of range",
            "literal '-%s' does not fit in type '%s'",
            "use a wider type, or cast explicitly if truncation is intended",
            s->filename, SF_LOG_SEMANTIC_LITERAL_OVERFLOW, un->base.span,
            SF_LOG_SEVERITY_ERROR, lit->value, sf_type_value_name(target)
        );

        return false;
    }

    lit->base.resolved = target;
    un->base.resolved = target;

    return true;
}

static bool analyze_bitwise_not(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
) {
    if (!sf_semantic_expr(s, un->operand, expected)) {
        return false;
    }

    sf_value_type child_type = un->operand->resolved;

    if (!sf_type_value_is_integer(child_type)) {
        sf_log(
            "invalid operand to '~'",
            "operator '~' requires an integer operand, got '%s'", NULL,
            s->filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, un->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(child_type)
        );

        return false;
    }

    un->base.resolved = child_type;

    return true;
}

static bool analyze_increment(
    sf_semantic *s, sf_unary_expr_node *un, sf_value_type expected
) {
    if (un->operand->type != SF_NODE_IDENTIFIER) {
        sf_log(
            "invalid operand", "operand of '++'/'--' must be a lvalue",
            "only lvalues can be incremented or decremented", s->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, un->base.span, SF_LOG_SEVERITY_ERROR
        );

        return false;
    }

    if (!sf_semantic_expr(s, un->operand, expected)) {
        return false;
    }

    sf_value_type child_type = un->operand->resolved;

    if (!sf_type_value_is_integer(child_type)) {
        sf_log(
            "invalid operand type",
            "operator '++'/'--' requires an integer operand, got '%s'", NULL,
            s->filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, un->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(child_type)
        );

        return false;
    }

    un->base.resolved = child_type;

    return true;
}

static bool analyze_logical_not(sf_semantic *s, sf_unary_expr_node *un) {
    if (!sf_semantic_expr(s, un->operand, SF_VAL_TYPE_BOOL)) {
        return false;
    }

    if (un->operand->resolved != SF_VAL_TYPE_BOOL) {
        sf_log(
            "invalid operand to '!'",
            "operator '!' requires a boolean operand, got '%s'", NULL,
            s->filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, un->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(un->operand->resolved)
        );

        return false;
    }

    sf_const_warn(s, &un->base, "this expression always evaluates to '%s'");

    un->base.resolved = SF_VAL_TYPE_BOOL;

    return true;
}

static bool analyze_cast(sf_semantic *s, sf_cast_expr_node *cast) {
    if (!sf_semantic_expr(s, cast->operand, SF_VAL_TYPE_UNRESOLVED)) {
        return false;
    }

    sf_value_type from_type = cast->operand->resolved;
    sf_value_type to_type = cast->target_type;

    if (from_type == SF_VAL_TYPE_UNRESOLVED) {
        return false;
    }

    if (from_type == to_type) {
        sf_log(
            "useless cast",
            "the expression already has type '%s'",
            "remove the cast", s->filename,
            SF_LOG_SEMANTIC_USELESS_CAST, cast->base.span,
            SF_LOG_SEVERITY_WARNING, sf_type_value_name(to_type)
        );
    }

    if (!sf_type_value_is_castable(from_type, to_type)) {
        sf_log(
            "invalid cast", "cannot cast '%s' to '%s'",
            "these types are not compatible for casting", s->filename,
            SF_LOG_SEMANTIC_INVALID_EXPLICIT_CAST, cast->base.span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(from_type),
            sf_type_value_name(to_type)
        );

        return false;
    }

    cast->base.resolved = to_type;

    return true;
}

static bool analyze_literal(
    sf_semantic *s, sf_literal_node *lit, sf_value_type expected
) {
    if (lit->token_type == SF_TOKEN_TYPE_KW_TRUE || lit->token_type == SF_TOKEN_TYPE_KW_FALSE) {
        lit->base.resolved = SF_VAL_TYPE_BOOL;
        return true;
    }

    if (lit->token_type != SF_TOKEN_TYPE_INTEGER) {
        return true;
    }

    sf_value_type target = expected;

    if (target == SF_VAL_TYPE_UNRESOLVED || !sf_type_value_is_integer(target)) {
        target = SF_VAL_TYPE_I64;
    }

    errno = 0;
    uint64_t raw = strtoull(lit->value, NULL, 10);

    if (errno == ERANGE) {
        sf_log(
            "integer literal out of range",
            "literal '%s' is too large to represent",
            "this value does not fit in any integer type", s->filename,
            SF_LOG_SEMANTIC_LITERAL_OVERFLOW, lit->base.span,
            SF_LOG_SEVERITY_ERROR, lit->value
        );

        return false;
    }

    if (!sf_type_value_uint_literal_fits(target, raw)) {
        sf_log(
            "integer literal out of range",
            "literal '%s' does not fit in type '%s'",
            "use a wider type, or cast explicitly if truncation is intended",
            s->filename, SF_LOG_SEMANTIC_LITERAL_OVERFLOW, lit->base.span,
            SF_LOG_SEVERITY_ERROR, lit->value, sf_type_value_name(target)
        );

        return false;
    }

    lit->base.resolved = target;

    return true;
}

static bool analyze_identifier(sf_semantic *s, sf_identifier_node *id) {
    sf_var_symbol *sym = scope_lookup(s->scope, id->name);

    if (sym == NULL) {
        sf_semantic_report_undeclared(s, id->name, id->base.span);
        return false;
    }

    if (!sym->initialized) {
        sf_log(
            "uninitialized variable", "'%s' is used before being initialized",
            "assign a value to the variable before using it", s->filename,
            SF_LOG_SEMANTIC_UNINITIALIZED, id->base.span,
            SF_LOG_SEVERITY_ERROR, id->name
        );

        return false;
    }

    id->base.resolved = sym->type;
    id->depth = sym->depth;
    id->id = sym->id;

    return true;
}

static bool analyze_func_call(sf_semantic *s, sf_func_call_node *call) {
    sf_identifier_node *callee = (sf_identifier_node *)call->callee;
    const sf_func_symbol *sym = sf_func_symbol_table_lookup(s->funcs, callee->name);

    if (sym == NULL) {
        const char* closest = sf_func_symbol_table_find_closest(s->funcs, callee->name);

        if (closest != NULL) {
            sf_log(
                "undeclared function", "function '%s' is not declared until this call",
                "did you mean '%s'?", s->filename, SF_LOG_SEMANTIC_UNDECLARED,
                callee->base.span, SF_LOG_SEVERITY_ERROR, callee->name, closest
            );
        } else {
            sf_log(
                "undeclared function", "function '%s' is not declared until this call",
                "declare the function before calling", s->filename, SF_LOG_SEMANTIC_UNDECLARED,
                callee->base.span, SF_LOG_SEVERITY_ERROR, callee->name
            );
        }

        for (size_t i = 0; i < call->argument_count; i++) {
            sf_semantic_expr(s, call->arguments[i], SF_VAL_TYPE_UNRESOLVED);
        }

        return false;
    }

    bool ok = true;

    for (size_t i = 0; i < call->argument_count; i++) {
        sf_value_type expected = SF_VAL_TYPE_UNRESOLVED;
        bool has_param = i < sym->parameter_count;

        if (has_param) {
            expected = sym->parameters[i].type;
        }

        sf_ast_node *arg = call->arguments[i];

        if (!sf_semantic_expr(s, arg, expected)) {
            ok = false;
            continue;
        }

        if (
            has_param &&
            !check_argument_type(
                arg->resolved, expected, sym->name,
                i, arg->span, s->filename
            )
        ) {
            ok = false;
        }
    }

    if (call->argument_count != sym->parameter_count) {
        sf_log(
            "wrong argument count",
            "function '%s' expects %zu arguments, but %zu were provided",
            "pass exactly the number of arguments the function declares",
            s->filename, SF_LOG_SEMANTIC_ARGUMENT_COUNT,
            callee->base.span, SF_LOG_SEVERITY_ERROR, sym->name,
            sym->parameter_count, call->argument_count
        );

        ok = false;
    }

    if (ok == true) {
        call->base.resolved = sym->return_type;
    }

    return ok;
}

static bool check_argument_type(
    sf_value_type resolved, sf_value_type param_type,
    const char *func_name, size_t index, sf_span span,
    const char *filename
) {
    if (resolved == SF_VAL_TYPE_UNRESOLVED) {
        return true;
    }

    if (!sf_type_value_is_same_group(resolved, param_type)) {
        sf_log(
            "type mismatch",
            "argument %zu of '%s' expects '%s', but this expression has type '%s'",
            "make sure the argument matches the parameter type, or cast it",
            filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, span,
            SF_LOG_SEVERITY_ERROR, index + 1, func_name,
            sf_type_value_name(param_type), sf_type_value_name(resolved)
        );

        return false;
    }

    if (
        sf_type_value_width_bytes(resolved) >
        sf_type_value_width_bytes(param_type)
    ) {
        sf_log(
            "narrowing conversion",
            "cannot implicitly narrow '%s' to '%s' in argument %zu of '%s'",
            "cast the argument explicitly, or change the parameter type",
            filename, SF_LOG_SEMANTIC_INVALID_IMPLICIT_CAST, span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(resolved),
            sf_type_value_name(param_type), index + 1, func_name
        );

        return false;
    }

    return true;
}
