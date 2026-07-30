#pragma once

#include <stdint.h>

#include "sulfur/pipeline/backend/ir.h"
#include "sulfur/pipeline/frontend/ast.h"

typedef int64_t sf_stack_offset_size_t;
typedef uint32_t sf_stack_map_size_t;

typedef struct {
    char* name;
    sf_stack_offset_size_t offset;
    sf_value_type type;
} sf_stack_entry;

typedef struct {
    sf_stack_entry* entries;
    sf_stack_map_size_t count;
    sf_stack_map_size_t capacity;
} sf_stack_map;

sf_stack_offset_size_t sf_stack_lookup(
    const sf_stack_map* map, const char* name
);
sf_stack_entry* sf_stack_lookup_entry(
    const sf_stack_map* map, const char* name
);
void sf_stack_register_operand(
    sf_stack_map* map, sf_operand op, sf_stack_offset_size_t* next_offset
);
void sf_stack_push(sf_stack_map* map, sf_stack_entry entry);
void sf_stack_populate(sf_stack_map* map, const sf_ir_program* program);
void sf_stack_free(sf_stack_map* map);