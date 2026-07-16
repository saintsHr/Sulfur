#include "sulfur/pipeline/frontend/preprocessor.h"

#include <stdlib.h>
#include <string.h>

#include "sulfur/utils/log.h"

static void remove_carriage_return(char* str);
static void remove_comments(char* str);

char* sf_preprocess(const char* src, long src_size, const char* filename) {
    char* out = malloc(src_size + 1);
    if (!out) {
        sf_log(
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

    return out;
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
    char* read = str;
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
