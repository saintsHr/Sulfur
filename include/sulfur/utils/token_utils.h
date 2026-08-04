#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"

bool token_is_type(sf_token token);
bool token_is_ident(sf_token token);
bool token_is_block(sf_token token);
bool token_is_if(sf_token token);

sf_value_type token_to_type(sf_token token);
sf_operation_type token_to_binary_op(sf_token token);
sf_operation_type token_to_unary_op(sf_token token);