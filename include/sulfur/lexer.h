#pragma once

#define SF_MAX_TOKEN_VALUE_SIZE 64

#include <stddef.h>

typedef enum {
    SF_TOKEN_TYPE_IDENTIFIER,
    SF_TOKEN_TYPE_NUMBER,
    SF_TOKEN_TYPE_STRING,

    SF_TOKEN_TYPE_PLUS,
    SF_TOKEN_TYPE_MINUS,
    SF_TOKEN_TYPE_MULT,
    SF_TOKEN_TYPE_DIV,
    SF_TOKEN_TYPE_EQUALS,

    SF_TOKEN_TYPE_SEMICOLON,

    SF_TOKEN_TYPE_KW_I8,
    SF_TOKEN_TYPE_KW_I16,
    SF_TOKEN_TYPE_KW_I32,
    SF_TOKEN_TYPE_KW_I64,

    SF_TOKEN_TYPE_KW_U8,
    SF_TOKEN_TYPE_KW_U16,
    SF_TOKEN_TYPE_KW_U32,
    SF_TOKEN_TYPE_KW_U64,

    SF_TOKEN_TYPE_KW_F32,
    SF_TOKEN_TYPE_KW_F64,

    SF_TOKEN_TYPE_UNDEFINED,
    SF_TOKEN_TYPE_EOF,
} sfTokenType;

typedef struct {
    sfTokenType type;
    char value[SF_MAX_TOKEN_VALUE_SIZE];
    int line;
    int column;
} sfToken;

typedef struct {
    sfToken* tokens;
    size_t count;
    size_t capacity;
} sfTokenList;

sfTokenList tokenize(const char* input, const char* filename);
void addToken(sfTokenList* list, sfToken token);
