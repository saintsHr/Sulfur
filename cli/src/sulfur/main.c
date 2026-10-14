#include "sulfur/cmd.h"
#include <stdlib.h>

int main(int argc, char **argv) {
    if (!cmd_parse(argc, argv)){
        cmd_print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
