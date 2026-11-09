#include "sulfur/utils/mangle.h"

#include <stdio.h>
#include <string.h>

#include "sulfur/utils/log.h"

static const char k_empty_name[] = "";

static size_t count_segments(const char *name);

const char *sf_mangle_variable(sf_arena *arena, const char *name, size_t id) {
    int needed = snprintf(NULL, 0, "%s@%zu", name, id);

    if (needed < 0) {
        return k_empty_name;
    }

    size_t length = (size_t)needed + 1;
    char *mangled = sf_arena_alloc(arena, length);

    if (mangled == NULL) {
        sf_log(
            "insufficient memory", "cannot allocate memory for compiling",
            "free some memory and try again", NULL,
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );
        return k_empty_name;
    }

    snprintf(mangled, length, "%s@%zu", name, id);

    return mangled;
}

const char *sf_mangle_function(
    sf_arena *arena, const char *prefix, const char *name, bool keep_name
) {
    if (keep_name) {
        return name;
    }

    bool qualified = strstr(name, "::") != NULL;

    if (prefix == NULL && !qualified) {
        return name;
    }

    const char *head = prefix != NULL ? prefix : "_SF_";

    size_t capacity = strlen(head) + strlen(name) + count_segments(name) * 21 + 1;
    char *result = sf_arena_alloc(arena, capacity);

    if (result == NULL) {
        sf_log(
            "insufficient memory", "cannot allocate memory for compiling",
            "free some memory and try again", NULL,
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );
        return name;
    }

    size_t n = strlen(head);
    memcpy(result, head, n);

    const char *segment = name;

    for (;;) {
        const char *sep = strstr(segment, "::");
        size_t length = sep != NULL ? (size_t)(sep - segment) : strlen(segment);

        n += (size_t)sprintf(result + n, "%zu", length);
        memcpy(result + n, segment, length);
        n += length;

        if (sep == NULL) {
            break;
        }

        segment = sep + 2;
    }

    result[n] = '\0';
    return result;
}

const char *sf_mangle_module_prefix(sf_arena *arena, const char *module_path) {
    size_t len = strlen(module_path);

    const char *dot = strrchr(module_path, '.');
    const char *slash = strrchr(module_path, '/');

    size_t end =
        (dot != NULL && (slash == NULL || dot > slash))
        ? (size_t)(dot - module_path) : len;

    char *out = sf_arena_alloc(arena, 3 + end * 2 + 1);

    if (out == NULL) {
        sf_log(
            "insufficient memory", "cannot allocate memory for compiling",
            "free some memory and try again", NULL,
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );
        return NULL;
    }

    size_t n = 0;
    out[n++] = '_';
    out[n++] = 'S';
    out[n++] = 'F';

    size_t start = 0;

    for (size_t i = 0; i <= end; i++) {
        if (i != end && module_path[i] != '/') continue;

        size_t part = i - start;

        if (part > 0) {
            n += (size_t)sprintf(out + n, "%zu", part);
            memcpy(out + n, module_path + start, part);
            n += part;
        }

        start = i + 1;
    }

    out[n] = '\0';
    return out;
}

static size_t count_segments(const char *name) {
    size_t count = 1;

    for (const char *p = strstr(name, "::"); p != NULL; p = strstr(p + 2, "::")) {
        count++;
    }

    return count;
}
