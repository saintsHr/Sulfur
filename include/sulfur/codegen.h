#pragma once

#include "sulfur/ir.h"
#include <stdint.h>

typedef struct {
    char* name;
    int16_t offset;
    sf_value_type type;
} sf_stack_entry;

typedef struct {
    sf_stack_entry* entries;
    uint16_t count;
    uint16_t capacity;
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
