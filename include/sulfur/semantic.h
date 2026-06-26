#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/ast.h"

typedef struct {
    char* name;
    sf_value_type type;
    bool initialized;
    uint32_t depth;
    uint32_t id;
} sf_symbol;

typedef struct {
    sf_symbol* symbols;
    uint32_t count;
    uint32_t capacity;
} sf_symbol_table;

typedef struct {
    sf_symbol_table* stack;
    uint32_t depth;
    uint32_t capacity;
    uint32_t next_id;
} sf_scope;

void sf_analyze(sf_program_node* program, const char* filename);
