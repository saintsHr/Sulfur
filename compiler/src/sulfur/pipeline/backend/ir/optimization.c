#include "sulfur/pipeline/backend/ir/optimization.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sulfur/pipeline/backend/ir/ir.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/type_utils.h"

static const size_t k_number_buffer_size = 32;
static const uint64_t k_bit_width = 64;

static bool is_arithmetic_opcode(sf_opcode opcode);
static bool is_relational_opcode(sf_opcode opcode);
static bool is_logical_opcode(sf_opcode opcode);
static bool is_bitwise_opcode(sf_opcode opcode);
static bool is_unary_opcode(sf_opcode opcode);

static int64_t parse_signed(const sf_operand *operand);
static uint64_t parse_unsigned(const sf_operand *operand);

static int64_t truncate_signed(int64_t value, sf_value_type type);
static uint64_t truncate_unsigned(uint64_t value, sf_value_type type);

static bool make_bool_immediate(bool value, sf_operand *out);
static bool make_signed_immediate(
    sf_arena *arena, sf_value_type type, int64_t value, sf_operand *out
);
static bool make_unsigned_immediate(
    sf_arena *arena, sf_value_type type, uint64_t value, sf_operand *out
);

static bool apply_relational_signed(
    sf_opcode opcode, int64_t l, int64_t r, bool *result
);
static bool apply_relational_unsigned(
    sf_opcode opcode, uint64_t l, uint64_t r, bool *result
);
static bool apply_arithmetic_signed(
    sf_opcode opcode, int64_t l, int64_t r, int64_t *result
);
static bool apply_arithmetic_unsigned(
    sf_opcode opcode, uint64_t l, uint64_t r, uint64_t *result
);
static bool apply_bitwise_signed(
    sf_opcode opcode, int64_t l, int64_t r, int64_t *result
);
static bool apply_bitwise_unsigned(
    sf_opcode opcode, uint64_t l, uint64_t r, uint64_t *result
);
static bool apply_unary_signed(sf_opcode opcode, int64_t o, int64_t *result);
static bool apply_unary_unsigned(sf_opcode opcode, uint64_t o, uint64_t *result);

static bool fold_relational(
    sf_opcode opcode, sf_operand left, sf_operand right,
    bool is_signed, sf_operand *out
);
static bool fold_logical(
    sf_opcode opcode, sf_operand left, sf_operand right, sf_operand *out
);
static bool fold_arithmetic(
    sf_arena *arena, sf_opcode opcode, sf_operand left, sf_operand right,
    bool is_signed, sf_value_type result_type, sf_operand *out
);
static bool fold_bitwise(
    sf_arena *arena, sf_opcode opcode, sf_operand left, sf_operand right,
    bool is_signed, sf_value_type result_type, sf_operand *out
);
static bool fold_unary(
    sf_arena *arena, sf_opcode opcode, sf_operand operand,
    bool is_signed, sf_value_type result_type, sf_operand *out
);

bool sf_fold_constants(
    sf_arena *arena, sf_operand left, sf_operand right,
    sf_opcode opcode, sf_value_type result_type, sf_operand *out
) {
    bool is_unary = is_unary_opcode(opcode);

    if (left.type != SF_OPERAND_TYPE_IMMEDIATE) {
        return false;
    }

    if (!is_unary && right.type != SF_OPERAND_TYPE_IMMEDIATE) {
        return false;
    }

    if (is_relational_opcode(opcode)) {
        return fold_relational(
            opcode, left, right, sf_type_value_is_signed(left.value_type), out
        );
    }

    if (is_logical_opcode(opcode)) {
        return fold_logical(opcode, left, right, out);
    }

    bool is_signed = sf_type_value_is_signed(result_type);

    if (is_bitwise_opcode(opcode)) {
        return fold_bitwise(arena, opcode, left, right, is_signed, result_type, out);
    }

    if (is_unary) {
        return fold_unary(arena, opcode, left, is_signed, result_type, out);
    }

    if (is_arithmetic_opcode(opcode)) {
        return fold_arithmetic(
            arena, opcode, left, right, is_signed, result_type, out
        );
    }

    return false;
}

