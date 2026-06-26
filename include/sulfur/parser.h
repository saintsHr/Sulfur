#pragma once

#include "sulfur/ast.h"
#include "sulfur/lexer.h"

sf_program_node* sf_parse(sf_token_list list, const char* filename);
