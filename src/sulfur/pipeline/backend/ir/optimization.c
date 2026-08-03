#include "sulfur/pipeline/backend/ir/optimization.h"

#include <stdio.h>

#include "sulfur/pipeline/backend/ir/ir.h"
#include "sulfur/utils/type_utils.h"

static bool sf_apply_relational_s(
    sf_opcode opcode, int64_t l, int64_t r, bool* result
) {
    switch (opcode) {
        case SF_OPCODE_RELATIONAL_EQUAL:
            *result = l == r;
            return true;
        case SF_OPCODE_RELATIONAL_NOT_EQUAL:
            *result = l != r;
            return true;
        case SF_OPCODE_RELATIONAL_LESS:
            *result = l < r;
            return true;
        case SF_OPCODE_RELATIONAL_LESS_EQUAL:
            *result = l <= r;
            return true;
        case SF_OPCODE_RELATIONAL_GREATER:
            *result = l > r;
            return true;
        case SF_OPCODE_RELATIONAL_GREATER_EQUAL:
            *result = l >= r;
            return true;
        default:
            return false;
    }
}

static bool sf_apply_relational_u(
    sf_opcode opcode, uint64_t l, uint64_t r, bool* result
) {
    switch (opcode) {
        case SF_OPCODE_RELATIONAL_EQUAL:
            *result = l == r;
            return true;
        case SF_OPCODE_RELATIONAL_NOT_EQUAL:
            *result = l != r;
            return true;
        case SF_OPCODE_RELATIONAL_LESS:
            *result = l < r;
            return true;
        case SF_OPCODE_RELATIONAL_LESS_EQUAL:
            *result = l <= r;
            return true;
        case SF_OPCODE_RELATIONAL_GREATER:
            *result = l > r;
            return true;
        case SF_OPCODE_RELATIONAL_GREATER_EQUAL:
            *result = l >= r;
            return true;
        default:
            return false;
    }
}

static bool sf_apply_arith_s(
    sf_opcode opcode, int64_t l, int64_t r, int64_t* result
) {
    switch (opcode) {
        case SF_OPCODE_ADD:
            *result = l + r;
            return true;
        case SF_OPCODE_SUB:
            *result = l - r;
            return true;
        case SF_OPCODE_MULT:
            *result = l * r;
            return true;
        case SF_OPCODE_DIV:
            if (r == 0) return false;
            *result = l / r;
            return true;
        default:
            return false;
    }
}

static bool sf_apply_arith_u(
    sf_opcode opcode, uint64_t l, uint64_t r, uint64_t* result
) {
    switch (opcode) {
        case SF_OPCODE_ADD:
            *result = l + r;
            return true;
        case SF_OPCODE_SUB:
            *result = l - r;
            return true;
        case SF_OPCODE_MULT:
            *result = l * r;
            return true;
        case SF_OPCODE_DIV:
            if (r == 0) return false;
            *result = l / r;
            return true;
        default:
            return false;
    }
}

static bool sf_apply_bitwise_s(
    sf_opcode opcode, int64_t l, int64_t r, int64_t* result
) {
    switch (opcode) {
        case SF_OPCODE_BITWISE_AND:
            *result = l & r;
            return true;
        case SF_OPCODE_BITWISE_OR:
            *result = l | r;
            return true;
        case SF_OPCODE_BITWISE_XOR:
            *result = l ^ r;
            return true;
        case SF_OPCODE_BITWISE_RSHIFT:
            *result = l >> r;
            return true;
        case SF_OPCODE_BITWISE_LSHIFT:
            *result = l << r;
            return true;
        default:
            return false;
    }
}

static bool sf_apply_bitwise_u(
    sf_opcode opcode, uint64_t l, uint64_t r, uint64_t* result
) {
    switch (opcode) {
        case SF_OPCODE_BITWISE_AND:
            *result = l & r;
            return true;
        case SF_OPCODE_BITWISE_OR:
            *result = l | r;
            return true;
        case SF_OPCODE_BITWISE_XOR:
            *result = l ^ r;
            return true;
        case SF_OPCODE_BITWISE_RSHIFT:
            *result = l >> r;
            return true;
        case SF_OPCODE_BITWISE_LSHIFT:
            *result = l << r;
            return true;
        default:
            return false;
    }
}

static bool sf_apply_unary_s(sf_opcode opcode, int64_t o, int64_t* result) {
    switch (opcode) {
        case SF_OPCODE_BITWISE_NOT:
            *result = ~o;
            return true;
        case SF_OPCODE_LOGICAL_NOT:
            *result = !o;
            return true;
        case SF_OPCODE_NEGATE:
            *result = -o;
            return true;
        default:
            return false;
    }
}

static bool sf_apply_unary_u(sf_opcode opcode, uint64_t o, uint64_t* result) {
    switch (opcode) {
        case SF_OPCODE_BITWISE_NOT:
            *result = ~o;
            return true;
        case SF_OPCODE_LOGICAL_NOT:
            *result = !o;
            return true;
        case SF_OPCODE_NEGATE:
            *result = -o;
            return true;
        default:
            return false;
    }
}

