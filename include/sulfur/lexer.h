#pragma once

#define SF_MAX_TOKEN_VALUE_SIZE 64

#include <stddef.h>

typedef enum {
    SF_TOKEN_TYPE_IDENTIFIER, // 0
    SF_TOKEN_TYPE_NUMBER,     // 1
    SF_TOKEN_TYPE_STRING,     // 2

    SF_TOKEN_TYPE_PLUS,   // 3
    SF_TOKEN_TYPE_MINUS,  // 4
    SF_TOKEN_TYPE_MULT,   // 5
    SF_TOKEN_TYPE_DIV,    // 6
    SF_TOKEN_TYPE_EQUALS, // 7

    SF_TOKEN_TYPE_SEMICOLON, // 8

    SF_TOKEN_TYPE_KW_I8,  // 9
    SF_TOKEN_TYPE_KW_I16, // 10
    SF_TOKEN_TYPE_KW_I32, // 11
    SF_TOKEN_TYPE_KW_I64, // 12

    SF_TOKEN_TYPE_KW_U8,  // 13
    SF_TOKEN_TYPE_KW_U16, // 14
    SF_TOKEN_TYPE_KW_U32, // 15
    SF_TOKEN_TYPE_KW_U64, // 16

    SF_TOKEN_TYPE_KW_F32, // 17
    SF_TOKEN_TYPE_KW_F64, // 18

    SF_TOKEN_TYPE_UNDEFINED, // 19
    SF_TOKEN_TYPE_EOF,       // 20
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
