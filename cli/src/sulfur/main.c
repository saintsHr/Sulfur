#include <stdlib.h>

#include "sulfur/cmd.h"

int main(
    int argc,
    char **argv
) {
    if (!cmd_parse(argc, argv)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
