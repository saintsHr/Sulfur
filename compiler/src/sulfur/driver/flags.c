#include <string.h>

#include "sulfur/driver/flags.h"

#include "sulfur/utils/log.h"

void sf_flags_parse(int argc, char *argv[], sf_compiler_options *options) {
    for (int i = 1; i < argc; i++) {
        bool has_next = i + 1 < argc;

        const char *this_arg = argv[i];
        const char *next_arg = argv[i + 1];

        if (strcmp(this_arg, FLAGS_INPUT_FILE) == 0) {
            if (!has_next || next_arg[0] == '-') {
                sf_log(
                    "No input file", "No input file was provided after '-i'.",
                    "Provide a filename after '%s', or remove the flag.",
                    "N/A", SF_LOG_MAIN_NO_INPUT_FILE, (sf_span){0},
                    SF_LOG_SEVERITY_FATAL, FLAGS_INPUT_FILE
                );

                return;
            }

            options->file_input = argv[++i];

            continue;
        }

        if (strcmp(this_arg, FLAGS_OUTPUT_FILE) == 0) {
            if (!has_next || next_arg[0] == '-') {
                sf_log(
                    "No output file.", "No output file was provided after '-o'.",
                    "Provide a filename after '%s', or remove the flag.", "N/A",
                    SF_LOG_MAIN_NO_OUTPUT_FILE, (sf_span){0}, SF_LOG_SEVERITY_FATAL,
                    FLAGS_OUTPUT_FILE
                );

                return;
            }

            options->file_output = argv[++i];

            continue;
        }

        if (strcmp(this_arg, FLAGS_DEBUG_DUMP_STAGES) == 0) {
            options->debug_dump_stages = true;

            continue;
        }

        sf_log(
            "Unknown flag.", "Unrecognized flag '%s'.",
            "Check available flags or remove this flag.",
            "N/A", SF_LOG_MAIN_UNKNOWN_FLAG, (sf_span){0}, SF_LOG_SEVERITY_FATAL, argv[i]
        );
    }
}
