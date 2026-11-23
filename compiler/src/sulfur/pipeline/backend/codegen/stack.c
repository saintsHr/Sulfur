#include "sulfur/pipeline/backend/codegen/stack.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/io/log.h"
#include "sulfur/shared/string.h"
#include "sulfur/shared/type.h"

static const sf_stack_map_size k_initial_capacity = 8;

enum {
    k_temporary_name_size = 16
};

static bool ensure_capacity(sf_stack_map *map);
static void add_entry(
    sf_stack_map *map, const char *name, const sf_type *type,
    sf_stack_offset_size *next_offset
);
static sf_stack_offset_size next_aligned_offset(
    sf_stack_offset_size current, uint8_t size
);

sf_stack_offset_size sf_stack_lookup(const sf_stack_map *map, const char *name) {
    const sf_stack_entry *entry = sf_stack_lookup_entry(map, name);

    return entry != NULL ? entry->offset : 0;
}

sf_stack_entry *sf_stack_lookup_entry(const sf_stack_map *map, const char *name) {
    if (map == NULL || name == NULL) {
        return NULL;
    }

    for (sf_stack_map_size i = 0; i < map->entry_count; i++) {
        if (strcmp(map->entries[i].name, name) == 0) {
            return &map->entries[i];
        }
    }

    return NULL;
}

void sf_stack_push(sf_stack_map *map, sf_stack_entry entry) {
    if (map == NULL) {
        return;
    }

    if (!ensure_capacity(map)) {
        return;
    }

    map->entries[map->entry_count++] = entry;
}

void sf_stack_register_operand(
    sf_stack_map *map, sf_ir_operand op, sf_stack_offset_size *next_offset
) {
    switch (op.type) {
        case SF_IR_OPERAND_TYPE_VARIABLE: {
            add_entry(map, op.variable_name, op.value_type, next_offset);
            break;
        }
        case SF_IR_OPERAND_TYPE_TEMPORARY: {
            char name[k_temporary_name_size];
            snprintf(name, sizeof(name), "t%zu", op.temporary_id);

            add_entry(map, name, op.value_type, next_offset);
            break;
        }

        default: {
            break;
        }
    }
}

void sf_stack_populate(
    sf_stack_map *map, const sf_parameter *params, size_t param_count,
    const sf_ir_operation *operations, size_t operation_count
) {
    if (map == NULL) {
        return;
    }

    sf_stack_offset_size next_offset = 0;

    for (size_t i = 0; i < param_count; i++) {
        sf_stack_register_operand(
            map,
            (sf_ir_operand){
                .type = SF_IR_OPERAND_TYPE_VARIABLE,
                .variable_name = params[i].name,
                .value_type = params[i].type,
            },
            &next_offset
        );
    }

    if (operations == NULL) {
        return;
    }

    for (size_t i = 0; i < operation_count; i++) {
        const sf_ir_operation *op = &operations[i];

        sf_stack_register_operand(map, op->operand1, &next_offset);
        sf_stack_register_operand(map, op->operand2, &next_offset);

        if (op->opcode != SF_IR_OPCODE_ASSIGN) {
            sf_stack_register_operand(map, op->operand3, &next_offset);
        }
    }
}

void sf_stack_free(sf_stack_map *map) {
    if (map == NULL) {
        return;
    }

    for (sf_stack_map_size i = 0; i < map->entry_count; i++) {
        free(map->entries[i].name);
    }

    free(map->entries);

    map->entries = NULL;
    map->entry_count = 0;
    map->entry_capacity = 0;
}

static bool ensure_capacity(sf_stack_map *map) {
    if (map->entry_count < map->entry_capacity) {
        return true;
    }

    sf_stack_map_size new_capacity =
        map->entry_capacity == 0 ? k_initial_capacity : map->entry_capacity * 2;

    sf_stack_entry *grown = realloc(map->entries, new_capacity * sizeof(sf_stack_entry));

    if (grown == NULL) {
        sf_log_out_of_memory("N/A");

        return false;
    }

    map->entries = grown;
    map->entry_capacity = new_capacity;

    return true;
}

static void add_entry(
    sf_stack_map *map, const char *name, const sf_type *type,
    sf_stack_offset_size *next_offset
) {
    if (map == NULL || name == NULL) {
        return;
    }

    uint8_t size = sf_type_kind_width_bytes(type->kind);

    if (size == 0) {
        return;
    }

    if (sf_stack_lookup_entry(map, name) != NULL) {
        return;
    }

    if (!ensure_capacity(map)) {
        return;
    }

    char *owned_name = sf_string_dup(name);

    if (owned_name == NULL) {
        sf_log_out_of_memory("N/A");

        return;
    }

    *next_offset = next_aligned_offset(*next_offset, size);

    sf_stack_push(
        map, (sf_stack_entry){
                 .name = owned_name,
                 .offset = *next_offset,
                 .type = type,
             }
    );
}

static sf_stack_offset_size next_aligned_offset(
    sf_stack_offset_size current, uint8_t size
) {
    uint64_t position = (uint64_t)(-current) + size;
    position = (position + size - 1) / size * size;

    return -(sf_stack_offset_size)position;
}
