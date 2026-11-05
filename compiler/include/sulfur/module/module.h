#pragma once

#include "sulfur/pipeline/frontend/ast.h"

typedef enum {
    SF_MODULE_STATE_UNLOADED,
    SF_MODULE_STATE_LOADING,
    SF_MODULE_STATE_LOADED,
    SF_MODULE_STATE_FAILED,
} sf_module_state;

typedef struct {
    sf_module_state state;
    sf_program_node *ast;
    char *filename;
} sf_module;

typedef struct {
    sf_module *modules;
    size_t module_count;
    size_t module_capacity;
} sf_module_list;

void sf_module_resolve(
    sf_import_node **imports,
    size_t import_count,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
);
