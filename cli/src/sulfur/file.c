#define _DEFAULT_SOURCE

#include "sulfur/file.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PERMS 0755
#define PATH_BUFFER_SIZE 1024

static bool is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool make_dir(const char *path) {
    if (mkdir(path, PERMS) == 0) {
        return true;
    }

    if (errno != EEXIST) {
        return false;
    }

    if (is_dir(path)) {
        return true;
    }

    errno = ENOTDIR;
    return false;
}

static void keep_errno(int *first) {
    if (*first == 0) {
        *first = errno;
    }
}

bool sf_create_folder(const char *path) {
    if (path == NULL) {
        errno = EINVAL;
        return false;
    }

    return make_dir(path);
}

bool sf_create_folders(const char *path) {
    if (path == NULL) {
        errno = EINVAL;
        return false;
    }

    char buffer[PATH_BUFFER_SIZE];

    if (strlen(path) >= sizeof(buffer)) {
        errno = ENAMETOOLONG;
        return false;
    }

    strcpy(buffer, path);

    for (char *p = buffer + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';

            if (!make_dir(buffer)) {
                return false;
            }

            *p = '/';
        }
    }

    return make_dir(buffer);
}

bool sf_delete_folder(const char *path) {
    if (path == NULL) {
        errno = EINVAL;
        return false;
    }

    DIR *dir = opendir(path);

    if (dir == NULL) {
        return false;
    }

    struct dirent *entry;
    bool success = true;
    int first_errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullpath[PATH_MAX];

        int len =
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (len < 0 || (size_t)len >= sizeof(fullpath)) {
            errno = ENAMETOOLONG;
            keep_errno(&first_errno);
            success = false;
            continue;
        }

        struct stat st;

        if (lstat(fullpath, &st) != 0) {
            keep_errno(&first_errno);
            success = false;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!sf_delete_folder(fullpath)) {
                keep_errno(&first_errno);
                success = false;
            }
        } else {
            if (!sf_delete_file(fullpath)) {
                keep_errno(&first_errno);
                success = false;
            }
        }
    }

    if (closedir(dir) != 0) {
        keep_errno(&first_errno);
        success = false;
    }

    if (!success) {
        errno = first_errno;
        return false;
    }

    return rmdir(path) == 0;
}

bool sf_create_file(const char *path) {
    if (path == NULL) {
        errno = EINVAL;
        return false;
    }

    FILE *file = fopen(path, "a");

    if (file == NULL) {
        return false;
    }

    return fclose(file) == 0;
}

bool sf_delete_file(const char *path) {
    if (path == NULL) {
        errno = EINVAL;
        return false;
    }

    return remove(path) == 0;
}

bool sf_write_file(const char *path, const char *content) {
    if (path == NULL || content == NULL) {
        errno = EINVAL;
        return false;
    }

    int fd = open(path, O_WRONLY | O_TRUNC);

    if (fd == -1) {
        return false;
    }

    const char *p = content;
    size_t remaining = strlen(content);

    while (remaining > 0) {
        ssize_t n = write(fd, p, remaining);

        if (n < 0 && errno == EINTR) {
            continue;
        }

        if (n <= 0) {
            int saved = errno;

            close(fd);
            errno = saved ? saved : EIO;
            return false;
        }

        p += n;
        remaining -= (size_t)n;
    }

    return close(fd) == 0;
}

bool sf_read_file(const char *path, char **content) {
    if (path == NULL || content == NULL) {
        errno = EINVAL;
        return false;
    }

    *content = NULL;

    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        return false;
    }

    char *buffer = NULL;
    long size;
    size_t bytes_read;

    if (fseek(file, 0, SEEK_END) != 0) {
        goto fail;
    }

    size = ftell(file);

    if (size < 0) {
        goto fail;
    }

    rewind(file);

    buffer = malloc((size_t)size + 1);

    if (buffer == NULL) {
        goto fail;
    }

    bytes_read = fread(buffer, 1, (size_t)size, file);

    if (bytes_read != (size_t)size || ferror(file)) {
        if (!ferror(file)) {
            errno = EIO;
        }

        goto fail;
    }

    buffer[bytes_read] = '\0';

    if (fclose(file) != 0) {
        int saved = errno;

        free(buffer);
        errno = saved;
        return false;
    }

    *content = buffer;
    return true;

fail:
    {
        int saved = errno;

        free(buffer);
        fclose(file);
        errno = saved;
        return false;
    }
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
    int first_errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t len = strlen(dir_path) + strlen(entry->d_name) + 2;
        char *path = malloc(len);

        if (path == NULL) {
            keep_errno(&first_errno);
            success = false;
            break;
        }

        snprintf(path, len, "%s/%s", dir_path, entry->d_name);

        struct stat st;

        if (lstat(path, &st) != 0) {
            keep_errno(&first_errno);
            free(path);
            success = false;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!collect_slfr_files(path, files)) {
                keep_errno(&first_errno);
                success = false;
            }

            free(path);
        } else if (S_ISREG(st.st_mode) && has_slfr_extension(entry->d_name)) {
            char **new_paths = realloc(
                files->paths, (files->count + 1) * sizeof(*files->paths)
            );

            if (new_paths == NULL) {
                keep_errno(&first_errno);
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
        keep_errno(&first_errno);
        success = false;
    }

    if (!success) {
        errno = first_errno;
    }

    return success;
}
