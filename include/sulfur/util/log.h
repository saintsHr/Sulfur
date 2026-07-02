#pragma once
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "sulfur/util/span.h"

#define SF_COLOR_BLACK    "\x1b[30m"
#define SF_COLOR_RED      "\x1b[31m"
#define SF_COLOR_GREEN    "\x1b[32m"
#define SF_COLOR_YELLOW   "\x1b[33m"
#define SF_COLOR_BLUE     "\x1b[34m"
#define SF_COLOR_MAGENTA  "\x1b[35m"
#define SF_COLOR_CYAN     "\x1b[36m"
#define SF_COLOR_WHITE    "\x1b[37m"
#define SF_COLOR_BBLACK   "\x1b[90m"
#define SF_COLOR_BRED     "\x1b[91m"
#define SF_COLOR_BGREEN   "\x1b[92m"
#define SF_COLOR_BYELLOW  "\x1b[93m"
#define SF_COLOR_BBLUE    "\x1b[94m"
#define SF_COLOR_BMAGENTA "\x1b[95m"
#define SF_COLOR_BCYAN    "\x1b[96m"
#define SF_COLOR_BWHITE   "\x1b[97m"
#define SF_COLOR_RESET    "\x1b[0m"

#define SF_INDENT "   "

typedef enum {
    SF_SEV_INFO,
    SF_SEV_WARNING,
    SF_SEV_ERROR,
    SF_SEV_FATAL
} sf_severity;

typedef struct {
    const char* title;
    const char* desc;
    const char* hint;
    const char* file;
    uint16_t code;
    sf_span span;
    sf_severity sev;
} sf_log_info;

typedef enum {
    SF_MAIN_NO_INPUT_FILE             = 0x0000,
    SF_MAIN_NO_OUTPUT_FILE            = 0x0001,
    SF_MAIN_UNKNOWN_FLAG              = 0x0002,
    SF_MAIN_CANNOT_OPEN_FILE          = 0x0003,
    SF_MAIN_CANNOT_MALLOC_FILE_BUFFER = 0x0004,

    SF_PREP_TOO_MANY_DEFINES             = 0x1000,
    SF_PREP_CANNOT_MALLOC_DEFINES_BUFFER = 0x1001,
    SF_PREP_CANNOT_MALLOC_OUTPUT         = 0x1002,

    SF_LEXER_UNDEFINED_TOKEN = 0x2000,

    SF_AST_REALLOC_FAILED = 0x3000,

    SF_PARSER_UNEXPECTED_TOKEN    = 0x4000,
    SF_PARSER_UNDECLARED_VARIABLE = 0x4001,

    SF_SEMANTIC_REDECLARATION         = 0x5000,
    SF_SEMANTIC_UNDECLARED            = 0x5001,
    SF_SEMANTIC_UNINITIALIZED         = 0x5002,
    SF_SEMANTIC_TYPE_MISMATCH         = 0x5003,
    SF_SEMANTIC_INVALID_EXPLICIT_CAST = 0x5004,
    SF_SEMANTIC_INVALID_IMPLICIT_CAST = 0x5005,
} sf_error_code;

void sf_log_set_source(const char* filename, const char* content);

void sf_log(
    const char* title,
    const char* desc,
    const char* hint,
    const char* file,
    uint16_t code,
    sf_span span,
    sf_severity sev,
    ...
);

void sf_log_init(void);
bool sf_log_had_fatal(void);
bool sf_log_had_errors(void);