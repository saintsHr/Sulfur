#pragma once

#include <stdint.h>

#include "sulfur/pipeline/backend/ir.h"

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

typedef enum {
    SF_REGISTER_RAX,
    SF_REGISTER_EAX,
    SF_REGISTER_AX,
    SF_REGISTER_AL,
} sf_register;

typedef enum {
    SF_PREFIX_BYTE,
    SF_PREFIX_WORD,
    SF_PREFIX_DWORD,
    SF_PREFIX_QWORD,
} sf_size_prefix;

char* sf_generate_assembly(const sf_ir_program* program);
