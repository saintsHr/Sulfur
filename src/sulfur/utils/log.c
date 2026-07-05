#include "sulfur/utils/log.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void emit_log(sf_log_info info, va_list args);
static void sf_log_internal(sf_log_info info, ...);
static void print_source_snippet(sf_span span, const char* msg);
static const char* find_line(const char* content, uint32_t target_line, size_t* out_len);
static void format_into(char* buf, size_t buf_size, size_t* buf_pos, const char* fmt, va_list args);

static const char* g_source_filename = NULL;
static const char* g_source_content  = NULL;
static bool g_had_fatal = false;
static uint8_t g_error_count = 0;

void sf_log_set_source(const char* filename, const char* content) {
    g_source_filename = filename;
    g_source_content  = content;
}

void sf_log(
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

void sf_log_init(void) {
    g_had_fatal = false;
}

bool sf_log_had_fatal(void) {
    return g_had_fatal;
}

bool sf_log_had_errors(void) {
    return g_error_count > 0;
};

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

    size_t pos;

    pos = 0;
    if (info.title) {
        format_into(
            titleBuffer,
            sizeof(titleBuffer),
            &pos,
            info.title,
            args
        );
    } else {
        titleBuffer[0] = '\0';
    }

    pos = 0;
    if (info.desc) {
        format_into(
            descBuffer,
            sizeof(descBuffer),
            &pos,
            info.desc,
            args
        );
    } else {
        descBuffer[0] = '\0';
    }

    pos = 0;
    if (info.hint) {
        format_into(
            hintBuffer,
            sizeof(hintBuffer),
            &pos,
            info.hint,
            args
        );
    } else {
        hintBuffer[0] = '\0';
    }

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

    if (info.sev == SF_SEV_ERROR || info.sev == SF_SEV_FATAL) g_error_count++;
    if (info.sev == SF_SEV_FATAL) g_had_fatal = true;
}

static void sf_log_internal(sf_log_info info, ...) {
    va_list args;
    va_start(args, info);
    emit_log(info, args);
    va_end(args);
}

