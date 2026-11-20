#pragma once

#include "sulfur/shared/span.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct sf_path_segment {
    sf_span span;
    const char *name;
} sf_path_segment;

typedef struct sf_path {
    sf_path_segment *segments;
    size_t segment_count;
    size_t segment_capacity;
    bool is_global;
} sf_path;
