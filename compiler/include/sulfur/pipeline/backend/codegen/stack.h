#pragma once

#include <stdint.h>

#include "sulfur/pipeline/midend/ir/ir.h"

typedef int64_t sf_stack_offset_size;
typedef uint64_t sf_stack_map_size;

typedef struct {
    char *name;
    sf_stack_offset_size offset;
    const sf_type *type;
} sf_stack_entry;

typedef struct {
    sf_stack_entry *entries;
    sf_stack_map_size entry_count;
    sf_stack_map_size entry_capacity;
} sf_stack_map;

sf_stack_offset_size sf_stack_lookup(const sf_stack_map *map, const char *name);
sf_stack_entry *sf_stack_lookup_entry(const sf_stack_map *map, const char *name);
void sf_stack_register_operand(
    sf_stack_map *map, sf_ir_operand op, sf_stack_offset_size *next_offset
);
void sf_stack_push(sf_stack_map *map, sf_stack_entry entry);
void sf_stack_populate(
    sf_stack_map *map, const sf_parameter *params, size_t param_count,
    const sf_ir_operation *operations, size_t operation_count
);
void sf_stack_free(sf_stack_map *map);
