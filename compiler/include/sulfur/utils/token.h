#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/frontend/lexer/lexer.h"
#include "sulfur/shared/operation.h"
#include "sulfur/shared/type.h"

bool sf_token_is_type(sf_lexer_token token);
bool sf_token_is_ident(sf_lexer_token token);
bool sf_token_is_block(sf_lexer_token token);
bool sf_token_is_if(sf_lexer_token token);
bool sf_token_is_while(sf_lexer_token token);
bool sf_token_is_fn(sf_lexer_token token);
bool sf_token_is_return(sf_lexer_token token);
bool sf_token_is_qual(sf_lexer_token token);
bool sf_token_is_attr(sf_lexer_token token);
bool sf_token_is_import(sf_lexer_token token);
bool sf_token_is_namespace(sf_lexer_token token);
bool sf_token_is_assign(sf_lexer_token token);

sf_type_kind sf_token_to_type(sf_lexer_token token);
sf_operation_type sf_token_to_binary_op(sf_lexer_token token);
sf_operation_type sf_token_to_unary_op(sf_lexer_token token);
sf_operation_type sf_token_to_postfix_op(sf_lexer_token token);
