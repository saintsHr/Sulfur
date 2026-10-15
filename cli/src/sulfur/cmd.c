#define _DEFAULT_SOURCE

#include "sulfur/cmd.h"

#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "sulfur/file.h"

#define SF_COLOR_RESET "\033[0m"
#define SF_COLOR_BOLD "\033[1m"

#define SF_COLOR_CYAN "\033[36m"
#define SF_COLOR_GREEN "\033[32m"
#define SF_COLOR_YELLOW "\033[33m"
#define SF_COLOR_RED "\033[31m"
#define SF_COLOR_GRAY "\033[37m"

#define PATH_BUFFER_SIZE 256

#define CMD_STRING_HELP "--help"
#define CMD_STRING_VERSION "--version"

#define CMD_STRING_NEW "new"
#define CMD_STRING_BUILD "build"
#define CMD_STRING_CLEAN "clean"

typedef struct {
    char **paths;
    size_t count;
} source_files_t;

static void command_help(int argc);
static void command_version(int argc);

static void command_new(int argc, char **argv);
static void command_build(int argc, char **argv);
static void command_clean(int argc, char **argv);

static void print_version(void);

static bool has_slfr_extension(const char *name);

static bool collect_slfr_files(const char *path, source_files_t *files);
static void free_slfr_files(source_files_t *files);

bool cmd_parse(int argc, char **argv) {
    if (argc < 2) {
        return false;
    }

    const char* cmd = argv[1];

    if (strcmp(cmd, CMD_STRING_HELP) == 0) {
        command_help(argc);
        return true;
    }
    if (strcmp(cmd, CMD_STRING_VERSION) == 0) {
        command_version(argc);
        return true;
    }

    if (strcmp(cmd, CMD_STRING_NEW) == 0) {
        command_new(argc, argv);
        return true;
    }
    if (strcmp(cmd, CMD_STRING_BUILD) == 0) {
        command_build(argc, argv);
        return true;
    }
    if (strcmp(cmd, CMD_STRING_CLEAN) == 0) {
        command_clean(argc, argv);
        return true;
    }

    return false;
}

void cmd_print_usage(void) {
    printf(
        SF_COLOR_BOLD SF_COLOR_CYAN "Usage:\n" SF_COLOR_RESET
        "   sulfur <command> [arguments]\n"

        SF_COLOR_BOLD SF_COLOR_CYAN "Commands:\n" SF_COLOR_RESET

        "   " SF_COLOR_GREEN "new" SF_COLOR_RESET
        " <name> - Creates a new project folder.\n"

        "   " SF_COLOR_GREEN "build" SF_COLOR_RESET
        "      - Builds the project.\n"

        "   " SF_COLOR_GREEN "clean" SF_COLOR_RESET
        "      - Cleans build artifacts.\n"

        "   " SF_COLOR_YELLOW "--version" SF_COLOR_RESET
        "  - Display compiler information.\n"

        "   " SF_COLOR_YELLOW "--help" SF_COLOR_RESET
        "     - Display this help message.\n"
    );
}

static void command_help(int argc) {
    if (argc != 2) {
        cmd_print_usage();
        return;
    }

    cmd_print_usage();
}

static void command_version(int argc) {
    if (argc != 2) {
        cmd_print_usage();
        return;
    }

    print_version();
}

static void command_new(int argc, char **argv) {
    if (argc != 3) {
        cmd_print_usage();
        return;
    }

    static const char* src_folder_name = "src";
    static const char* main_file_name = "main.slfr";
    static const char* toml_file_name = "sulfur.toml";

    static const char* main_file_content =
        "fn main() void {\n"
        "\t\n"
        "}\n"
    ;

    char project_folder_path[PATH_BUFFER_SIZE] = "";
    char src_folder_path[PATH_BUFFER_SIZE] = "";
    char main_file_path[PATH_BUFFER_SIZE] = "";
    char toml_file_path[PATH_BUFFER_SIZE] = "";

    snprintf(project_folder_path, sizeof(project_folder_path), "%s", argv[2]);
    snprintf(src_folder_path, sizeof(src_folder_path), "%s/%s", project_folder_path, src_folder_name);
    snprintf(main_file_path, sizeof(main_file_path), "%s/%s", src_folder_path, main_file_name);
    snprintf(toml_file_path, sizeof(toml_file_path), "%s/%s", project_folder_path, toml_file_name);

    sf_create_folder(project_folder_path);
    sf_create_folder(src_folder_path);

    sf_create_file(main_file_path);
    sf_create_file(toml_file_path);

    sf_write_file(main_file_path, main_file_content);
}

static void command_build(int argc, char **argv) {
    static const char *build_folder_path = "build";

    source_files_t files = {0};

    if (!collect_slfr_files("src", &files)) {
        printf(
            SF_COLOR_RED "Build failed.\n"
            SF_COLOR_RESET "   Failed to collect .slfr files in source folder.\n"
        );

        free_slfr_files(&files);
        return;
    }

    if (!sf_create_folder(build_folder_path)) {
        printf(
            SF_COLOR_RED "Build failed.\n"
            SF_COLOR_RESET "   Failed to create directory for artifacts.\n"
        );

        free_slfr_files(&files);
        return;
    }

    // compile here

    free_slfr_files(&files);

    printf(
        SF_COLOR_GREEN "Build done.\n"
        SF_COLOR_RESET
    );
}

static void command_clean(int argc, char **argv) {
    static const char* build_folder_path = "build";

    if (sf_delete_folder(build_folder_path)) {
        printf(
            SF_COLOR_GREEN "Clean done.\n"
            SF_COLOR_RESET
        );
    } else {
        printf(
            SF_COLOR_RED "Clean failed.\n"
            SF_COLOR_RESET "   Check if theres any build artifacts on current directory.\n"
        );

        return;
    }
}

static void print_version(void) {
    printf(
        SF_COLOR_CYAN "Version v0.0.0\n" SF_COLOR_RESET
        "Copyright (C) 2026 " SF_COLOR_GREEN "saintsHr\n" SF_COLOR_RESET
        "Licensed under the " SF_COLOR_YELLOW "MIT license\n" SF_COLOR_RESET
    );
}

static void free_slfr_files(source_files_t *files) {
    for (size_t i = 0; i < files->count; i++) {
        free(files->paths[i]);
    }

    free(files->paths);
    files->paths = NULL;
    files->count = 0;
}

static bool has_slfr_extension(const char *name) {
    const char *ext = strrchr(name, '.');
    return ext != NULL && strcmp(ext, ".slfr") == 0;
}

static bool collect_slfr_files(const char *dir_path, source_files_t *files) {
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
