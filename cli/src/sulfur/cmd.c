#include "sulfur/cmd.h"

#include <string.h>
#include <stdio.h>

#include "sulfur/file.h"

#define PATH_BUFFER_SIZE 256

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

static void print_version(void);

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
        "\n"
        "------------------------------------------------\n"
        "                  Sulfur Help\n"
        "------------------------------------------------\n"
        "Usage:\n"
        "   sulfur <command> [arguments]\n"
        "------------------------------------------------\n"
        "Commands:\n"
        "   new <name> - Creates a new project folder.\n"
        "   build      - Builds the project.\n"
        "   clean      - Cleans build artifacts.\n"
        "   --version  - Display compiler information.\n"
        "   --help     - Display this help message.\n"
        "------------------------------------------------\n"
        "\n"
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

}

static void command_clean(int argc, char **argv) {

}

static void print_version(void) {
    printf(
        "\n"
        "---------------------------------\n"
        "         Sulfur v0.0.0\n"
        "---------------------------------\n"
        "Copyright (C) 2026 saintsHr\n"
        "Licensed under the MIT license\n"
        "---------------------------------\n"
        "\n"
    );
}
