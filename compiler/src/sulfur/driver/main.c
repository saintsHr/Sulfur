#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/driver/main.h"
#include "sulfur/driver/flags.h"

#include "sulfur/pipeline/backend/codegen/codegen.h"
#include "sulfur/pipeline/backend/ir/ir.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/pipeline/frontend/parser/parser.h"
#include "sulfur/pipeline/frontend/preprocessor.h"
#include "sulfur/pipeline/frontend/semantic/semantic.h"
#include "sulfur/utils/arena.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/file.h"

static bool compilation_run(sf_compilation *compilation, const sf_compiler_options *options);
static void compilation_free(sf_compilation *compilation);

static void dump_stages(sf_compilation *compilation);

int main(int argc, char *argv[]) {
    sf_compiler_options options = {
        .file_input = "input.slfr",
        .file_output = "output.asm",
        .dump_stages = false,
        .no_entry = false,
    };

    sf_compilation compilation = {0};

    sf_log_init();
    sf_flags_parse(argc, argv, &options);
    if (sf_log_had_fatal()) return EXIT_FAILURE;

    bool ok = compilation_run(&compilation, &options);
    compilation_free(&compilation);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

static bool compilation_run(sf_compilation *compilation, const sf_compiler_options *options) {
    compilation->input_source = sf_file_read(options->file_input, &compilation->input_size);
    if (compilation->input_source == NULL) return false;

    sf_log_set_source(options->file_input, compilation->input_source);
    sf_arena_init(&compilation->arena, 32);

    compilation->preprocessed = sf_preprocess(compilation->input_source, compilation->input_size, options->file_input);
    if (sf_log_had_fatal()) return false;

    compilation->tokens = sf_tokenize(compilation->preprocessed, options->file_input);
    if (sf_log_had_fatal()) return false;

    compilation->ast = sf_parse(&compilation->arena, compilation->tokens, options->file_input);
    if (sf_log_had_fatal()) return false;

    sf_analyze(compilation->ast, options->file_input);
    if (sf_log_had_fatal()) return false;

    compilation->ir = sf_ir_generate(&compilation->arena, compilation->ast);
    if (sf_log_had_fatal()) return false;

    compilation->assembly = sf_generate_assembly(&compilation->ir, options);
    if (sf_log_had_fatal()) return false;

    if (sf_log_had_errors()) return false;

    if (options->dump_stages) dump_stages(compilation);

    return sf_file_write(options->file_output, compilation->assembly);
}

static void compilation_free(sf_compilation *compilation) {
    free(compilation->assembly);
    sf_tokens_free(&compilation->tokens);
    free(compilation->preprocessed);
    sf_arena_free(&compilation->arena);
    free(compilation->input_source);
}

static void dump_stages(sf_compilation *compilation) {
    printf("%s", compilation->input_source);
    printf("\n");
    sf_tokens_print(&compilation->tokens);
    printf("\n");
    sf_print_ast((sf_ast_node *)compilation->ast);
    printf("\n");
    sf_ir_print(&compilation->ir);
    printf("\n");
    printf("%s", compilation->assembly);
}
