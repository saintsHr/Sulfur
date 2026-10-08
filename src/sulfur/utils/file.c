#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/utils/file.h"

#include "sulfur/utils/log.h"

char *sf_file_read(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        sf_log(
            "Cannot open file.", "Unable to open file '%s' for reading.",
            "Make sure the file exists and the path is correct.", filename,
            SF_LOG_MAIN_CANNOT_OPEN_FILE, (sf_span){0}, SF_LOG_SEVERITY_FATAL, filename
        );

        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t read_size = ftell(file);
    rewind(file);
    *size = read_size;

    char *content = malloc(read_size + 1);

    if (content == NULL) {
        fclose(file);

        sf_log(
            "Insufficient Memory.", "Cannot allocate memory for compiling.",
            "Free some memory and try again.", NULL, SF_LOG_GENERAL_INSUFFICIENT_MEMORY,
            (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );

        return NULL;
    }

    fread(content, sizeof(char), read_size, file);
    content[read_size] = '\0';
    fclose(file);

    return content;
}

bool sf_file_write(const char *filename, const char *content) {
    FILE *file = fopen(filename, "wb");

    if (file == NULL) {
        sf_log(
            "cannot open file", "unable to open file '%s' for writing",
            "check disk space and write permissions, then try again", filename,
            SF_LOG_MAIN_CANNOT_OPEN_FILE, (sf_span){0}, SF_LOG_SEVERITY_FATAL, filename
        );

        return false;
    }

    fwrite(content, sizeof(char), strlen(content), file);
    fclose(file);

    return true;
}
