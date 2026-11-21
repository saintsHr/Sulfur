#pragma once

#include <stdbool.h>

#include "sulfur/shared/operation.h"
#include "sulfur/shared/span.h"
#include "sulfur/shared/type.h"

#define SF_TOKEN_VALUE_SIZE 255

typedef enum sf_token_type {
    SF_TOKEN_TYPE_UNDEFINED,
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
    SF_TOKEN_TYPE_DOT,
    SF_TOKEN_TYPE_AT,
    SF_TOKEN_TYPE_CHAR,
    SF_TOKEN_TYPE_COLON_COLON,
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
    SF_TOKEN_TYPE_KW_MUT,
    SF_TOKEN_TYPE_KW_PUB,
    SF_TOKEN_TYPE_KW_EXTERN,
    SF_TOKEN_TYPE_KW_IMPORT,
    SF_TOKEN_TYPE_KW_NAMESPACE,
    SF_TOKEN_TYPE_EOF
} sf_token_type;

typedef struct sf_token {
    sf_token_type type;
    char value[SF_TOKEN_VALUE_SIZE];
    sf_span span;
} sf_token;

typedef struct sf_token_list {
    sf_token *tokens;
    size_t count;
    size_t capacity;
} sf_token_list;

void sf_token_list_print(const sf_token_list *list);
void sf_token_list_free(sf_token_list *list);

bool sf_token_is_type(sf_token token);
bool sf_token_is_ident(sf_token token);
bool sf_token_is_block(sf_token token);
bool sf_token_is_if(sf_token token);
bool sf_token_is_while(sf_token token);
bool sf_token_is_fn(sf_token token);
bool sf_token_is_return(sf_token token);
bool sf_token_is_qual(sf_token token);
bool sf_token_is_attr(sf_token token);
bool sf_token_is_import(sf_token token);
bool sf_token_is_namespace(sf_token token);
bool sf_token_is_assign(sf_token token);
sf_type_kind sf_token_to_type(sf_token token);
sf_operation_type sf_token_to_binary_op(sf_token token);
sf_operation_type sf_token_to_unary_op(sf_token token);
sf_operation_type sf_token_to_postfix_op(sf_token token);
