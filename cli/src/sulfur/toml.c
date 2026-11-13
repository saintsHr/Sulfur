#include "sulfur/toml.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tomlc17.h>
#include <unistd.h>

#define SF_PRINTF(fmt, first) __attribute__((format(printf, fmt, first)))

static char error_detail[256];

static void set_detail(const char *fmt, ...) SF_PRINTF(1, 2);

static char *string_dup(const char *string);
static void list_free(sf_string_list *list);
static bool name_is_valid(const char *name);
static sf_toml_parse_result parse_emit(toml_datum_t datum, sf_emit *out);
static sf_toml_parse_result parse_list(
    toml_datum_t datum, sf_toml_parse_result invalid, sf_string_list *out
);

const char *sf_toml_error_detail(void) {
    return error_detail;
}

sf_toml_parse_result sf_toml_parse_project(
    const char *filename, sf_project_cfg *output
) {
    error_detail[0] = '\0';

    if (!filename || !output) {
        return SF_TOML_PARSE_RESULT_INVALID_ARGUMENT;
    }

    *output = (sf_project_cfg){0};

    toml_result_t toml = toml_parse_file_ex(filename);

    if (!toml.ok) {
        sf_toml_parse_result failure;

        if (access(filename, R_OK) != 0) {
            set_detail("%s", strerror(errno));
            failure = SF_TOML_PARSE_RESULT_FAILED_TO_OPEN;
        } else {
            set_detail("%s", toml.errmsg);
            failure = SF_TOML_PARSE_RESULT_SYNTAX_ERROR;
        }

        toml_free(toml);
        return failure;
    }

    sf_toml_parse_result result = SF_TOML_PARSE_RESULT_OK;

    toml_datum_t name_d = toml_seek(toml.toptab, "project.name");
    toml_datum_t emit_d = toml_seek(toml.toptab, "build.emit");
    toml_datum_t slfr_flags_d = toml_seek(toml.toptab, "build.slfr_flags");
    toml_datum_t asm_flags_d = toml_seek(toml.toptab, "build.asm_flags");
    toml_datum_t link_flags_d = toml_seek(toml.toptab, "build.link_flags");

    if (name_d.type == TOML_UNKNOWN) {
        result = SF_TOML_PARSE_RESULT_NAME_MISSING;
        goto done;
    }

    if (name_d.type != TOML_STRING) {
        set_detail("expected a string");
        result = SF_TOML_PARSE_RESULT_NAME_INVALID;
        goto done;
    }

    if (!name_is_valid(name_d.u.s)) {
        set_detail("got '%s'", name_d.u.s);
        result = SF_TOML_PARSE_RESULT_NAME_INVALID;
        goto done;
    }

    result = parse_emit(emit_d, &output->emit);
    if (result != SF_TOML_PARSE_RESULT_OK) {
        goto done;
    }

    output->name = string_dup(name_d.u.s);
    if (!output->name) {
        result = SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY;
        goto done;
    }

    result = parse_list(
        slfr_flags_d, SF_TOML_PARSE_RESULT_SLFR_FLAGS_INVALID,
        &output->slfr_flags
    );
    if (result != SF_TOML_PARSE_RESULT_OK) {
        goto done;
    }

    result = parse_list(
        asm_flags_d, SF_TOML_PARSE_RESULT_ASM_FLAGS_INVALID, &output->asm_flags
    );
    if (result != SF_TOML_PARSE_RESULT_OK) {
        goto done;
    }

    result = parse_list(
        link_flags_d, SF_TOML_PARSE_RESULT_LINK_FLAGS_INVALID,
        &output->link_flags
    );

done:
    if (result != SF_TOML_PARSE_RESULT_OK) {
        sf_toml_free_project(output);
    }

    toml_free(toml);
    return result;
}

void sf_toml_free_project(sf_project_cfg *cfg) {
    if (!cfg) {
        return;
    }

    free(cfg->name);

    list_free(&cfg->slfr_flags);
    list_free(&cfg->asm_flags);
    list_free(&cfg->link_flags);

    *cfg = (sf_project_cfg){0};
}

static void set_detail(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(error_detail, sizeof(error_detail), fmt, args);
    va_end(args);
}

static bool name_is_valid(const char *name) {
    return name[0] != '\0' && name[0] != '.' && strchr(name, '/') == NULL;
}

static sf_toml_parse_result parse_emit(toml_datum_t datum, sf_emit *out) {
    static const struct {
        const char *name;
        sf_emit value;
    } table[] = {
        {"asm", SF_EMIT_ASM},
        {"obj", SF_EMIT_OBJ},
        {"bin", SF_EMIT_BIN},
        {"lib", SF_EMIT_LIB},
    };

    if (datum.type == TOML_UNKNOWN) {
        *out = SF_EMIT_BIN;
        return SF_TOML_PARSE_RESULT_OK;
    }

    if (datum.type != TOML_STRING) {
        set_detail("expected a string");
        return SF_TOML_PARSE_RESULT_EMIT_INVALID;
    }

    for (size_t i = 0; i < sizeof(table) / sizeof(*table); i++) {
        if (strcmp(datum.u.s, table[i].name) == 0) {
            *out = table[i].value;
            return SF_TOML_PARSE_RESULT_OK;
        }
    }

    set_detail("got '%s'", datum.u.s);
    return SF_TOML_PARSE_RESULT_EMIT_INVALID;
}

static sf_toml_parse_result parse_list(
    toml_datum_t datum, sf_toml_parse_result invalid, sf_string_list *out
) {
    if (datum.type == TOML_UNKNOWN) {
        return SF_TOML_PARSE_RESULT_OK;
    }

    if (datum.type != TOML_ARRAY) {
        set_detail("expected an array of strings");
        return invalid;
    }

    size_t count = (size_t)datum.u.arr.size;

    if (count == 0) {
        return SF_TOML_PARSE_RESULT_OK;
    }

    sf_string_list list = {
        .items = calloc(count, sizeof(*list.items)),
        .count = count,
    };

    if (!list.items) {
        return SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY;
    }

    for (size_t i = 0; i < count; i++) {
        toml_datum_t element = datum.u.arr.elem[i];

        if (element.type != TOML_STRING) {
            set_detail("element %zu is not a string", i + 1);
            list_free(&list);
            return invalid;
        }

        list.items[i] = string_dup(element.u.s);

        if (!list.items[i]) {
            list_free(&list);
            return SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY;
        }
    }

    *out = list;
    return SF_TOML_PARSE_RESULT_OK;
}

static void list_free(sf_string_list *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i]);
    }

    free(list->items);
    *list = (sf_string_list){0};
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
