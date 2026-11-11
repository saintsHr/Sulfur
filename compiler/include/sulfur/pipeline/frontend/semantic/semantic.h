#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/driver/main.h"
#include "sulfur/pipeline/frontend/ast.h"

bool sf_analyze(
    sf_arena *arena,
    sf_program_node *program,
    const char *filename,
    const sf_compiler_options *options,
    sf_module_list *modules
);
