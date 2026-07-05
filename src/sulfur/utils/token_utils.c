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

    return false;
}

bool token_is_ident(sf_token token) {
    return (token.type == SF_TOKEN_TYPE_IDENTIFIER);
}

bool token_is_block(sf_token token) {
    return (token.type == SF_TOKEN_TYPE_LBRACE);
}