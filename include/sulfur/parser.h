#pragma once

#include "sulfur/ast.h"
#include "sulfur/lexer.h"

typedef struct {
    char* name;
    sfValueType type;
} sfSymbol;

typedef struct {
    sfSymbol* symbols;
    size_t count;
    size_t capacity;
} sfSymbolTable;

sfProgramNode* parse(sfTokenList list, const char* filename);
