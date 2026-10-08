#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/pipeline/backend/codegen/codegen.h"
#include "sulfur/pipeline/backend/ir/ir.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/pipeline/frontend/parser.h"
#include "sulfur/pipeline/frontend/preprocessor.h"
#include "sulfur/pipeline/frontend/semantic/semantic.h"
#include "sulfur/utils/arena.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/span.h"

#define FLAGS_INPUT_FILE "-i"
#define FLAGS_OUTPUT_FILE "-o"
#define FLAGS_DEBUG_DUMP_STAGES "--dump-stages"

typedef struct {
    const char *file_input;
    const char *file_output;

    bool debug_dump_stages;
} sf_compiler_options;

typedef struct {
    char *input_source;
    size_t input_size;

    sf_arena arena;

    char *preprocessed;
    sf_token_list tokens;
    sf_program_node *ast;
    sf_ir_program ir;
    char *assembly;
} sf_compilation;

static void flags_parse(int argc, char *argv[], sf_compiler_options *options);

static char *file_read(const char *filename, size_t *size);
static bool file_write(const char *filename, const char *content);

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

    flags_parse(argc, argv, &options);

    bool ok = compilation_run(&options, &c);
    compilation_free(&c);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

static bool compilation_run(const sf_compiler_options *options, sf_compilation *c) {
    c->input_source = file_read(options->file_input, &c->input_size);
    if (c->input_source == NULL) return false;

    sf_log_set_source(options->file_input, c->input_source);
    sf_log_init();

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

    if (options->debug_dump_stages) debug_dump_stages(c);

    if (sf_log_had_errors()) return false;

    return file_write(options->file_output, c->assembly);
}

static void debug_dump_stages(sf_compilation *c) {
    printf("%s", c->input_source);
    printf("\n");
    sf_print_tokens(&c->tokens);
    printf("\n");
    sf_print_ast((sf_ast_node *)c->ast);
    printf("\n");
    sf_print_ir(&c->ir);
    printf("\n");
    printf("%s", c->assembly);
}

static void compilation_free(sf_compilation *c) {
    free(c->assembly);
    sf_free_tokens(&c->tokens);
    free(c->preprocessed);
    sf_arena_free(&c->arena);
    free(c->input_source);
}

static void flags_parse(int argc, char *argv[], sf_compiler_options *options) {
    for (int i = 1; i < argc; i++) {
        bool has_next = i + 1 < argc;

        const char *this_arg = argv[i];
        const char *next_arg = argv[i + 1];

        if (strcmp(this_arg, FLAGS_INPUT_FILE) == 0) {
            if (!has_next || next_arg[0] == '-') {
                sf_log(
                    "No input file", "No input file was provided after '-i'.",
                    "Provide a filename after '%s', or remove the flag.",
                    "N/A", SF_MAIN_NO_INPUT_FILE, (sf_span){0}, SF_SEV_FATAL,
                    FLAGS_INPUT_FILE
                );

                return;
            }

            options->file_input = argv[++i];

            continue;
        }

        if (strcmp(this_arg, FLAGS_OUTPUT_FILE) == 0) {
            if (!has_next || next_arg[0] == '-') {
                sf_log(
                    "No output file.", "No output file was provided after '-o'.",
                    "Provide a filename after '%s', or remove the flag.", "N/A",
                    SF_MAIN_NO_OUTPUT_FILE, (sf_span){0}, SF_SEV_FATAL,
                    FLAGS_OUTPUT_FILE
                );

                return;
            }

            options->file_output = argv[++i];

            continue;
        }

        if (strcmp(this_arg, FLAGS_DEBUG_DUMP_STAGES) == 0) {
            options->debug_dump_stages = true;

            continue;
        }

        sf_log(
            "Unknown flag.", "Unrecognized flag '%s'.",
            "Check available flags or remove this flag.",
            "N/A", SF_MAIN_UNKNOWN_FLAG, (sf_span){0}, SF_SEV_FATAL, argv[i]
        );
    }
}

static char *file_read(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        sf_log(
            "Cannot open file.", "Unable to open file '%s' for reading.",
            "Make sure the file exists and the path is correct.", filename,
            SF_MAIN_CANNOT_OPEN_FILE, (sf_span){0}, SF_SEV_FATAL, filename
        );

        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t read_size = ftell(file);
    rewind(file);
    *size = read_size;

    char *content = malloc(read_size + 1);

    if (content == NULL) {
        fclose(file);

        sf_log(
            "Insufficient Memory.", "Cannot allocate memory for compiling.",
            "Free some memory and try again.", NULL, SF_GENERAL_INSUFFICIENT_MEMORY,
            (sf_span){0}, SF_SEV_FATAL
        );

        return NULL;
    }

    fread(content, sizeof(char), read_size, file);
    content[read_size] = '\0';
    fclose(file);

    return content;
}

static bool file_write(const char *filename, const char *content) {
    FILE *file = fopen(filename, "wb");

    if (file == NULL) {
        sf_log(
            "cannot open file", "unable to open file '%s' for writing",
            "check disk space and write permissions, then try again", filename,
            SF_MAIN_CANNOT_OPEN_FILE, (sf_span){0}, SF_SEV_FATAL, filename
        );

        return false;
    }

    fwrite(content, sizeof(char), strlen(content), file);
    fclose(file);

    return true;
}
