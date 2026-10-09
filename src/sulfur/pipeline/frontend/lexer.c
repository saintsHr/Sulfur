#include "sulfur/pipeline/frontend/lexer.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/utils/log.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    sf_token_type type;
    const char *text;
} sf_token_spelling;

typedef struct {
    const char *input;
    const char *filename;
    size_t pos;
    size_t line;
    size_t col;
    sf_token_list list;
} sf_lexer;

static const sf_token_spelling k_symbols[] = {
    {SF_TOKEN_TYPE_LEFT_SHIFT_EQUAL, "<<="},
    {SF_TOKEN_TYPE_RIGHT_SHIFT_EQUAL, ">>="},

    {SF_TOKEN_TYPE_PLUS_PLUS, "++"},
    {SF_TOKEN_TYPE_MINUS_MINUS, "--"},
    {SF_TOKEN_TYPE_PLUS_EQUAL, "+="},
    {SF_TOKEN_TYPE_MINUS_EQUAL, "-="},
    {SF_TOKEN_TYPE_STAR_EQUAL, "*="},
    {SF_TOKEN_TYPE_SLASH_EQUAL, "/="},
    {SF_TOKEN_TYPE_AMP_EQUAL, "&="},
    {SF_TOKEN_TYPE_PIPE_EQUAL, "|="},
    {SF_TOKEN_TYPE_CARET_EQUAL, "^="},
    {SF_TOKEN_TYPE_EQUAL_EQUAL, "=="},
    {SF_TOKEN_TYPE_BANG_EQUAL, "!="},
    {SF_TOKEN_TYPE_LESS_EQUAL, "<="},
    {SF_TOKEN_TYPE_GREATER_EQUAL, ">="},
    {SF_TOKEN_TYPE_AMP_AMP, "&&"},
    {SF_TOKEN_TYPE_PIPE_PIPE, "||"},
    {SF_TOKEN_TYPE_LEFT_SHIFT, "<<"},
    {SF_TOKEN_TYPE_RIGHT_SHIFT, ">>"},

    {SF_TOKEN_TYPE_PLUS, "+"},
    {SF_TOKEN_TYPE_MINUS, "-"},
    {SF_TOKEN_TYPE_STAR, "*"},
    {SF_TOKEN_TYPE_SLASH, "/"},
    {SF_TOKEN_TYPE_EQUAL, "="},
    {SF_TOKEN_TYPE_BANG, "!"},
    {SF_TOKEN_TYPE_LESS, "<"},
    {SF_TOKEN_TYPE_GREATER, ">"},
    {SF_TOKEN_TYPE_AMP, "&"},
    {SF_TOKEN_TYPE_PIPE, "|"},
    {SF_TOKEN_TYPE_CARET, "^"},
    {SF_TOKEN_TYPE_TILDE, "~"},

    {SF_TOKEN_TYPE_SEMICOLON, ";"},
    {SF_TOKEN_TYPE_LPAREN, "("},
    {SF_TOKEN_TYPE_RPAREN, ")"},
    {SF_TOKEN_TYPE_LBRACE, "{"},
    {SF_TOKEN_TYPE_RBRACE, "}"},
    {SF_TOKEN_TYPE_COMMA, ","},
};

static const sf_token_spelling k_keywords[] = {
    {SF_TOKEN_TYPE_KW_TRUE, "true"},
    {SF_TOKEN_TYPE_KW_FALSE, "false"},

    {SF_TOKEN_TYPE_KW_I8, "i8"},
    {SF_TOKEN_TYPE_KW_I16, "i16"},
    {SF_TOKEN_TYPE_KW_I32, "i32"},
    {SF_TOKEN_TYPE_KW_I64, "i64"},

    {SF_TOKEN_TYPE_KW_U8, "u8"},
    {SF_TOKEN_TYPE_KW_U16, "u16"},
    {SF_TOKEN_TYPE_KW_U32, "u32"},
    {SF_TOKEN_TYPE_KW_U64, "u64"},

    {SF_TOKEN_TYPE_KW_BOOL, "bool"},
    {SF_TOKEN_TYPE_KW_VOID, "void"},

    {SF_TOKEN_TYPE_KW_AS, "as"},

    {SF_TOKEN_TYPE_KW_IF, "if"},
    {SF_TOKEN_TYPE_KW_ELSE, "else"},
    {SF_TOKEN_TYPE_KW_WHILE, "while"},

    {SF_TOKEN_TYPE_KW_FN, "fn"},
    {SF_TOKEN_TYPE_KW_RETURN, "return"},
};

