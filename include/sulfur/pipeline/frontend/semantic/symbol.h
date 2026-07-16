#pragma once

#include "sulfur/pipeline/frontend/ast.h"
#include <stdbool.h>

typedef struct {
    char* name;
    sf_value_type type;
    bool initialized;
    uint32_t depth;
    uint32_t id;
    sf_span span;
} sf_symbol;

typedef struct {
    sf_symbol* symbols;
    uint32_t count;
    uint32_t capacity;
} sf_symbol_table;

void symbol_table_init(sf_symbol_table* table);
sf_symbol* symbol_table_lookup(sf_symbol_table* table, const char* name);
void symbol_table_free(sf_symbol_table* table);
void symbol_table_insert(sf_symbol_table* table, sf_symbol symbol, const char* filename);