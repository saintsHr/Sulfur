#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/utils/span.h"

#define SF_LOG_COLOR_BLACK "\x1b[30m"
#define SF_LOG_COLOR_RED "\x1b[31m"
#define SF_LOG_COLOR_GREEN "\x1b[32m"
#define SF_LOG_COLOR_YELLOW "\x1b[33m"
#define SF_LOG_COLOR_BLUE "\x1b[34m"
#define SF_LOG_COLOR_MAGENTA "\x1b[35m"
#define SF_LOG_COLOR_CYAN "\x1b[36m"
#define SF_LOG_COLOR_WHITE "\x1b[37m"
#define SF_LOG_COLOR_BRIGHT_BLACK "\x1b[90m"
#define SF_LOG_COLOR_BRIGHT_RED "\x1b[91m"
#define SF_LOG_COLOR_BRIGHT_GREEN "\x1b[92m"
#define SF_LOG_COLOR_BRIGHT_YELLOW "\x1b[93m"
#define SF_LOG_COLOR_BRIGHT_BLUE "\x1b[94m"
#define SF_LOG_COLOR_BRIGHT_MAGENTA "\x1b[95m"
#define SF_LOG_COLOR_BRIGHT_CYAN "\x1b[96m"
#define SF_LOG_COLOR_BRIGHT_WHITE "\x1b[97m"
#define SF_LOG_COLOR_RESET "\x1b[0m"

#define SF_LOG_INDENT "   "

typedef uint16_t sf_log_error_code_num;

typedef enum {
    SF_LOG_SEVERITY_INFO,
    SF_LOG_SEVERITY_WARNING,
    SF_LOG_SEVERITY_ERROR,
    SF_LOG_SEVERITY_FATAL
} sf_log_severity;

typedef enum {
    SF_LOG_GENERAL_INSUFFICIENT_MEMORY = 0x0000,

    SF_LOG_MAIN_NO_INPUT_FILE = 0x1000,
    SF_LOG_MAIN_NO_OUTPUT_FILE = 0x1001,
    SF_LOG_MAIN_UNKNOWN_FLAG = 0x1002,
    SF_LOG_MAIN_CANNOT_OPEN_FILE = 0x1003,

    SF_LOG_PREP_TOO_MANY_DEFINES = 0x2000,

    SF_LOG_LEXER_UNDEFINED_TOKEN = 0x3000,

    SF_LOG_PARSER_UNEXPECTED_TOKEN = 0x5000,
    SF_LOG_PARSER_UNDECLARED_VARIABLE = 0x5001,

    SF_LOG_SEMANTIC_REDECLARATION = 0x6000,
    SF_LOG_SEMANTIC_UNDECLARED = 0x6001,
    SF_LOG_SEMANTIC_UNINITIALIZED = 0x6002,
    SF_LOG_SEMANTIC_TYPE_MISMATCH = 0x6003,
    SF_LOG_SEMANTIC_INVALID_EXPLICIT_CAST = 0x6004,
    SF_LOG_SEMANTIC_INVALID_IMPLICIT_CAST = 0x6005,
    SF_LOG_SEMANTIC_LITERAL_OVERFLOW = 0x6006,
    SF_LOG_SEMANTIC_DIVISION_BY_ZERO = 0x6007,
    SF_LOG_SEMANTIC_CONSTANT_EXPR = 0x6008,
    SF_LOG_SEMANTIC_RETURN_OUTSIDE_FUNCTION = 0x6009,
    SF_LOG_SEMANTIC_NO_VALUE_RETURN = 0x600A,
    SF_LOG_SEMANTIC_VOID_RETURN_VALUE = 0x600B,
    SF_LOG_SEMANTIC_MISSING_RETURN = 0x600C,
    SF_LOG_SEMANTIC_INVALID_TYPE = 0x600D,
} sf_log_error_code;

typedef struct {
    const char *title;
    const char *desc;
    const char *hint;
    const char *file;
    sf_log_error_code_num code;
    sf_span span;
    sf_log_severity sev;
} sf_log_info;

void sf_log_init(void);
void sf_log_set_source(const char *filename, const char *content);

void sf_log(
    const char *title, const char *desc, const char *hint, const char *file,
    sf_log_error_code_num code, sf_span span, sf_log_severity sev, ...
);

bool sf_log_had_fatal(void);
bool sf_log_had_errors(void);
