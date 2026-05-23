#pragma once

#include "sulfur/ast.h"
#include "sulfur/lexer.h"

sfProgramNode* parse(sfTokenList list, const char* filename);
