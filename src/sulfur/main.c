#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sulfur/codegen.h"
#include "sulfur/ir.h"
#include "sulfur/semantic.h"
#include "sulfur/util/log.h"
#include "sulfur/preprocessor.h"
#include "sulfur/lexer.h"
#include "sulfur/ast.h"
#include "sulfur/parser.h"
#include "sulfur/ir.h"

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

    // stages
    sf_token_list tokens = {0};
    sf_program_node* ast = NULL;
    sf_ir_program ir = {0};
    char* assembly = NULL;
    
    // compilation pipeline
    output = sf_preprocess(input, inputSize, options.input_file);
    tokens = sf_tokenize(output, options.input_file);
    ast = sf_parse(tokens, options.input_file);
    sf_analyze(ast, options.input_file);
    ir = sf_generate_ir(ast);
    assembly = sf_generate_assembly(&ir);

    // debug
    sf_print_tokens(&tokens);
    printf("\n\n");
    sf_print_ast((sf_ast_node*)ast);
    printf("\n\n");
    sf_print_ir(&ir);
    printf("\n\n");
    printf("%s", assembly);

    // writes to output
    output = assembly;
    write_file(options.output_file, output);

    free(output);
    free(input);
    
    return 0;
}

static void parse_flags(int argc, char* argv[], sf_compiler_options* options) {
    for (int i = 1; i < argc; i++) {
        bool has_next = i + 1 < argc;

        if (strcmp(argv[i], "-i") == 0) {
            if (!has_next || argv[i + 1][0] == '-') {
                sf_log_helper(
                    "No input file",
                    "Input file not provided",
                    "Choose a input file or remove the '-i' flag",
                    "N/A",
                    SF_MAIN_NO_INPUT_FILE,
                    0,
                    0,
                    SF_SEV_FATAL
                );
            }

            options->input_file = argv[i + 1];
            i++;
            continue;
        }

        if (strcmp(argv[i], "-o") == 0) {
            if (!has_next || argv[i + 1][0] == '-') {
                sf_log_helper(
                    "No output file",
                    "Output file not provided",
                    "Choose a output file or remove the '-o' flag",
                    "N/A",
                    SF_MAIN_NO_OUTPUT_FILE,
                    0,
                    0,
                    SF_SEV_FATAL
                );
            }

            options->output_file = argv[i + 1];
            i++;
            continue;
        }

        sf_log_helper(
            "Unknown flag",
            "An unknown flag (%s) has been provided.",
            "Try using the --help flag or removing this flag.",
            "N/A",
            SF_MAIN_UNKNOWN_FLAG,
            0,
            0,
            SF_SEV_FATAL,
            argv[i]
        );
    }
}


static char* read_file(const char* filename, uint32_t* out_size) {
    FILE* file = fopen(filename, "rb");

    if (file == NULL) {
        sf_log_helper(
            "Cannot open file",
            "Unable to open file (%s) provided.",
            "Make sure you are providing the correct filename",
            filename,
            SF_MAIN_CANNOT_OPEN_FILE,
            0,
            0,
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
        
        sf_log_helper(
            "Memory allocation failed",
            "File read memory allocation failed.",
            "Make sure you have enough memory and try again.",
            filename,
            SF_MAIN_CANNOT_MALLOC_FILE_BUFFER,
            0,
            0,
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
        sf_log_helper(
            "Cannot open file.",
            "Unable to open file (%s) provided.",
            "Make sure you have enough disk space & write permission and try again.",
            filename,
            SF_MAIN_CANNOT_OPEN_FILE,
            0,
            0,
            SF_SEV_FATAL,
            filename
        );
    }

    fwrite(content, sizeof(char), strlen(content), file);

    fclose(file);
}