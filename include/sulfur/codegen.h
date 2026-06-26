#pragma once

#include "sulfur/ir.h"
#include <stdint.h>

typedef struct {
    char* name;
    int16_t offset;
} sf_stack_entry;

typedef struct {
    sf_stack_entry* entries;
    uint16_t count;
    uint16_t capacity;
} sf_stack_map;

char* sf_generate_assembly(const sf_ir_program* program);
