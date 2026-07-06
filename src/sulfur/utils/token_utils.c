#include "sulfur/utils/token_utils.h"

bool token_is_type(sf_token token) {
    if (token.type == SF_TOKEN_TYPE_KW_I8)  return true;
    if (token.type == SF_TOKEN_TYPE_KW_I16) return true;
    if (token.type == SF_TOKEN_TYPE_KW_I32) return true;
    if (token.type == SF_TOKEN_TYPE_KW_I64) return true;

    if (token.type == SF_TOKEN_TYPE_KW_U8)  return true;
    if (token.type == SF_TOKEN_TYPE_KW_U16) return true;
    if (token.type == SF_TOKEN_TYPE_KW_U32) return true;
    if (token.type == SF_TOKEN_TYPE_KW_U64) return true;

    if (token.type == SF_TOKEN_TYPE_KW_BOOL) return true;

    return false;
}

bool token_is_ident(sf_token token) {
    return (token.type == SF_TOKEN_TYPE_IDENTIFIER);
}

bool token_is_block(sf_token token) {
    return (token.type == SF_TOKEN_TYPE_LBRACE);
}

sf_value_type token_to_type(sf_token token) {
    switch (token.type) {
        case SF_TOKEN_TYPE_KW_I8:  return SF_VAL_TYPE_I8;
        case SF_TOKEN_TYPE_KW_I16: return SF_VAL_TYPE_I16;
        case SF_TOKEN_TYPE_KW_I32: return SF_VAL_TYPE_I32;
        case SF_TOKEN_TYPE_KW_I64: return SF_VAL_TYPE_I64;
            
        case SF_TOKEN_TYPE_KW_U8:  return SF_VAL_TYPE_U8;
        case SF_TOKEN_TYPE_KW_U16: return SF_VAL_TYPE_U16;
        case SF_TOKEN_TYPE_KW_U32: return SF_VAL_TYPE_U32;
        case SF_TOKEN_TYPE_KW_U64: return SF_VAL_TYPE_U64;

        case SF_TOKEN_TYPE_KW_BOOL: return SF_VAL_TYPE_BOOL;
            
        default: return SF_VAL_TYPE_UNRESOLVED;
    }
}