bool sf_fold_constants(
    sf_arena* arena,
    sf_operand left,
    sf_operand right,
    sf_opcode opcode,
    sf_value_type result_type,
    sf_operand* out
) {
    bool is_arithmetic =
        (opcode == SF_OPCODE_ADD || opcode == SF_OPCODE_SUB ||
         opcode == SF_OPCODE_MULT || opcode == SF_OPCODE_DIV);

    bool is_relational =
        (opcode == SF_OPCODE_RELATIONAL_EQUAL ||
         opcode == SF_OPCODE_RELATIONAL_NOT_EQUAL ||
         opcode == SF_OPCODE_RELATIONAL_LESS ||
         opcode == SF_OPCODE_RELATIONAL_LESS_EQUAL ||
         opcode == SF_OPCODE_RELATIONAL_GREATER ||
         opcode == SF_OPCODE_RELATIONAL_GREATER_EQUAL);

    bool is_logical =
        (opcode == SF_OPCODE_LOGICAL_AND || opcode == SF_OPCODE_LOGICAL_OR);

    bool is_signed = is_relational ? type_value_is_signed(left.value_type)
                                   : type_value_is_signed(result_type);

    bool is_bitwise =
        (opcode == SF_OPCODE_BITWISE_AND || opcode == SF_OPCODE_BITWISE_OR ||
         opcode == SF_OPCODE_BITWISE_XOR ||
         opcode == SF_OPCODE_BITWISE_RSHIFT ||
         opcode == SF_OPCODE_BITWISE_LSHIFT);

    bool is_unary =
        (opcode == SF_OPCODE_BITWISE_NOT || opcode == SF_OPCODE_LOGICAL_NOT ||
         opcode == SF_OPCODE_NEGATE);

    if (!is_unary && right.type != SF_OPERAND_TYPE_IMMEDIATE) return false;
    if (left.type != SF_OPERAND_TYPE_IMMEDIATE) return false;

    if (!is_arithmetic && !is_relational && !is_logical && !is_bitwise &&
        !is_unary)
        return false;

    if (is_relational) {
        bool result, ok;

        if (is_signed) {
            ok = sf_apply_relational_s(
                opcode,
                strtoll(left.immediate_value, NULL, 10),
                strtoll(right.immediate_value, NULL, 10),
                &result
            );
        } else {
            ok = sf_apply_relational_u(
                opcode,
                strtoull(left.immediate_value, NULL, 10),
                strtoull(right.immediate_value, NULL, 10),
                &result
            );
        }

        if (!ok) return false;

        *out = (sf_operand){
            .type = SF_OPERAND_TYPE_IMMEDIATE,
            .value_type = SF_VAL_TYPE_BOOL,
            .immediate_value = result ? "1" : "0"
        };

        return true;
    }

    if (is_logical) {
        uint64_t l = strtoull(left.immediate_value, NULL, 10);
        uint64_t r = strtoull(right.immediate_value, NULL, 10);

        bool result;

        switch (opcode) {
            case SF_OPCODE_LOGICAL_AND:
                result = l && r;
                break;
            case SF_OPCODE_LOGICAL_OR:
                result = l || r;
                break;
            default:
                return false;
        }

        *out = (sf_operand){
            .type = SF_OPERAND_TYPE_IMMEDIATE,
            .value_type = SF_VAL_TYPE_BOOL,
            .immediate_value = result ? "1" : "0"
        };

        return true;
    }

    if (is_bitwise) {
        char* buf = sf_arena_alloc(arena, 32);
        bool ok;

        if (is_signed) {
            int64_t result;

            ok = sf_apply_bitwise_s(
                opcode,
                strtoll(left.immediate_value, NULL, 10),
                strtoll(right.immediate_value, NULL, 10),
                &result
            );

            if (ok) snprintf(buf, 32, "%lld", (long long)result);
        } else {
            uint64_t result;

            ok = sf_apply_bitwise_u(
                opcode,
                strtoull(left.immediate_value, NULL, 10),
                strtoull(right.immediate_value, NULL, 10),
                &result
            );

            if (ok) snprintf(buf, 32, "%llu", (unsigned long long)result);
        }

        if (!ok) return false;

        *out = (sf_operand){
            .type = SF_OPERAND_TYPE_IMMEDIATE,
            .value_type = result_type,
            .immediate_value = buf
        };

        return true;
    }

    if (is_unary) {
        char* buf = sf_arena_alloc(arena, 32);
        bool ok;

        if (is_signed) {
            int64_t result;

            ok = sf_apply_unary_s(
                opcode, strtoll(left.immediate_value, NULL, 10), &result
            );

            if (ok) snprintf(buf, 32, "%lld", (long long)result);
        } else {
            uint64_t result;

            ok = sf_apply_unary_u(
                opcode, strtoull(left.immediate_value, NULL, 10), &result
            );

            if (ok) snprintf(buf, 32, "%llu", (unsigned long long)result);
        }

        if (!ok) return false;

        *out = (sf_operand){
            .type = SF_OPERAND_TYPE_IMMEDIATE,
            .value_type = result_type,
            .immediate_value = buf
        };

        return true;
    }

    char* buf = sf_arena_alloc(arena, 32);
    bool ok;

    if (is_signed) {
        int64_t result;

        ok = sf_apply_arith_s(
            opcode,
            strtoll(left.immediate_value, NULL, 10),
            strtoll(right.immediate_value, NULL, 10),
            &result
        );

        if (ok) snprintf(buf, 32, "%lld", (long long)result);
    } else {
        uint64_t result;

        ok = sf_apply_arith_u(
            opcode,
            strtoull(left.immediate_value, NULL, 10),
            strtoull(right.immediate_value, NULL, 10),
            &result
        );

        if (ok) snprintf(buf, 32, "%llu", (unsigned long long)result);
    }

    if (!ok) return false;

    *out = (sf_operand){
        .type = SF_OPERAND_TYPE_IMMEDIATE,
        .value_type = result_type,
        .immediate_value = buf
    };

    return true;
}