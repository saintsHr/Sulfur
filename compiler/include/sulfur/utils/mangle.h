#pragma once

#include "sulfur/utils/arena.h"
#include <stdbool.h>

const char *sf_mangle_function(
    sf_arena *arena,
    const char *name,
    bool keep_name
);

const char *sf_mangle_variable(
    sf_arena *arena,
    const char *name,
    size_t id
);
