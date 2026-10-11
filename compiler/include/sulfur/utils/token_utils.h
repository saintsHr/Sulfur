#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"

bool sf_token_is_type(sf_token token);
bool sf_token_is_ident(sf_token token);
bool sf_token_is_block(sf_token token);
bool sf_token_is_if(sf_token token);
bool sf_token_is_while(sf_token token);
bool sf_token_is_fn(sf_token token);
bool sf_token_is_return(sf_token token);
bool sf_token_is_assignment_op(sf_token_type t);

sf_value_type sf_token_to_type(sf_token token);
sf_operation_type sf_token_to_binary_op(sf_token token);
sf_operation_type sf_token_to_unary_op(sf_token token);
sf_operation_type sf_token_to_postfix_op(sf_token token);
sf_operation_type sf_token_assign_to_binary_op(sf_token_type t);
