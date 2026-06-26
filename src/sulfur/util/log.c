#include "sulfur/util/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void sf_print_indented(const char* text);
static void emit_log(sf_log_info info, va_list args);

void sf_log(sf_log_info info, ...) {
    va_list args;

    va_start(args, info);
    emit_log(info, args);
    va_end(args);
}

void sf_log_helper(
    const char* title,
    const char* desc,
    const char* hint,
    const char* file,
    uint16_t code,
    int line,
    int col,
    sf_severity sev,
    ...
) {
    sf_log_info l = {
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
    emit_log(l, args);
    va_end(args);
}

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

static void emit_log(sf_log_info info, va_list args) {
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