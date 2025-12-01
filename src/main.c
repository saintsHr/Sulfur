#include <stdint.h>
#include "error.h"

static inline int errorExit(const char *msg) {
    err er;
    er.sev = ERROR;
    er.msg = msg;
    printErr(er);
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return errorExit("Too few arguments");
    if (argc < 2) return errorExit("Too many arguments");

    return 0;
}