static void format_into(char* buf, size_t buf_size, size_t* buf_pos, const char* fmt, va_list args) {
    if (!fmt) return;
    char num_buf[64];
    char spec_buf[32];

    for (const char* p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            if (*buf_pos < buf_size - 1) buf[(*buf_pos)++] = *p;
            continue;
        }

        const char* spec_start = p;
        p++;
        if (*p == '\0') break;

        while (*p == '-' || *p == '0' || *p == '+' || *p == ' ' || *p == '#') p++;

        while (*p >= '0' && *p <= '9') p++;

        if (*p == '.') {
            p++;
            while (*p >= '0' && *p <= '9') p++;
        }

        int len_mod = 0;
        if (*p == 'l') {
            p++;
            if (*p == 'l') { len_mod = 2; p++; }
            else len_mod = 1;
        } else if (*p == 'h') {
            p++;
            if (*p == 'h') { len_mod = -2; p++; }
            else len_mod = -1;
        } else if (*p == 'z') {
            len_mod = 3;
            p++;
        }

        if (*p == '\0') break;

        size_t prefix_len = (size_t)(p - spec_start);
        char flags_width_prec[24];
        {
            const char* q = spec_start + 1;
            size_t k = 0;
            while ((*q == '-' || *q == '0' || *q == '+' || *q == ' ' || *q == '#' ||
                    (*q >= '0' && *q <= '9') || *q == '.') && k < sizeof(flags_width_prec) - 1) {
                flags_width_prec[k++] = *q;
                q++;
            }
            flags_width_prec[k] = '\0';
        }

        switch (*p) {
            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";
                snprintf(spec_buf, sizeof(spec_buf), "%%%ss", flags_width_prec);
                int n = snprintf(num_buf, sizeof(num_buf), spec_buf, s);
                size_t space = buf_size - 1 - *buf_pos;
                size_t to_copy = (size_t)n < space ? (size_t)n : space;
                memcpy(buf + *buf_pos, num_buf, to_copy);
                *buf_pos += to_copy;
                break;
            }

            case 'd':
            case 'i': {
                int n;
                if (len_mod == 2) {
                    long long v = va_arg(args, long long);
                    snprintf(spec_buf, sizeof(spec_buf), "%%%slld", flags_width_prec);
                    n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                } else if (len_mod == 1) {
                    long v = va_arg(args, long);
                    snprintf(spec_buf, sizeof(spec_buf), "%%%sld", flags_width_prec);
                    n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                } else if (len_mod == 3) {
                    ptrdiff_t v = va_arg(args, ptrdiff_t);
                    snprintf(spec_buf, sizeof(spec_buf), "%%%std", flags_width_prec);
                    n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                } else {
                    int v = va_arg(args, int);
                    snprintf(spec_buf, sizeof(spec_buf), "%%%sd", flags_width_prec);
                    n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                }
                size_t space = buf_size - 1 - *buf_pos;
                size_t to_copy = (size_t)n < space ? (size_t)n : space;
                memcpy(buf + *buf_pos, num_buf, to_copy);
                *buf_pos += to_copy;
                break;
            }

            case 'u':
            case 'x':
            case 'X':
            case 'o': {
                char conv = *p;
                int n;
                if (len_mod == 2) {
                    unsigned long long v = va_arg(args, unsigned long long);
                    snprintf(spec_buf, sizeof(spec_buf), "%%%sll%c", flags_width_prec, conv);
                    n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                } else if (len_mod == 1) {
                    unsigned long v = va_arg(args, unsigned long);
                    snprintf(spec_buf, sizeof(spec_buf), "%%%sl%c", flags_width_prec, conv);
                    n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                } else if (len_mod == 3) {
                    size_t v = va_arg(args, size_t);
                    snprintf(spec_buf, sizeof(spec_buf), "%%%sz%c", flags_width_prec, conv);
                    n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                } else {
                    unsigned int v = va_arg(args, unsigned int);
                    snprintf(spec_buf, sizeof(spec_buf), "%%%s%c", flags_width_prec, conv);
                    n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                }
                size_t space = buf_size - 1 - *buf_pos;
                size_t to_copy = (size_t)n < space ? (size_t)n : space;
                memcpy(buf + *buf_pos, num_buf, to_copy);
                *buf_pos += to_copy;
                break;
            }

            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G': {
                double v = va_arg(args, double);
                snprintf(spec_buf, sizeof(spec_buf), "%%%s%c", flags_width_prec, *p);
                int n = snprintf(num_buf, sizeof(num_buf), spec_buf, v);
                size_t space = buf_size - 1 - *buf_pos;
                size_t to_copy = (size_t)n < space ? (size_t)n : space;
                memcpy(buf + *buf_pos, num_buf, to_copy);
                *buf_pos += to_copy;
                break;
            }

            case 'p': {
                void* v = va_arg(args, void*);
                int n = snprintf(num_buf, sizeof(num_buf), "%p", v);
                size_t space = buf_size - 1 - *buf_pos;
                size_t to_copy = (size_t)n < space ? (size_t)n : space;
                memcpy(buf + *buf_pos, num_buf, to_copy);
                *buf_pos += to_copy;
                break;
            }

            case 'c': {
                int v = va_arg(args, int);
                if (*buf_pos < buf_size - 1) buf[(*buf_pos)++] = (char)v;
                break;
            }

            case '%': {
                if (*buf_pos < buf_size - 1) buf[(*buf_pos)++] = '%';
                break;
            }

            default: {
                if (*buf_pos < buf_size - 1) buf[(*buf_pos)++] = '%';
                if (*buf_pos < buf_size - 1) buf[(*buf_pos)++] = *p;
                break;
            }
        }
    }

    buf[*buf_pos < buf_size ? *buf_pos : buf_size - 1] = '\0';
}