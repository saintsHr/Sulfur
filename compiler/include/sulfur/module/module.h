#pragma once

#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"

typedef enum sf_module_state {
    SF_MODULE_STATE_UNLOADED,
    SF_MODULE_STATE_LOADING,
    SF_MODULE_STATE_LOADED,
    SF_MODULE_STATE_FAILED,
} sf_module_state;

typedef enum sf_module_analysis {
    SF_MODULE_ANALYSIS_PENDING,
    SF_MODULE_ANALYSIS_IN_PROGRESS,
    SF_MODULE_ANALYSIS_DONE,
    SF_MODULE_ANALYSIS_FAILED,
} sf_module_analysis;

typedef struct sf_module {
    sf_module_state state;
    sf_ast_node_program *ast;
    const char *filename;
    const char *name;
    sf_module_analysis analysis;
    sf_func_symbol_table funcs;
} sf_module;

typedef struct sf_module_list {
    sf_module *modules;
    size_t module_count;
    size_t module_capacity;
} sf_module_list;

void sf_module_resolve(
    sf_ast_node_import **imports, size_t import_count, const char *importer_filename,
    sf_module_list *modules, const char **module_paths, size_t module_path_count,
    sf_arena *arena
);

const char *sf_module_name_from_path(
    const char *file, const char **module_paths, size_t module_path_count
);
