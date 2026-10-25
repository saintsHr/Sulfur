#define _DEFAULT_SOURCE

#include "sulfur/cmd.h"

#include <stddef.h>
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
#define CMD_BUFFER_SIZE 1024

#define CMD_STRING_HELP "--help"
#define CMD_STRING_VERSION "--version"

#define CMD_STRING_NEW "new"
#define CMD_STRING_BUILD "build"
#define CMD_STRING_CLEAN "clean"

static void command_help(int argc);
static void command_version(int argc);
static void command_new(int argc, char **argv);
static void command_build(int argc, char **argv);
static void command_clean(int argc, char **argv);

static bool compile_file(const char *input, const char *output, const char* flags);

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

    printf(
        SF_COLOR_CYAN "Version v0.0.0\n" SF_COLOR_RESET
        "Copyright (C) 2026 " SF_COLOR_GREEN "saintsHr\n" SF_COLOR_RESET
        "Licensed under the " SF_COLOR_YELLOW "MIT license\n" SF_COLOR_RESET
    );
}

static void command_new(int argc, char **argv) {
    if (argc != 3) {
        cmd_print_usage();
        return;
    }

    const char *project_name = argv[2];

    static const char *src_folder_name = "src";
    static const char *main_file_name = "main.slfr";
    static const char *toml_file_name = "sulfur.toml";

    static const char *main_file_content =
        "fn main() i32 {\n"
        "\treturn 0;\n"
        "}\n"
    ;

    char toml_file_content[1024];

    snprintf(
        toml_file_content,
        sizeof(toml_file_content),

        "[project]\n"
        "name = \"%s\"\n"
        "version = \"1.0.0\"\n"
        "\n"
        "[build]\n"
        "flags = []\n"
        "entry = \"src/main.slfr\"\n",

        project_name
    );

    char project_folder_path[PATH_BUFFER_SIZE] = "";
    char src_folder_path[PATH_BUFFER_SIZE] = "";
    char main_file_path[PATH_BUFFER_SIZE] = "";
    char toml_file_path[PATH_BUFFER_SIZE] = "";

    snprintf(project_folder_path, sizeof(project_folder_path), "%s", project_name);
    snprintf(src_folder_path, sizeof(src_folder_path), "%s/%s", project_folder_path, src_folder_name);
    snprintf(main_file_path, sizeof(main_file_path), "%s/%s", src_folder_path, main_file_name);
    snprintf(toml_file_path, sizeof(toml_file_path), "%s/%s", project_folder_path, toml_file_name);

    sf_create_folder(project_folder_path);
    sf_create_folder(src_folder_path);

    sf_create_file(main_file_path);
    sf_create_file(toml_file_path);

    sf_write_file(main_file_path, main_file_content);
    sf_write_file(toml_file_path, toml_file_content);

    printf(
        SF_COLOR_GREEN "Created project '%s'\n" SF_COLOR_RESET,
        project_name
    );
}


static void command_build(int argc, char **argv) {
    static const char *src_folder_name = "src";
    static const char *build_folder_name = "build";
    static const char *asm_file_extension = ".asm";

    sf_files source_files = {0};

    if (!collect_slfr_files(src_folder_name, &source_files)) {
        printf(
            SF_COLOR_RED "Build failed.\n"
            SF_COLOR_RESET "   Failed to collect .slfr files in source folder.\n"
        );

        free_slfr_files(&source_files);
        return;
    }

    if (!sf_create_folder(build_folder_name)) {
        printf(
            SF_COLOR_RED "Build failed.\n"
            SF_COLOR_RESET "   Failed to create directory for artifacts.\n"
        );

        free_slfr_files(&source_files);
        return;
    }

    for (size_t i = 0; i < source_files.count; i++) {
        const char *source_file_path = source_files.paths[i];
        const char *relative_file_path = source_file_path;

        char output_file_path[PATH_BUFFER_SIZE] = "";
        char output_folder_path[PATH_BUFFER_SIZE] = "";

        if (strncmp(source_file_path, "src/", 4) == 0) {
            relative_file_path = source_file_path + 4;
        }

        snprintf(
            output_file_path,
            sizeof(output_file_path),
            "%s/%s%s",
            build_folder_name,
            relative_file_path,
            asm_file_extension
        );

        snprintf(
            output_folder_path,
            sizeof(output_folder_path),
            "%s",
            output_file_path
        );

        char *last_slash = strrchr(output_folder_path, '/');

        if (last_slash != NULL) {
            *last_slash = '\0';

            if (!sf_create_folders(output_folder_path)) {
                fprintf(
                    stderr,
                    "Failed to create directory: '%s'\n",
                    output_folder_path
                );

                free_slfr_files(&source_files);
                return;
            }
        }

        if (!compile_file(source_file_path, output_file_path, "")) {
            printf(
                SF_COLOR_RED "Build failed.\n"
                SF_COLOR_RESET "   Failed to compile '%s'.\n",
                source_file_path
            );

            free_slfr_files(&source_files);
            return;
        }

        printf(
            SF_COLOR_CYAN "[Compiled] " SF_COLOR_RESET "'%s' -> '%s'\n",
            source_file_path,
            output_file_path
        );
    }

    free_slfr_files(&source_files);

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

static bool compile_file(const char *input, const char *output, const char* flags) {
    char cmd[CMD_BUFFER_SIZE];

    snprintf(
        cmd, sizeof(cmd),
        "sulfur-compiler -i \"%s\" -o \"%s\" %s",
        input, output, flags
    );

    return system(cmd) == 0;
}
