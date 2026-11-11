#include "sulfur/utils/log.h"
#include "sulfur/utils/string.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SF_LOG_COLOR_RED "\x1b[31m"
#define SF_LOG_COLOR_BRIGHT_BLACK "\x1b[90m"
#define SF_LOG_COLOR_BRIGHT_RED "\x1b[91m"
#define SF_LOG_COLOR_BRIGHT_GREEN "\x1b[92m"
#define SF_LOG_COLOR_BRIGHT_BLUE "\x1b[94m"
#define SF_LOG_COLOR_BRIGHT_MAGENTA "\x1b[95m"
#define SF_LOG_COLOR_BRIGHT_CYAN "\x1b[96m"
#define SF_LOG_COLOR_RESET "\x1b[0m"

#define SF_LOG_BUFFER_SIZE (1024 * 4)

static const char *g_source_filename = NULL;
static const char *g_source_content = NULL;

static char *g_cache_filename = NULL;
static char *g_cache_content = NULL;

static size_t g_error_count = 0;

static void emit_log(
    sf_log_info info,
    va_list *args
);

static const char *severity_label(sf_log_severity sev);
static void format_message(
    char *buf,
    size_t size,
    const char *fmt,
    va_list *args
);
static const char *find_line(
    const char *content,
    size_t target_line,
    size_t *out_len
);

static void print_source_snippet(
    sf_span span,
    const char *content,
    const char *msg
);
static void print_highlighted_line(
    const char *line_num,
    const char *line,
    size_t line_len,
    size_t col_start,
    size_t col_end
);
static void print_underline(
    int gutter,
    const char *line,
    size_t line_len,
    size_t col,
    size_t carets,
    const char *msg
);

static const char *load_source(const char *file);

void sf_log_set_source(
    const char *filename,
    const char *content
) {
    g_source_filename = filename;
    g_source_content = content;
}

void sf_log(
    const char *title,
    const char *desc,
    const char *hint,
    const char *file,
    sf_log_error_code_num code,
    sf_span span,
    sf_log_severity sev,
    ...
) {
    sf_log_info info = {
        .title = title,
        .desc = desc,
        .hint = hint,
        .file = file,
        .code = code,
        .span = span,
        .sev = sev,
    };

    va_list args;
    va_start(args, sev);
    emit_log(info, &args);
    va_end(args);
}

void sf_log_init(void) { g_error_count = 0; }

void sf_log_shutdown(void) {
    free(g_cache_filename);
    free(g_cache_content);
    g_cache_filename = NULL;
    g_cache_content = NULL;
}

bool sf_log_had_errors(void) { return g_error_count > 0; }

static void emit_log(
    sf_log_info info,
    va_list *args
) {
    char title[SF_LOG_BUFFER_SIZE];
    char desc[SF_LOG_BUFFER_SIZE];
    char hint[SF_LOG_BUFFER_SIZE];

    format_message(title, sizeof(title), info.title, args);
    format_message(desc, sizeof(desc), info.desc, args);
    format_message(hint, sizeof(hint), info.hint, args);

    const char *file = info.file ? info.file : g_source_filename;
    if (file == NULL) {
        file = "<unknown>";
    }

    printf(
        "%s " SF_LOG_COLOR_BRIGHT_BLACK "[0x%04x]" SF_LOG_COLOR_RESET ": %s\n",
        severity_label(info.sev),
        (unsigned int)info.code,
        title
    );

    if (info.span.line > 0) {
        printf(
            SF_LOG_COLOR_BRIGHT_BLUE "  --> from: " SF_LOG_COLOR_RESET
                                     "%s:%zu:%zu\n",
            file,
            info.span.line,
            info.span.col
        );

        print_source_snippet(
            info.span, load_source(info.file), info.desc ? desc : NULL
        );
    } else if (info.desc) {
        printf("  %s\n", desc);
    }

    if (info.hint) {
        printf(
            SF_LOG_COLOR_BRIGHT_GREEN "  --> hint: " SF_LOG_COLOR_RESET "%s\n",
            hint
        );
    }

    printf("\n");

    if (info.sev == SF_LOG_SEVERITY_ERROR ||
        info.sev == SF_LOG_SEVERITY_FATAL) {
        g_error_count++;
    }

    if (info.sev == SF_LOG_SEVERITY_FATAL) {
        exit(EXIT_FAILURE);
    }
}

