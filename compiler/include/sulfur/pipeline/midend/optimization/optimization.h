#pragma once

#include "sulfur/pipeline/midend/ir/ir.h"

typedef enum sf_optimization {
    SF_OPTIMIZATION_CONSTANT_FOLDING,

    SF_OPTIMIZATION_COUNT
} sf_optimization;

typedef enum sf_optimization_level {
    SF_OPTIMIZATION_LEVEL_O0,
    SF_OPTIMIZATION_LEVEL_O1,
    SF_OPTIMIZATION_LEVEL_O2,
    SF_OPTIMIZATION_LEVEL_O3,

    SF_OPTIMIZATION_LEVEL_COUNT
} sf_optimization_level;

typedef void (*sf_optimization_pass)(sf_ir_program *ir);

sf_ir_program sf_optimization_run(sf_ir_program *ir, sf_optimization_level level);
