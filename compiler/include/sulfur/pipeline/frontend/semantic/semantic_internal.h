#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/driver/main.h"
#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/semantic/namespace.h"
#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/shared/arena.h"
#include "sulfur/shared/span.h"

#define SF_SEMANTIC_ENTRY_NAME "main"

typedef struct {
    sf_scope *scope;
    sf_namespace_stack ns;
    sf_symbol_func_table *funcs;
    sf_ast_node_func_decl *current_func;
    const char *filename;
    sf_arena *arena;
    const sf_compilation_options *options;
} sf_semantic;

const sf_symbol_func *sf_semantic_resolve_func(sf_semantic *s, const sf_path *path);
void sf_semantic_report_undeclared(const sf_semantic *s, const char *name, sf_span span);
const char *sf_semantic_suggest_func(sf_semantic *s, const sf_path *path);
