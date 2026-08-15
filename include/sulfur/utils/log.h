#pragma once
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "sulfur/utils/span.h"

#define SF_COLOR_BLACK "\x1b[30m"
#define SF_COLOR_RED "\x1b[31m"
#define SF_COLOR_GREEN "\x1b[32m"
#define SF_COLOR_YELLOW "\x1b[33m"
#define SF_COLOR_BLUE "\x1b[34m"
#define SF_COLOR_MAGENTA "\x1b[35m"
#define SF_COLOR_CYAN "\x1b[36m"
#define SF_COLOR_WHITE "\x1b[37m"
#define SF_COLOR_BBLACK "\x1b[90m"
#define SF_COLOR_BRED "\x1b[91m"
#define SF_COLOR_BGREEN "\x1b[92m"
#define SF_COLOR_BYELLOW "\x1b[93m"
#define SF_COLOR_BBLUE "\x1b[94m"
#define SF_COLOR_BMAGENTA "\x1b[95m"
#define SF_COLOR_BCYAN "\x1b[96m"
#define SF_COLOR_BWHITE "\x1b[97m"
#define SF_COLOR_RESET "\x1b[0m"

#define SF_INDENT "   "

typedef enum {
  SF_SEV_INFO,
  SF_SEV_WARNING,
  SF_SEV_ERROR,
  SF_SEV_FATAL
} sf_severity;

typedef struct {
  const char *title;
  const char *desc;
  const char *hint;
  const char *file;
  uint16_t code;
  sf_span span;
  sf_severity sev;
} sf_log_info;

typedef enum {
  SF_GENERAL_INSUFFICIENT_MEMORY = 0x0000,

  SF_MAIN_NO_INPUT_FILE = 0x1000,
  SF_MAIN_NO_OUTPUT_FILE = 0x1001,
  SF_MAIN_UNKNOWN_FLAG = 0x1002,
  SF_MAIN_CANNOT_OPEN_FILE = 0x1003,

  SF_PREP_TOO_MANY_DEFINES = 0x2000,

  SF_LEXER_UNDEFINED_TOKEN = 0x3000,

  SF_PARSER_UNEXPECTED_TOKEN = 0x5000,
  SF_PARSER_UNDECLARED_VARIABLE = 0x5001,

  SF_SEMANTIC_REDECLARATION = 0x6000,
  SF_SEMANTIC_UNDECLARED = 0x6001,
  SF_SEMANTIC_UNINITIALIZED = 0x6002,
  SF_SEMANTIC_TYPE_MISMATCH = 0x6003,
  SF_SEMANTIC_INVALID_EXPLICIT_CAST = 0x6004,
  SF_SEMANTIC_INVALID_IMPLICIT_CAST = 0x6005,
  SF_SEMANTIC_LITERAL_OVERFLOW = 0x6006,
  SF_SEMANTIC_DIVISION_BY_ZERO = 0x6007,
  SF_SEMANTIC_CONSTANT_EXPR = 0x6008,
  SF_SEMANTIC_RETURN_OUTSIDE_FUNCTION = 0x6009,
  SF_SEMANTIC_NO_VALUE_RETURN = 0x6010,
  SF_SEMANTIC_VOID_RETURN_VALUE = 0x6011,
  SF_SEMANTIC_MISSING_RETURN = 0x6012,
  SF_SEMANTIC_INVALID_TYPE = 0x6013,
} sf_error_code;

void sf_log_set_source(const char *filename, const char *content);

void sf_log(const char *title, const char *desc, const char *hint,
            const char *file, uint16_t code, sf_span span, sf_severity sev,
            ...);

void sf_log_init(void);
bool sf_log_had_fatal(void);
bool sf_log_had_errors(void);