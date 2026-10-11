#include "sulfur/utils/string.h"

#include <stdlib.h>
#include <string.h>

#define SF_STRING_MIN_CAPACITY ((size_t)16)

char *sf_string_dup(const char *str) {
    size_t size = strlen(str) + 1;

    char *copy = malloc(size);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, str, size);

    return copy;
}

char *sf_string_dup_arena(sf_arena *arena, const char *str) {
    size_t size = strlen(str) + 1;

    char *copy = sf_arena_alloc(arena, size);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, str, size);

    return copy;
}

bool sf_string_push(const char *src, char **dst, size_t *len, size_t *capacity) {
    if (src == NULL || dst == NULL || *dst == NULL || len == NULL ||
        capacity == NULL) {
        return false;
    }

    size_t src_len = strlen(src);

    if (src_len > SIZE_MAX - 1 || *len > SIZE_MAX - 1 - src_len) {
        return false;
    }

    size_t needed = *len + src_len + 1;

    if (needed > *capacity) {
        size_t new_capacity = *capacity > 0 ? *capacity : SF_STRING_MIN_CAPACITY;

        while (new_capacity < needed) {
            if (new_capacity > SIZE_MAX / 2) {
                new_capacity = needed;
                break;
            }

            new_capacity *= 2;
        }

        char *grown = realloc(*dst, new_capacity);
        if (grown == NULL) {
            return false;
        }

        *dst = grown;
        *capacity = new_capacity;
    }

    memcpy(*dst + *len, src, src_len + 1);
    *len += src_len;

    return true;
}
