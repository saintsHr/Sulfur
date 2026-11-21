#include "sulfur/pipeline/frontend/lexer/lexer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/io/log.h"
#include "sulfur/pipeline/frontend/lexer/token.h"

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
    {SF_TOKEN_TYPE_LEFT_SHIFT_EQUAL,  "<<="},
    {SF_TOKEN_TYPE_RIGHT_SHIFT_EQUAL, ">>="},
    {SF_TOKEN_TYPE_PLUS_PLUS,         "++" },
    {SF_TOKEN_TYPE_MINUS_MINUS,       "--" },
    {SF_TOKEN_TYPE_PLUS_EQUAL,        "+=" },
    {SF_TOKEN_TYPE_MINUS_EQUAL,       "-=" },
    {SF_TOKEN_TYPE_STAR_EQUAL,        "*=" },
    {SF_TOKEN_TYPE_SLASH_EQUAL,       "/=" },
    {SF_TOKEN_TYPE_AMP_EQUAL,         "&=" },
    {SF_TOKEN_TYPE_PIPE_EQUAL,        "|=" },
    {SF_TOKEN_TYPE_CARET_EQUAL,       "^=" },
    {SF_TOKEN_TYPE_EQUAL_EQUAL,       "==" },
    {SF_TOKEN_TYPE_BANG_EQUAL,        "!=" },
    {SF_TOKEN_TYPE_LESS_EQUAL,        "<=" },
    {SF_TOKEN_TYPE_GREATER_EQUAL,     ">=" },
    {SF_TOKEN_TYPE_AMP_AMP,           "&&" },
    {SF_TOKEN_TYPE_PIPE_PIPE,         "||" },
    {SF_TOKEN_TYPE_LEFT_SHIFT,        "<<" },
    {SF_TOKEN_TYPE_RIGHT_SHIFT,       ">>" },
    {SF_TOKEN_TYPE_COLON_COLON,       "::" },
    {SF_TOKEN_TYPE_PLUS,              "+"  },
    {SF_TOKEN_TYPE_MINUS,             "-"  },
    {SF_TOKEN_TYPE_STAR,              "*"  },
    {SF_TOKEN_TYPE_SLASH,             "/"  },
    {SF_TOKEN_TYPE_EQUAL,             "="  },
    {SF_TOKEN_TYPE_BANG,              "!"  },
    {SF_TOKEN_TYPE_LESS,              "<"  },
    {SF_TOKEN_TYPE_GREATER,           ">"  },
    {SF_TOKEN_TYPE_AMP,               "&"  },
    {SF_TOKEN_TYPE_PIPE,              "|"  },
    {SF_TOKEN_TYPE_CARET,             "^"  },
    {SF_TOKEN_TYPE_TILDE,             "~"  },
    {SF_TOKEN_TYPE_SEMICOLON,         ";"  },
    {SF_TOKEN_TYPE_LPAREN,            "("  },
    {SF_TOKEN_TYPE_RPAREN,            ")"  },
    {SF_TOKEN_TYPE_LBRACE,            "{"  },
    {SF_TOKEN_TYPE_RBRACE,            "}"  },
    {SF_TOKEN_TYPE_COMMA,             ","  },
    {SF_TOKEN_TYPE_DOT,               "."  },
    {SF_TOKEN_TYPE_AT,                "@"  }
};

