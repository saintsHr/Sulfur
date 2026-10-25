#include "sulfur/toml.h"

#include <string.h>
#include <stdlib.h>
#include <tomlc17.h>

static char *string_dup(const char *string);

sf_toml_parse_result sf_toml_parse_project(const char *filename, sf_project_cfg *output) {
    if (!filename || !output) {
        return SF_TOML_PARSE_RESULT_FAILED_TO_OPEN;
    }

    *output = (sf_project_cfg){0};

    toml_result_t toml = toml_parse_file_ex(filename);

    if (!toml.ok) {
        toml_free(toml);
        return SF_TOML_PARSE_RESULT_FAILED_TO_OPEN;
    }

    sf_toml_parse_result result = SF_TOML_PARSE_RESULT_OK;

    toml_datum_t name_d = toml_seek(toml.toptab, "project.name");
    toml_datum_t version_d = toml_seek(toml.toptab, "project.version");
    toml_datum_t flags_d = toml_seek(toml.toptab, "build.flags");
    toml_datum_t entry_d = toml_seek(toml.toptab, "build.entry");

    if (name_d.type != TOML_STRING) {
        result = SF_TOML_PARSE_RESULT_NAME_INVALID;
        goto cleanup;
    }
    if (version_d.type != TOML_STRING) {
        result = SF_TOML_PARSE_RESULT_VERSION_INVALID;
        goto cleanup;
    }
    if (flags_d.type != TOML_ARRAY) {
        result = SF_TOML_PARSE_RESULT_FLAGS_INVALID;
        goto cleanup;
    }
    if (entry_d.type != TOML_STRING) {
        result = SF_TOML_PARSE_RESULT_ENTRY_INVALID;
        goto cleanup;
    }

    output->name = string_dup(name_d.u.s);
    output->version = string_dup(version_d.u.s);
    output->entry = string_dup(entry_d.u.s);

    if (!output->name || !output->version || !output->entry) {
        result = SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY;
        goto cleanup;
    }

    output->flags_count = flags_d.u.arr.size;

    if (output->flags_count > 0) {
        output->flags = calloc(
            output->flags_count,
            sizeof(*output->flags)
        );

        if (!output->flags) {
            result = SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY;
            goto cleanup;
        }

        for (size_t i = 0; i < output->flags_count; i++) {
            toml_datum_t flag = flags_d.u.arr.elem[i];

            if (flag.type != TOML_STRING) {
                result = SF_TOML_PARSE_RESULT_FLAGS_INVALID;
                goto cleanup;
            }

            output->flags[i] = string_dup(flag.u.s);

            if (!output->flags[i]) {
                result = SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY;
                goto cleanup;
            }
        }
    }

    goto done;

    cleanup: {
        sf_toml_free_project(output);
    }

    done: {
        toml_free(toml);
        return result;
    }
}

void sf_toml_free_project(sf_project_cfg *cfg) {
    if (!cfg) {
        return;
    }

    free(cfg->name);
    free(cfg->version);
    free(cfg->entry);

    for (size_t i = 0; i < cfg->flags_count; i++) {
        free(cfg->flags[i]);
    }

    free(cfg->flags);

    *cfg = (sf_project_cfg){0};
}

static char *string_dup(const char *string) {
    size_t len = strlen(string) + 1;
    char *copy = malloc(len);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, string, len);
    return copy;
}
