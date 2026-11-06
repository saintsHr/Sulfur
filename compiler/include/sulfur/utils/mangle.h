#pragma once

#include <stdbool.h>
#include "sulfur/utils/arena.h"

const char *sf_mangle_module_prefix(sf_arena *arena, const char *filename);

const char *sf_mangle_function(
    sf_arena *arena, const char *prefix, const char *name, bool is_extern
);

const char *sf_mangle_variable(sf_arena *arena, const char *name, size_t id);
