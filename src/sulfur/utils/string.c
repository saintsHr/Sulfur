#include "sulfur/utils/string.h"

#include <stdlib.h>
#include <string.h>

char* sf_strdup(const char* str) {
    size_t len = strlen(str) + 1;

    char* copy = malloc(len);
    if (!copy) return NULL;

    memcpy(copy, str, len);

    return copy;
}

char* sf_strdup_arena(sf_arena* arena, const char* s) {
    size_t len = strlen(s) + 1;

    char* copy = sf_arena_alloc(arena, len);
    if (!copy) return NULL;

    memcpy(copy, s, len);

    return copy;
}

void sf_strpush(
    const char* src, char** dst, uint64_t* len, uint64_t* capacity
) {
    if (dst == NULL || *dst == NULL || src == NULL) return;

    size_t src_len = strlen(src);

    while (*len + src_len + 1 > *capacity) {
        *capacity *= 2;
        *dst = realloc(*dst, *capacity);
    }

    memcpy(*dst + *len, src, src_len + 1);
    *len += src_len;
}