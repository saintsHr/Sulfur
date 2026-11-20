#pragma once

#include "sulfur/driver/main.h"

#define FLAGS_STR_INPUT_FILE "-i"
#define FLAGS_STR_OUTPUT_FILE "-o"
#define FLAGS_STR_DUMP_STAGES "--dump-stages"
#define FLAGS_STR_MODULE_PATH "-m"
#define FLAGS_STR_MODULE "--module"
#define FLAGS_STR_NO_ENTRY "--no-entry"

void sf_flags_parse(int argc, char **argv, sf_compilation_options *options);
