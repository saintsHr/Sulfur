#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define SF_COLOR_BLACK   "\x1b[30m"
#define SF_COLOR_RED     "\x1b[31m"
#define SF_COLOR_GREEN   "\x1b[32m"
#define SF_COLOR_YELLOW  "\x1b[33m"
#define SF_COLOR_BLUE    "\x1b[34m"
#define SF_COLOR_MAGENTA "\x1b[35m"
#define SF_COLOR_CYAN    "\x1b[36m"
#define SF_COLOR_WHITE   "\x1b[37m"

#define SF_COLOR_BBLACK   "\x1b[90m"
#define SF_COLOR_BRED     "\x1b[91m"
#define SF_COLOR_BGREEN   "\x1b[92m"
#define SF_COLOR_BYELLOW  "\x1b[93m"
#define SF_COLOR_BBLUE    "\x1b[94m"
#define SF_COLOR_BMAGENTA "\x1b[95m"
#define SF_COLOR_BCYAN    "\x1b[96m"
#define SF_COLOR_BWHITE   "\x1b[97m"

#define SF_COLOR_RESET "\x1b[0m"

#define SF_INDENT "   "

typedef enum {
    SF_SEV_INFO,
    SF_SEV_WARNING,
    SF_SEV_ERROR,
    SF_SEV_FATAL
} sfSeverity;

typedef struct {
    const char* title;
    const char* desc;
    const char* hint;
    const char* file;
    uint16_t code;
    int line;
    int col;
    sfSeverity sev;
} sfLogInfo;

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

    SF_SEMANTIC_REDECLARATION = 0x5000,
    SF_SEMANTIC_UNDECLARED    = 0x5001,
    SF_SEMANTIC_UNINITIALIZED = 0x5002,
    SF_SEMANTIC_TYPE_MISMATCH = 0x5003,
} sfErrorCode;

static void sf_print_indented(const char* text) {
    const char* start = text;

    while (*start) {
        const char* end = strchr(start, '\n');

        if (!end) {
            printf(SF_INDENT "%s\n", start);
            break;
        }

        printf(SF_INDENT "%.*s\n", (int)(end - start), start);
        start = end + 1;
    }
}

static void sf_emit_log_v(sfLogInfo info, va_list args) {
    const char *sevStr = "";
    switch (info.sev) {
        case SF_SEV_INFO:    sevStr = SF_COLOR_BCYAN    "INFO"    SF_COLOR_RESET; break;
        case SF_SEV_WARNING: sevStr = SF_COLOR_BMAGENTA "WARNING" SF_COLOR_RESET; break;
        case SF_SEV_ERROR:   sevStr = SF_COLOR_BRED     "ERROR"   SF_COLOR_RESET; break;
        case SF_SEV_FATAL:   sevStr = SF_COLOR_RED      "FATAL"   SF_COLOR_RESET; break;
    }

    char titleBuffer[1024];
    char descBuffer[1024];
    char hintBuffer[1024];

    va_list args2, args3;
    va_copy(args2, args);
    va_copy(args3, args);

    if (info.title) vsnprintf(titleBuffer, sizeof(titleBuffer), info.title, args);
    else titleBuffer[0] = '\0';

    if (info.desc) vsnprintf(descBuffer, sizeof(descBuffer), info.desc, args2);
    else descBuffer[0] = '\0';

    if (info.hint) vsnprintf(hintBuffer, sizeof(hintBuffer), info.hint, args3);
    else hintBuffer[0] = '\0';

    va_end(args2);
    va_end(args3);

    printf(
        "[%s][%s][%d:%d][0x%04x]: %s\n",
        sevStr, info.file, info.line, info.col, info.code, titleBuffer
    );
    if (info.desc) {
        printf(SF_COLOR_BBLUE " Description:\n" SF_COLOR_RESET);
        sf_print_indented(descBuffer);
    }
    if (info.hint) {
        printf("\n" SF_COLOR_BGREEN " Hint:\n" SF_COLOR_RESET);
        sf_print_indented(hintBuffer);
    }

    if (info.sev == SF_SEV_FATAL) exit(EXIT_FAILURE);
}

static void sfLog(sfLogInfo info, ...) {
    va_list args;

    va_start(args, info);
    sf_emit_log_v(info, args);
    va_end(args);
}

static void sfLogHelper(
    const char* title,
    const char* desc,
    const char* hint,
    const char* file,
    uint16_t code,
    int line,
    int col,
    sfSeverity sev,
    ...
) {
    sfLogInfo l = {
        .title = title,
        .desc  = desc,
        .hint  = hint,
        .file  = file,
        .code  = code,
        .line  = line,
        .col   = col,
        .sev   = sev
    };

    va_list args;
    va_start(args, sev);
    sf_emit_log_v(l, args);
    va_end(args);
}
