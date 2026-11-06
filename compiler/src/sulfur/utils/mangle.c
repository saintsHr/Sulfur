#include "sulfur/utils/mangle.h"

#include <stdio.h>
#include <string.h>

#include "sulfur/utils/log.h"

static const char k_empty_name[] = "";

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
    if (prefix == NULL || keep_name) {
        return name;
    }

    size_t name_len = strlen(name);

    int needed = snprintf(NULL, 0, "%s%zu%s", prefix, name_len, name);

    if (needed < 0) {
        return name;
    }

    size_t length = (size_t)needed + 1;
    char *result = sf_arena_alloc(arena, length);

    if (result == NULL) {
        sf_log(
            "insufficient memory", "cannot allocate memory for compiling",
            "free some memory and try again", NULL,
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );
        return name;
    }

    snprintf(result, length, "%s%zu%s", prefix, name_len, name);
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
