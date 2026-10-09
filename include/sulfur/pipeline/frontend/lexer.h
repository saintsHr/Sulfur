#pragma once

#include <stddef.h>

#include "sulfur/utils/span.h"

#define SF_MAX_TOKEN_VALUE_SIZE 255

typedef enum {
    SF_TOKEN_TYPE_IDENTIFIER,
    SF_TOKEN_TYPE_INTEGER,
    SF_TOKEN_TYPE_KW_TRUE,
    SF_TOKEN_TYPE_KW_FALSE,

    SF_TOKEN_TYPE_PLUS,
    SF_TOKEN_TYPE_MINUS,
    SF_TOKEN_TYPE_STAR,
    SF_TOKEN_TYPE_SLASH,

    SF_TOKEN_TYPE_EQUAL,
    SF_TOKEN_TYPE_EQUAL_EQUAL,
    SF_TOKEN_TYPE_BANG,
    SF_TOKEN_TYPE_BANG_EQUAL,
    SF_TOKEN_TYPE_LESS,
    SF_TOKEN_TYPE_LESS_EQUAL,
    SF_TOKEN_TYPE_GREATER,
    SF_TOKEN_TYPE_GREATER_EQUAL,

    SF_TOKEN_TYPE_PLUS_EQUAL,
    SF_TOKEN_TYPE_MINUS_EQUAL,
    SF_TOKEN_TYPE_STAR_EQUAL,
    SF_TOKEN_TYPE_SLASH_EQUAL,
    SF_TOKEN_TYPE_AMP_EQUAL,
    SF_TOKEN_TYPE_PIPE_EQUAL,
    SF_TOKEN_TYPE_CARET_EQUAL,
    SF_TOKEN_TYPE_LEFT_SHIFT_EQUAL,
    SF_TOKEN_TYPE_RIGHT_SHIFT_EQUAL,

    SF_TOKEN_TYPE_PLUS_PLUS,
    SF_TOKEN_TYPE_MINUS_MINUS,

    SF_TOKEN_TYPE_AMP_AMP,
    SF_TOKEN_TYPE_PIPE_PIPE,

    SF_TOKEN_TYPE_AMP,
    SF_TOKEN_TYPE_PIPE,
    SF_TOKEN_TYPE_CARET,
    SF_TOKEN_TYPE_TILDE,
    SF_TOKEN_TYPE_LEFT_SHIFT,
    SF_TOKEN_TYPE_RIGHT_SHIFT,

    SF_TOKEN_TYPE_SEMICOLON,
    SF_TOKEN_TYPE_LPAREN,
    SF_TOKEN_TYPE_RPAREN,
    SF_TOKEN_TYPE_LBRACE,
    SF_TOKEN_TYPE_RBRACE,
    SF_TOKEN_TYPE_COMMA,

    SF_TOKEN_TYPE_KW_I8,
    SF_TOKEN_TYPE_KW_I16,
    SF_TOKEN_TYPE_KW_I32,
    SF_TOKEN_TYPE_KW_I64,

    SF_TOKEN_TYPE_KW_U8,
    SF_TOKEN_TYPE_KW_U16,
    SF_TOKEN_TYPE_KW_U32,
    SF_TOKEN_TYPE_KW_U64,

    SF_TOKEN_TYPE_KW_BOOL,

    SF_TOKEN_TYPE_KW_VOID,

    SF_TOKEN_TYPE_KW_AS,

    SF_TOKEN_TYPE_KW_IF,
    SF_TOKEN_TYPE_KW_ELSE,
    SF_TOKEN_TYPE_KW_WHILE,

    SF_TOKEN_TYPE_KW_FN,
    SF_TOKEN_TYPE_KW_RETURN,

    SF_TOKEN_TYPE_UNDEFINED,
    SF_TOKEN_TYPE_EOF,
} sf_token_type;

typedef struct {
    sf_token_type type;
    char value[SF_MAX_TOKEN_VALUE_SIZE];
    sf_span span;
} sf_token;

typedef struct {
    sf_token *tokens;
    size_t count;
    size_t capacity;
} sf_token_list;

sf_token_list sf_tokenize(const char *input, const char *filename);

void sf_tokens_print(const sf_token_list *list);
void sf_tokens_free(sf_token_list *list);

const char *sf_token_type_name(sf_token_type type);
