#pragma once

#include <stddef.h>

#include "sulfur/pipeline/frontend/lexer/token.h"

sf_token_list sf_lexer_run(const char *input, const char *filename);
const char *sf_lexer_token_type_name(sf_token_type type);
