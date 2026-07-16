#include "sulfur/utils/type_utils.h"
#include "sulfur/pipeline/frontend/ast.h"

bool type_value_uint_literal_fits(sf_value_type type, uint64_t value) {
    switch (type) {
        case SF_VAL_TYPE_I8:  return value <= (uint64_t)INT8_MAX;
        case SF_VAL_TYPE_I16: return value <= (uint64_t)INT16_MAX;
        case SF_VAL_TYPE_I32: return value <= (uint64_t)INT32_MAX;
        case SF_VAL_TYPE_I64: return value <= (uint64_t)INT64_MAX;
        case SF_VAL_TYPE_U8:  return value <= UINT8_MAX;
        case SF_VAL_TYPE_U16: return value <= UINT16_MAX;
        case SF_VAL_TYPE_U32: return value <= UINT32_MAX;
        case SF_VAL_TYPE_U64: return true;
        default: return false;
    }
}

bool type_value_signed_literal_fits_negated(sf_value_type type, uint64_t magnitude) {
    switch (type) {
        case SF_VAL_TYPE_I8:  return magnitude <= (uint64_t)INT8_MAX  + 1;
        case SF_VAL_TYPE_I16: return magnitude <= (uint64_t)INT16_MAX + 1;
        case SF_VAL_TYPE_I32: return magnitude <= (uint64_t)INT32_MAX + 1;
        case SF_VAL_TYPE_I64: return magnitude <= (uint64_t)INT64_MAX + 1;
        default: return false;
    }
}

const char* type_value_name(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:  return "i8";
        case SF_VAL_TYPE_I16: return "i16";
        case SF_VAL_TYPE_I32: return "i32";
        case SF_VAL_TYPE_I64: return "i64";

        case SF_VAL_TYPE_U8:  return "u8";
        case SF_VAL_TYPE_U16: return "u16";
        case SF_VAL_TYPE_U32: return "u32";
        case SF_VAL_TYPE_U64: return "u64";

        case SF_VAL_TYPE_BOOL: return "bool";

        default: return "?";
    }
}

const char* type_operation_name(sf_operation_type op) {
    switch(op) {
        case SF_OP_TYPE_ADD:    return "+";
        case SF_OP_TYPE_SUB:    return "-";
        case SF_OP_TYPE_MUL:    return "*";
        case SF_OP_TYPE_DIV:    return "/";
        case SF_OP_TYPE_NEGATE: return "-";

        case SF_OP_TYPE_BITWISE_AND:    return "&";
        case SF_OP_TYPE_BITWISE_OR:     return "|";
        case SF_OP_TYPE_BITWISE_XOR:    return "^";
        case SF_OP_TYPE_BITWISE_RSHIFT: return ">>";
        case SF_OP_TYPE_BITWISE_LSHIFT: return "<<";    
        case SF_OP_TYPE_BITWISE_NOT:    return "~";

        default: return "?";
    }
}

bool type_value_is_unsigned(sf_value_type type) {
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

bool type_value_is_signed(sf_value_type type) {
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

bool type_value_is_integer(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_U8:
        case SF_VAL_TYPE_U16:
        case SF_VAL_TYPE_U32:
        case SF_VAL_TYPE_U64:
        case SF_VAL_TYPE_I8:
        case SF_VAL_TYPE_I16:
        case SF_VAL_TYPE_I32:
        case SF_VAL_TYPE_I64:
            return true;

        default:
            return false;
    }
}

bool type_value_is_same_group(sf_value_type a, sf_value_type b) {
    if (type_value_is_unsigned(a) && type_value_is_unsigned(b)) return true;
    if (type_value_is_signed(a) && type_value_is_signed(b)) return true;
    if (a == SF_VAL_TYPE_BOOL && b == SF_VAL_TYPE_BOOL) return true;

    return false;
}

bool type_value_is_castable(sf_value_type from, sf_value_type to) {
	if ((from == SF_VAL_TYPE_BOOL) != (to == SF_VAL_TYPE_BOOL)) return false;

    return true;
}

uint8_t type_value_width_bits(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:  return 8;
        case SF_VAL_TYPE_I16: return 16;
        case SF_VAL_TYPE_I32: return 32;
        case SF_VAL_TYPE_I64: return 64;

        case SF_VAL_TYPE_U8:  return 8;
        case SF_VAL_TYPE_U16: return 16;
        case SF_VAL_TYPE_U32: return 32;
        case SF_VAL_TYPE_U64: return 64;

        case SF_VAL_TYPE_BOOL: return 8;

        default: return 64;
    }
}

uint8_t type_value_width_bytes(sf_value_type type) {
    return type_value_width_bits(type) / 8;
}

sf_value_type type_value_promote(sf_value_type a, sf_value_type b) {
    return type_value_width_bits(a) >= type_value_width_bits(b) ? a : b;
}

sf_opcode type_operation_to_opcode(sf_operation_type type) {
    sf_opcode op;

    switch (type) {
        case SF_OP_TYPE_ADD: op = SF_OPCODE_ADD;  break;
        case SF_OP_TYPE_SUB: op = SF_OPCODE_SUB;  break;
        case SF_OP_TYPE_MUL: op = SF_OPCODE_MULT; break;
        case SF_OP_TYPE_DIV: op = SF_OPCODE_DIV;  break;

        case SF_OP_TYPE_NEGATE: op = SF_OPCODE_NEGATE; break;

        case SF_OP_TYPE_BITWISE_AND:    op = SF_OPCODE_BITWISE_AND;    break;
        case SF_OP_TYPE_BITWISE_OR:     op = SF_OPCODE_BITWISE_OR;     break;
        case SF_OP_TYPE_BITWISE_XOR:    op = SF_OPCODE_BITWISE_XOR;    break;
        case SF_OP_TYPE_BITWISE_RSHIFT: op = SF_OPCODE_BITWISE_RSHIFT; break;
        case SF_OP_TYPE_BITWISE_LSHIFT: op = SF_OPCODE_BITWISE_LSHIFT; break;
        case SF_OP_TYPE_BITWISE_NOT:    op = SF_OPCODE_BITWISE_NOT;    break;

        default: break;
    }

    return op;
}