static bool is_arithmetic_opcode(sf_opcode opcode) {
    return
        opcode == SF_OPCODE_ADD ||
        opcode == SF_OPCODE_SUB ||
        opcode == SF_OPCODE_MULT ||
        opcode == SF_OPCODE_DIV;
}

static bool is_relational_opcode(sf_opcode opcode) {
    return
        opcode == SF_OPCODE_RELATIONAL_EQUAL ||
        opcode == SF_OPCODE_RELATIONAL_NOT_EQUAL ||
        opcode == SF_OPCODE_RELATIONAL_LESS ||
        opcode == SF_OPCODE_RELATIONAL_LESS_EQUAL ||
        opcode == SF_OPCODE_RELATIONAL_GREATER ||
        opcode == SF_OPCODE_RELATIONAL_GREATER_EQUAL;
}

static bool is_logical_opcode(sf_opcode opcode) {
    return opcode == SF_OPCODE_LOGICAL_AND || opcode == SF_OPCODE_LOGICAL_OR;
}

static bool is_bitwise_opcode(sf_opcode opcode) {
    return
        opcode == SF_OPCODE_BITWISE_AND ||
        opcode == SF_OPCODE_BITWISE_OR ||
        opcode == SF_OPCODE_BITWISE_XOR ||
        opcode == SF_OPCODE_BITWISE_RSHIFT ||
        opcode == SF_OPCODE_BITWISE_LSHIFT;
}

static bool is_unary_opcode(sf_opcode opcode) {
    return
        opcode == SF_OPCODE_BITWISE_NOT ||
        opcode == SF_OPCODE_LOGICAL_NOT ||
        opcode == SF_OPCODE_NEGATE;
}

static int64_t parse_signed(const sf_operand *operand) {
    return strtoll(operand->immediate_value, NULL, 10);
}

static uint64_t parse_unsigned(const sf_operand *operand) {
    return strtoull(operand->immediate_value, NULL, 10);
}

static int64_t truncate_signed(int64_t value, sf_value_type type) {
    uint8_t bits = sf_type_value_width_bits(type);

    if (bits == 0 || bits >= 64) {
        return value;
    }

    uint64_t mask = (UINT64_C(1) << bits) - 1;
    uint64_t sign_bit = UINT64_C(1) << (bits - 1);
    uint64_t masked = (uint64_t)value & mask;

    return (int64_t)((masked ^ sign_bit) - sign_bit);
}

static uint64_t truncate_unsigned(uint64_t value, sf_value_type type) {
    uint8_t bits = sf_type_value_width_bits(type);

    if (bits == 0 || bits >= 64) {
        return value;
    }

    return value & ((UINT64_C(1) << bits) - 1);
}

static bool make_bool_immediate(bool value, sf_operand *out) {
    *out = (sf_operand){
        .type = SF_OPERAND_TYPE_IMMEDIATE,
        .value_type = SF_VAL_TYPE_BOOL,
        .immediate_value = value ? "1" : "0",
    };

    return true;
}

static bool make_signed_immediate(
    sf_arena *arena, sf_value_type type, int64_t value, sf_operand *out
) {
    char *buffer = sf_arena_alloc(arena, k_number_buffer_size);

    if (buffer == NULL) {
        sf_log(
            "Insufficient Memory.", "Cannot allocate memory for compiling.",
            "Free some memory and try again.", NULL,
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );

        return false;
    }

    snprintf(
        buffer, k_number_buffer_size, "%lld",
        (long long)truncate_signed(value, type)
    );

    *out = (sf_operand){
        .type = SF_OPERAND_TYPE_IMMEDIATE,
        .value_type = type,
        .immediate_value = buffer,
    };

    return true;
}

static bool make_unsigned_immediate(
    sf_arena *arena, sf_value_type type, uint64_t value, sf_operand *out
) {
    char *buffer = sf_arena_alloc(arena, k_number_buffer_size);

    if (buffer == NULL) {
        sf_log(
            "Insufficient Memory.", "Cannot allocate memory for compiling.",
            "Free some memory and try again.", NULL,
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );

        return false;
    }

    snprintf(
        buffer, k_number_buffer_size, "%llu",
        (unsigned long long)truncate_unsigned(value, type)
    );

    *out = (sf_operand){
        .type = SF_OPERAND_TYPE_IMMEDIATE,
        .value_type = type,
        .immediate_value = buffer,
    };

    return true;
}

