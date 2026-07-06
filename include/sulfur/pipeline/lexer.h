#pragma once

#include "sulfur/utils/span.h"
#include <stddef.h>

#define SF_MAX_TOKEN_VALUE_SIZE 255

typedef enum {
    SF_TOKEN_TYPE_IDENTIFIER,

    SF_TOKEN_TYPE_INTEGER,

    SF_TOKEN_TYPE_PLUS,
    SF_TOKEN_TYPE_MINUS,
    SF_TOKEN_TYPE_MULT,
    SF_TOKEN_TYPE_DIV,
    SF_TOKEN_TYPE_EQUALS,

    SF_TOKEN_TYPE_AMPERSAND,
    SF_TOKEN_TYPE_PIPE,
    SF_TOKEN_TYPE_CARET,
    SF_TOKEN_TYPE_RIGHT_SHIFT,
    SF_TOKEN_TYPE_LEFT_SHIFT,
    SF_TOKEN_TYPE_TILDE,

    SF_TOKEN_TYPE_SEMICOLON,

    SF_TOKEN_TYPE_LBRACE,
    SF_TOKEN_TYPE_RBRACE,

    SF_TOKEN_TYPE_LPAREN,
    SF_TOKEN_TYPE_RPAREN,

    SF_TOKEN_TYPE_KW_I8,
    SF_TOKEN_TYPE_KW_I16,
    SF_TOKEN_TYPE_KW_I32,
    SF_TOKEN_TYPE_KW_I64,

    SF_TOKEN_TYPE_KW_U8,
    SF_TOKEN_TYPE_KW_U16,
    SF_TOKEN_TYPE_KW_U32,
    SF_TOKEN_TYPE_KW_U64,

    SF_TOKEN_TYPE_KW_BOOL,
    SF_TOKEN_TYPE_KW_TRUE,
    SF_TOKEN_TYPE_KW_FALSE,

    SF_TOKEN_TYPE_KW_AS,

    SF_TOKEN_TYPE_UNDEFINED,
    SF_TOKEN_TYPE_EOF,
} sf_token_type;

typedef struct {
    sf_token_type type;
    char value[SF_MAX_TOKEN_VALUE_SIZE];
    sf_span span;
} sf_token;

typedef struct {
    sf_token* tokens;
    size_t count;
    size_t capacity;
} sf_token_list;

sf_token_list sf_tokenize(const char* input, const char* filename);

void sf_print_tokens(const sf_token_list* list);
const char* sf_token_type_name(sf_token_type type);