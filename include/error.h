#pragma once

#include <stdint.h>
#include <stdio.h>

typedef enum {
    INFO,
    WARNING,
    ERROR,
} SEVERITY;

typedef struct {
    char* msg;
    SEVERITY sev;
} err;

static inline void printErr(err er) {
    switch (er.sev) {
        case INFO:
            printf("[ \033[1;36mINFO\033[0m ] %s\n", er.msg);
            break;
        case WARNING:
            printf("[ \033[1;33mWARNING\033[0m ] %s\n", er.msg);
            break;
        case ERROR:
            printf("[ \033[1;31mERROR\033[0m ] %s\n", er.msg);
            break;
    }
}
