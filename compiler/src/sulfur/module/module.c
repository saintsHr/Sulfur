
#include "sulfur/module/module.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/pipeline/frontend/parser/parser.h"
#include "sulfur/pipeline/frontend/preprocessor.h"
#include "sulfur/utils/arena.h"
#include "sulfur/utils/file.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/string.h"

static sf_module *find_module(
    sf_module_list *list,
    const char *filename
);

static void add_module(
    sf_arena *arena,
    sf_module_list *list,
    sf_module module,
    size_t *index
);

static void resolve_import(
    sf_import_node *import,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
);

static void resolve_imports(
    sf_import_node **imports,
    size_t import_count,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
);

static char *join_path(
    const char *directory,
    const char *filename
);

static bool file_exists(const char *filename);

static char *find_module_path(
    const char *filename,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
);

void sf_module_resolve(
    sf_import_node **imports,
    size_t import_count,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
) {
    resolve_imports(
        imports,
        import_count,
        modules,
        module_paths,
        module_path_count,
        arena
    );
}

static sf_module *find_module(
    sf_module_list *list,
    const char *filename
) {
    for (size_t i = 0; i < list->module_count; i++) {
        if (strcmp(list->modules[i].filename, filename) == 0) {
            return &list->modules[i];
        }
    }

    return NULL;
}

static void add_module(
    sf_arena *arena,
    sf_module_list *list,
    sf_module module,
    size_t *index
) {
    sf_module *modules = sf_arena_grow_one(
        arena,
        list->modules,
        list->module_count,
        &list->module_capacity,
        sizeof(sf_module)
    );

    if (modules == NULL) {
        sf_log(
            "insufficient memory", "cannot allocate memory for compiling",
            "free some memory and try again", "N/A",
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );
        return;
    }

    list->modules = modules;
    *index = list->module_count;
    list->modules[list->module_count++] = module;
}

static void resolve_imports(
    sf_import_node **imports,
    size_t import_count,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
) {
    for (size_t i = 0; i < import_count; i++) {
        resolve_import(
            imports[i],
            modules,
            module_paths,
            module_path_count,
            arena
        );

        if (sf_log_had_fatal()) {
            return;
        }
    }
}

static void resolve_import(
    sf_import_node *import,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
) {
    char *filename = find_module_path(
        import->path,
        module_paths,
        module_path_count,
        arena
    );

    if (filename == NULL) {
        sf_log(
            "module not found",
            "unable to find module '%s'",
            "check the import path and configured module search directories",
            import->path, SF_LOG_MODULE_NOT_FOUND, import->base.span,
            SF_LOG_SEVERITY_ERROR, import->path
        );
        return;
    }

    sf_module *existing = find_module(modules, filename);

    if (existing != NULL) {
        return;
    }

    sf_module module = {
        .filename = filename,
        .state = SF_MODULE_STATE_LOADING,
        .ast = NULL
    };

    size_t module_index;

    add_module(arena, modules, module, &module_index);

    if (sf_log_had_errors()) {
        return;
    }

    size_t input_size = 0;
    char *input = sf_file_read(filename, &input_size);

    if (input == NULL) {
        return;
    }

    char *preprocessed = sf_preprocess(input, input_size, filename);

    if (preprocessed == NULL || sf_log_had_errors()) {
        return;
    }

    sf_token_list tokens = sf_tokenize(preprocessed, filename);

    if (sf_log_had_errors()) {
        return;
    }

    sf_program_node *ast = sf_parse(arena, tokens, filename);

    if (ast == NULL || sf_log_had_errors()) {
        return;
    }

    modules->modules[module_index].ast = ast;

    resolve_imports(
        ast->imports,
        ast->import_count,
        modules,
        module_paths,
        module_path_count,
        arena
    );

    if (sf_log_had_fatal()) {
        return;
    }

    modules->modules[module_index].state = SF_MODULE_STATE_LOADED;

    sf_tokens_free(&tokens);
    free(preprocessed);
    free(input);
}

static char *join_path(
    const char *directory,
    const char *filename
) {
    size_t dir_len = strlen(directory);
    size_t file_len = strlen(filename);

    bool needs_separator = dir_len > 0 && directory[dir_len - 1] != '/';
    size_t size = dir_len + (needs_separator ? 1 : 0) + file_len + 1;

    char *path = malloc(size);

    if (path == NULL) {
        sf_log(
            "insufficient memory", "cannot allocate memory for compiling",
            "free some memory and try again", "N/A",
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );
        return NULL;
    }

    snprintf(
        path,
        size,
        "%s%s%s",
        directory,
        needs_separator ? "/" : "",
        filename
    );

    return path;
}

static bool file_exists(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        return false;
    }

    fclose(file);
    return true;
}

static char *find_module_path(
    const char *filename,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
) {
    if (file_exists(filename)) {
        return sf_string_dup_arena(arena, filename);
    }

    for (size_t i = 0; i < module_path_count; i++) {
        char *candidate = join_path(module_paths[i], filename);

        if (candidate == NULL) {
            return NULL;
        }

        if (file_exists(candidate)) {
            char *result = sf_string_dup_arena(arena, candidate);
            free(candidate);
            return result;
        }

        free(candidate);
    }

    return NULL;
}
