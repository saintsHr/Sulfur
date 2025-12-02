#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "error.h"

int main(int argc, char* argv[]) {

// === checking stage ========================================= //

    // checks for correct usage
    if (argc < 2) return errorExit("Too few arguments");
    if (argc > 2) return errorExit("Too many arguments");

    // checks if input file exists and open it
    FILE *input = fopen(argv[1], "r");
    if (!input) errorExit("Cannot open source file");

    // checks for correct extension (.slfr) 
    const char *ext = strrchr(argv[1], '.');
    if (ext == NULL || strcmp(ext, ".slfr") != 0) {
        fclose(input);
        errorExit("Input file is not a .slfr");
    }

    // checks if file isnt empty
    fseek(input, 0, SEEK_END);
    unsigned long inputSize = ftell(input);
    if (inputSize == 0) errorExit("Input file is empty");
    fseek(input, 0, SEEK_SET);

    // fills a buffer with the content
    char* inputBuffer = malloc(inputSize + 1);
    if (!inputBuffer) errorExit("Out of memory");
    if (fread(inputBuffer, 1, inputSize, input) != inputSize) errorExit("Cannot read source file");

// =========================================================== //



// =========================================================== //
    free(inputBuffer);
    fclose(input);
    return 0;
}