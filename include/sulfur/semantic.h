#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/ast.h"

typedef struct {
    char*       name;
    sfValueType type;
    bool        initialized;
} sfSymbol;

typedef struct {
    sfSymbol* symbols;
    uint64_t  count;
    uint64_t  capacity;
} sfSymbolTable;

void sfAnalyze(sfProgramNode* program, const char* filename);

sfSymbol* sfSymbolTableLookup(sfSymbolTable* table, const char* name);
void sfSymbolTableInsert(sfSymbolTable* table, sfSymbol symbol,  const char* filename);
void sfSymbolTableInit(sfSymbolTable* table);