static const sf_token_spelling k_keywords[] = {
    {SF_TOKEN_TYPE_KW_TRUE,      "true"     },
    {SF_TOKEN_TYPE_KW_FALSE,     "false"    },

    {SF_TOKEN_TYPE_KW_I8,        "i8"       },
    {SF_TOKEN_TYPE_KW_I16,       "i16"      },
    {SF_TOKEN_TYPE_KW_I32,       "i32"      },
    {SF_TOKEN_TYPE_KW_I64,       "i64"      },

    {SF_TOKEN_TYPE_KW_U8,        "u8"       },
    {SF_TOKEN_TYPE_KW_U16,       "u16"      },
    {SF_TOKEN_TYPE_KW_U32,       "u32"      },
    {SF_TOKEN_TYPE_KW_U64,       "u64"      },

    {SF_TOKEN_TYPE_KW_BOOL,      "bool"     },
    {SF_TOKEN_TYPE_KW_VOID,      "void"     },

    {SF_TOKEN_TYPE_KW_AS,        "as"       },

    {SF_TOKEN_TYPE_KW_IF,        "if"       },
    {SF_TOKEN_TYPE_KW_ELSE,      "else"     },
    {SF_TOKEN_TYPE_KW_WHILE,     "while"    },

    {SF_TOKEN_TYPE_KW_FN,        "fn"       },
    {SF_TOKEN_TYPE_KW_RETURN,    "return"   },

    {SF_TOKEN_TYPE_KW_MUT,       "mut"      },
    {SF_TOKEN_TYPE_KW_PUB,       "pub"      },
    {SF_TOKEN_TYPE_KW_EXTERN,    "extern"   },

    {SF_TOKEN_TYPE_KW_IMPORT,    "import"   },

    {SF_TOKEN_TYPE_KW_NAMESPACE, "namespace"}
};

static bool is_digit(char c);
static bool is_space(char c);
static bool is_ident_start(char c);
static bool is_ident_char(char c);
static char lexer_current(const sf_lexer *lx);
static char lexer_next(const sf_lexer *lx);
static void lexer_advance(sf_lexer *lx, size_t count);
static sf_span lexer_span_here(const sf_lexer *lx);
static bool lexer_emit(sf_lexer *lx, sf_token token, size_t start_pos, size_t start_col);
static bool list_add(sf_token_list *list, sf_token token);
static sf_token make_token(sf_token_type type, sf_span span);
static bool lexer_report_undefined(sf_lexer *lx, size_t start_pos, size_t start_col);
static bool undefined_run_continues(const sf_lexer *lx, bool number);
static sf_token read_number(sf_lexer *lx);
static sf_token read_identifier(sf_lexer *lx);
static sf_token read_symbol(sf_lexer *lx);
static sf_token read_char(sf_lexer *lx);
static int digit_value(char c, int base);
static sf_token_type resolve_keyword(const char *value);
static const sf_token_spelling *find_symbol(const char *text);
static const char *find_spelling(
    const sf_token_spelling *table, size_t count, sf_token_type type
);
static bool lexer_report_invalid_char(
    sf_lexer *lx, sf_span span, const char *message, const char *hint
);
static sf_token lexer_invalid_char(
    sf_lexer *lx, size_t start_pos, sf_span span, const char *message, const char *hint
);

sf_token_list sf_lexer_run(const char *input, const char *filename) {
    if (input == NULL) {
        return (sf_token_list){0};
    }

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

        size_t start_pos = lx.pos;
        size_t start_col = lx.col;
        sf_token tk;

        if (is_digit(c)) {
            tk = read_number(&lx);
        } else if (is_ident_start(c)) {
            tk = read_identifier(&lx);
        } else if (c == '\'') {
            tk = read_char(&lx);
        } else {
            tk = read_symbol(&lx);
        }

        if (!lexer_emit(&lx, tk, start_pos, start_col)) {
            return lx.list;
        }
    }

    list_add(&lx.list, make_token(SF_TOKEN_TYPE_EOF, lexer_span_here(&lx)));

    return lx.list;
}

const char *sf_lexer_token_type_name(sf_token_type type) {
    switch (type) {
        case SF_TOKEN_TYPE_IDENTIFIER: {
            return "an identifier";
        }
        case SF_TOKEN_TYPE_INTEGER: {
            return "an integer";
        }
        case SF_TOKEN_TYPE_EOF: {
            return "end of file";
        }
        case SF_TOKEN_TYPE_UNDEFINED: {
            return "an invalid token";
        }

        default: {
            break;
        }
    }

    const char *name =
        find_spelling(k_symbols, sizeof(k_symbols) / sizeof(k_symbols[0]), type);

    if (name != NULL) {
        return name;
    }

    name = find_spelling(k_keywords, sizeof(k_keywords) / sizeof(k_keywords[0]), type);

    if (name != NULL) {
        return name;
    }

    return "a token";
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
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
            sf_log_out_of_memory("N/A");
            return false;
        }

        list->tokens = new_tokens;
        list->capacity = new_capacity;
    }

    list->tokens[list->count++] = token;

    return true;
}

