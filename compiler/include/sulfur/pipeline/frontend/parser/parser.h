#pragma once

#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/lexer/lexer.h"

sf_ast_node_program *sf_parse(
    sf_arena *arena, sf_lexer_token_list list, const char *filename
);
