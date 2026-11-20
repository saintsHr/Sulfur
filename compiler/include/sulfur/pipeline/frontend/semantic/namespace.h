#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sulfur/shared/path.h"

#define SF_NS_DEPTH_MAX 32
#define SF_NS_QNAME_MAX 512

typedef struct {
    const char *segments[SF_NS_DEPTH_MAX];
    size_t depth;
} sf_namespace_stack;

bool sf_namespace_push_path(sf_namespace_stack *ns, const sf_path *path);
bool sf_namespace_format(
    char *buf, size_t cap, const sf_namespace_stack *ns, size_t prefix_depth,
    const sf_path_segment *segs, size_t seg_count
);
