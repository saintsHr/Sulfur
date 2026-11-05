#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sulfur/module/module.h"
#include "sulfur/utils/arena.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/backend/ir/ir.h"

typedef struct {
    const char *file_input;
    const char *file_output;

    bool dump_stages;
    bool no_entry;

    const char **module_paths;
    size_t module_path_count;
    size_t module_path_capacity;
} sf_compiler_options;

typedef struct {
    char *input_source;
    size_t input_size;

    sf_arena arena;

    char *preprocessed;
    sf_token_list tokens;
    sf_program_node *ast;
    sf_ir_program ir;
    char *assembly;

    sf_module_list modules;
} sf_compilation;
