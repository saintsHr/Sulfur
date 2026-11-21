#include "sulfur/shared/operation.h"

const char *sf_operation_name(sf_operation_type op) {
    switch (op) {
        case SF_OPERATION_TYPE_ADD: {
            return "+";
        }
        case SF_OPERATION_TYPE_SUB: {
            return "-";
        }
        case SF_OPERATION_TYPE_MUL: {
            return "*";
        }
        case SF_OPERATION_TYPE_DIV: {
            return "/";
        }
        case SF_OPERATION_TYPE_NEGATE: {
            return "-";
        }

        case SF_OPERATION_TYPE_BITWISE_AND: {
            return "&";
        }
        case SF_OPERATION_TYPE_BITWISE_OR: {
            return "|";
        }
        case SF_OPERATION_TYPE_BITWISE_XOR: {
            return "^";
        }
        case SF_OPERATION_TYPE_BITWISE_RSHIFT: {
            return ">>";
        }
        case SF_OPERATION_TYPE_BITWISE_LSHIFT: {
            return "<<";
        }
        case SF_OPERATION_TYPE_BITWISE_NOT: {
            return "~";
        }

        case SF_OPERATION_TYPE_RELATIONAL_EQUAL: {
            return "==";
        }
        case SF_OPERATION_TYPE_RELATIONAL_NOT_EQUAL: {
            return "!=";
        }
        case SF_OPERATION_TYPE_RELATIONAL_LESS: {
            return "<";
        }
        case SF_OPERATION_TYPE_RELATIONAL_LESS_EQUAL: {
            return "<=";
        }
        case SF_OPERATION_TYPE_RELATIONAL_GREATER: {
            return ">";
        }
        case SF_OPERATION_TYPE_RELATIONAL_GREATER_EQUAL: {
            return ">=";
        }

        case SF_OPERATION_TYPE_LOGICAL_AND: {
            return "&&";
        }
        case SF_OPERATION_TYPE_LOGICAL_OR: {
            return "||";
        }
        case SF_OPERATION_TYPE_LOGICAL_NOT: {
            return "!";
        }

        case SF_OPERATION_TYPE_PREFIX_INCREMENT: {
            return "++x";
        }
        case SF_OPERATION_TYPE_PREFIX_DECREMENT: {
            return "--x";
        }
        case SF_OPERATION_TYPE_POSTFIX_INCREMENT: {
            return "x++";
        }
        case SF_OPERATION_TYPE_POSTFIX_DECREMENT: {
            return "x--";
        }

        default: {
            return "?";
        }
    }
}

bool sf_operation_is_increment_or_decrement(sf_operation_type op) {
    return (
        op == SF_OPERATION_TYPE_PREFIX_INCREMENT ||
        op == SF_OPERATION_TYPE_PREFIX_DECREMENT ||
        op == SF_OPERATION_TYPE_POSTFIX_INCREMENT ||
        op == SF_OPERATION_TYPE_POSTFIX_DECREMENT
    );
}

sf_ir_opcode sf_operation_to_opcode(sf_operation_type type) {
    switch (type) {
        case SF_OPERATION_TYPE_ADD: {
            return SF_IR_OPCODE_ADD;
        }
        case SF_OPERATION_TYPE_SUB: {
            return SF_IR_OPCODE_SUB;
        }
        case SF_OPERATION_TYPE_MUL: {
            return SF_IR_OPCODE_MULT;
        }
        case SF_OPERATION_TYPE_DIV: {
            return SF_IR_OPCODE_DIV;
        }
        case SF_OPERATION_TYPE_NEGATE: {
            return SF_IR_OPCODE_NEGATE;
        }

        case SF_OPERATION_TYPE_BITWISE_AND: {
            return SF_IR_OPCODE_BITWISE_AND;
        }
        case SF_OPERATION_TYPE_BITWISE_OR: {
            return SF_IR_OPCODE_BITWISE_OR;
        }
        case SF_OPERATION_TYPE_BITWISE_XOR: {
            return SF_IR_OPCODE_BITWISE_XOR;
        }
        case SF_OPERATION_TYPE_BITWISE_RSHIFT: {
            return SF_IR_OPCODE_BITWISE_RSHIFT;
        }
        case SF_OPERATION_TYPE_BITWISE_LSHIFT: {
            return SF_IR_OPCODE_BITWISE_LSHIFT;
        }
        case SF_OPERATION_TYPE_BITWISE_NOT: {
            return SF_IR_OPCODE_BITWISE_NOT;
        }

        case SF_OPERATION_TYPE_RELATIONAL_EQUAL: {
            return SF_IR_OPCODE_RELATIONAL_EQUAL;
        }
        case SF_OPERATION_TYPE_RELATIONAL_NOT_EQUAL: {
            return SF_IR_OPCODE_RELATIONAL_NOT_EQUAL;
        }
        case SF_OPERATION_TYPE_RELATIONAL_LESS: {
            return SF_IR_OPCODE_RELATIONAL_LESS;
        }
        case SF_OPERATION_TYPE_RELATIONAL_LESS_EQUAL: {
            return SF_IR_OPCODE_RELATIONAL_LESS_EQUAL;
        }
        case SF_OPERATION_TYPE_RELATIONAL_GREATER: {
            return SF_IR_OPCODE_RELATIONAL_GREATER;
        }
        case SF_OPERATION_TYPE_RELATIONAL_GREATER_EQUAL: {
            return SF_IR_OPCODE_RELATIONAL_GREATER_EQUAL;
        }

        case SF_OPERATION_TYPE_LOGICAL_AND: {
            return SF_IR_OPCODE_LOGICAL_AND;
        }
        case SF_OPERATION_TYPE_LOGICAL_OR: {
            return SF_IR_OPCODE_LOGICAL_OR;
        }
        case SF_OPERATION_TYPE_LOGICAL_NOT: {
            return SF_IR_OPCODE_LOGICAL_NOT;
        }

        default: {
            return SF_IR_OPCODE_INVALID;
        }
    }
}

bool sf_operation_is_logical(sf_operation_type op) {
    return op == SF_OPERATION_TYPE_LOGICAL_AND || op == SF_OPERATION_TYPE_LOGICAL_OR;
}

bool sf_operation_is_shift(sf_operation_type op) {
    return op == SF_OPERATION_TYPE_BITWISE_LSHIFT ||
           op == SF_OPERATION_TYPE_BITWISE_RSHIFT;
}

bool sf_operation_is_relational(sf_operation_type op) {
    return op == SF_OPERATION_TYPE_RELATIONAL_EQUAL ||
           op == SF_OPERATION_TYPE_RELATIONAL_NOT_EQUAL ||
           op == SF_OPERATION_TYPE_RELATIONAL_LESS ||
           op == SF_OPERATION_TYPE_RELATIONAL_LESS_EQUAL ||
           op == SF_OPERATION_TYPE_RELATIONAL_GREATER ||
           op == SF_OPERATION_TYPE_RELATIONAL_GREATER_EQUAL;
}
