#pragma once

#include <stddef.h>

typedef enum {
    SF_TOML_PARSE_RESULT_OK,
    SF_TOML_PARSE_RESULT_FAILED_TO_OPEN,
    SF_TOML_PARSE_RESULT_NAME_INVALID,
    SF_TOML_PARSE_RESULT_EMIT_INVALID,
    SF_TOML_PARSE_RESULT_SLFR_FLAGS_INVALID,
    SF_TOML_PARSE_RESULT_ASM_FLAGS_INVALID,
    SF_TOML_PARSE_RESULT_LINK_FLAGS_INVALID,
    SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY
} sf_toml_parse_result;

typedef enum {
    SF_EMIT_BIN,
    SF_EMIT_ASM,
    SF_EMIT_OBJ,
    SF_EMIT_LIB
} sf_emit;

typedef struct {
    char **items;
    size_t count;
} sf_string_list;

typedef struct {
    char *name;
    sf_emit emit;

    sf_string_list slfr_flags;
    sf_string_list asm_flags;
    sf_string_list link_flags;
} sf_project_cfg;

sf_toml_parse_result sf_toml_parse_project(const char *filename, sf_project_cfg *output);
void sf_toml_free_project(sf_project_cfg *cfg);
