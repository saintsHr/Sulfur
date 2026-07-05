#include "sulfur/util/log.h"
#include "sulfur/pipeline/codegen.h"
#include "sulfur/pipeline/semantic.h"
#include "sulfur/pipeline/preprocessor.h"
#include "sulfur/pipeline/lexer.h"
#include "sulfur/pipeline/ast.h"
#include "sulfur/pipeline/parser.h"
#include "sulfur/pipeline/ir.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char* output_file;
    char* input_file;
} sf_compiler_options;

static void parse_flags(int argc, char* argv[], sf_compiler_options* options);
static char* read_file(const char* filename, uint32_t* out_size);
static void write_file(const char* filename, const char* content);

int main(int argc, char* argv[]) {
    sf_compiler_options options = {
        .input_file         = "input.slfr",
        .output_file        = "output.asm",
    };

    parse_flags(argc, argv, &options);

    uint32_t inputSize = 0;
    char* input = read_file(options.input_file, &inputSize);

    char* output = NULL;

    sf_log_set_source(options.input_file, input);
    sf_log_init();

    // stages
    sf_token_list tokens = {0};
    sf_program_node* ast = NULL;
    sf_ir_program ir = {0};
    char* assembly = NULL;
    
    // compilation pipeline
    output = sf_preprocess(input, inputSize, options.input_file);
    if (sf_log_had_fatal()) return EXIT_FAILURE;

    tokens = sf_tokenize(output, options.input_file);
    if (sf_log_had_fatal()) return EXIT_FAILURE;

    ast = sf_parse(tokens, options.input_file);
    if (sf_log_had_fatal()) return EXIT_FAILURE;

    sf_analyze(ast, options.input_file);
    if (sf_log_had_fatal()) return EXIT_FAILURE;

    ir = sf_generate_ir(ast);
    if (sf_log_had_fatal()) return EXIT_FAILURE;

    assembly = sf_generate_assembly(&ir);
    if (sf_log_had_fatal()) return EXIT_FAILURE;

    // debug
    printf("%s", input);
    printf("\n\n");
    sf_print_tokens(&tokens);
    printf("\n\n");
    sf_print_ast((sf_ast_node*)ast);
    printf("\n\n");
    sf_print_ir(&ir);
    printf("\n\n");
    printf("%s", assembly);

    if (sf_log_had_errors()) return EXIT_FAILURE;

    // writes to output
    output = assembly;
    write_file(options.output_file, output);

    sf_free_ast((sf_ast_node*)ast);
    sf_free_ir(&ir);
    free(output);
    free(input);
    
    return 0;
}

static void parse_flags(int argc, char* argv[], sf_compiler_options* options) {
    for (int i = 1; i < argc; i++) {
        bool has_next = i + 1 < argc;

        if (strcmp(argv[i], "-i") == 0) {
            if (!has_next || argv[i + 1][0] == '-') {
                sf_log(
                    "no input file",
                    "no input file was provided after '-i'",
                    "provide a filename after '-i', or remove the flag",
                    "N/A",
                    SF_MAIN_NO_INPUT_FILE,
                    (sf_span){0},
                    SF_SEV_FATAL
                );
            }

            options->input_file = argv[i + 1];
            i++;
            continue;
        }

        if (strcmp(argv[i], "-o") == 0) {
            if (!has_next || argv[i + 1][0] == '-') {
                sf_log(
                    "no output file",
                    "no output file was provided after '-o'",
                    "provide a filename after '-o', or remove the flag",
                    "N/A",
                    SF_MAIN_NO_OUTPUT_FILE,
                    (sf_span){0},
                    SF_SEV_FATAL
                );
            }

            options->output_file = argv[i + 1];
            i++;
            continue;
        }

        sf_log(
            "unknown flag",
            "unrecognized flag '%s'",
            "check available flags with --help, or remove this flag",
            "N/A",
            SF_MAIN_UNKNOWN_FLAG,
            (sf_span){0},
            SF_SEV_FATAL,
            argv[i]
        );
    }
}


static char* read_file(const char* filename, uint32_t* out_size) {
    FILE* file = fopen(filename, "rb");

    if (file == NULL) {
        sf_log(
            "cannot open file",
            "unable to open file '%s' for reading",
            "make sure the file exists and the path is correct",
            filename,
            SF_MAIN_CANNOT_OPEN_FILE,
            (sf_span){0},
            SF_SEV_FATAL,
            filename
        );
    }

    fseek(file, 0, SEEK_END);
    uint32_t size = ftell(file);
    rewind(file);
    *out_size = size;

    char* content = malloc(size + 1);

    if (content == NULL) {
        fclose(file);
        
        sf_log(
            "memory allocation failed",
            "failed to allocate buffer to read file '%s'",
            "free up memory and try again",
            filename,
            SF_MAIN_CANNOT_MALLOC_FILE_BUFFER,
            (sf_span){0},
            SF_SEV_FATAL,
            filename
        );
    }

    fread(content, sizeof(char), size, file);
    content[size] = '\0';
    
    fclose(file);
    return content;
}

void write_file(const char* filename, const char* content) {
    FILE* file = fopen(filename, "wb");

    if (file == NULL) {
        sf_log(
            "cannot open file",
            "unable to open file '%s' for writing",
            "check disk space and write permissions, then try again",
            filename,
            SF_MAIN_CANNOT_OPEN_FILE,
            (sf_span){0},
            SF_SEV_FATAL,
            filename
        );
    }

    fwrite(content, sizeof(char), strlen(content), file);

    fclose(file);
}