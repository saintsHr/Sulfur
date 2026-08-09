#include "sulfur/pipeline/frontend/lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/utils/log.h"

static void undefined(sf_token_list *list, const char *input, int *i, int *col,
                      int line, const char *filename);

static void add_token(sf_token_list *list, sf_token token);
static sf_token make_token(sf_token_type type, sf_span span);
static void make_two_char_token(sf_token *tk, int *i, int *col, char c1,
                                char c2, sf_token_type type);

static sf_token_type resolve_keyword(const char *value);

static sf_token read_number(const char *input, int *i, int *col, int line);
static sf_token read_identifier(const char *input, int *i, int *col, int line);
static sf_token read_symbol(const char *input, int *i, int *col, int line);

sf_token_list sf_tokenize(const char *input, const char *filename) {
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
      sf_token tk = read_number(input, &i, &col, line);

      if (tk.type == SF_TOKEN_TYPE_UNDEFINED) {
        undefined(&list, input, &i, &col, line, filename);
      } else {
        add_token(&list, tk);
      }

      continue;
    }

    if (isalpha(c) || c == '_') {
      add_token(&list, read_identifier(input, &i, &col, line));
      continue;
    }

    sf_token tk = read_symbol(input, &i, &col, line);

    if (tk.type == SF_TOKEN_TYPE_UNDEFINED) {
      undefined(&list, input, &i, &col, line, filename);
    } else {
      add_token(&list, tk);
    }
  }

  add_token(&list, make_token(SF_TOKEN_TYPE_EOF,
                              (sf_span){.line = line, .col = col, .len = 0}));

  return list;
}

void sf_print_tokens(const sf_token_list *list) {
  for (size_t i = 0; i < list->count; i++) {
    printf("Token %zu: type=%d value=%s (%u:%u)\n", i, list->tokens[i].type,
           list->tokens[i].value, list->tokens[i].span.line,
           list->tokens[i].span.col);
  }
}

void sf_free_tokens(sf_token_list *list) {
  free(list->tokens);

  list->tokens = NULL;
  list->count = 0;
  list->capacity = 0;
}

const char *sf_token_type_name(sf_token_type type) {
  for (uint64_t i = 0;
       i < sizeof(token_string_map) / sizeof(token_string_map[0]); i++) {
    if (type == token_string_map[i].type)
      return token_string_map[i].string;
  }

  return "a token";
}

static void undefined(sf_token_list *list, const char *input, int *i, int *col,
                      int line, const char *filename) {
  sf_token tk = {0};
  tk.type = SF_TOKEN_TYPE_UNDEFINED;
  tk.span.line = line;
  tk.span.col = *col;

  int j = 0;

  while (input[*i] != '\0' && !isspace(input[*i])) {
    if (j < SF_MAX_TOKEN_VALUE_SIZE - 1)
      tk.value[j++] = input[*i];
    (*i)++;
    (*col)++;
  }

  tk.value[j] = '\0';
  tk.span.len = (uint8_t)j;
  add_token(list, tk);

  sf_log("undefined token", "unrecognized token '%s' in source file",
         "check for typos or invalid characters, and follow the language "
         "grammar",
         filename, SF_LEXER_UNDEFINED_TOKEN, tk.span, SF_SEV_ERROR, tk.value);
}

static void add_token(sf_token_list *list, sf_token token) {
  if (list->count >= list->capacity) {
    list->capacity = list->capacity == 0 ? 16 : list->capacity * 2;

    sf_token *new_tokens =
        realloc(list->tokens, list->capacity * sizeof(sf_token));

    if (new_tokens == NULL) {
      sf_log("Insufficient Memory.", "Cannot allocate memory for compiling.",
             "Free some memory and try again.", NULL,
             SF_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_SEV_FATAL);
    }

    list->tokens = new_tokens;
  }

  list->tokens[list->count++] = token;
}

static sf_token make_token(sf_token_type type, sf_span span) {
  sf_token tk = {0};
  tk.type = type;
  tk.span = span;
  return tk;
}

static void make_two_char_token(sf_token *tk, int *i, int *col, char c1,
                                char c2, sf_token_type type) {
  tk->type = type;

  tk->value[0] = c1;
  tk->value[1] = c2;
  tk->value[2] = '\0';

  tk->span.len = 2;

  (*i) += 2;
  (*col) += 2;
}

