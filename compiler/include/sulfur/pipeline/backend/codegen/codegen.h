#pragma once

#include "sulfur/driver/main.h"
#include "sulfur/pipeline/midend/ir/ir.h"

char *sf_generate_assembly(
    const sf_ir_program *program, const sf_compilation_options *options
);
