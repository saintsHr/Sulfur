#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/type_utils.h"

static bool literal_magnitude(const sf_ast_node *node, uint64_t *out_value);

static bool eval_uint_binary(
    sf_operation_type op, uint64_t l, uint64_t r, uint64_t *out_value
);
static bool eval_int_binary(
    sf_operation_type op, int64_t l, int64_t r, int64_t *out_value
);
static bool eval_int_negate(const sf_unary_expr_node *un, int64_t *out_value);
static bool eval_bool_binary(sf_binary_expr_node *bin, bool *out_value);
static bool eval_relational(sf_binary_expr_node *bin, bool *out_value);

static bool compare_int(sf_operation_type op, int64_t l, int64_t r, bool *out_value);
static bool compare_uint(sf_operation_type op, uint64_t l, uint64_t r, bool *out_value);

static bool signed_fits(sf_value_type type, int64_t value);
static bool check_signed_overflow(
    sf_ast_node *node, sf_value_type resolved, const char *filename
);
static bool check_unsigned_overflow(
    sf_ast_node *node, sf_value_type resolved, const char *filename
);

bool sf_const_eval_uint(sf_ast_node *node, uint64_t *out_value) {
    if (node->type == SF_NODE_LITERAL) {
        return literal_magnitude(node, out_value);
    }

    if (node->type != SF_NODE_BINARY_EXPR) {
        return false;
    }

    sf_binary_expr_node *bin = (sf_binary_expr_node *)node;
    uint64_t l, r;

    if (!sf_const_eval_uint(bin->left, &l)) {
        return false;
    }

    if (!sf_const_eval_uint(bin->right, &r)) {
        return false;
    }

    return eval_uint_binary(bin->op, l, r, out_value);
}

bool sf_const_eval_int(sf_ast_node *node, int64_t *out_value) {
    if (node->type == SF_NODE_LITERAL) {
        uint64_t magnitude;

        if (!literal_magnitude(node, &magnitude)) {
            return false;
        }

        if (magnitude > (uint64_t)INT64_MAX) {
            return false;
        }

        *out_value = (int64_t)magnitude;
        return true;
    }

    if (node->type == SF_NODE_UNARY_EXPR) {
        return eval_int_negate((const sf_unary_expr_node *)node, out_value);
    }

    if (node->type != SF_NODE_BINARY_EXPR) {
        return false;
    }

    sf_binary_expr_node *bin = (sf_binary_expr_node *)node;
    int64_t l, r;

    if (!sf_const_eval_int(bin->left, &l)) {
        return false;
    }

    if (!sf_const_eval_int(bin->right, &r)) {
        return false;
    }

    return eval_int_binary(bin->op, l, r, out_value);
}

bool sf_const_eval_bool(sf_ast_node *node, bool *out_value) {
    if (node->type == SF_NODE_LITERAL) {
        sf_literal_node *lit = (sf_literal_node *)node;

        if (lit->token_type == SF_TOKEN_TYPE_KW_TRUE) {
            *out_value = true;
            return true;
        }

        if (lit->token_type == SF_TOKEN_TYPE_KW_FALSE) {
            *out_value = false;
            return true;
        }

        return false;
    }

    if (node->type == SF_NODE_UNARY_EXPR) {
        sf_unary_expr_node *un = (sf_unary_expr_node *)node;

        if (un->op != SF_OP_TYPE_LOGICAL_NOT) {
            return false;
        }

        bool operand_value;

        if (!sf_const_eval_bool(un->operand, &operand_value)) {
            return false;
        }

        *out_value = !operand_value;
        return true;
    }

    if (node->type == SF_NODE_BINARY_EXPR) {
        return eval_bool_binary((sf_binary_expr_node *)node, out_value);
    }

    return false;
}

bool sf_const_check_overflow(
    sf_ast_node *node, sf_value_type resolved, const char *filename
) {
    if (sf_type_value_is_signed(resolved)) {
        return check_signed_overflow(node, resolved, filename);
    }

    return check_unsigned_overflow(node, resolved, filename);
}

bool sf_const_is_zero(sf_ast_node *node, bool is_signed) {
    if (is_signed) {
        int64_t value;
        return sf_const_eval_int(node, &value) && value == 0;
    }

    uint64_t value;
    return sf_const_eval_uint(node, &value) && value == 0;
}

void sf_const_warn(sf_semantic *s, sf_ast_node *node, const char *desc) {
    bool value;

    if (!sf_const_eval_bool(node, &value)) {
        return;
    }

    sf_log(
        "constant condition", desc, "check the operands and operators",
        s->filename, SF_LOG_SEMANTIC_CONSTANT_EXPR, node->span,
        SF_LOG_SEVERITY_WARNING, value ? "true" : "false"
    );
}

