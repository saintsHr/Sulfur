#include "sulfur/driver/flags.h"

#include <stdlib.h>
#include <string.h>

#include "sulfur/utils/log.h"

static bool add_module_path(
    sf_compiler_options *options,
    const char *path
);

void sf_flags_parse(int argc, char *argv[], sf_compiler_options *options) {
    for (int i = 1; i < argc; i++) {
        bool has_next = i + 1 < argc;

        const char *this_arg = argv[i];
        const char *next_arg = has_next ? argv[i + 1] : NULL;

        if (strcmp(this_arg, FLAGS_INPUT_FILE) == 0) {
            if (!has_next || next_arg[0] == '-') {
                sf_log(
                    "no input file", "No input file was provided after '%s'",
                    "provide a filename after '%s', or remove the flag",
                    "N/A", SF_LOG_DRIVER_NO_INPUT_FILE, (sf_span){0},
                    SF_LOG_SEVERITY_FATAL, FLAGS_INPUT_FILE, FLAGS_INPUT_FILE
                );

                return;
            }

            options->file_input = argv[++i];

            continue;
        }

        if (strcmp(this_arg, FLAGS_OUTPUT_FILE) == 0) {
            if (!has_next || next_arg[0] == '-') {
                sf_log(
                    "no output file", "no output file was provided after '%s'",
                    "provide a filename after '%s', or remove the flag", "N/A",
                    SF_LOG_DRIVER_NO_OUTPUT_FILE, (sf_span){0}, SF_LOG_SEVERITY_FATAL,
                    FLAGS_OUTPUT_FILE, FLAGS_OUTPUT_FILE
                );

                return;
            }

            options->file_output = argv[++i];

            continue;
        }

        if (strcmp(this_arg, FLAGS_DUMP_STAGES) == 0) {
            options->dump_stages = true;

            continue;
        }

        if (strcmp(this_arg, FLAGS_MODULE_PATH) == 0) {
            size_t added = 0;

            while (has_next && argv[i + 1][0] != '-') {
                if (!add_module_path(options, argv[++i])) {
                    sf_log(
                        "insufficient memory", "cannot allocate memory for compiling",
                        "free some memory and try again", "N/A",
                        SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0},
                        SF_LOG_SEVERITY_FATAL
                    );

                    return;
                }

                added++;
                has_next = i + 1 < argc;
            }

            if (added == 0) {
                sf_log(
                    "no module path",
                    "no module search directories were provided after '%s'",
                    "provide at least one directory after '%s'",
                    "N/A", SF_LOG_DRIVER_NO_MODULE_PATH, (sf_span){0},
                    SF_LOG_SEVERITY_FATAL, FLAGS_MODULE_PATH, FLAGS_MODULE_PATH
                );

                return;
            }

            continue;
        }

        if (strcmp(this_arg, FLAGS_MODULE) == 0) {
            options->is_module = true;
            continue;
        }

        sf_log(
            "unknown flag", "unrecognized flag '%s'",
            "check available flags or remove this flag",
            "N/A", SF_LOG_DRIVER_UNKNOWN_FLAG, (sf_span){0},
            SF_LOG_SEVERITY_FATAL, argv[i]
        );
    }
}

static bool add_module_path(
    sf_compiler_options *options,
    const char *path
) {
    if (path == NULL) {
        return false;
    }

    if (options->module_path_count == options->module_path_capacity) {
        size_t new_capacity = options->module_path_capacity == 0
            ? 4
            : options->module_path_capacity * 2;

        const char **paths = realloc(
            options->module_paths,
            new_capacity * sizeof(char *)
        );

        if (paths == NULL) {
            return false;
        }

        options->module_paths = paths;
        options->module_path_capacity = new_capacity;
    }

    options->module_paths[options->module_path_count++] = path;

    return true;
}
