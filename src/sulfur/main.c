#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

typedef struct {
  char *output_file;
  char *input_file;
} sf_compiler_options;

static void parse_flags(int argc, char *argv[], sf_compiler_options *options);
static char *read_file(const char *filename, uint32_t *out_size);
static void write_file(const char *filename, const char *content);

int main(int argc, char *argv[]) {
  sf_compiler_options options = {
      .input_file = "input.slfr",
      .output_file = "output.asm",
  };

  int status = EXIT_SUCCESS;

  parse_flags(argc, argv, &options);

  uint32_t inputSize = 0;
  char *input = read_file(options.input_file, &inputSize);

  sf_log_set_source(options.input_file, input);
  sf_log_init();

  // stage resources
  char *preprocessed = NULL;
  sf_token_list tokens = {0};
  sf_arena arena = {0};
  sf_program_node *ast = NULL;
  sf_ir_program ir = {0};
  char *assembly = NULL;

  // compilation pipeline
  sf_arena_init(&arena, 32);

  preprocessed = sf_preprocess(input, inputSize, options.input_file);
  if (sf_log_had_fatal()) {
    status = EXIT_FAILURE;
    goto cleanup;
  }

  tokens = sf_tokenize(preprocessed, options.input_file);
  if (sf_log_had_fatal()) {
    status = EXIT_FAILURE;
    goto cleanup;
  }

  ast = sf_parse(&arena, tokens, options.input_file);
  if (sf_log_had_fatal()) {
    status = EXIT_FAILURE;
    goto cleanup;
  }

  sf_analyze(ast, options.input_file);
  if (sf_log_had_fatal()) {
    status = EXIT_FAILURE;
    goto cleanup;
  }

  ir = sf_generate_ir(&arena, ast);
  if (sf_log_had_fatal()) {
    status = EXIT_FAILURE;
    goto cleanup;
  }

  assembly = sf_generate_assembly(&ir);
  if (sf_log_had_fatal()) {
    status = EXIT_FAILURE;
    goto cleanup;
  }

  bool debug = true;
  if (debug) {
    printf("%s", input);
    printf("\n\n");
    sf_print_tokens(&tokens);
    printf("\n\n");
    sf_print_ast((sf_ast_node *)ast);
    printf("\n\n");
    sf_print_ir(&ir);
    printf("\n\n");
    printf("%s", assembly);
  }

  if (sf_log_had_errors()) {
    status = EXIT_FAILURE;
    goto cleanup;
  }

  // writes to output
  write_file(options.output_file, assembly);

cleanup:
  free(assembly);
  sf_free_tokens(&tokens);
  free(preprocessed);
  sf_free_arena(&arena);
  free(input);

  return status;
}

static void parse_flags(int argc, char *argv[], sf_compiler_options *options) {
  for (int i = 1; i < argc; i++) {
    bool has_next = i + 1 < argc;

    if (strcmp(argv[i], "-i") == 0) {
      if (!has_next || argv[i + 1][0] == '-') {
        sf_log("no input file", "no input file was provided after '-i'",
               "provide a filename after '-i', or remove the flag", "N/A",
               SF_MAIN_NO_INPUT_FILE, (sf_span){0}, SF_SEV_FATAL);
      }

      options->input_file = argv[i + 1];
      i++;
      continue;
    }

    if (strcmp(argv[i], "-o") == 0) {
      if (!has_next || argv[i + 1][0] == '-') {
        sf_log("no output file", "no output file was provided after '-o'",
               "provide a filename after '-o', or remove the flag", "N/A",
               SF_MAIN_NO_OUTPUT_FILE, (sf_span){0}, SF_SEV_FATAL);
      }

      options->output_file = argv[i + 1];
      i++;
      continue;
    }

    sf_log("unknown flag", "unrecognized flag '%s'",
           "check available flags with --help, or remove this flag", "N/A",
           SF_MAIN_UNKNOWN_FLAG, (sf_span){0}, SF_SEV_FATAL, argv[i]);
  }
}

static char *read_file(const char *filename, uint32_t *out_size) {
  FILE *file = fopen(filename, "rb");

  if (file == NULL) {
    sf_log("cannot open file", "unable to open file '%s' for reading",
           "make sure the file exists and the path is correct", filename,
           SF_MAIN_CANNOT_OPEN_FILE, (sf_span){0}, SF_SEV_FATAL, filename);
  }

  fseek(file, 0, SEEK_END);
  uint32_t size = ftell(file);
  rewind(file);
  *out_size = size;

  char *content = malloc(size + 1);

  if (content == NULL) {
    fclose(file);

    sf_log("Insufficient Memory.", "Cannot allocate memory for compiling.",
           "Free some memory and try again.", NULL,
           SF_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_SEV_FATAL);
  }

  fread(content, sizeof(char), size, file);
  content[size] = '\0';

  fclose(file);
  return content;
}

void write_file(const char *filename, const char *content) {
  FILE *file = fopen(filename, "wb");

  if (file == NULL) {
    sf_log("cannot open file", "unable to open file '%s' for writing",
           "check disk space and write permissions, then try again", filename,
           SF_MAIN_CANNOT_OPEN_FILE, (sf_span){0}, SF_SEV_FATAL, filename);
  }

  fwrite(content, sizeof(char), strlen(content), file);

  fclose(file);
}