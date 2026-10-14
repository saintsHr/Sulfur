#include "sulfur/file.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PATH_BUFFER_SIZE 256

#define CMD_STRING_NEW "new"
#define CMD_STRING_VERSION "--version"

static void print_usage(void);
static void parse_cmd(int argc, char **argv);

static void cmd_new(int argc, char **argv);
static void cmd_version(int argc, char **argv);

int main(int argc, char **argv) {
    if (argc < 2) return EXIT_FAILURE;

    parse_cmd(argc, argv);

    return EXIT_SUCCESS;
}

static void parse_cmd(int argc, char **argv) {
    const char* cmd = argv[1];

    if (strcmp(cmd, CMD_STRING_NEW) == 0) {
        cmd_new(argc, argv);
    }
    if (strcmp(cmd, CMD_STRING_VERSION) == 0) {
        cmd_version(argc, argv);
    }
}

static void cmd_new(int argc, char **argv) {
    if (argc != 3) {
        print_usage();
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

static void cmd_version(int argc, char **argv) {
    if (argc != 2) {
        print_usage();
        return;
    }

    printf(
        "\n"
        "sulfur (Sulfur) v0.0.0\n"
        "Copyright (C) 2026 saintsHr\n"
        "Licensed under the MIT license\n"
        "\n"
    );
}

static void print_usage(void) {

}
