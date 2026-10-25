#include <stdarg.h>
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
#define CONTENT_BUFFER_SIZE 1024

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
static bool assemble_file(const char *input, const char *output, const char* flags);
static bool link_files(const char **inputs, size_t count, const char *output, const char *flags);

static void log_failure(const char *title, const char *msg, ...);
static void log_success(const char *msg, ...);
static void log_action(const char *action, const char *msg, ...);

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

    char main_file_content[CONTENT_BUFFER_SIZE] = "";
    char toml_file_content[CONTENT_BUFFER_SIZE] = "";

    char project_folder_path[PATH_BUFFER_SIZE] = "";
    char src_folder_path[PATH_BUFFER_SIZE] = "";
    char main_file_path[PATH_BUFFER_SIZE] = "";
    char toml_file_path[PATH_BUFFER_SIZE] = "";

    snprintf(
        toml_file_content,
        sizeof(toml_file_content),

        "[project]\n"
        "name = \"%s\"\n"
        "version = \"0.0.0\"\n"
        "\n"
        "[build]\n"
        "flags = []\n"
        "entry = \"main.slfr\"\n",

        project_name
    );

    snprintf(
        main_file_content,
        sizeof(main_file_content),

        "fn main() i32 {\n"
        "\t return 0;\n"
        "}\n"
    );

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

    log_success("Created project '%s'", project_name);
}


static void command_build(int argc, char **argv) {
    (void)argc;
    (void)argv;

    static const char *src_folder = "src";
    static const char *build_folder = "build";
    static const char *asm_folder = "build/asm";
    static const char *obj_folder = "build/obj";
    static const char *link_folder = "build/bin";
    static const char *binary_name = "binary";

    sf_files source_files = {0};
    char **object_files = NULL;
    char binary_path[PATH_BUFFER_SIZE];

    if (!collect_slfr_files(src_folder, &source_files)) {
        log_failure("Build failed.", "Failed to collect .slfr files.");
        goto cleanup;
    }

    if (source_files.count == 0) {
        log_failure("Build failed.", "No source files found.");
        goto cleanup;
    }

    if (
        !sf_create_folders(asm_folder) ||
        !sf_create_folders(obj_folder) ||
        !sf_create_folders(link_folder)
    ) {
        log_failure("Build failed.", "Failed to create build directories.");
        goto cleanup;
    }

    object_files = calloc(source_files.count, sizeof(*object_files));
    if (!object_files) {
        log_failure("Build failed.", "Failed to allocate memory.");
        goto cleanup;
    }

    for (size_t i = 0; i < source_files.count; i++) {
        const char *source = source_files.paths[i];
        const char *relative = source;

        char relative_path[PATH_BUFFER_SIZE];
        char asm_path[PATH_BUFFER_SIZE];
        char obj_path[PATH_BUFFER_SIZE];
        char directory[PATH_BUFFER_SIZE];

        if (strncmp(source, "src/", 4) == 0) {
            relative = source + 4;
        }

        snprintf(relative_path, sizeof(relative_path), "%s", relative);

        char *extension = strrchr(relative_path, '.');
        if (!extension || strcmp(extension, ".slfr") != 0) {
            log_failure("Build failed.", "Invalid source extension: '%s'", source);
            goto cleanup;
        }
        *extension = '\0';

        snprintf(
            asm_path, sizeof(asm_path), "%s/%s.asm",
            asm_folder, relative_path
        );

        snprintf(
            obj_path, sizeof(obj_path), "%s/%s.o",
            obj_folder, relative_path
        );

        const char *paths[] = {asm_path, obj_path};

        for (size_t j = 0; j < 2; j++) {
            snprintf(directory, sizeof(directory), "%s", paths[j]);

            char *slash = strrchr(directory, '/');
            if (slash) {
                *slash = '\0';

                if (!sf_create_folders(directory)) {
                    log_failure("Build failed.", "Failed to create '%s'", directory);
                    goto cleanup;
                }
            }
        }

        if (!compile_file(source, asm_path, "")) {
            log_failure("Build failed.", "Failed to compile '%s'.", source);
            goto cleanup;
        }

        log_action("Compiled", "'%s' -> '%s'", source, asm_path);

        if (!assemble_file(asm_path, obj_path, "-f elf64")) {
            log_failure("Build failed.", "Failed to assemble '%s'.", asm_path);
            goto cleanup;
        }

        log_action("Assembled", "'%s' -> '%s'", asm_path, obj_path);

        object_files[i] = strdup(obj_path);
        if (!object_files[i]) {
            log_failure("Build failed.", "Failed to allocate memory.");
            goto cleanup;
        }
    }

    snprintf(
        binary_path, sizeof(binary_path),
        "%s/%s", link_folder, binary_name
    );

    if (
        !link_files(
            (const char **)object_files,
            source_files.count, binary_path, ""
        )
    ) {
        log_failure("Build failed.", "Failed to link binary '%s'.", binary_name);
        goto cleanup;
    }

    log_action("Linked", "'%s'", binary_path);

    log_success("Build done.");

    cleanup: {
        if (object_files) {
            for (size_t i = 0; i < source_files.count; i++) {
                free(object_files[i]);
            }
            free(object_files);
        }
    }

    free_slfr_files(&source_files);
}

static void command_clean(int argc, char **argv) {
    static const char* build_folder_path = "build";

    if (sf_delete_folder(build_folder_path)) {
        log_success("Clean done.");
    } else {
        log_failure("Clean failed.", "No build artifacts found.");
        return;
    }
}

static bool compile_file(const char *input, const char *output, const char* flags) {
    static const char *compiler_cmd = "sulfur-compiler";

    char cmd[CMD_BUFFER_SIZE];

    snprintf(
        cmd, sizeof(cmd),
        "%s -i \"%s\" -o \"%s\" %s",
        compiler_cmd, input, output, flags
    );

    return system(cmd) == 0;
}

static bool assemble_file(const char *input, const char *output, const char* flags) {
    static const char *assembler_cmd = "nasm";

    char cmd[CMD_BUFFER_SIZE];

    snprintf(
        cmd, sizeof(cmd),
        "%s \"%s\" -o \"%s\" %s",
        assembler_cmd, input, output, flags
    );

    return system(cmd) == 0;
}

static bool link_files(const char **inputs, size_t count, const char *output, const char *flags) {
    size_t size = 32 + strlen(output) + (flags ? strlen(flags) : 0);

    for (size_t i = 0; i < count; i++) {
        size += strlen(inputs[i]) + 3;
    }

    char *command = malloc(size);
    if (!command) {
        return false;
    }

    snprintf(command, size, "ld");

    for (size_t i = 0; i < count; i++) {
        strcat(command, " \"");
        strcat(command, inputs[i]);
        strcat(command, "\"");
    }

    strcat(command, " -o \"");
    strcat(command, output);
    strcat(command, "\"");

    if (flags && *flags) {
        strcat(command, " ");
        strcat(command, flags);
    }

    bool status = system(command) == 0;
    free(command);

    return status;
}

static void log_failure(const char *title, const char *msg, ...) {
    printf(SF_COLOR_RED "%s\n" SF_COLOR_RESET "   ", title);

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    putchar('\n');
}

static void log_success(const char *msg, ...) {
    printf(SF_COLOR_GREEN);

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf(SF_COLOR_RESET "\n");
}

static void log_action(const char *action, const char *msg, ...) {
    printf(SF_COLOR_CYAN "[%s] " SF_COLOR_RESET, action);

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    putchar('\n');
}
