#pragma once

#include <stddef.h>

#include "sulfur/utils/span.h"

#define SF_MAX_TOKEN_VALUE_SIZE 255

typedef enum {
  // literals
  SF_TOKEN_TYPE_IDENTIFIER, // ident
  SF_TOKEN_TYPE_INTEGER,    // 123
  SF_TOKEN_TYPE_KW_TRUE,    // true
  SF_TOKEN_TYPE_KW_FALSE,   // false

  // arithmetic
  SF_TOKEN_TYPE_PLUS,  // +
  SF_TOKEN_TYPE_MINUS, // -
  SF_TOKEN_TYPE_STAR,  // *
  SF_TOKEN_TYPE_SLASH, // /

  // assignment & comparison
  SF_TOKEN_TYPE_EQUAL,         // =
  SF_TOKEN_TYPE_EQUAL_EQUAL,   // ==
  SF_TOKEN_TYPE_BANG,          // !
  SF_TOKEN_TYPE_BANG_EQUAL,    // !=
  SF_TOKEN_TYPE_LESS,          // <
  SF_TOKEN_TYPE_LESS_EQUAL,    // <=
  SF_TOKEN_TYPE_GREATER,       // >
  SF_TOKEN_TYPE_GREATER_EQUAL, // >=

  // logical
  SF_TOKEN_TYPE_AMP_AMP,   // &&
  SF_TOKEN_TYPE_PIPE_PIPE, // ||

  // bitwise
  SF_TOKEN_TYPE_AMP,         // &
  SF_TOKEN_TYPE_PIPE,        // |
  SF_TOKEN_TYPE_CARET,       // ^
  SF_TOKEN_TYPE_TILDE,       // ~
  SF_TOKEN_TYPE_LEFT_SHIFT,  // <<
  SF_TOKEN_TYPE_RIGHT_SHIFT, // >>

  // delimiters
  SF_TOKEN_TYPE_SEMICOLON, // ;
  SF_TOKEN_TYPE_LPAREN,    // (
  SF_TOKEN_TYPE_RPAREN,    // )
  SF_TOKEN_TYPE_LBRACE,    // {
  SF_TOKEN_TYPE_RBRACE,    // }

  // types
  SF_TOKEN_TYPE_KW_I8,  // i8
  SF_TOKEN_TYPE_KW_I16, // i16
  SF_TOKEN_TYPE_KW_I32, // i32
  SF_TOKEN_TYPE_KW_I64, // i64

  SF_TOKEN_TYPE_KW_U8,  // u8
  SF_TOKEN_TYPE_KW_U16, // u16
  SF_TOKEN_TYPE_KW_U32, // u32
  SF_TOKEN_TYPE_KW_U64, // u64

  SF_TOKEN_TYPE_KW_BOOL, // bool

  // cast
  SF_TOKEN_TYPE_KW_AS, // as

  // flux control
  SF_TOKEN_TYPE_KW_IF,   // if
  SF_TOKEN_TYPE_KW_ELSE, // else

  // special
  SF_TOKEN_TYPE_UNDEFINED,
  SF_TOKEN_TYPE_EOF,
} sf_token_type;

typedef struct {
  sf_token_type type;
  const char *string;
} sf_map_type_string;

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

static const sf_map_type_string token_string_map[] = {
    // literals
    {SF_TOKEN_TYPE_IDENTIFIER, "an identifier"},
    {SF_TOKEN_TYPE_INTEGER, "a integer"},
    {SF_TOKEN_TYPE_KW_TRUE, "true"},
    {SF_TOKEN_TYPE_KW_FALSE, "false"},

    // arithmetic
    {SF_TOKEN_TYPE_PLUS, "+"},
    {SF_TOKEN_TYPE_MINUS, "-"},
    {SF_TOKEN_TYPE_STAR, "*"},
    {SF_TOKEN_TYPE_SLASH, "/"},

    // assignment & comparison
    {SF_TOKEN_TYPE_EQUAL, "="},
    {SF_TOKEN_TYPE_EQUAL_EQUAL, "=="},
    {SF_TOKEN_TYPE_BANG, "!"},
    {SF_TOKEN_TYPE_BANG_EQUAL, "!="},
    {SF_TOKEN_TYPE_LESS, "<"},
    {SF_TOKEN_TYPE_LESS_EQUAL, "<="},
    {SF_TOKEN_TYPE_GREATER, ">"},
    {SF_TOKEN_TYPE_GREATER_EQUAL, ">="},

    // logical
    {SF_TOKEN_TYPE_AMP_AMP, "&&"},
    {SF_TOKEN_TYPE_PIPE_PIPE, "||"},

    // bitwise
    {SF_TOKEN_TYPE_AMP, "&"},
    {SF_TOKEN_TYPE_PIPE, "|"},
    {SF_TOKEN_TYPE_CARET, "^"},
    {SF_TOKEN_TYPE_TILDE, "~"},
    {SF_TOKEN_TYPE_LEFT_SHIFT, "<<"},
    {SF_TOKEN_TYPE_RIGHT_SHIFT, ">>"},

    // delimiters
    {SF_TOKEN_TYPE_SEMICOLON, ";"},
    {SF_TOKEN_TYPE_LPAREN, "("},
    {SF_TOKEN_TYPE_RPAREN, ")"},
    {SF_TOKEN_TYPE_LBRACE, "{"},
    {SF_TOKEN_TYPE_RBRACE, "}"},

    // types
    {SF_TOKEN_TYPE_KW_I8, "i8"},
    {SF_TOKEN_TYPE_KW_I16, "i16"},
    {SF_TOKEN_TYPE_KW_I32, "i32"},
    {SF_TOKEN_TYPE_KW_I64, "i64"},
    {SF_TOKEN_TYPE_KW_U8, "u8"},
    {SF_TOKEN_TYPE_KW_U16, "u16"},
    {SF_TOKEN_TYPE_KW_U32, "u32"},
    {SF_TOKEN_TYPE_KW_U64, "u64"},
    {SF_TOKEN_TYPE_KW_BOOL, "bool"},

    // cast
    {SF_TOKEN_TYPE_KW_AS, "as"},

    // flow control
    {SF_TOKEN_TYPE_KW_IF, "if"},
    {SF_TOKEN_TYPE_KW_ELSE, "else"},
};

sf_token_list sf_tokenize(const char *input, const char *filename);

void sf_print_tokens(const sf_token_list *list);
void sf_free_tokens(sf_token_list *list);
const char *sf_token_type_name(sf_token_type type);