static bool is_digit(char c);
static bool is_space(char c);
static bool is_ident_start(char c);
static bool is_ident_char(char c);

static char lexer_current(const sf_lexer *lx);
static char lexer_next(const sf_lexer *lx);
static void lexer_advance(sf_lexer *lx, size_t count);
static sf_span lexer_span_here(const sf_lexer *lx);

static bool list_add(sf_token_list *list, sf_token token);
static sf_token make_token(sf_token_type type, sf_span span);

static bool lexer_emit(sf_lexer *lx, sf_token token);
static bool lexer_report_undefined(sf_lexer *lx);

static sf_token read_number(sf_lexer *lx);
static sf_token read_identifier(sf_lexer *lx);
static sf_token read_symbol(sf_lexer *lx);

static int digit_value(char c, int base);
static sf_token_type resolve_keyword(const char *value);
static const char *find_spelling(const sf_token_spelling *table, size_t count, sf_token_type type);

sf_token_list sf_tokenize(const char *input, const char *filename) {
    sf_lexer lx = {
        .input = input,
        .filename = filename,
        .pos = 0,
        .line = 1,
        .col = 1,
        .list = {0},
    };

    while (lexer_current(&lx) != '\0') {
        char c = lexer_current(&lx);

        if (c == '\n') {
            lx.line++;
            lx.col = 1;
            lx.pos++;
            continue;
        }

        if (is_space(c)) {
            lexer_advance(&lx, 1);
            continue;
        }

        sf_token tk;

        if (is_digit(c)) {
            tk = read_number(&lx);
        } else if (is_ident_start(c)) {
            tk = read_identifier(&lx);
        } else {
            tk = read_symbol(&lx);
        }

        if (!lexer_emit(&lx, tk)) {
            return lx.list;
        }
    }

    lexer_emit(&lx, make_token(SF_TOKEN_TYPE_EOF, lexer_span_here(&lx)));

    return lx.list;
}

void sf_tokens_print(const sf_token_list *list) {
    for (size_t i = 0; i < list->count; i++) {
        printf(
            "Token %zu: type= %d value= %s (%lu:%lu)\n", i,
            list->tokens[i].type, list->tokens[i].value,
            list->tokens[i].span.line, list->tokens[i].span.col
        );
    }
}

void sf_tokens_free(sf_token_list *list) {
    free(list->tokens);

    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

const char *sf_token_type_name(sf_token_type type) {
    switch (type) {
        case SF_TOKEN_TYPE_IDENTIFIER: {
            return "an identifier";
        }
        case SF_TOKEN_TYPE_INTEGER: {
            return "a integer";
        }

        default: {
            break;
        }
    }

    const char *name = find_spelling(k_symbols, ARRAY_LEN(k_symbols), type);
    if (name != NULL) {
        return name;
    }

    name = find_spelling(k_keywords, ARRAY_LEN(k_keywords), type);
    if (name != NULL) {
        return name;
    }

    return "a token";
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' ||
           c == '\r';
}

static bool is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_ident_char(char c) {
    return is_ident_start(c) || is_digit(c);
}

static char lexer_current(const sf_lexer *lx) {
    return lx->input[lx->pos];
}

static char lexer_next(const sf_lexer *lx) {
    return lx->input[lx->pos] == '\0' ? '\0' : lx->input[lx->pos + 1];
}

static void lexer_advance(sf_lexer *lx, size_t count) {
    lx->pos += count;
    lx->col += count;
}

static sf_span lexer_span_here(const sf_lexer *lx) {
    return (sf_span){.line = lx->line, .col = lx->col, .len = 0};
}

static sf_token make_token(sf_token_type type, sf_span span) {
    sf_token tk = {0};
    tk.type = type;
    tk.span = span;

    return tk;
}

static bool list_add(sf_token_list *list, sf_token token) {
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 16 : list->capacity * 2;

        sf_token *new_tokens = NULL;

        if (new_capacity <= SIZE_MAX / sizeof(sf_token)) {
            new_tokens = realloc(list->tokens, new_capacity * sizeof(sf_token));
        }

        if (new_tokens == NULL) {
            sf_log(
                "Insufficient Memory.", "Cannot allocate memory for compiling.",
                "Free some memory and try again.", NULL,
                SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0},
                SF_LOG_SEVERITY_FATAL
            );
            return false;
        }

        list->tokens = new_tokens;
        list->capacity = new_capacity;
    }

    list->tokens[list->count++] = token;

    return true;
}

