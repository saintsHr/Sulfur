#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/driver/main.h"
#include "sulfur/pipeline/frontend/ast/ast.h"

bool sf_analyze(
    sf_arena *arena, sf_ast_node_program *program, const char *filename,
    const sf_compilation_options *options, sf_module_list *modules
);
