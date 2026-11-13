#include "sulfur/utils/file.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sulfur/utils/log.h"

char *sf_file_read(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        sf_log(
            "File Open Failed.", "Unable to open file '%s' for reading.",
            "Verify that the file exists, the path is correct, and read "
            "permissions are granted.",
            filename, SF_LOG_DRIVER_CANNOT_OPEN_FILE, (sf_span){0},
            SF_LOG_SEVERITY_FATAL, filename
        );

        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t read_size = (size_t)ftell(file);
    rewind(file);
    *size = read_size;

    char *content = malloc(read_size + 2);

    if (content == NULL) {
        fclose(file);

        sf_log_out_of_memory("N/A");

        return NULL;
    }

    size_t read_result = fread(content, sizeof(char), read_size, file);
    if (read_result < read_size || read_result == 0) {
        return NULL;
    }

    if (read_size == 0 || content[read_size - 1] != '\n') {
        content[read_size++] = '\n';
    }
    content[read_size] = '\0';

    fclose(file);
    return content;
}

bool sf_file_write(const char *filename, const char *content) {
    FILE *file = fopen(filename, "wb");

    if (file == NULL) {
        sf_log(
            "File Open Failed.", "Unable to open file '%s' for writing.",
            "Verify that the destination path is valid, the directory exists, "
            "and write permissions are granted.",
            filename, SF_LOG_DRIVER_CANNOT_OPEN_FILE, (sf_span){0},
            SF_LOG_SEVERITY_FATAL, filename
        );

        return false;
    }

    fwrite(content, sizeof(char), strlen(content), file);
    fclose(file);

    return true;
}

bool sf_file_make_parents(const char *path) {
    char *tmp = malloc(strlen(path) + 1);
    if (tmp == NULL) {
        sf_log_out_of_memory("N/A");

        return false;
    }

    strcpy(tmp, path);

    for (char *p = tmp + 1; *p != '\0'; p++) {
        if (*p != '/') {
            continue;
        }

        *p = '\0';
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            free(tmp);
            return false;
        }
        *p = '/';
    }

    free(tmp);
    return true;
}
