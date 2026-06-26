#pragma once

#include <stddef.h>

#define SF_MAX_TOKEN_VALUE_SIZE 64

typedef enum {
    SF_TOKEN_TYPE_IDENTIFIER,

    SF_TOKEN_TYPE_INTEGER,

    SF_TOKEN_TYPE_PLUS,
    SF_TOKEN_TYPE_MINUS,
    SF_TOKEN_TYPE_MULT,
    SF_TOKEN_TYPE_DIV,
    SF_TOKEN_TYPE_EQUALS,

    SF_TOKEN_TYPE_SEMICOLON,
    SF_TOKEN_TYPE_LBRACE,
    SF_TOKEN_TYPE_RBRACE,

    SF_TOKEN_TYPE_KW_I8,
    SF_TOKEN_TYPE_KW_I16,
    SF_TOKEN_TYPE_KW_I32,
    SF_TOKEN_TYPE_KW_I64,

    SF_TOKEN_TYPE_KW_U8,
    SF_TOKEN_TYPE_KW_U16,
    SF_TOKEN_TYPE_KW_U32,
    SF_TOKEN_TYPE_KW_U64,

    SF_TOKEN_TYPE_UNDEFINED,
    SF_TOKEN_TYPE_EOF,
} sf_token_type;

typedef struct {
    sf_token_type type;
    char value[SF_MAX_TOKEN_VALUE_SIZE];
    int line;
    int column;
} sf_token;

typedef struct {
    sf_token* tokens;
    size_t count;
    size_t capacity;
} sf_token_list;

sf_token_list sf_tokenize(const char* input, const char* filename);
void sf_print_tokens(const sf_token_list* list);
