#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    SF_TOML_PARSE_RESULT_OK,

    SF_TOML_PARSE_RESULT_FAILED_TO_OPEN,

    SF_TOML_PARSE_RESULT_NAME_INVALID,
    SF_TOML_PARSE_RESULT_VERSION_INVALID,
    SF_TOML_PARSE_RESULT_FLAGS_INVALID,
    SF_TOML_PARSE_RESULT_TARGET_INVALID,

    SF_TOML_PARSE_RESULT_OUT_OF_MEMORY
} sf_toml_parse_result;

typedef enum {
    SF_TOML_TARGET_INVALID,

    SF_TOML_TARGET_EXECUTABLE
} sf_toml_target;

typedef struct {
    char *name;
    char *version;

    sf_toml_target target;

    char **flags;
    size_t flags_count;
} sf_project_cfg;

sf_toml_parse_result sf_toml_parse_project(const char *filename, sf_project_cfg *output);
