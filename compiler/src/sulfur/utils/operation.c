#include "sulfur/utils/operation.h"

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
