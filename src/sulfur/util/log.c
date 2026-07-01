#include "sulfur/util/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void emit_log(sf_log_info info, va_list args);
static void print_source_snippet(sf_span span, const char* msg);
static const char* find_line(const char* content, uint32_t target_line, size_t* out_len);

static const char* g_source_filename = NULL;
static const char* g_source_content  = NULL;

void sf_log_set_source(const char* filename, const char* content) {
    g_source_filename = filename;
    g_source_content  = content;
}

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
    sf_span span,
    sf_severity sev,
    ...
) {
    sf_log_info l = {
        .title = title,
        .desc  = desc,
        .hint  = hint,
        .file  = file,
        .code  = code,
        .span  = span,
        .sev   = sev
    };

    va_list args;
    va_start(args, sev);
    emit_log(l, args);
    va_end(args);
}

static const char* find_line(const char* content, uint32_t target_line, size_t* out_len) {
    if (!content) return NULL;

    const char* p = content;
    uint32_t current_line = 1;

    while (current_line < target_line) {
        const char* nl = strchr(p, '\n');
        if (!nl) return NULL;
        p = nl + 1;
        current_line++;
    }

    const char* nl = strchr(p, '\n');
    *out_len = nl ? (size_t)(nl - p) : strlen(p);
    return p;
}

static void print_source_snippet(sf_span span, const char* msg) {
    if (span.line == 0 || !g_source_content) return;

    size_t line_len = 0;
    const char* line_start = find_line(g_source_content, span.line, &line_len);
    if (!line_start) return;

    char line_num_str[16];
    snprintf(line_num_str, sizeof(line_num_str), "%u", span.line);
    int gutter_width = (int)strlen(line_num_str);

    printf(SF_COLOR_BBLUE "%*s| \n" SF_COLOR_RESET, gutter_width + 1, "");

    uint32_t col_start = span.col > 0 ? span.col - 1 : 0;
    uint8_t carets = span.len > 0 ? span.len : 1;
    uint32_t col_end = col_start + carets;
    if (col_end > line_len) col_end = (uint32_t)line_len;

    printf(SF_COLOR_BBLUE "%s | " SF_COLOR_RESET, line_num_str);

    if (col_start > 0) {
        printf("%.*s", (int)col_start, line_start);
    }

    if (col_end > col_start) {
        printf(SF_COLOR_BRED "%.*s" SF_COLOR_RESET, (int)(col_end - col_start), line_start + col_start);
    }

    if (col_end < line_len) {
        printf("%.*s", (int)(line_len - col_end), line_start + col_end);
    }

    printf("\n");

    printf(SF_COLOR_BBLUE "%*s| " SF_COLOR_RESET, gutter_width + 1, "");
    for (uint32_t i = 1; i < span.col; i++) {
        putchar((i - 1 < line_len && line_start[i - 1] == '\t') ? '\t' : ' ');
    }

    printf(SF_COLOR_BRED);
    for (uint8_t i = 0; i < carets; i++) putchar('^');
    printf(SF_COLOR_RESET);

    if (msg && msg[0] != '\0') printf(" %s", msg);
    printf("\n");

    printf(SF_COLOR_BBLUE "%*s |\n" SF_COLOR_RESET, gutter_width, "");
}

static void emit_log(sf_log_info info, va_list args) {
    const char *sevStr = "";
    switch (info.sev) {
        case SF_SEV_INFO:    sevStr = SF_COLOR_BCYAN    "info"    SF_COLOR_RESET; break;
        case SF_SEV_WARNING: sevStr = SF_COLOR_BMAGENTA "warning" SF_COLOR_RESET; break;
        case SF_SEV_ERROR:   sevStr = SF_COLOR_BRED     "error"   SF_COLOR_RESET; break;
        case SF_SEV_FATAL:   sevStr = SF_COLOR_RED      "fatal"   SF_COLOR_RESET; break;
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
        "%s " SF_COLOR_BBLACK "[0x%04x]" SF_COLOR_RESET ": %s\n",
        sevStr, info.code, titleBuffer
    );

    if (info.span.line > 0) {
        printf(
            SF_COLOR_BBLUE "  --> from: " SF_COLOR_RESET "%s:%u:%u\n",
            info.file, info.span.line, info.span.col
        );
        print_source_snippet(info.span, info.desc ? descBuffer : NULL);
    } else if (info.desc) {
        printf("  %s\n", descBuffer);
    }

    if (info.hint) {
        printf(SF_COLOR_BGREEN "  --> hint: " SF_COLOR_RESET "%s\n", hintBuffer);
    }

    printf("\n");

    if (info.sev == SF_SEV_FATAL) exit(EXIT_FAILURE);
}