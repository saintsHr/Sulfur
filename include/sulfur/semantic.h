#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/ast.h"

typedef struct {
    char* name;
    sf_value_type type;
    bool initialized;
} sf_symbol;

typedef struct {
    sf_symbol* symbols;
    uint32_t count;
    uint32_t capacity;
} sf_symbol_table;

void sf_analyze(sf_program_node* program, const char* filename);