static sf_token read_number(const char *input, int *i, int *col, int line) {
  int start_i = *i;
  int start_col = *col;

  sf_token tk = make_token(SF_TOKEN_TYPE_INTEGER,
                           (sf_span){.line = line, .col = *col, .len = 0});

  unsigned long long value = 0;
  int base = 10;

  if (input[*i] == '0') {
    char next = input[*i + 1];

    if (next == 'x' || next == 'X') {
      base = 16;
      (*i) += 2;
      (*col) += 2;
    } else if (next == 'b' || next == 'B') {
      base = 2;
      (*i) += 2;
      (*col) += 2;
    }
  }

  bool has_digit = false;

  while (input[*i] != '\0') {
    char c = input[*i];
    int digit = -1;

    if (base == 10) {
      if (isdigit(c))
        digit = c - '0';
    } else if (base == 16) {
      if (isdigit(c))
        digit = c - '0';
      else if (c >= 'a' && c <= 'f')
        digit = c - 'a' + 10;
      else if (c >= 'A' && c <= 'F')
        digit = c - 'A' + 10;
    } else if (base == 2) {
      if (c == '0' || c == '1')
        digit = c - '0';
    }

    if (digit == -1)
      break;

    has_digit = true;

    value = value * base + digit;

    (*i)++;
    (*col)++;
  }

  if (!has_digit) {
    return make_token(SF_TOKEN_TYPE_UNDEFINED,
                      (sf_span){.line = line, .col = start_col, .len = 0});
  }

  if (*i == start_i) {
    return make_token(SF_TOKEN_TYPE_UNDEFINED,
                      (sf_span){.line = line, .col = start_col, .len = 0});
  }

  snprintf(tk.value, SF_MAX_TOKEN_VALUE_SIZE, "%llu", value);
  tk.span.len = (uint8_t)strlen(tk.value);

  if (isalnum(input[*i]) || input[*i] == '_') {
    tk.type = SF_TOKEN_TYPE_UNDEFINED;
  }

  return tk;
}

static sf_token_type resolve_keyword(const char *value) {
  for (uint64_t i = 0;
       i < sizeof(token_string_map) / sizeof(token_string_map[0]); i++) {
    if (strcmp(value, token_string_map[i].string) == 0)
      return token_string_map[i].type;
  }

  return SF_TOKEN_TYPE_IDENTIFIER;
}

static sf_token read_identifier(const char *input, int *i, int *col, int line) {
  sf_token tk = make_token(SF_TOKEN_TYPE_IDENTIFIER,
                           (sf_span){.line = line, .col = *col, .len = 0});

  int j = 0;

  while (isalnum(input[*i]) || input[*i] == '_') {
    if (j < SF_MAX_TOKEN_VALUE_SIZE - 1)
      tk.value[j++] = input[*i];
    (*i)++;
    (*col)++;
  }

  tk.value[j] = '\0';
  tk.span.len = (uint8_t)j;
  tk.type = resolve_keyword(tk.value);

  return tk;
}

static sf_token read_symbol(const char *input, int *i, int *col, int line) {
  char c1 = input[*i];
  char c2 = input[*i + 1];

  sf_token tk = make_token(SF_TOKEN_TYPE_UNDEFINED,
                           (sf_span){.line = line, .col = *col, .len = 0});

  switch (c1) {
  case '+':
    if (c2 == '+') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_PLUS_PLUS);
      return tk;
    }
    if (c2 == '=') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_PLUS_EQUAL);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_PLUS;
    break;

  case '-':
    if (c2 == '-') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_MINUS_MINUS);
      return tk;
    }
    if (c2 == '=') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_MINUS_EQUAL);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_MINUS;
    break;

  case '*':
    if (c2 == '=') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_STAR_EQUAL);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_STAR;
    break;

  case '/':
    if (c2 == '=') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_SLASH_EQUAL);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_SLASH;
    break;

  case '=':
    if (c2 == '=') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_EQUAL_EQUAL);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_EQUAL;
    break;

  case ';':
    tk.type = SF_TOKEN_TYPE_SEMICOLON;
    break;

  case '{':
    tk.type = SF_TOKEN_TYPE_LBRACE;
    break;

  case '}':
    tk.type = SF_TOKEN_TYPE_RBRACE;
    break;

  case '(':
    tk.type = SF_TOKEN_TYPE_LPAREN;
    break;

  case ')':
    tk.type = SF_TOKEN_TYPE_RPAREN;
    break;

  case '&': {
    if (c2 == '&') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_AMP_AMP);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_AMP;
    break;
  }

  case '|': {
    if (c2 == '|') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_PIPE_PIPE);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_PIPE;
    break;
  }

  case '^':
    tk.type = SF_TOKEN_TYPE_CARET;
    break;

  case '~':
    tk.type = SF_TOKEN_TYPE_TILDE;
    break;

  case '<': {
    if (c2 == '<') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_LEFT_SHIFT);
      return tk;
    }
    if (c2 == '=') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_LESS_EQUAL);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_LESS;
    break;
  }

  case '>': {
    if (c2 == '>') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_RIGHT_SHIFT);
      return tk;
    }
    if (c2 == '=') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_GREATER_EQUAL);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_GREATER;
    break;
  }

  case '!': {
    if (c2 == '=') {
      make_two_char_token(&tk, i, col, c1, c2, SF_TOKEN_TYPE_BANG_EQUAL);
      return tk;
    }

    tk.type = SF_TOKEN_TYPE_BANG;
    break;
  }

  default:
    return tk;
  }

  tk.value[0] = c1;
  tk.value[1] = '\0';

  tk.span.len = 1;

  (*i)++;
  (*col)++;

  return tk;
}