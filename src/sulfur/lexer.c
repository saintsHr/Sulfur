#include "sulfur/lexer.h"
#include "sulfur/util/log.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void undefined(sfTokenList* list, const char* input, int* i, int* col, int line, const char* filename) {
    sfToken tk = {0};
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
    addToken(list, tk);

    sfLogHelper(
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

void addToken(sfTokenList* list, sfToken token) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        list->tokens = realloc(list->tokens, list->capacity * sizeof(sfToken));
    }

    list->tokens[list->count++] = token;
}

sfTokenList tokenize(const char* input, const char* filename) {
    sfTokenList list = {0};

    int i = 0;
    int line = 1;
    int col = 1;

    while (input[i] != '\0') {
        char c = input[i];

        if (c == '\n') {
            line++;
            col = 1;
            i++;
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\r') {
            i++;
            col++;
            continue;
        }

        if (isdigit(c)) {
            int start_i = i;
            sfToken tk = {0};
            tk.type = SF_TOKEN_TYPE_NUMBER;
            tk.line = line;
            tk.column = col;

            int j = 0;
            bool has_dot = false;

            while (isdigit(input[i]) || (input[i] == '.' && !has_dot)) {
                if (input[i] == '.') has_dot = true;
                if (j < SF_MAX_TOKEN_VALUE_SIZE - 1)
                    tk.value[j++] = input[i];
                i++;
                col++;
            }

            tk.value[j] = '\0';

            if (isalpha(input[i])) {
                i = start_i;
                col = tk.column;
                undefined(&list, input, &i, &col, line, filename);
                continue;
            }

            addToken(&list, tk);
            continue;
        }

        if (isalpha(c) || c == '_') {
            sfToken tk = {0};
            tk.line = line;
            tk.column = col;

            int j = 0;

            while (isalnum(input[i]) || input[i] == '_') {
                if (j < SF_MAX_TOKEN_VALUE_SIZE - 1)
                    tk.value[j++] = input[i];
                i++;
                col++;
            }

            tk.value[j] = '\0';

            if      (strcmp(tk.value, "i8")  == 0)    tk.type = SF_TOKEN_TYPE_KW_I8;
            else if (strcmp(tk.value, "i16") == 0)    tk.type = SF_TOKEN_TYPE_KW_I16;
            else if (strcmp(tk.value, "i32") == 0)    tk.type = SF_TOKEN_TYPE_KW_I32;
            else if (strcmp(tk.value, "i64") == 0)    tk.type = SF_TOKEN_TYPE_KW_I64;
            else if (strcmp(tk.value, "u8")  == 0)    tk.type = SF_TOKEN_TYPE_KW_U8;
            else if (strcmp(tk.value, "u16") == 0)    tk.type = SF_TOKEN_TYPE_KW_U16;
            else if (strcmp(tk.value, "u32") == 0)    tk.type = SF_TOKEN_TYPE_KW_U32;
            else if (strcmp(tk.value, "u64") == 0)    tk.type = SF_TOKEN_TYPE_KW_U64;
            else if (strcmp(tk.value, "f32") == 0)    tk.type = SF_TOKEN_TYPE_KW_F32;
            else if (strcmp(tk.value, "f64") == 0)    tk.type = SF_TOKEN_TYPE_KW_F64;
            else if (strcmp(tk.value, "string") == 0) tk.type = SF_TOKEN_TYPE_KW_STRING;
            else tk.type = SF_TOKEN_TYPE_IDENTIFIER;

            addToken(&list, tk);
            continue;
        }

        if (c == '"') {
            sfToken tk = {0};
            tk.type = SF_TOKEN_TYPE_STRING;
            tk.line = line;
            tk.column = col;

            i++;
            col++;

            int j = 0;

            while (input[i] != '"' && input[i] != '\0') {
                if (j < SF_MAX_TOKEN_VALUE_SIZE - 1)
                    tk.value[j++] = input[i];
                i++;
                col++;
            }

            tk.value[j] = '\0';

            if (input[i] == '"') {
                i++;
                col++;
            } else {
                sfLogHelper(
                    "Unterminated String",
                    "String literal was never closed.",
                    "Add a closing '\"' to the string.",
                    filename,
                    SF_LEXER_UNDEFINED_TOKEN,
                    tk.line,
                    tk.column,
                    SF_SEV_FATAL
                );
            }

            addToken(&list, tk);
            continue;
        }

        {
            sfToken tk = {0};
            tk.line = line;
            tk.column = col;

            switch (c) {
                case '+':
                    tk.type = SF_TOKEN_TYPE_PLUS;
                    break;
                case '-':
                    tk.type = SF_TOKEN_TYPE_MINUS;
                    break;
                case '*':
                    tk.type = SF_TOKEN_TYPE_MULT;
                    break;
                case '/':
                    tk.type = SF_TOKEN_TYPE_DIV;
                    break;
                case '=':
                    tk.type = SF_TOKEN_TYPE_EQUALS;
                    break;
                case ';':
                    tk.type = SF_TOKEN_TYPE_SEMICOLON;
                    break;
                default:
                    undefined(&list, input, &i, &col, line, filename);
                    continue;
            }

            tk.value[0] = c;
            tk.value[1] = '\0';
            addToken(&list, tk);
            col++;
            i++;
            continue;
        }
    }

    sfToken tk = {0};
    tk.line = line;
    tk.column = col;
    tk.type = SF_TOKEN_TYPE_EOF;
    tk.value[0] = '\0';
    addToken(&list, tk);

    return list;
}
