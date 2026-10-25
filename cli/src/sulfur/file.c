#define _DEFAULT_SOURCE

#include "sulfur/file.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

#define PERMS 0755
#define PATH_BUFFER_SIZE 256

bool sf_create_folder(const char *path) {
    if (mkdir(path, PERMS) == 0) {
        return true;
    }

    if (errno == EEXIST) {
        struct stat st;
        return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
    }

    return false;
}

bool sf_create_folders(const char *path) {
    char buffer[PATH_BUFFER_SIZE];

    if (strlen(path) >= sizeof(buffer)) {
        return false;
    }

    strcpy(buffer, path);

    for (char *p = buffer + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';

            if (mkdir(buffer, 0755) != 0 && errno != EEXIST) {
                return false;
            }

            *p = '/';
        }
    }

    if (mkdir(buffer, 0755) != 0 && errno != EEXIST) {
        return false;
    }

    return true;
}

bool sf_delete_folder(const char *path) {
    if (path == NULL) {
        return false;
    }

    DIR *dir = opendir(path);
    if (dir == NULL) {
        return false;
    }

    struct dirent *entry;
    bool success = true;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullpath[PATH_MAX];

        int len = snprintf(
            fullpath,
            sizeof(fullpath),
            "%s/%s",
            path,
            entry->d_name
        );

        if (len < 0 || (size_t)len >= sizeof(fullpath)) {
            success = false;
            continue;
        }

        struct stat st;

        if (lstat(fullpath, &st) != 0) {
            success = false;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!sf_delete_folder(fullpath)) {
                success = false;
            }
        } else {
            if (!sf_delete_file(fullpath)) {
                success = false;
            }
        }
    }

    if (closedir(dir) != 0) {
        success = false;
    }

    if (!success) {
        return false;
    }

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

void free_slfr_files(sf_files *files) {
    for (size_t i = 0; i < files->count; i++) {
        free(files->paths[i]);
    }

    free(files->paths);
    files->paths = NULL;
    files->count = 0;
}

bool has_slfr_extension(const char *path) {
    const char *ext = strrchr(path, '.');
    return ext != NULL && strcmp(ext, ".slfr") == 0;
}

bool collect_slfr_files(const char *dir_path, sf_files *files) {
    DIR *dir = opendir(dir_path);

    if (dir == NULL) {
        return false;
    }

    struct dirent *entry;
    bool success = true;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t len = strlen(dir_path) + strlen(entry->d_name) + 2;
        char *path = malloc(len);

        if (path == NULL) {
            success = false;
            break;
        }

        snprintf(path, len, "%s/%s", dir_path, entry->d_name);

        struct stat st;

        if (lstat(path, &st) != 0) {
            free(path);
            success = false;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!collect_slfr_files(path, files)) {
                success = false;
            }

            free(path);
        } else if (S_ISREG(st.st_mode) &&
                   has_slfr_extension(entry->d_name)) {
            char **new_paths = realloc(
                files->paths,
                (files->count + 1) * sizeof(*files->paths)
            );

            if (new_paths == NULL) {
                free(path);
                success = false;
                break;
            }

            files->paths = new_paths;
            files->paths[files->count++] = path;
        } else {
            free(path);
        }
    }

    if (closedir(dir) != 0) {
        success = false;
    }

    return success;
}
