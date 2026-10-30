#include "sulfur/utils/string.h"

#include <stdlib.h>
#include <string.h>

#define SF_STRING_MIN_CAPACITY ((size_t)16)
#define SF_STRING_DISTANCE_MAX 256

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

size_t sf_string_distance(const char *a, const char *b) {
    size_t a_len = strlen(a);
    size_t b_len = strlen(b);

    if (b_len >= SF_STRING_DISTANCE_MAX) {
        return UINT32_MAX;
    }

    size_t previous[SF_STRING_DISTANCE_MAX];
    size_t current[SF_STRING_DISTANCE_MAX];

    for (size_t j = 0; j <= b_len; j++) {
        previous[j] = j;
    }

    for (size_t i = 1; i <= a_len; i++) {
        current[0] = i;

        for (size_t j = 1; j <= b_len; j++) {
            size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            size_t best = previous[j - 1] + cost;

            if (previous[j] + 1 < best) {
                best = previous[j] + 1;
            }

            if (current[j - 1] + 1 < best) {
                best = current[j - 1] + 1;
            }

            current[j] = best;
        }

        memcpy(previous, current, (b_len + 1) * sizeof(size_t));
    }

    return previous[b_len];
}
