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

static bool compilation_run(const sf_compiler_options *options, sf_compilation *c);
static void compilation_free(sf_compilation *c);

static void debug_dump_stages(sf_compilation *c);

int main(int argc, char *argv[]) {
    sf_compiler_options options = {
        .file_input = "input.slfr",
        .file_output = "output.asm",
        .debug_dump_stages = false,
    };

    sf_compilation c = {0};

    sf_log_init();
    sf_flags_parse(argc, argv, &options);
    if (sf_log_had_fatal()) return EXIT_FAILURE;

    bool ok = compilation_run(&options, &c);
    compilation_free(&c);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

static bool compilation_run(const sf_compiler_options *options, sf_compilation *c) {
    c->input_source = sf_file_read(options->file_input, &c->input_size);
    if (c->input_source == NULL) return false;

    sf_log_set_source(options->file_input, c->input_source);

    sf_arena_init(&c->arena, 32);

    c->preprocessed = sf_preprocess(c->input_source, c->input_size, options->file_input);
    if (sf_log_had_fatal()) return false;

    c->tokens = sf_tokenize(c->preprocessed, options->file_input);
    if (sf_log_had_fatal()) return false;

    c->ast = sf_parse(&c->arena, c->tokens, options->file_input);
    if (sf_log_had_fatal()) return false;

    sf_analyze(c->ast, options->file_input);
    if (sf_log_had_fatal()) return false;

    c->ir = sf_generate_ir(&c->arena, c->ast);
    if (sf_log_had_fatal()) return false;

    c->assembly = sf_generate_assembly(&c->ir);
    if (sf_log_had_fatal()) return false;

    if (sf_log_had_errors()) return false;

    if (options->debug_dump_stages) debug_dump_stages(c);

    return sf_file_write(options->file_output, c->assembly);
}

static void compilation_free(sf_compilation *c) {
    free(c->assembly);
    sf_tokens_free(&c->tokens);
    free(c->preprocessed);
    sf_arena_free(&c->arena);
    free(c->input_source);
}

static void debug_dump_stages(sf_compilation *c) {
    printf("%s", c->input_source);
    printf("\n");
    sf_tokens_print(&c->tokens);
    printf("\n");
    sf_print_ast((sf_ast_node *)c->ast);
    printf("\n");
    sf_print_ir(&c->ir);
    printf("\n");
    printf("%s", c->assembly);
}
