#include "sulfur/module/module.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
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

static bool find_module_index(
    sf_module_list *list,
    const char *filename,
    size_t *index
);

static bool add_module(
    sf_arena *arena,
    sf_module_list *list,
    sf_module module,
    size_t *index
);

static void resolve_import(
    sf_import_node *import,
    const char *importer_filename,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
);

static void resolve_imports(
    sf_import_node **imports,
    size_t import_count,
    const char *importer_filename,
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
    const char *importer_filename,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
) {
    resolve_imports(
        imports,
        import_count,
        importer_filename,
        modules,
        module_paths,
        module_path_count,
        arena
    );
}

const char *sf_module_name_from_path(
    const char *file,
    const char **module_paths,
    size_t module_path_count
) {
    if (strncmp(file, "./", 2) == 0) {
        file += 2;
    }

    for (size_t i = 0; i < module_path_count; i++) {
        const char *dir = module_paths[i];

        if (strncmp(dir, "./", 2) == 0) {
            dir += 2;
        }

        size_t len = strlen(dir);
        while (len > 0 && dir[len - 1] == '/') {
            len--;
        }

        if (len > 0 && strncmp(file, dir, len) == 0 && file[len] == '/') {
            return file + len + 1;
        }
    }

    return NULL;
}

static bool find_module_index(
    sf_module_list *list,
    const char *filename,
    size_t *index
) {
    for (size_t i = 0; i < list->module_count; i++) {
        if (strcmp(list->modules[i].filename, filename) == 0) {
            *index = i;
            return true;
        }
    }

    return false;
}

static bool add_module(
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
            "Insufficient Memory.",
            "Unable to allocate memory while compiling.",
            "Free some memory and try again.",
            "N/A",
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY,
            (sf_span){0},
            SF_LOG_SEVERITY_FATAL
        );

        return false;
    }

    list->modules = modules;
    *index = list->module_count;
    list->modules[list->module_count++] = module;

    return true;
}

static void resolve_imports(
    sf_import_node **imports,
    size_t import_count,
    const char *importer_filename,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
) {
    for (size_t i = 0; i < import_count; i++) {
        resolve_import(
            imports[i],
            importer_filename,
            modules,
            module_paths,
            module_path_count,
            arena
        );
    }
}

static void resolve_import(
    sf_import_node *import,
    const char *importer_filename,
    sf_module_list *modules,
    const char **module_paths,
    size_t module_path_count,
    sf_arena *arena
) {
    char *filename =
        find_module_path(import->path, module_paths, module_path_count, arena);

    if (filename == NULL) {
        sf_log(
            "Module Not Found.",
            "Unable to find module '%s'.",
            "Check the import path and configured module search directories.",
            importer_filename,
            SF_LOG_MODULE_NOT_FOUND,
            import->base.span,
            SF_LOG_SEVERITY_ERROR,
            import->path
        );

        return;
    }

    size_t module_index = 0;

    if (find_module_index(modules, filename, &module_index)) {
        import->module_index = module_index;
        import->resolved = true;

        return;
    }

    sf_module module = {
        .filename = filename,
        .state = SF_MODULE_STATE_LOADING,
        .analysis = SF_MODULE_ANALYSIS_PENDING,
        .ast = NULL,
        .funcs = {0},
        .name = import->path
    };

    add_module(arena, modules, module, &module_index);

    import->module_index = module_index;
    import->resolved = true;

    char *input = NULL;
    char *preprocessed = NULL;
    sf_token_list tokens = {0};
    sf_program_node *ast = NULL;
    size_t input_size = 0;

    input = sf_file_read(filename, &input_size);
    preprocessed = sf_preprocess(input, input_size, filename);
    tokens = sf_tokenize(preprocessed, filename);
    ast = sf_parse(arena, tokens, filename);
    modules->modules[module_index].ast = ast;

    resolve_imports(
        ast->imports,
        ast->import_count,
        filename,
        modules,
        module_paths,
        module_path_count,
        arena
    );

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
            "Insufficient Memory.",
            "Unable to allocate memory while compiling.",
            "Free some memory and try again.",
            "N/A",
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY,
            (sf_span){0},
            SF_LOG_SEVERITY_FATAL
        );

        return NULL;
    }

    snprintf(
        path, size, "%s%s%s", directory, needs_separator ? "/" : "", filename
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
