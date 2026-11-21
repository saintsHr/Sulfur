#pragma once

#include <stddef.h>

#include "sulfur/pipeline/frontend/semantic/symbol.h"

typedef struct {
    sf_symbol_var_table *stack;
    size_t depth;
    size_t capacity;
    size_t next_id;
    size_t failed_pushes;
} sf_scope;

typedef struct {
    bool *flags;
    size_t count;
} sf_scope_flow;

void sf_scope_init(sf_scope *scope);
void sf_scope_push(sf_scope *scope);
void sf_scope_pop(sf_scope *scope);
sf_symbol_var *sf_scope_lookup(sf_scope *scope, const char *name);
void sf_scope_insert(sf_scope *scope, sf_symbol_var sym, const char *filename);
void sf_scope_free(sf_scope *scope);

sf_scope_flow sf_scope_flow_save(const sf_scope *scope);
void sf_scope_flow_restore(sf_scope *scope, const sf_scope_flow *flow);
void sf_scope_flow_fill(sf_scope_flow *flow, bool value);
void sf_scope_flow_and(sf_scope_flow *flow_dst, const sf_scope_flow *flow_other);
void sf_scope_flow_free(sf_scope_flow *flow);

const char *sf_scope_find_closest(sf_scope *scope, const char *name);
