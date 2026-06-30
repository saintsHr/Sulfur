#include "sulfur/lexer.h"
#include "sulfur/util/log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void undefined(sf_token_list* list, const char* input, int* i, int* col, int line, const char* filename);

static void add_token(sf_token_list* list, sf_token token);
static sf_token make_token(sf_token_type type, int line, int col);

static sf_token_type resolve_keyword(const char* value);

static sf_token read_number(const char* input, int* i, int* col, int line);
static sf_token read_identifier(const char* input, int* i, int* col, int line);
static sf_token read_symbol(const char* input, int* i, int* col, int line);

sf_token_list sf_tokenize(const char* input, const char* filename) {
    sf_token_list list = {0};
    
    int i = 0, line = 1, col = 1;

    while (input[i] != '\0') {
        char c = input[i];

        if (c == '\n') {
            line++;
            col = 1;
            i++;
            continue;
        }

        if (isspace(c)) {
            col++;
            i++;
            continue;
        }

        if (isdigit(c)) {
            add_token(
                &list,
                read_number(input, &i, &col, line)
            );
            continue;
        }

        if (isalpha(c) || c == '_') {
            add_token(
                &list,
                read_identifier(input, &i, &col, line)
            );
            continue;
        }

        sf_token tk = read_symbol(input, &i, &col, line);

        if (tk.type == SF_TOKEN_TYPE_UNDEFINED) {
            undefined(&list, input, &i, &col, line, filename);
        } else {
            add_token(&list, tk);
        }
    }

    add_token(&list, make_token(SF_TOKEN_TYPE_EOF, line, col));
    return list;
}

void sf_print_tokens(const sf_token_list* list) {
    for (size_t i = 0; i < list->count; i++) {
        printf("Token %zu: type=%d value=%s (%d:%d)\n",
            i,
            list->tokens[i].type,
            list->tokens[i].value,
            list->tokens[i].line,
            list->tokens[i].column
        );
    }
}

static void undefined(sf_token_list* list, const char* input, int* i, int* col, int line, const char* filename) {
    sf_token tk = {0};
    tk.type = SF_TOKEN_TYPE_UNDEFINED;
    tk.line = line;
    tk.column = *col;

    int j = 0;

    while (input[*i] != '\0' && !isspace(input[*i])) {
        if (j < SF_MAX_TOKEN_VALUE_SIZE - 1) tk.value[j++] = input[*i];
        (*i)++;
        (*col)++;
    }

    tk.value[j] = '\0';
    add_token(list, tk);

    sf_log_helper(
        "Undefined Token",
        "Undefined token in source file (%s).",
        "follow the language grammar.",
        filename,
        SF_LEXER_UNDEFINED_TOKEN,
        tk.line,
        tk.column,
        SF_SEV_FATAL,
        tk.value
    );
}

static void add_token(sf_token_list* list, sf_token token) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        list->tokens = realloc(list->tokens, list->capacity * sizeof(sf_token));
    }

    list->tokens[list->count++] = token;
}

static sf_token make_token(sf_token_type type, int line, int col) {
    sf_token tk = {0};
    tk.type   = type;
    tk.line   = line;
    tk.column = col;
    return tk;
}

static sf_token read_number(const char* input, int* i, int* col, int line) {
    int start_i   = *i;
    int start_col = *col;

    sf_token tk = make_token(SF_TOKEN_TYPE_INTEGER, line, *col);

    int j = 0;

    while (isdigit(input[*i])) {
        if (j < SF_MAX_TOKEN_VALUE_SIZE - 1) {
            tk.value[j++] = input[*i];
        }

        (*i)++;
        (*col)++;
    }

    tk.value[j] = '\0';

    if (isalpha(input[*i])) {
        *i   = start_i;
        *col = start_col;
        tk.type     = SF_TOKEN_TYPE_UNDEFINED;
        tk.value[0] = '\0';
    }

    return tk;
}

static sf_token_type resolve_keyword(const char* value) {
    if (strcmp(value, "i8")  == 0) return SF_TOKEN_TYPE_KW_I8;
    if (strcmp(value, "i16") == 0) return SF_TOKEN_TYPE_KW_I16;
    if (strcmp(value, "i32") == 0) return SF_TOKEN_TYPE_KW_I32;
    if (strcmp(value, "i64") == 0) return SF_TOKEN_TYPE_KW_I64;

    if (strcmp(value, "u8")  == 0) return SF_TOKEN_TYPE_KW_U8;
    if (strcmp(value, "u16") == 0) return SF_TOKEN_TYPE_KW_U16;
    if (strcmp(value, "u32") == 0) return SF_TOKEN_TYPE_KW_U32;
    if (strcmp(value, "u64") == 0) return SF_TOKEN_TYPE_KW_U64;

    return SF_TOKEN_TYPE_IDENTIFIER;
}

static sf_token read_identifier(const char* input, int* i, int* col, int line) {
    sf_token tk = make_token(SF_TOKEN_TYPE_IDENTIFIER, line, *col);

    int j = 0;

    while (isalnum(input[*i]) || input[*i] == '_') {
        if (j < SF_MAX_TOKEN_VALUE_SIZE - 1)
            tk.value[j++] = input[*i];
        (*i)++;
        (*col)++;
    }

    tk.value[j] = '\0';
    tk.type = resolve_keyword(tk.value);

    return tk;
}

static sf_token read_symbol(const char* input, int* i, int* col, int line) {
    char c  = input[*i];
    sf_token tk = make_token(SF_TOKEN_TYPE_UNDEFINED, line, *col);

    switch (c) {
        case '+': tk.type = SF_TOKEN_TYPE_PLUS;      break;
        case '-': tk.type = SF_TOKEN_TYPE_MINUS;     break;
        case '*': tk.type = SF_TOKEN_TYPE_MULT;      break;
        case '/': tk.type = SF_TOKEN_TYPE_DIV;       break;
        case '=': tk.type = SF_TOKEN_TYPE_EQUALS;    break;
        case ';': tk.type = SF_TOKEN_TYPE_SEMICOLON; break;
        case '{': tk.type = SF_TOKEN_TYPE_LBRACE;    break;
        case '}': tk.type = SF_TOKEN_TYPE_RBRACE;    break;
        case '(': tk.type = SF_TOKEN_TYPE_LPAREN;    break;
        case ')': tk.type = SF_TOKEN_TYPE_RPAREN;    break;

        default:  return tk;
    }

    tk.value[0] = c;
    tk.value[1] = '\0';
    (*i)++;
    (*col)++;

    return tk;
}