static const char *severity_label(sf_log_severity sev) {
    switch (sev) {
    case SF_LOG_SEVERITY_INFO: {
        return SF_LOG_COLOR_BRIGHT_CYAN "info" SF_LOG_COLOR_RESET;
    }
    case SF_LOG_SEVERITY_WARNING: {
        return SF_LOG_COLOR_BRIGHT_MAGENTA "warning" SF_LOG_COLOR_RESET;
    }
    case SF_LOG_SEVERITY_ERROR: {
        return SF_LOG_COLOR_BRIGHT_RED "error" SF_LOG_COLOR_RESET;
    }
    case SF_LOG_SEVERITY_FATAL: {
        return SF_LOG_COLOR_RED "fatal" SF_LOG_COLOR_RESET;
    }

    default: {
        return "";
    }
    }
}

__attribute__((format(
    printf,
    3,
    0
))) static void
format_message(
    char *buf,
    size_t size,
    const char *fmt,
    va_list *args
) {
    if (fmt == NULL) {
        buf[0] = '\0';
        return;
    }

    vsnprintf(buf, size, fmt, *args);
}

static const char *find_line(
    const char *content,
    size_t target_line,
    size_t *out_len
) {
    if (content == NULL) {
        return NULL;
    }

    const char *p = content;

    for (size_t line = 1; line < target_line; line++) {
        const char *newline = strchr(p, '\n');

        if (newline == NULL) {
            return NULL;
        }

        p = newline + 1;
    }

    const char *newline = strchr(p, '\n');
    *out_len = newline ? (size_t)(newline - p) : strlen(p);

    return p;
}

static void print_source_snippet(
    sf_span span,
    const char *content,
    const char *msg
) {
    if (span.line == 0 || content == NULL) {
        return;
    }

    size_t line_len = 0;
    const char *line = find_line(content, span.line, &line_len);
    if (line == NULL) {
        return;
    }

    char line_num[sizeof(size_t) * 3 + 1];
    snprintf(line_num, sizeof(line_num), "%zu", span.line);
    int gutter = (int)strlen(line_num);

    size_t col_start = span.col > 0 ? span.col - 1 : 0;
    size_t carets = span.len > 0 ? span.len : 1;
    size_t col_end = col_start + carets;

    if (col_start > line_len) {
        col_start = line_len;
    }
    if (col_end > line_len) {
        col_end = line_len;
    }

    printf(
        SF_LOG_COLOR_BRIGHT_BLUE "%*s| \n" SF_LOG_COLOR_RESET, gutter + 1, ""
    );
    print_highlighted_line(line_num, line, line_len, col_start, col_end);
    print_underline(gutter, line, line_len, span.col, carets, msg);
    printf(SF_LOG_COLOR_BRIGHT_BLUE "%*s |\n" SF_LOG_COLOR_RESET, gutter, "");
}

static void print_highlighted_line(
    const char *line_num,
    const char *line,
    size_t line_len,
    size_t col_start,
    size_t col_end
) {
    printf(SF_LOG_COLOR_BRIGHT_BLUE "%s | " SF_LOG_COLOR_RESET, line_num);

    printf("%.*s", (int)col_start, line);

    if (col_end > col_start) {
        printf(
            SF_LOG_COLOR_BRIGHT_RED "%.*s" SF_LOG_COLOR_RESET,
            (int)(col_end - col_start),
            line + col_start
        );
    }

    if (col_end < line_len) {
        printf("%.*s", (int)(line_len - col_end), line + col_end);
    }

    printf("\n");
}

static void print_underline(
    int gutter,
    const char *line,
    size_t line_len,
    size_t col,
    size_t carets,
    const char *msg
) {
    printf(SF_LOG_COLOR_BRIGHT_BLUE "%*s| " SF_LOG_COLOR_RESET, gutter + 1, "");

    for (size_t i = 1; i < col; i++) {
        putchar((i - 1 < line_len && line[i - 1] == '\t') ? '\t' : ' ');
    }

    printf(SF_LOG_COLOR_BRIGHT_RED);
    for (size_t i = 0; i < carets; i++) {
        putchar('^');
    }
    printf(SF_LOG_COLOR_RESET);

    if (msg && msg[0] != '\0') {
        printf(" %s", msg);
    }

    printf("\n");
}

static const char *load_source(const char *file) {
    if (file == NULL) {
        return g_source_content;
    }

    if (g_source_filename != NULL && strcmp(file, g_source_filename) == 0) {
        return g_source_content;
    }

    if (g_cache_filename != NULL && strcmp(file, g_cache_filename) == 0) {
        return g_cache_content;
    }

    FILE *f = fopen(file, "rb");
    if (f == NULL) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *content = size >= 0 ? malloc((size_t)size + 1) : NULL;
    if (content == NULL) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(content, 1, (size_t)size, f);
    content[read] = '\0';
    fclose(f);

    free(g_cache_filename);
    free(g_cache_content);
    g_cache_filename = sf_string_dup(file);
    g_cache_content = content;

    return g_cache_content;
}
