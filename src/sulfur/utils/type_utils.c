#include "sulfur/utils/type_utils.h"

#include "sulfur/pipeline/backend/ir/ir.h"
#include "sulfur/pipeline/frontend/ast.h"

bool sf_type_value_uint_literal_fits(sf_value_type type, uint64_t value) {
    switch (type) {
        case SF_VAL_TYPE_I8: {
            return value <= (uint64_t)INT8_MAX;
        }
        case SF_VAL_TYPE_I16: {
            return value <= (uint64_t)INT16_MAX;
        }
        case SF_VAL_TYPE_I32: {
            return value <= (uint64_t)INT32_MAX;
        }
        case SF_VAL_TYPE_I64: {
            return value <= (uint64_t)INT64_MAX;
        }
        case SF_VAL_TYPE_U8: {
            return value <= UINT8_MAX;
        }
        case SF_VAL_TYPE_U16: {
            return value <= UINT16_MAX;
        }
        case SF_VAL_TYPE_U32: {
            return value <= UINT32_MAX;
        }
        case SF_VAL_TYPE_U64: {
            return true;
        }
        default: {
            return false;
        }
    }
}

bool sf_type_value_signed_literal_fits_negated(sf_value_type type, uint64_t magnitude) {
    switch (type) {
        case SF_VAL_TYPE_I8: {
            return magnitude <= (uint64_t)INT8_MAX + 1;
        }
        case SF_VAL_TYPE_I16: {
            return magnitude <= (uint64_t)INT16_MAX + 1;
        }
        case SF_VAL_TYPE_I32: {
            return magnitude <= (uint64_t)INT32_MAX + 1;
        }
        case SF_VAL_TYPE_I64: {
            return magnitude <= (uint64_t)INT64_MAX + 1;
        }
        default: {
            return false;
        }
    }
}

const char *sf_type_value_name(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8: {
            return "i8";
        }
        case SF_VAL_TYPE_I16: {
            return "i16";
        }
        case SF_VAL_TYPE_I32: {
            return "i32";
        }
        case SF_VAL_TYPE_I64: {
            return "i64";
        }

        case SF_VAL_TYPE_U8: {
            return "u8";
        }
        case SF_VAL_TYPE_U16: {
            return "u16";
        }
        case SF_VAL_TYPE_U32: {
            return "u32";
        }
        case SF_VAL_TYPE_U64: {
            return "u64";
        }

        case SF_VAL_TYPE_BOOL: {
            return "bool";
        }

        case SF_VAL_TYPE_VOID: {
            return "void";
        }

        default: {
            return "?";
        }
    }
}

const char *sf_type_operation_name(sf_operation_type op) {
    switch (op) {
        case SF_OP_TYPE_ADD: {
            return "+";
        }
        case SF_OP_TYPE_SUB: {
            return "-";
        }
        case SF_OP_TYPE_MUL: {
            return "*";
        }
        case SF_OP_TYPE_DIV: {
            return "/";
        }
        case SF_OP_TYPE_NEGATE: {
            return "-";
        }

        case SF_OP_TYPE_BITWISE_AND: {
            return "&";
        }
        case SF_OP_TYPE_BITWISE_OR: {
            return "|";
        }
        case SF_OP_TYPE_BITWISE_XOR: {
            return "^";
        }
        case SF_OP_TYPE_BITWISE_RSHIFT: {
            return ">>";
        }
        case SF_OP_TYPE_BITWISE_LSHIFT: {
            return "<<";
        }
        case SF_OP_TYPE_BITWISE_NOT: {
            return "~";
        }

        case SF_OP_TYPE_RELATIONAL_EQUAL: {
            return "==";
        }
        case SF_OP_TYPE_RELATIONAL_NOT_EQUAL: {
            return "!=";
        }
        case SF_OP_TYPE_RELATIONAL_LESS: {
            return "<";
        }
        case SF_OP_TYPE_RELATIONAL_LESS_EQUAL: {
            return "<=";
        }
        case SF_OP_TYPE_RELATIONAL_GREATER: {
            return ">";
        }
        case SF_OP_TYPE_RELATIONAL_GREATER_EQUAL: {
            return ">=";
        }

        case SF_OP_TYPE_LOGICAL_AND: {
            return "&&";
        }
        case SF_OP_TYPE_LOGICAL_OR: {
            return "||";
        }
        case SF_OP_TYPE_LOGICAL_NOT: {
            return "!";
        }

        case SF_OP_TYPE_PREFIX_INCREMENT: {
            return "++x";
        }
        case SF_OP_TYPE_PREFIX_DECREMENT: {
            return "--x";
        }
        case SF_OP_TYPE_POSTFIX_INCREMENT: {
            return "x++";
        }
        case SF_OP_TYPE_POSTFIX_DECREMENT: {
            return "x--";
        }

        default: {
            return "?";
        }
    }
}

bool sf_type_value_is_unsigned(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_U8:
        case SF_VAL_TYPE_U16:
        case SF_VAL_TYPE_U32:
        case SF_VAL_TYPE_U64:
            return true;
        default:
            return false;
    }
}

