#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/backend/ir/ir.h"

bool sf_fold_constants(
    sf_arena* arena,
    sf_operand left,
    sf_operand right,
    sf_opcode opcode,
    sf_value_type result_type,
    sf_operand* out
);