static bool lexer_emit(sf_lexer *lx, sf_token token, size_t start_pos, size_t start_col) {
    if (token.type == SF_TOKEN_TYPE_UNDEFINED) {
        if (lx->input[start_pos] == '\'') {
            return list_add(&lx->list, token);
        }

        return lexer_report_undefined(lx, start_pos, start_col);
    }

    return list_add(&lx->list, token);
}

static bool undefined_run_continues(const sf_lexer *lx, bool number) {
    char c = lexer_current(lx);

    if (c == '\0' || is_space(c)) {
        return false;
    }

    if (number) {
        return is_ident_char(c);
    }

    return !is_ident_char(c) && find_symbol(lx->input + lx->pos) == NULL;
}

static bool lexer_report_undefined(sf_lexer *lx, size_t start_pos, size_t start_col) {
    lx->pos = start_pos;
    lx->col = start_col;

    sf_token tk = make_token(SF_TOKEN_TYPE_UNDEFINED, lexer_span_here(lx));

    bool number = is_digit(lexer_current(lx));
    size_t j = 0;

    do {
        if (j < SF_TOKEN_VALUE_SIZE - 1) {
            tk.value[j++] = lexer_current(lx);
        }

        lexer_advance(lx, 1);
    } while (undefined_run_continues(lx, number));

    tk.value[j] = '\0';
    tk.span.len = (uint8_t)j;

    if (!list_add(&lx->list, tk)) {
        return false;
    }

    sf_log(
        "Undefined Token", "The token '%s' is not recognized.",
        "Check for typographical errors.", lx->filename,
        SF_LOG_CODE_LEXER_UNDEFINED_TOKEN, tk.span, SF_LOG_SEVERITY_ERROR, tk.value
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
    size_t start = lx->pos;
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

    unsigned long long value = 0;
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
        value = value * (unsigned long long)base + (unsigned long long)digit;

        lexer_advance(lx, 1);
    }

    if (!has_digit || is_ident_char(lexer_current(lx))) {
        return make_token(SF_TOKEN_TYPE_UNDEFINED, span);
    }

    size_t len = lx->pos - start;

    snprintf(tk.value, SF_TOKEN_VALUE_SIZE, "%llu", value);
    tk.span.len = (uint8_t)(len > UINT8_MAX ? UINT8_MAX : len);

    return tk;
}

static sf_token read_identifier(sf_lexer *lx) {
    sf_token tk = make_token(SF_TOKEN_TYPE_IDENTIFIER, lexer_span_here(lx));

    size_t j = 0;

    while (is_ident_char(lexer_current(lx))) {
        if (j < SF_TOKEN_VALUE_SIZE - 1) {
            tk.value[j++] = lexer_current(lx);
        }

        lexer_advance(lx, 1);
    }

    tk.value[j] = '\0';
    tk.span.len = (uint8_t)j;
    tk.type = resolve_keyword(tk.value);

    return tk;
}

static sf_token read_symbol(sf_lexer *lx) {
    sf_token tk = make_token(SF_TOKEN_TYPE_UNDEFINED, lexer_span_here(lx));
    const sf_token_spelling *symbol = find_symbol(lx->input + lx->pos);

    if (symbol == NULL) {
        return tk;
    }

    size_t len = strlen(symbol->text);

    tk.type = symbol->type;
    memcpy(tk.value, symbol->text, len + 1);
    tk.span.len = (uint8_t)len;

    lexer_advance(lx, len);

    return tk;
}