bool sf_type_value_is_signed(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:
        case SF_VAL_TYPE_I16:
        case SF_VAL_TYPE_I32:
        case SF_VAL_TYPE_I64:
            return true;
        default:
            return false;
    }
}

bool sf_type_value_is_integer(sf_value_type type) {
    return sf_type_value_is_signed(type) || sf_type_value_is_unsigned(type);
}

bool sf_type_value_is_same_group(sf_value_type a, sf_value_type b) {
    if (sf_type_value_is_unsigned(a) && sf_type_value_is_unsigned(b)) {
        return true;
    }

    if (sf_type_value_is_signed(a) && sf_type_value_is_signed(b)) {
        return true;
    }

    if (a == SF_VAL_TYPE_BOOL && b == SF_VAL_TYPE_BOOL) {
        return true;
    }

    return false;
}

bool sf_type_value_is_castable(sf_value_type from, sf_value_type to) {
    if ((from == SF_VAL_TYPE_UNRESOLVED) || (to == SF_VAL_TYPE_UNRESOLVED)) {
        return false;
    }

    if (from == to) {
        return true;
    }

    bool castable = true;

    if (
        ((from == SF_VAL_TYPE_BOOL) || (to == SF_VAL_TYPE_BOOL)) ||
        ((from == SF_VAL_TYPE_VOID) || (to == SF_VAL_TYPE_VOID))
    ) {
        castable = false;
    }

    return castable;
}

uint8_t sf_type_value_width_bits(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:
        case SF_VAL_TYPE_U8:
        case SF_VAL_TYPE_BOOL:
            return 8;
        case SF_VAL_TYPE_I16:
        case SF_VAL_TYPE_U16:
            return 16;
        case SF_VAL_TYPE_I32:
        case SF_VAL_TYPE_U32:
            return 32;
        case SF_VAL_TYPE_I64:
        case SF_VAL_TYPE_U64:
            return 64;
        case SF_VAL_TYPE_VOID:
            return 0;
        default:
            return 64;
    }
}

uint8_t sf_type_value_width_bytes(sf_value_type type) {
    return sf_type_value_width_bits(type) / 8;
}

sf_value_type sf_type_value_promote(sf_value_type a, sf_value_type b) {
    return sf_type_value_width_bits(a) >= sf_type_value_width_bits(b) ? a : b;
}

sf_opcode sf_type_operation_to_opcode(sf_operation_type type) {
    switch (type) {
        case SF_OP_TYPE_ADD: {
            return SF_OPCODE_ADD;
        }
        case SF_OP_TYPE_SUB: {
            return SF_OPCODE_SUB;
        }
        case SF_OP_TYPE_MUL: {
            return SF_OPCODE_MULT;
        }
        case SF_OP_TYPE_DIV: {
            return SF_OPCODE_DIV;
        }
        case SF_OP_TYPE_NEGATE: {
            return SF_OPCODE_NEGATE;
        }

        case SF_OP_TYPE_BITWISE_AND: {
            return SF_OPCODE_BITWISE_AND;
        }
        case SF_OP_TYPE_BITWISE_OR: {
            return SF_OPCODE_BITWISE_OR;
        }
        case SF_OP_TYPE_BITWISE_XOR: {
            return SF_OPCODE_BITWISE_XOR;
        }
        case SF_OP_TYPE_BITWISE_RSHIFT: {
            return SF_OPCODE_BITWISE_RSHIFT;
        }
        case SF_OP_TYPE_BITWISE_LSHIFT: {
            return SF_OPCODE_BITWISE_LSHIFT;
        }
        case SF_OP_TYPE_BITWISE_NOT: {
            return SF_OPCODE_BITWISE_NOT;
        }

        case SF_OP_TYPE_RELATIONAL_EQUAL: {
            return SF_OPCODE_RELATIONAL_EQUAL;
        }
        case SF_OP_TYPE_RELATIONAL_NOT_EQUAL: {
            return SF_OPCODE_RELATIONAL_NOT_EQUAL;
        }
        case SF_OP_TYPE_RELATIONAL_LESS: {
            return SF_OPCODE_RELATIONAL_LESS;
        }
        case SF_OP_TYPE_RELATIONAL_LESS_EQUAL: {
            return SF_OPCODE_RELATIONAL_LESS_EQUAL;
        }
        case SF_OP_TYPE_RELATIONAL_GREATER: {
            return SF_OPCODE_RELATIONAL_GREATER;
        }
        case SF_OP_TYPE_RELATIONAL_GREATER_EQUAL: {
            return SF_OPCODE_RELATIONAL_GREATER_EQUAL;
        }

        case SF_OP_TYPE_LOGICAL_AND: {
            return SF_OPCODE_LOGICAL_AND;
        }
        case SF_OP_TYPE_LOGICAL_OR: {
            return SF_OPCODE_LOGICAL_OR;
        }
        case SF_OP_TYPE_LOGICAL_NOT: {
            return SF_OPCODE_LOGICAL_NOT;
        }

        default: {
            break;
        }
    }

    return (sf_opcode)0;
}
