#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/pipeline/frontend/ast.h"

void sf_analyze(sf_program_node* program, const char* filename);
