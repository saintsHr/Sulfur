#pragma once

#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/lexer/lexer.h"

sf_program_node *sf_parse(
    sf_arena *arena, sf_token_list list, const char *filename
);
