#pragma once

#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/lexer/token.h"

sf_ast_node_program *sf_parser_run(
    sf_arena *arena, sf_token_list list, const char *filename
);