static bool lexer_emit(sf_lexer *lx, sf_token token) {
    if (token.type == SF_TOKEN_TYPE_UNDEFINED) {
        return lexer_report_undefined(lx);
    }

    return list_add(&lx->list, token);
}

static bool lexer_report_undefined(sf_lexer *lx) {
    sf_token tk = make_token(SF_TOKEN_TYPE_UNDEFINED, lexer_span_here(lx));

    size_t j = 0;

    while (lexer_current(lx) != '\0' && !is_space(lexer_current(lx))) {
        if (j < SF_MAX_TOKEN_VALUE_SIZE - 1) {
            tk.value[j++] = lexer_current(lx);
        }

        lexer_advance(lx, 1);
    }

    tk.value[j] = '\0';
    tk.span.len = j;

    if (!list_add(&lx->list, tk)) {
        return false;
    }

    sf_log(
        "undefined token", "unrecognized token '%s' in source file",
        "check for typos or invalid characters, and follow the language "
        "grammar",
        lx->filename, SF_LOG_LEXER_UNDEFINED_TOKEN, tk.span,
        SF_LOG_SEVERITY_ERROR, tk.value
    );

    return true;
}

static int digit_value(char c, int base) {
    int digit;

    if (c >= '0' && c <= '9') {
        digit = c - '0';
    } else if (c >= 'a' && c <= 'f') {
        digit = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        digit = c - 'A' + 10;
    } else {
        return -1;
    }

    return digit < base ? digit : -1;
}

static sf_token read_number(sf_lexer *lx) {
    sf_span span = lexer_span_here(lx);
    sf_token tk = make_token(SF_TOKEN_TYPE_INTEGER, span);

    int base = 10;

    if (lexer_current(lx) == '0') {
        char next = lexer_next(lx);

        if (next == 'x' || next == 'X') {
            base = 16;
        } else if (next == 'b' || next == 'B') {
            base = 2;
        }

        if (base != 10) {
            lexer_advance(lx, 2);
        }
    }

    size_t value = 0;
    bool has_digit = false;

    while (lexer_current(lx) != '\0') {
        char c = lexer_current(lx);

        if (c == '_') {
            lexer_advance(lx, 1);
            continue;
        }

        int digit = digit_value(c, base);
        if (digit < 0) {
            break;
        }

        has_digit = true;
        value = value * base + digit;

        lexer_advance(lx, 1);
    }

    if (!has_digit) {
        return make_token(SF_TOKEN_TYPE_UNDEFINED, span);
    }

    snprintf(tk.value, SF_MAX_TOKEN_VALUE_SIZE, "%lu", value);
    tk.span.len = (uint8_t)strlen(tk.value);

    if (is_ident_char(lexer_current(lx))) {
        tk.type = SF_TOKEN_TYPE_UNDEFINED;
    }

    return tk;
}

static sf_token read_identifier(sf_lexer *lx) {
    sf_token tk = make_token(SF_TOKEN_TYPE_IDENTIFIER, lexer_span_here(lx));

    size_t j = 0;

    while (is_ident_char(lexer_current(lx))) {
        if (j < SF_MAX_TOKEN_VALUE_SIZE - 1) {
            tk.value[j++] = lexer_current(lx);
        }

        lexer_advance(lx, 1);
    }

    tk.value[j] = '\0';
    tk.span.len = j;
    tk.type = resolve_keyword(tk.value);

    return tk;
}

static sf_token read_symbol(sf_lexer *lx) {
    sf_token tk = make_token(SF_TOKEN_TYPE_UNDEFINED, lexer_span_here(lx));
    const char *rest = lx->input + lx->pos;

    for (size_t i = 0; i < ARRAY_LEN(k_symbols); i++) {
        const char *text = k_symbols[i].text;
        size_t len = strlen(text);

        if (strncmp(rest, text, len) != 0) {
            continue;
        }

        tk.type = k_symbols[i].type;
        memcpy(tk.value, text, len + 1);
        tk.span.len = len;

        lexer_advance(lx, len);

        return tk;
    }

    return tk;
}

static sf_token_type resolve_keyword(const char *value) {
    for (size_t i = 0; i < ARRAY_LEN(k_keywords); i++) {
        if (strcmp(value, k_keywords[i].text) == 0) {
            return k_keywords[i].type;
        }
    }

    return SF_TOKEN_TYPE_IDENTIFIER;
}

static const char *find_spelling(const sf_token_spelling *table, size_t count, sf_token_type type) {
    for (size_t i = 0; i < count; i++) {
        if (table[i].type == type) {
            return table[i].text;
        }
    }

    return NULL;
}
