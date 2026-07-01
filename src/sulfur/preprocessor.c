#include "sulfur/preprocessor.h"
#include "sulfur/util/log.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

static int is_identifier_char(char c);

static char* skip_whitespace(char* p);

static void ensure_capacity(char** buffer, char** write, size_t* buffer_size, size_t needed, const char* filename);

static void parse_define(sf_preprocessor_context* ctx, char* line, const char* filename);
static char* extract_defines(sf_preprocessor_context* ctx, char* str, const char* filename);
static char* apply_defines(sf_preprocessor_context* ctx, char* str, const char* filename);

static void remove_carriage_return(char* str);
static void remove_comments(char* str);

char* sf_preprocess(const char* src, long src_size, const char* filename) {
    sf_preprocessor_context ctx = {0};

    char* out = malloc(src_size + 1);
    if (!out) {
        sf_log_helper(
            "memory allocation failed",
            "failed to allocate memory for preprocessor output",
            "free up memory and try again",
            "N/A",
            SF_PREP_CANNOT_MALLOC_OUTPUT,
            (sf_span){0},
            SF_SEV_FATAL
        );
    }

    memcpy(out, src, src_size);
    out[src_size] = '\0';

    remove_carriage_return(out);
    remove_comments(out);

    char* tmp;

    tmp = extract_defines(&ctx, out, filename);
    free(out);
    out = tmp;

    tmp = apply_defines(&ctx, out, filename);
    free(out);
    out = tmp;

    return out;
}

static int is_identifier_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static char* skip_whitespace(char* p) {
    while (isspace((unsigned char)*p)) p++;
    return p;
}

static void ensure_capacity(
    char** buffer,
    char** write,
    size_t* buffer_size,
    size_t needed,
    const char* filename
) {
    size_t used = *write - *buffer;

    if (used + needed + 1 > *buffer_size) {
        *buffer_size = (*buffer_size * 2) + needed;

        char* tmp = realloc(*buffer, *buffer_size);

        if (!tmp) {
            sf_log_helper(
                "memory allocation failed",
                "failed to grow buffer while processing defines",
                "free up memory and try again",
                filename,
                SF_PREP_CANNOT_MALLOC_DEFINES_BUFFER,
                (sf_span){0},
                SF_SEV_FATAL
            );
            
            free(*buffer);
            return;
        }

        *buffer = tmp;
        *write  = *buffer + used;
    }
}

static void parse_define(sf_preprocessor_context* ctx, char* line, const char* filename) {
    if (ctx->define_count >= SF_MAX_DEFINES) {
        sf_log_helper(
            "too many defines",
            "'%s' exceeds the maximum number of defines allowed",
            "remove unused defines to stay under the limit",
            filename,
            SF_PREP_TOO_MANY_DEFINES,
            (sf_span){0},
            SF_SEV_FATAL,
            filename
        );
        return;
    }

    char name [SF_MAX_DEFINE_NAME_LENGTH]  = {0};
    char value[SF_MAX_DEFINE_VALUE_LENGTH] = {0};

    char* p = line + strlen(DEFINE_KEYWORD);
    p = skip_whitespace(p);

    char* n = name;
    while (*p && !isspace((unsigned char)*p)) {
        if ((size_t)(n - name) < SF_MAX_DEFINE_NAME_LENGTH - 1) *n++ = *p;
        p++;
    }
    *n = '\0';

    p = skip_whitespace(p);

    snprintf(value, sizeof(value), "%s", p);

    sf_define* def = &ctx->defines[ctx->define_count];

    snprintf(def->name, SF_MAX_DEFINE_NAME_LENGTH, "%s", name);
    snprintf(def->value, SF_MAX_DEFINE_VALUE_LENGTH, "%s", value);

    def->name_lenght  = strlen(def->name);
    def->value_lenght = strlen(def->value);

    ctx->define_count++;
}

static char* extract_defines(sf_preprocessor_context* ctx, char* str, const char* filename) {
    size_t buffer_size = strlen(str) + 128;
    char*  buffer      = malloc(buffer_size);
    char*  read        = str;
    char*  write       = buffer;
    char   line[512]   = {0};

    const size_t keywordLen = strlen(DEFINE_KEYWORD);

    while (*read) {
        size_t i = 0;
        while (*read && *read != '\n' && i < sizeof(line) - 1) line[i++] = *read++;
        line[i] = '\0';

        if (*read == '\n') read++;

        char* trim = skip_whitespace(line);

        if (strncmp(trim, DEFINE_KEYWORD, keywordLen) == 0 &&
            isspace((unsigned char)trim[keywordLen])) {
            parse_define(ctx, trim, filename);
            ensure_capacity(&buffer, &write, &buffer_size, 1, filename);
            *write++ = '\n';
        } else {
            size_t len = strlen(line);
            ensure_capacity(&buffer, &write, &buffer_size, len + 1, filename);
            memcpy(write, line, len);
            write += len;
            *write++ = '\n';
        }
    }

    ensure_capacity(&buffer, &write, &buffer_size, 1, filename);
    *write = '\0';
    return buffer;
}

static char* apply_defines(sf_preprocessor_context* ctx, char* str, const char* filename) {
    if (!str) return NULL;

    size_t buffer_size = strlen(str) + 128;
    char* buffer = malloc(buffer_size);

    if (!buffer) {
        sf_log_helper(
            "memory allocation failed",
            "failed to allocate memory for define expansion",
            "free up memory and try again",
            filename,
            SF_PREP_CANNOT_MALLOC_DEFINES_BUFFER,
            (sf_span){0},
            SF_SEV_FATAL
        );
        return NULL;
    }

    char* read  = str;
    char* write = buffer;

    while (*read) {
        int replaced = 0;

        for (int i = 0; i < ctx->define_count; i++) {
            sf_define* def = &ctx->defines[i];

            if (strncmp(read, def->name, def->name_lenght) != 0) continue;

            char before = (read == str) ? ' ' : read[-1];
            char after  = read[def->name_lenght];

            if (!is_identifier_char(before) && !is_identifier_char(after)) {
                ensure_capacity(&buffer, &write, &buffer_size, def->value_lenght, filename);

                memcpy(write, def->value, def->value_lenght);
                write += def->value_lenght;
                read  += def->name_lenght;

                replaced = 1;
                break;
            }
        }

        if (!replaced) {
            ensure_capacity(&buffer, &write, &buffer_size, 1, filename);
            *write++ = *read++;
        }
    }

    *write = '\0';
    return buffer;
}

static void remove_carriage_return(char* str) {
    char* read = str;
    char* write = str;

    while (*read) {
        if (*read != '\r') *write++ = *read;
        read++;
    }

    *write = '\0';
}

static void remove_comments(char* str) {
    char* read  = str;
    char* write = str;

    while (*read) {
        if (*read == '/' && *(read + 1) == '/') {
            while (*read && *read != '\n') read++;
        } else {
            *write++ = *read++;
        }
    }

    *write = '\0';
}
