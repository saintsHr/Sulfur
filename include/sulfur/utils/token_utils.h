#pragma once

#include "sulfur/pipeline/lexer.h"
#include <stdbool.h>

bool token_is_type(sf_token token);
bool token_is_ident(sf_token token);
bool token_is_block(sf_token token);