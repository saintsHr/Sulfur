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
    sf_arena *arena, const char *name, bool keep_name
) {
    if (keep_name) {
        return name;
    }

    if (strcmp(name, "main") == 0) {
        return name;
    }

    const char *head = "_SF_";

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

static size_t count_segments(const char *name) {
    size_t count = 1;

    for (const char *p = strstr(name, "::"); p != NULL; p = strstr(p + 2, "::")) {
        count++;
    }

    return count;
}
