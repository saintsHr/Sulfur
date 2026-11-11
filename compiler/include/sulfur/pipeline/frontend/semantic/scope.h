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

typedef struct {
    bool *flags;
    size_t count;
} sf_flow_state;

void scope_init(sf_scope *scope);
void scope_push(sf_scope *scope);
void scope_pop(sf_scope *scope);
sf_var_symbol *scope_lookup(
    sf_scope *scope,
    const char *name
);
void scope_insert(
    sf_scope *scope,
    sf_var_symbol symbol,
    const char *filename
);
void scope_free(sf_scope *scope);

sf_flow_state scope_flow_save(const sf_scope *scope);
void scope_flow_restore(
    sf_scope *scope,
    const sf_flow_state *state
);
void scope_flow_fill(
    sf_flow_state *state,
    bool value
);
void scope_flow_and(
    sf_flow_state *dst,
    const sf_flow_state *other
);
void scope_flow_free(sf_flow_state *state);

const char *scope_find_closest(
    sf_scope *scope,
    const char *name
);
