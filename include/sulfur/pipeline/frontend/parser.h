#pragma once

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"

sf_program_node* sf_parse(sf_token_list list, const char* filename);
