#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"

bool token_is_type(sf_token token);
bool token_is_ident(sf_token token);
bool token_is_block(sf_token token);

sf_value_type token_to_type(sf_token token);