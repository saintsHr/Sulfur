#pragma once

#include "sulfur/ast.h"
#include "sulfur/lexer.h"

typedef struct {
    char* name;
    sfValueType type;
} sfSymbol;

typedef struct {
    size_t count;
    size_t capacity;
    sfSymbol* symbols;
} sfSymbolTable;

sfProgramNode* parse(sfTokenList list, const char* filename);
