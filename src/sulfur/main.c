#include <stdbool.h>
#include <string.h>

#include "sulfur/util/log.h"
#include "sulfur/preprocessor.h"
#include "sulfur/lexer.h"
#include "sulfur/ast.h"
#include "sulfur/parser.h"

typedef struct {
    char* output_file;
    char* input_file;
    int   optimization_level;
    bool  warnings;
} sfCompilerOptions;

void  printTokens (sfTokenList* list);
void  parseFlags  (int argc, char* argv[], sfCompilerOptions* options);
char* readFile    (const char* filename, long* outSize);
void  writeFile   (const char* filename, const char* content);

int main(int argc, char* argv[]) {
    sfCompilerOptions options = {
        .warnings           = false,
        .input_file         = "input.in",
        .output_file        = "output.out",
        .optimization_level = 0
    };

    parseFlags(argc, argv, &options);

    long   inputSize = 0;
    char*  input     = readFile(options.input_file, &inputSize);

    char*       output = NULL;
    sfTokenList tokens = {0};
    sfASTNode*  ast    = NULL;
    
    output = preprocess(input, inputSize, options.input_file);
    tokens = tokenize(output, options.output_file);
    ast    = (sfASTNode*)parse(tokens, options.input_file);

    printTokens(&tokens);
    printf("\n\n");
    sfPrintAST(ast);

    writeFile(options.output_file, output);

    free(output);
    free(input);
    return 0;
}

void writeFile(const char* filename, const char* content) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        sfLogHelper(
            "Cannot open file.",
            "Unable to open file (%s) provided.",
            "Make sure you have enough disk sface & write permission and try again.",
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

char* readFile(const char* filename, long* outSize) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        sfLogHelper(
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
    long size = ftell(file);
    rewind(file);
    *outSize = size;

    char* content = malloc(size + 1);
    if (content == NULL) {
        fclose(file);
        sfLogHelper(
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

void printTokens(sfTokenList* list) {
    for (size_t i = 0; i < list->count; i++) {
        printf("Token %zu: type=%d value=%s (%d:%d)\n",
            i,
            list->tokens[i].type,
            list->tokens[i].value,
            list->tokens[i].line,
            list->tokens[i].column
        );
    }
}

void parseFlags(int argc, char* argv[], sfCompilerOptions* options) {
    for (int i = 1; i < argc; i++) {
        bool hasNext = i + 1 < argc;

        if (strcmp(argv[i], "-i") == 0) {
            if (!hasNext || argv[i + 1][0] == '-') {
                sfLogHelper(
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
            if (!hasNext || argv[i + 1][0] == '-') {
                sfLogHelper(
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

        if (strcmp(argv[i], "-w") == 0) {
            options->warnings = true;
            continue;
        }

        if (strncmp(argv[i], "-O", 2) == 0) {
            int level = argv[i][2] - '0';
            if (level >= 0 && level <= 3) {
                options->optimization_level = level;
                continue;
            }
        }

        sfLogHelper(
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
