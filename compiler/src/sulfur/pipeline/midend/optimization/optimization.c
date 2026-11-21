#include "sulfur/pipeline/midend/optimization/optimization.h"

#include <stddef.h>

#include "sulfur/pipeline/midend/optimization/passes.h"

static const bool k_opt_levels[SF_OPTIMIZATION_LEVEL_COUNT][SF_OPTIMIZATION_COUNT] = {
    [SF_OPTIMIZATION_LEVEL_O0] = {[SF_OPTIMIZATION_CONSTANT_FOLDING] = false},
    [SF_OPTIMIZATION_LEVEL_O1] = {[SF_OPTIMIZATION_CONSTANT_FOLDING] = false},
    [SF_OPTIMIZATION_LEVEL_O2] = {[SF_OPTIMIZATION_CONSTANT_FOLDING] = false},
    [SF_OPTIMIZATION_LEVEL_O3] = {[SF_OPTIMIZATION_CONSTANT_FOLDING] = false},
};

static const sf_optimization_pass k_opt_passes[SF_OPTIMIZATION_COUNT] = {
    [SF_OPTIMIZATION_CONSTANT_FOLDING] =
        sf_optimization_pass_constant_folding,
};

static bool sf_optimization_is_enabled(
    sf_optimization_level level, sf_optimization optimization
);

sf_ir_program sf_optimization_run(sf_ir_program *ir, sf_optimization_level level) {
    sf_ir_program opt = *ir;

    for (size_t i = 0; i < SF_OPTIMIZATION_COUNT; i++) {
        if (!sf_optimization_is_enabled(level, (sf_optimization)i)) {
            continue;
        }

        sf_optimization_pass pass = k_opt_passes[i];

        if (pass != NULL) {
            pass(&opt);
        }
    }

    return opt;
}

static bool sf_optimization_is_enabled(
    sf_optimization_level level, sf_optimization optimization
) {
    if (level >= SF_OPTIMIZATION_LEVEL_COUNT || optimization >= SF_OPTIMIZATION_COUNT) {
        return false;
    }

    return k_opt_levels[level][optimization];
}
