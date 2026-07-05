#pragma once

#include "sulfur/pipeline/ast.h"
#include "sulfur/pipeline/lexer.h"

sf_program_node* sf_parse(sf_token_list list, const char* filename);
