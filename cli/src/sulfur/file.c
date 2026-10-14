#include "sulfur/file.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#define PERMS 0755

bool sf_create_folder(const char *path) {
    return mkdir(path, PERMS) == 0;
}

bool sf_delete_folder(const char *path) {
    return rmdir(path) == 0;
}

bool sf_create_file(const char *path) {
    FILE *file = fopen(path, "a");

    if (file == NULL) {
        return false;
    }

    return fclose(file) == 0;
}

bool sf_delete_file(const char *path) {
    return remove(path) == 0;
}

bool sf_write_file(const char *path, const char *content) {
    if (path == NULL || content == NULL) {
        return false;
    }

    int fd = open(path, O_WRONLY | O_TRUNC);

    if (fd == -1) {
        return false;
    }

    const char *p = content;
    size_t remaining = 0;

    while (content[remaining] != '\0') {
        remaining++;
    }

    while (remaining > 0) {
        ssize_t n = write(fd, p, remaining);

        if (n < 0 && errno == EINTR) {
            continue;
        }

        if (n <= 0) {
            close(fd);
            return false;
        }

        p += n;
        remaining -= (size_t)n;
    }

    return close(fd) == 0;
}

bool sf_read_file(const char *path, char **content) {
    if (path == NULL || content == NULL) {
        return false;
    }

    *content = NULL;

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return false;
    }

    rewind(file);

    char *buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return false;
    }

    size_t bytes_read = fread(buffer, 1, (size_t)size, file);

    if (bytes_read != (size_t)size || ferror(file)) {
        free(buffer);
        fclose(file);
        return false;
    }

    buffer[bytes_read] = '\0';

    if (fclose(file) != 0) {
        free(buffer);
        return false;
    }

    *content = buffer;
    return true;
}