static bool apply_relational_signed(
    sf_opcode opcode, int64_t l, int64_t r, bool *result
) {
    switch (opcode) {
        case SF_OPCODE_RELATIONAL_EQUAL: {
            *result = l == r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_NOT_EQUAL: {
            *result = l != r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_LESS: {
            *result = l < r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_LESS_EQUAL: {
            *result = l <= r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_GREATER: {
            *result = l > r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_GREATER_EQUAL: {
            *result = l >= r;
            return true;
        }

        default: {
            return false;
        }
    }
}

static bool apply_relational_unsigned(
    sf_opcode opcode, uint64_t l, uint64_t r, bool *result
) {
    switch (opcode) {
        case SF_OPCODE_RELATIONAL_EQUAL: {
            *result = l == r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_NOT_EQUAL: {
            *result = l != r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_LESS: {
            *result = l < r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_LESS_EQUAL: {
            *result = l <= r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_GREATER: {
            *result = l > r;
            return true;
        }
        case SF_OPCODE_RELATIONAL_GREATER_EQUAL: {
            *result = l >= r;
            return true;
        }

        default: {
            return false;
        }
    }
}

static bool apply_arithmetic_signed(
    sf_opcode opcode, int64_t l, int64_t r, int64_t *result
) {
    switch (opcode) {
        case SF_OPCODE_ADD: {
            *result = (int64_t)((uint64_t)l + (uint64_t)r);
            return true;
        }
        case SF_OPCODE_SUB: {
            *result = (int64_t)((uint64_t)l - (uint64_t)r);
            return true;
        }
        case SF_OPCODE_MULT: {
            *result = (int64_t)((uint64_t)l * (uint64_t)r);
            return true;
        }
        case SF_OPCODE_DIV: {
            if (r == 0 || (l == INT64_MIN && r == -1)) {
                return false;
            }

            *result = l / r;
            return true;
        }

        default: {
            return false;
        }
    }
}

static bool apply_arithmetic_unsigned(
    sf_opcode opcode, uint64_t l, uint64_t r, uint64_t *result
) {
    switch (opcode) {
        case SF_OPCODE_ADD: {
            *result = l + r;
            return true;
        }
        case SF_OPCODE_SUB: {
            *result = l - r;
            return true;
        }
        case SF_OPCODE_MULT: {
            *result = l * r;
            return true;
        }
        case SF_OPCODE_DIV: {
            if (r == 0) {
                return false;
            }

            *result = l / r;
            return true;
        }

        default: {
            return false;
        }
    }
}

static bool apply_bitwise_signed(
    sf_opcode opcode, int64_t l, int64_t r, int64_t *result
) {
    switch (opcode) {
        case SF_OPCODE_BITWISE_AND: {
            *result = l & r;
            return true;
        }
        case SF_OPCODE_BITWISE_OR: {
            *result = l | r;
            return true;
        }
        case SF_OPCODE_BITWISE_XOR: {
            *result = l ^ r;
            return true;
        }
        case SF_OPCODE_BITWISE_RSHIFT: {
            if (r < 0 || (uint64_t)r >= k_bit_width) {
                return false;
            }

            *result = l >> r;
            return true;
        }
        case SF_OPCODE_BITWISE_LSHIFT: {
            if (r < 0 || (uint64_t)r >= k_bit_width) {
                return false;
            }

            *result = (int64_t)((uint64_t)l << r);
            return true;
        }

        default: {
            return false;
        }
    }
}

static bool apply_bitwise_unsigned(
    sf_opcode opcode, uint64_t l, uint64_t r, uint64_t *result
) {
    switch (opcode) {
        case SF_OPCODE_BITWISE_AND: {
            *result = l & r;
            return true;
        }
        case SF_OPCODE_BITWISE_OR: {
            *result = l | r;
            return true;
        }
        case SF_OPCODE_BITWISE_XOR: {
            *result = l ^ r;
            return true;
        }
        case SF_OPCODE_BITWISE_RSHIFT: {
            if (r >= k_bit_width) {
                return false;
            }

            *result = l >> r;
            return true;
        }
        case SF_OPCODE_BITWISE_LSHIFT: {
            if (r >= k_bit_width) {
                return false;
            }

            *result = l << r;
            return true;
        }

        default: {
            return false;
        }
    }
}

static bool apply_unary_signed(sf_opcode opcode, int64_t o, int64_t *result) {
    switch (opcode) {
        case SF_OPCODE_BITWISE_NOT: {
            *result = ~o;
            return true;
        }
        case SF_OPCODE_LOGICAL_NOT: {
            *result = !o;
            return true;
        }
        case SF_OPCODE_NEGATE: {
            *result = (int64_t)(0 - (uint64_t)o);
            return true;
        }

        default: {
            return false;
        }
    }
}

static bool apply_unary_unsigned(sf_opcode opcode, uint64_t o, uint64_t *result) {
    switch (opcode) {
        case SF_OPCODE_BITWISE_NOT: {
            *result = ~o;
            return true;
        }
        case SF_OPCODE_LOGICAL_NOT: {
            *result = !o;
            return true;
        }
        case SF_OPCODE_NEGATE: {
            *result = 0 - o;
            return true;
        }

        default: {
            return false;
        }
    }
}

static bool fold_relational(
    sf_opcode opcode, sf_operand left, sf_operand right,
    bool is_signed, sf_operand *out
) {
    bool result = false;
    bool ok;

    if (is_signed) {
        ok = apply_relational_signed(
            opcode, parse_signed(&left), parse_signed(&right), &result
        );
    } else {
        ok = apply_relational_unsigned(
            opcode, parse_unsigned(&left), parse_unsigned(&right), &result
        );
    }

    if (!ok) {
        return false;
    }

    return make_bool_immediate(result, out);
}

static bool fold_logical(
    sf_opcode opcode, sf_operand left, sf_operand right, sf_operand *out
) {
    uint64_t l = parse_unsigned(&left);
    uint64_t r = parse_unsigned(&right);

    switch (opcode) {
        case SF_OPCODE_LOGICAL_AND: {
            return make_bool_immediate(l && r, out);
        }
        case SF_OPCODE_LOGICAL_OR: {
            return make_bool_immediate(l || r, out);
        }

        default: {
            return false;
        }
    }
}

static bool fold_arithmetic(
    sf_arena *arena, sf_opcode opcode, sf_operand left, sf_operand right,
    bool is_signed, sf_value_type result_type, sf_operand *out
) {
    if (is_signed) {
        int64_t result;

        if (!apply_arithmetic_signed(
                opcode, parse_signed(&left), parse_signed(&right), &result
            )) {
            return false;
        }

        return make_signed_immediate(arena, result_type, result, out);
    }

    uint64_t result;

    if (!apply_arithmetic_unsigned(
            opcode, parse_unsigned(&left), parse_unsigned(&right), &result
        )) {
        return false;
    }

    return make_unsigned_immediate(arena, result_type, result, out);
}

static bool fold_bitwise(
    sf_arena *arena, sf_opcode opcode, sf_operand left, sf_operand right,
    bool is_signed, sf_value_type result_type, sf_operand *out
) {
    if (is_signed) {
        int64_t result;

        if (!apply_bitwise_signed(
                opcode, parse_signed(&left), parse_signed(&right), &result
            )) {
            return false;
        }

        return make_signed_immediate(arena, result_type, result, out);
    }

    uint64_t result;

    if (!apply_bitwise_unsigned(
            opcode, parse_unsigned(&left), parse_unsigned(&right), &result
        )) {
        return false;
    }

    return make_unsigned_immediate(arena, result_type, result, out);
}

static bool fold_unary(
    sf_arena *arena, sf_opcode opcode, sf_operand operand,
    bool is_signed, sf_value_type result_type, sf_operand *out
) {
    if (is_signed) {
        int64_t result;

        if (!apply_unary_signed(opcode, parse_signed(&operand), &result)) {
            return false;
        }

        return make_signed_immediate(arena, result_type, result, out);
    }

    uint64_t result;

    if (!apply_unary_unsigned(opcode, parse_unsigned(&operand), &result)) {
        return false;
    }

    return make_unsigned_immediate(arena, result_type, result, out);
}
