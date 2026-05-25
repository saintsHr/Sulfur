#pragma once

#include "sulfur/ir.h"
#include <stdint.h>

typedef struct {
    char*    name;
    int16_t offset;
} sfStackEntry;

typedef struct {
    sfStackEntry* entries;
    uint16_t      count;
    uint16_t      capacity;
} sfStackMap;

char* sfGenerateAssembly(const sfIRProgram* program);
