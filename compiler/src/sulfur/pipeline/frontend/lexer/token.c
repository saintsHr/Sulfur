#include "sulfur/pipeline/frontend/lexer/token.h"
#include <stdio.h>
#include <stdlib.h>

void sf_token_list_free(sf_token_list *list) {
    if (list == NULL) {
        return;
    }

    free(list->tokens);

    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

void sf_token_list_print(const sf_token_list *list) {
    for (size_t i = 0; i < list->count; i++) {
        printf(
            "Token %zu: type= %d value= %s (%zu:%zu)\n", i, list->tokens[i].type,
            list->tokens[i].value, list->tokens[i].span.line, list->tokens[i].span.col
        );
    }
}

bool sf_token_is_type(sf_token token) {
    switch (token.type) {
        case SF_TOKEN_TYPE_KW_I8:
        case SF_TOKEN_TYPE_KW_I16:
        case SF_TOKEN_TYPE_KW_I32:
        case SF_TOKEN_TYPE_KW_I64:
        case SF_TOKEN_TYPE_KW_U8:
        case SF_TOKEN_TYPE_KW_U16:
        case SF_TOKEN_TYPE_KW_U32:
        case SF_TOKEN_TYPE_KW_U64:
        case SF_TOKEN_TYPE_KW_BOOL:
        case SF_TOKEN_TYPE_KW_VOID: {
            return true;
        }

        default: {
            return false;
        }
    }
}

bool sf_token_is_ident(sf_token token) {
    return token.type == SF_TOKEN_TYPE_IDENTIFIER;
}

bool sf_token_is_block(sf_token token) {
    return token.type == SF_TOKEN_TYPE_LBRACE;
}

bool sf_token_is_if(sf_token token) {
    return token.type == SF_TOKEN_TYPE_KW_IF;
}

bool sf_token_is_while(sf_token token) {
    return token.type == SF_TOKEN_TYPE_KW_WHILE;
}

bool sf_token_is_fn(sf_token token) {
    return token.type == SF_TOKEN_TYPE_KW_FN;
}

bool sf_token_is_return(sf_token token) {
    return token.type == SF_TOKEN_TYPE_KW_RETURN;
}

bool sf_token_is_qual(sf_token token) {
    switch (token.type) {
        case SF_TOKEN_TYPE_KW_MUT:
        case SF_TOKEN_TYPE_KW_EXTERN:
        case SF_TOKEN_TYPE_KW_PUB: {
            return true;
        }

        default: {
            return false;
        }
    }
}

bool sf_token_is_attr(sf_token token) {
    return token.type == SF_TOKEN_TYPE_AT;
}

bool sf_token_is_import(sf_token token) {
    return token.type == SF_TOKEN_TYPE_KW_IMPORT;
}

bool sf_token_is_namespace(sf_token token) {
    return token.type == SF_TOKEN_TYPE_KW_NAMESPACE;
}

bool sf_token_is_assign(sf_token token) {
    switch (token.type) {
        case SF_TOKEN_TYPE_EQUAL:
        case SF_TOKEN_TYPE_PLUS_EQUAL:
        case SF_TOKEN_TYPE_MINUS_EQUAL:
        case SF_TOKEN_TYPE_STAR_EQUAL:
        case SF_TOKEN_TYPE_SLASH_EQUAL:
        case SF_TOKEN_TYPE_AMP_EQUAL:
        case SF_TOKEN_TYPE_PIPE_EQUAL:
        case SF_TOKEN_TYPE_CARET_EQUAL:
        case SF_TOKEN_TYPE_LEFT_SHIFT_EQUAL:
        case SF_TOKEN_TYPE_RIGHT_SHIFT_EQUAL: {
            return true;
        }

        default: {
            return false;
        }
    }
}

sf_type_kind sf_token_to_type(sf_token token) {
    switch (token.type) {
        case SF_TOKEN_TYPE_KW_I8: {
            return SF_TYPE_KIND_I8;
        }
        case SF_TOKEN_TYPE_KW_I16: {
            return SF_TYPE_KIND_I16;
        }
        case SF_TOKEN_TYPE_KW_I32: {
            return SF_TYPE_KIND_I32;
        }
        case SF_TOKEN_TYPE_KW_I64: {
            return SF_TYPE_KIND_I64;
        }

        case SF_TOKEN_TYPE_KW_U8: {
            return SF_TYPE_KIND_U8;
        }
        case SF_TOKEN_TYPE_KW_U16: {
            return SF_TYPE_KIND_U16;
        }
        case SF_TOKEN_TYPE_KW_U32: {
            return SF_TYPE_KIND_U32;
        }
        case SF_TOKEN_TYPE_KW_U64: {
            return SF_TYPE_KIND_U64;
        }

        case SF_TOKEN_TYPE_KW_BOOL: {
            return SF_TYPE_KIND_BOOL;
        }

        case SF_TOKEN_TYPE_KW_VOID: {
            return SF_TYPE_KIND_VOID;
        }

        default: {
            return SF_TYPE_KIND_UNRESOLVED;
        }
    }
}

sf_operation_type sf_token_to_unary_op(sf_token token) {
    switch (token.type) {
        case SF_TOKEN_TYPE_MINUS: {
            return SF_OPERATION_TYPE_NEGATE;
        }
        case SF_TOKEN_TYPE_TILDE: {
            return SF_OPERATION_TYPE_BITWISE_NOT;
        }
        case SF_TOKEN_TYPE_BANG: {
            return SF_OPERATION_TYPE_LOGICAL_NOT;
        }
        case SF_TOKEN_TYPE_PLUS_PLUS: {
            return SF_OPERATION_TYPE_PREFIX_INCREMENT;
        }
        case SF_TOKEN_TYPE_MINUS_MINUS: {
            return SF_OPERATION_TYPE_PREFIX_DECREMENT;
        }

        default: {
            return SF_OPERATION_TYPE_UNRESOLVED;
        }
    }
}

sf_operation_type sf_token_to_postfix_op(sf_token token) {
    switch (token.type) {
        case SF_TOKEN_TYPE_PLUS_PLUS: {
            return SF_OPERATION_TYPE_POSTFIX_INCREMENT;
        }
        case SF_TOKEN_TYPE_MINUS_MINUS: {
            return SF_OPERATION_TYPE_POSTFIX_DECREMENT;
        }

        default: {
            return SF_OPERATION_TYPE_UNRESOLVED;
        }
    }
}

sf_operation_type sf_token_to_binary_op(sf_token token) {
    switch (token.type) {
        case SF_TOKEN_TYPE_PLUS:
        case SF_TOKEN_TYPE_PLUS_EQUAL: {
            return SF_OPERATION_TYPE_ADD;
        }

        case SF_TOKEN_TYPE_MINUS:
        case SF_TOKEN_TYPE_MINUS_EQUAL: {
            return SF_OPERATION_TYPE_SUB;
        }

        case SF_TOKEN_TYPE_STAR:
        case SF_TOKEN_TYPE_STAR_EQUAL: {
            return SF_OPERATION_TYPE_MUL;
        }

        case SF_TOKEN_TYPE_SLASH:
        case SF_TOKEN_TYPE_SLASH_EQUAL: {
            return SF_OPERATION_TYPE_DIV;
        }

        case SF_TOKEN_TYPE_AMP:
        case SF_TOKEN_TYPE_AMP_EQUAL: {
            return SF_OPERATION_TYPE_BITWISE_AND;
        }

        case SF_TOKEN_TYPE_PIPE:
        case SF_TOKEN_TYPE_PIPE_EQUAL: {
            return SF_OPERATION_TYPE_BITWISE_OR;
        }

        case SF_TOKEN_TYPE_CARET:
        case SF_TOKEN_TYPE_CARET_EQUAL: {
            return SF_OPERATION_TYPE_BITWISE_XOR;
        }

        case SF_TOKEN_TYPE_RIGHT_SHIFT:
        case SF_TOKEN_TYPE_RIGHT_SHIFT_EQUAL: {
            return SF_OPERATION_TYPE_BITWISE_RSHIFT;
        }

        case SF_TOKEN_TYPE_LEFT_SHIFT:
        case SF_TOKEN_TYPE_LEFT_SHIFT_EQUAL: {
            return SF_OPERATION_TYPE_BITWISE_LSHIFT;
        }

        case SF_TOKEN_TYPE_EQUAL_EQUAL: {
            return SF_OPERATION_TYPE_RELATIONAL_EQUAL;
        }

        case SF_TOKEN_TYPE_BANG_EQUAL: {
            return SF_OPERATION_TYPE_RELATIONAL_NOT_EQUAL;
        }

        case SF_TOKEN_TYPE_LESS: {
            return SF_OPERATION_TYPE_RELATIONAL_LESS;
        }

        case SF_TOKEN_TYPE_LESS_EQUAL: {
            return SF_OPERATION_TYPE_RELATIONAL_LESS_EQUAL;
        }

        case SF_TOKEN_TYPE_GREATER: {
            return SF_OPERATION_TYPE_RELATIONAL_GREATER;
        }

        case SF_TOKEN_TYPE_GREATER_EQUAL: {
            return SF_OPERATION_TYPE_RELATIONAL_GREATER_EQUAL;
        }

        case SF_TOKEN_TYPE_AMP_AMP: {
            return SF_OPERATION_TYPE_LOGICAL_AND;
        }
        case SF_TOKEN_TYPE_PIPE_PIPE: {
            return SF_OPERATION_TYPE_LOGICAL_OR;
        }

        default: {
            return SF_OPERATION_TYPE_UNRESOLVED;
        }
    }
}