static bool literal_magnitude(const sf_ast_node *node, uint64_t *out_value) {
    if (node->type != SF_NODE_LITERAL) {
        return false;
    }

    const sf_literal_node *lit = (const sf_literal_node *)node;

    if (lit->token_type != SF_TOKEN_TYPE_INTEGER) {
        return false;
    }

    errno = 0;
    uint64_t value = strtoull(lit->value, NULL, 10);

    if (errno == ERANGE) {
        return false;
    }

    *out_value = value;
    return true;
}

static bool eval_uint_binary(
    sf_operation_type op, uint64_t l, uint64_t r, uint64_t *out_value
) {
    switch (op) {
        case SF_OP_TYPE_ADD: {
            if (r > UINT64_MAX - l) {
                return false;
            }

            *out_value = l + r;
            return true;
        }
        case SF_OP_TYPE_SUB: {
            if (l < r) {
                return false;
            }

            *out_value = l - r;
            return true;
        }
        case SF_OP_TYPE_MUL: {
            if (l != 0 && r > UINT64_MAX / l) {
                return false;
            }

            *out_value = l * r;
            return true;
        }
        case SF_OP_TYPE_DIV: {
            if (r == 0) {
                return false;
            }

            *out_value = l / r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_LSHIFT: {
            if (r >= 64) {
                return false;
            }

            *out_value = l << r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_RSHIFT: {
            if (r >= 64) {
                return false;
            }

            *out_value = l >> r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_AND: {
            *out_value = l & r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_OR: {
            *out_value = l | r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_XOR: {
            *out_value = l ^ r;
            return true;
        }
        default: {
            return false;
        }
    }
}

static bool eval_int_binary(
    sf_operation_type op, int64_t l, int64_t r, int64_t *out_value
) {
    switch (op) {
        case SF_OP_TYPE_ADD: {
            if (r > 0 && l > INT64_MAX - r) {
                return false;
            }

            if (r < 0 && l < INT64_MIN - r) {
                return false;
            }

            *out_value = l + r;
            return true;
        }
        case SF_OP_TYPE_SUB: {
            if (r < 0 && l > INT64_MAX + r) {
                return false;
            }

            if (r > 0 && l < INT64_MIN + r) {
                return false;
            }

            *out_value = l - r;
            return true;
        }
        case SF_OP_TYPE_MUL: {
            int64_t result;

            if (__builtin_mul_overflow(l, r, &result)) {
                return false;
            }

            *out_value = result;
            return true;
        }
        case SF_OP_TYPE_DIV: {
            if (r == 0) {
                return false;
            }

            if (l == INT64_MIN && r == -1) {
                return false;
            }

            *out_value = l / r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_LSHIFT: {
            if (r < 0 || r >= 64) {
                return false;
            }

            *out_value = (int64_t)((uint64_t)l << r);
            return true;
        }
        case SF_OP_TYPE_BITWISE_RSHIFT: {
            if (r < 0 || r >= 64) {
                return false;
            }

            *out_value = l >> r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_AND: {
            *out_value = l & r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_OR: {
            *out_value = l | r;
            return true;
        }
        case SF_OP_TYPE_BITWISE_XOR: {
            *out_value = l ^ r;
            return true;
        }
        default: {
            return false;
        }
    }
}

static bool eval_int_negate(const sf_unary_expr_node *un, int64_t *out_value) {
    if (un->op != SF_OP_TYPE_NEGATE) {
        return false;
    }

    if (un->operand->type == SF_NODE_LITERAL) {
        uint64_t magnitude;

        if (!literal_magnitude(un->operand, &magnitude)) {
            return false;
        }

        if (magnitude > (uint64_t)INT64_MAX + 1) {
            return false;
        }

        if (magnitude == (uint64_t)INT64_MAX + 1) {
            *out_value = INT64_MIN;
        } else {
            *out_value = -(int64_t)magnitude;
        }

        return true;
    }

    int64_t child;

    if (!sf_const_eval_int(un->operand, &child)) {
        return false;
    }

    if (child == INT64_MIN) {
        return false;
    }

    *out_value = -child;
    return true;
}

static bool eval_bool_binary(sf_binary_expr_node *bin, bool *out_value) {
    if (sf_op_is_logical(bin->op)) {
        bool l, r;

        if (!sf_const_eval_bool(bin->left, &l)) {
            return false;
        }

        if (!sf_const_eval_bool(bin->right, &r)) {
            return false;
        }

        *out_value = (bin->op == SF_OP_TYPE_LOGICAL_AND) ? (l && r) : (l || r);

        return true;
    }

    if (sf_op_is_relational(bin->op)) {
        return eval_relational(bin, out_value);
    }

    return false;
}

static bool eval_relational(sf_binary_expr_node *bin, bool *out_value) {
    sf_value_type type = bin->left->resolved;

    if (type == SF_VAL_TYPE_BOOL) {
        if (
            bin->op != SF_OP_TYPE_RELATIONAL_EQUAL &&
            bin->op != SF_OP_TYPE_RELATIONAL_NOT_EQUAL
        ) {
            return false;
        }

        bool l, r;

        if (!sf_const_eval_bool(bin->left, &l)) {
            return false;
        }

        if (!sf_const_eval_bool(bin->right, &r)) {
            return false;
        }

        *out_value = (bin->op == SF_OP_TYPE_RELATIONAL_EQUAL) ? (l == r) : (l != r);

        return true;
    }

    if (sf_type_value_is_signed(type)) {
        int64_t l, r;

        if (!sf_const_eval_int(bin->left, &l)) {
            return false;
        }

        if (!sf_const_eval_int(bin->right, &r)) {
            return false;
        }

        return compare_int(bin->op, l, r, out_value);
    }

    uint64_t l, r;

    if (!sf_const_eval_uint(bin->left, &l)) {
        return false;
    }

    if (!sf_const_eval_uint(bin->right, &r)) {
        return false;
    }

    return compare_uint(bin->op, l, r, out_value);
}

static bool compare_int(sf_operation_type op, int64_t l, int64_t r, bool *out_value) {
    switch (op) {
        case SF_OP_TYPE_RELATIONAL_EQUAL: {
            *out_value = l == r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_NOT_EQUAL: {
            *out_value = l != r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_LESS: {
            *out_value = l < r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_LESS_EQUAL: {
            *out_value = l <= r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_GREATER: {
            *out_value = l > r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_GREATER_EQUAL: {
            *out_value = l >= r;
            return true;
        }
        default: {
            return false;
        }
    }
}

static bool compare_uint(sf_operation_type op, uint64_t l, uint64_t r, bool *out_value) {
    switch (op) {
        case SF_OP_TYPE_RELATIONAL_EQUAL: {
            *out_value = l == r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_NOT_EQUAL: {
            *out_value = l != r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_LESS: {
            *out_value = l < r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_LESS_EQUAL: {
            *out_value = l <= r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_GREATER: {
            *out_value = l > r;
            return true;
        }
        case SF_OP_TYPE_RELATIONAL_GREATER_EQUAL: {
            *out_value = l >= r;
            return true;
        }
        default: {
            return false;
        }
    }
}

static bool signed_fits(sf_value_type type, int64_t value) {
    switch (type) {
        case SF_VAL_TYPE_I8: {
            return value >= INT8_MIN && value <= INT8_MAX;
        }
        case SF_VAL_TYPE_I16: {
            return value >= INT16_MIN && value <= INT16_MAX;
        }
        case SF_VAL_TYPE_I32: {
            return value >= INT32_MIN && value <= INT32_MAX;
        }
        default: {
            return true;
        }
    }
}

static bool check_signed_overflow(
    sf_ast_node *node, sf_value_type resolved, const char *filename
) {
    int64_t value;

    if (!sf_const_eval_int(node, &value)) {
        return true;
    }

    if (signed_fits(resolved, value)) {
        return true;
    }

    sf_log(
        "constant expression overflow",
        "the constant expression evaluates to '%lld', which does not fit in "
        "type '%s'",
        "the expression overflows at compile time; use a wider type or "
        "restructure the expression",
        filename, SF_LOG_SEMANTIC_LITERAL_OVERFLOW, node->span,
        SF_LOG_SEVERITY_ERROR, (long long)value, sf_type_value_name(resolved)
    );

    return false;
}

static bool check_unsigned_overflow(
    sf_ast_node *node, sf_value_type resolved, const char *filename
) {
    uint64_t value;

    if (!sf_const_eval_uint(node, &value)) {
        if (node->type != SF_NODE_BINARY_EXPR) {
            return true;
        }

        sf_binary_expr_node *bin = (sf_binary_expr_node *)node;

        if (bin->op != SF_OP_TYPE_SUB) {
            return true;
        }

        uint64_t l, r;

        if (
            sf_const_eval_uint(bin->left, &l) &&
            sf_const_eval_uint(bin->right, &r) && l < r
        ) {
            sf_log(
                "constant expression underflow",
                "the constant expression '%llu - %llu' underflows for an "
                "unsigned type",
                "unsigned subtraction cannot produce a negative result; check "
                "operand order or use a signed type",
                filename, SF_LOG_SEMANTIC_LITERAL_OVERFLOW, node->span,
                SF_LOG_SEVERITY_ERROR, (unsigned long long)l,
                (unsigned long long)r
            );

            return false;
        }

        return true;
    }

    if (sf_type_value_uint_literal_fits(resolved, value)) {
        return true;
    }

    sf_log(
        "constant expression overflow",
        "the constant expression evaluates to '%llu', which does not fit in "
        "type '%s'",
        "the expression overflows at compile time; use a wider type or "
        "restructure the expression",
        filename, SF_LOG_SEMANTIC_LITERAL_OVERFLOW, node->span,
        SF_LOG_SEVERITY_ERROR, (unsigned long long)value,
        sf_type_value_name(resolved)
    );

    return false;
}
