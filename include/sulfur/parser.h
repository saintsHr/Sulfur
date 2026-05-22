#pragma once

#include "sulfur/ast.h"
#include "sulfur/lexer.h"

typedef struct {
    char* name;
    sfValueType type;
} sfSymbol;

sfProgramNode* parse(sfTokenList list, const char* filename);
