#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/backend/ir/ir.h"

bool sf_fold_constants(
    sf_arena *arena, sf_ir_operand left, sf_ir_operand right,
    sf_ir_opcode opcode, sf_value_type result_type, sf_ir_operand *out
);
