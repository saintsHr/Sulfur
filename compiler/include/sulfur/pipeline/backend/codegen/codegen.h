#pragma once

#include "sulfur/driver/main.h"
#include "sulfur/pipeline/midend/ir/ir.h"

char *sf_codegen_run(const sf_ir_program *program, const sf_compilation_options *options);
