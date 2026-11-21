#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/driver/flags.h"
#include "sulfur/driver/main.h"

#include "sulfur/io/file.h"
#include "sulfur/io/log.h"
#include "sulfur/module/module.h"
#include "sulfur/pipeline/backend/codegen/codegen.h"
#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/lexer/lexer.h"
#include "sulfur/pipeline/frontend/parser/parser.h"
#include "sulfur/pipeline/frontend/preprocessor/preprocessor.h"
#include "sulfur/pipeline/frontend/semantic/semantic.h"
#include "sulfur/pipeline/midend/ir/ir.h"
#include "sulfur/pipeline/midend/optimization/optimization.h"
#include "sulfur/shared/arena.h"

static bool compilation_run(
    sf_compilation *compilation, const sf_compilation_options *options
);

static void compilation_free(sf_compilation *compilation);

static void dump_stages(sf_compilation *compilation);

int main(int argc, char *argv[]) {
    sf_compilation_options options = {
        .file_input = "input.slfr",
        .file_output = "output.asm",
        .dump_stages = false,
        .is_module = false,
        .module_paths = NULL,
        .module_path_capacity = 0,
        .module_path_count = 0,
    };

    sf_compilation compilation = {0};

    sf_log_init();

    sf_flags_parse(argc, argv, &options);

    if (options.is_module &&
        sf_module_name_from_path(
            options.file_input, options.module_paths, options.module_path_count
        ) == NULL) {
        sf_log(
            "Invalid Module Path.", "'%s' is not inside any module search directory.",
            "Pass the directory containing this file with '-m'.", "N/A",
            SF_LOG_CODE_DRIVER_INVALID_MODULE_PATH, (sf_span){0}, SF_LOG_SEVERITY_FATAL,
            options.file_input
        );
    }

    bool ok = compilation_run(&compilation, &options);

    compilation_free(&compilation);
    free(options.module_paths);
    sf_log_shutdown();

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

static bool compilation_run(
    sf_compilation *compilation, const sf_compilation_options *options
) {
    compilation->input_source =
        sf_file_read(options->file_input, &compilation->input_size);

    sf_log_set_source(options->file_input, compilation->input_source);

    sf_arena_init(&compilation->arena);

    compilation->preprocessed = sf_preprocessor_run(
        compilation->input_source, compilation->input_size, options->file_input
    );

    compilation->tokens = sf_lexer_run(compilation->preprocessed, options->file_input);

    compilation->ast =
        sf_parser_run(&compilation->arena, compilation->tokens, options->file_input);

    sf_module_resolve(
        compilation->ast->imports, compilation->ast->import_count, options->file_input,
        &compilation->modules, options->module_paths, options->module_path_count,
        &compilation->arena
    );

    sf_semantic_run(
        &compilation->arena, compilation->ast, options->file_input, options,
        &compilation->modules
    );

    compilation->ir = sf_ir_run(&compilation->arena, compilation->ast);

    compilation->ir_optimized =
        sf_optimization_run(&compilation->ir, SF_OPTIMIZATION_LEVEL_O0);

    compilation->assembly = sf_codegen_run(&compilation->ir_optimized, options);

    if (options->dump_stages) {
        dump_stages(compilation);
    }

    if (sf_log_had_errors()) {
        return false;
    }

    sf_file_write(options->file_output, compilation->assembly);

    return true;
}

static void compilation_free(sf_compilation *compilation) {
    free(compilation->assembly);
    sf_token_list_free(&compilation->tokens);
    free(compilation->preprocessed);
    sf_arena_free(&compilation->arena);
    free(compilation->input_source);
}

static void dump_stages(sf_compilation *compilation) {
    printf("\n[Source]:\n\n");
    printf("%s", compilation->input_source);

    printf("\n[Tokens]:\n\n");
    sf_token_list_print(&compilation->tokens);

    printf("\n[AST]:\n\n");
    sf_print_ast((sf_ast_node *)compilation->ast);

    printf("\n[IR (Original)]:\n\n");
    sf_ir_print(&compilation->ir);

    printf("\n[IR (Optimized)]:\n\n");
    sf_ir_print(&compilation->ir_optimized);

    printf("\n[Assembly]:\n\n");
    printf("%s", compilation->assembly);
}