static sf_token read_char(sf_lexer *lx) {
    sf_span span = lexer_span_here(lx);
    sf_token tk = make_token(SF_TOKEN_TYPE_CHAR, span);

    size_t start_pos = lx->pos;

    lexer_advance(lx, 1); // Opening quote

    char c = lexer_current(lx);

    if (c == '\0' || c == '\n') {
        return lexer_invalid_char(
            lx, start_pos, span, "Unterminated character literal.",
            "Add a closing single quote."
        );
    }

    if (c == '\'') {
        return lexer_invalid_char(
            lx, start_pos, span, "Empty character literal.",
            "A character literal must contain exactly one byte."
        );
    }

    uint8_t value;

    if (c == '\\') {
        lexer_advance(lx, 1);

        c = lexer_current(lx);

        if (c == '\0' || c == '\n') {
            return lexer_invalid_char(
                lx, start_pos, span, "Unterminated character literal.",
                "Add a valid escape sequence and a closing single quote."
            );
        }

        switch (c) {
            case '0':
                value = '\0';
                break;
            case 'n':
                value = '\n';
                break;
            case 'r':
                value = '\r';
                break;
            case 't':
                value = '\t';
                break;
            case '\\':
                value = '\\';
                break;
            case '\'':
                value = '\'';
                break;

            default:
                return lexer_invalid_char(
                    lx, start_pos, span, "Invalid escape sequence in character literal.",
                    "Use a supported escape sequence."
                );
        }

        lexer_advance(lx, 1);
    } else {
        if ((unsigned char)c >= 0x80) {
            return lexer_invalid_char(
                lx, start_pos, span, "Character literal must contain exactly one byte.",
                "UTF-8 multibyte characters cannot be represented by u8 "
                "character literals."
            );
        }

        value = (uint8_t)c;
        lexer_advance(lx, 1);
    }

    if (lexer_current(lx) != '\'') {
        return lexer_invalid_char(
            lx, start_pos, span, "Multiple characters in character literal.",
            "A character literal must contain exactly one byte."
        );
    }

    lexer_advance(lx, 1);

    snprintf(tk.value, SF_TOKEN_VALUE_SIZE, "%u", (unsigned)value);

    tk.span.len = (uint8_t)(lx->pos - start_pos);

    return tk;
}

static sf_token_type resolve_keyword(const char *value) {
    for (size_t i = 0; i < sizeof(k_keywords) / sizeof(k_keywords[0]); i++) {
        if (strcmp(value, k_keywords[i].text) == 0) {
            return k_keywords[i].type;
        }
    }

    return SF_TOKEN_TYPE_IDENTIFIER;
}

static const sf_token_spelling *find_symbol(const char *text) {
    for (size_t i = 0; i < sizeof(k_symbols) / sizeof(k_symbols[0]); i++) {
        if (strncmp(text, k_symbols[i].text, strlen(k_symbols[i].text)) == 0) {
            return &k_symbols[i];
        }
    }

    return NULL;
}

static const char *find_spelling(
    const sf_token_spelling *table, size_t count, sf_token_type type
) {
    for (size_t i = 0; i < count; i++) {
        if (table[i].type == type) {
            return table[i].text;
        }
    }

    return NULL;
}

static bool lexer_report_invalid_char(
    sf_lexer *lx, sf_span span, const char *message, const char *hint
) {
    char value[SF_TOKEN_VALUE_SIZE] = {0};

    size_t len = span.len;

    if (len >= sizeof(value)) {
        len = sizeof(value) - 1;
    }

    memcpy(value, lx->input + lx->pos - span.len, len);

    sf_log(
        "Invalid Character Literal", message, hint, lx->filename,
        SF_LOG_CODE_LEXER_UNDEFINED_TOKEN, span, SF_LOG_SEVERITY_ERROR, value
    );

    return true;
}

static sf_token lexer_invalid_char(
    sf_lexer *lx, size_t start_pos, sf_span span, const char *message, const char *hint
) {
    while (lexer_current(lx) != '\0' && lexer_current(lx) != '\n' &&
           lexer_current(lx) != '\'') {
        lexer_advance(lx, 1);
    }

    if (lexer_current(lx) == '\'') {
        lexer_advance(lx, 1);
    }

    size_t len = lx->pos - start_pos;

    span.len = (uint8_t)(len > UINT8_MAX ? UINT8_MAX : len);

    lexer_report_invalid_char(lx, span, message, hint);

    return make_token(SF_TOKEN_TYPE_UNDEFINED, span);
}
