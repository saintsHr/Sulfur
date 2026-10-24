#pragma once

#include "sulfur/driver/main.h"

#define FLAGS_INPUT_FILE "-i"
#define FLAGS_OUTPUT_FILE "-o"
#define FLAGS_DUMP_STAGES "--dump-stages"
#define FLAGS_NO_ENTRY "--no-entry"

void sf_flags_parse(int argc, char *argv[], sf_compiler_options *options);
