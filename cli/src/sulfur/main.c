#include "sulfur/file.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PATH_BUFFER_SIZE 256

#define CMD_STRING_HELP "--help"
#define CMD_STRING_VERSION "--version"

#define CMD_STRING_NEW "new"
#define CMD_STRING_BUILD "build"
#define CMD_STRING_CLEAN "clean"

static bool parse_cmd(int argc, char **argv);

static void cmd_help(void);
static void cmd_version(int argc);

static void cmd_new(int argc, char **argv);
static void cmd_build(int argc, char **argv);
static void cmd_clean(int argc, char **argv);

static void print_usage(void);

int main(int argc, char **argv) {
    if (!parse_cmd(argc, argv)){
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static bool parse_cmd(int argc, char **argv) {
    if (argc < 2) {
        return false;
    }

    const char* cmd = argv[1];

    if (strcmp(cmd, CMD_STRING_HELP) == 0) {
        cmd_help();
        return true;
    }
    if (strcmp(cmd, CMD_STRING_VERSION) == 0) {
        cmd_version(argc);
        return true;
    }

    if (strcmp(cmd, CMD_STRING_NEW) == 0) {
        cmd_new(argc, argv);
        return true;
    }
    if (strcmp(cmd, CMD_STRING_BUILD) == 0) {
        cmd_build(argc, argv);
        return true;
    }
    if (strcmp(cmd, CMD_STRING_CLEAN) == 0) {
        cmd_clean(argc, argv);
        return true;
    }

    return false;
}

static void cmd_help(void) {
    print_usage();
}

static void cmd_version(int argc) {
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

static void cmd_build(int argc, char **argv) {

}

static void cmd_clean(int argc, char **argv) {

}

static void print_usage(void) {
    printf(
        "\n"
        "sulfur - Sulfur CLI\n"
        "\n"
        "Usage:\n"
        "\tsulfur <command> [arguments]\n"
        "\n"
        "Commands:\n"
        "\tnew <name>  Creates a new project folder.\n"
        "\t--version   Display compiler information.\n"
        "\t--help      Display this help message.\n"
        "\n"
    );
}
