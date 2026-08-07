#include "sulfur/pipeline/backend/codegen/stack.h"

#include <stdio.h>
#include <string.h>

#include "sulfur/utils/string.h"
#include "sulfur/utils/type_utils.h"

static sf_stack_offset_size_t
next_aligned_offset(sf_stack_offset_size_t current, uint8_t size);

sf_stack_offset_size_t sf_stack_lookup(const sf_stack_map *map,
                                       const char *name) {
  if (map == NULL)
    return 0;
  if (name == NULL)
    return 0;

  for (sf_stack_offset_size_t i = 0; i < map->count; i++) {
    if (strcmp(map->entries[i].name, name) == 0) {
      return map->entries[i].offset;
    }
  }

  return 0;
}

sf_stack_entry *sf_stack_lookup_entry(const sf_stack_map *map,
                                      const char *name) {
  if (map == NULL)
    return NULL;
  if (name == NULL)
    return NULL;

  for (int64_t i = 0; i < map->count; i++) {
    if (strcmp(map->entries[i].name, name) == 0) {
      return &map->entries[i];
    }
  }

  return NULL;
}

void sf_stack_push(sf_stack_map *map, sf_stack_entry entry) {
  if (map == NULL)
    return;

  if (map->capacity <= 0) {
    map->entries = realloc(map->entries, 8 * sizeof(sf_stack_entry));
    map->capacity = 8;
  }

  if (map->count >= map->capacity) {
    map->entries =
        realloc(map->entries, (map->capacity * 2) * sizeof(sf_stack_entry));
    map->capacity *= 2;
  }

  map->entries[map->count++] = entry;
}

void sf_stack_register_operand(sf_stack_map *map, sf_operand op,
                               sf_stack_offset_size_t *next_offset) {
  if (op.type == SF_OPERAND_TYPE_IMMEDIATE)
    return;

  if (op.type == SF_OPERAND_TYPE_VARIABLE) {
    char *name = op.variable_name;
    int64_t offset = sf_stack_lookup(map, name);

    if (offset == 0) {
      *next_offset = next_aligned_offset(*next_offset,
                                         type_value_width_bytes(op.value_type));

      sf_stack_entry entry = {
          .name = sf_strdup(name),
          .offset = *next_offset,
          .type = op.value_type,
      };

      sf_stack_push(map, entry);
    }
  }

  if (op.type == SF_OPERAND_TYPE_TEMPORARY) {
    char *name = malloc(16 * sizeof(char));
    sprintf(name, "t%u", op.temporary_id);

    int64_t offset = sf_stack_lookup(map, name);

    if (offset == 0) {
      *next_offset = next_aligned_offset(*next_offset,
                                         type_value_width_bytes(op.value_type));

      sf_stack_entry entry = {
          .name = name,
          .offset = *next_offset,
          .type = op.value_type,
      };

      sf_stack_push(map, entry);
    } else {
      free(name);
    }
  }
}

void sf_stack_populate(sf_stack_map *map, const sf_ir_program *program) {
  if (map == NULL || program == NULL)
    return;

  sf_stack_offset_size_t next_offset = 0;

  for (uint64_t i = 0; i < program->count; i++) {
    sf_operation op = program->operations[i];

    if (op.operand1.type == SF_OPERAND_TYPE_VARIABLE ||
        op.operand1.type == SF_OPERAND_TYPE_TEMPORARY) {
      sf_stack_register_operand(map, op.operand1, &next_offset);
    }

    if (op.operand2.type == SF_OPERAND_TYPE_VARIABLE ||
        op.operand2.type == SF_OPERAND_TYPE_TEMPORARY) {
      sf_stack_register_operand(map, op.operand2, &next_offset);
    }

    if (op.opcode != SF_OPCODE_ASSIGN) {
      if (op.operand3.type == SF_OPERAND_TYPE_VARIABLE ||
          op.operand3.type == SF_OPERAND_TYPE_TEMPORARY) {
        sf_stack_register_operand(map, op.operand3, &next_offset);
      }
    }
  }
}

void sf_stack_free(sf_stack_map *map) {
  if (map == NULL)
    return;
  for (sf_stack_map_size_t i = 0; i < map->count; i++) {
    free(map->entries[i].name);
  }
  free(map->entries);
}

static sf_stack_offset_size_t
next_aligned_offset(sf_stack_offset_size_t current, uint8_t size) {
  if (size == 0)
    return 0;

  uint64_t pos = (uint64_t)(-current) + size;
  pos = (pos + size - 1) / size * size;
  return -(sf_stack_offset_size_t)pos;
}