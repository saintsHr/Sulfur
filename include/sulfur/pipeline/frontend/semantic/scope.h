#pragma once

#include "sulfur/pipeline/frontend/semantic/symbol.h"

typedef struct {
    sf_symbol_table* stack;
    uint32_t depth;
    uint32_t capacity;
    uint32_t next_id;
} sf_scope;

void scope_init(sf_scope* scope);
void scope_push(sf_scope* scope);
void scope_pop(sf_scope* scope);
sf_symbol* scope_lookup(sf_scope* scope, const char* name);
void scope_insert(sf_scope* scope, sf_symbol symbol, const char* filename);
void scope_free(sf_scope* scope);
const char* scope_find_closest(sf_scope* scope, const char* name);