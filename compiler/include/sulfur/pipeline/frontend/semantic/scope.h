#pragma once

#include <stddef.h>

#include "sulfur/pipeline/frontend/semantic/symbol.h"

typedef struct {
    sf_var_symbol_table *stack;
    size_t depth;
    size_t capacity;
    size_t next_id;
    size_t failed_pushes;
} sf_scope;

void scope_init(sf_scope *scope);
void scope_push(sf_scope *scope);
void scope_pop(sf_scope *scope);
sf_var_symbol *scope_lookup(sf_scope *scope, const char *name);
void scope_insert(sf_scope *scope, sf_var_symbol symbol, const char *filename);
void scope_free(sf_scope *scope);

const char *scope_find_closest(sf_scope *scope, const char *name);
