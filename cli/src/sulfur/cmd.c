#define _DEFAULT_SOURCE

#include "sulfur/cmd.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "sulfur/file.h"
#include "sulfur/toml.h"

#define SF_COLOR_RESET "\033[0m"
#define SF_COLOR_BOLD "\033[1m"

#define SF_COLOR_CYAN "\033[36m"
#define SF_COLOR_GREEN "\033[32m"
#define SF_COLOR_YELLOW "\033[33m"
#define SF_COLOR_RED "\033[31m"

#define PATH_BUFFER_SIZE 1024
#define CONTENT_BUFFER_SIZE 1024

#define CMD_STRING_HELP "--help"
#define CMD_STRING_VERSION "--version"

#define CMD_STRING_NEW "new"
#define CMD_STRING_BUILD "build"
#define CMD_STRING_CLEAN "clean"
#define CMD_STRING_RUN "run"

#define EXEC_NOT_FOUND 127

#define SOURCE_EXTENSION ".slfr"
#define ENTRY_FILE_NAME "main.slfr"

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} sf_args;

static bool args_push_owned(sf_args *args, char *item);
static bool args_push(sf_args *args, const char *item);
static bool args_push_words(sf_args *args, const char *text);
static bool args_push_list(sf_args *args, const sf_string_list *list);
static void args_free(sf_args *args);

static int spawn(const sf_args *args);
static bool run_ok(const sf_args *args);

static void command_help(int argc, char **argv);
static void command_version(int argc, char **argv);
static void command_new(int argc, char **argv);
static void command_build(int argc, char **argv);
static void command_clean(int argc, char **argv);
static void command_run(int argc, char **argv);

static bool compile_file(
    const sf_project_cfg *cfg, const char *input,
    const char *output, bool is_module
);
static bool assemble_file(
    const sf_project_cfg *cfg, const char *input, const char *output
);
static bool link_files(
    const sf_project_cfg *cfg, char **objects, size_t count, const char *output
);
static bool archive_files(char **objects, size_t count, const char *output);

static bool format_path(
    char *out, size_t size,
    const char *folder, const char *name, const char *extension
);
static bool create_parent_folders(const char *path);

static void log_failure(const char *title, const char *msg, ...);
static void log_success(const char *title, const char *msg, ...);
static void log_action(const char *action, const char *msg, ...);

bool cmd_parse(int argc, char **argv) {
    if (argc < 2) {
        return false;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, CMD_STRING_HELP) == 0) {
        command_help(argc, argv);
        return true;
    }
    if (strcmp(cmd, CMD_STRING_VERSION) == 0) {
        command_version(argc, argv);
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
    if (strcmp(cmd, CMD_STRING_RUN) == 0) {
        command_run(argc, argv);
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

        "   " SF_COLOR_GREEN "run" SF_COLOR_RESET
        "        - Runs the built project.\n"

        "   " SF_COLOR_GREEN "clean" SF_COLOR_RESET
        "      - Cleans build artifacts.\n"

        "   " SF_COLOR_YELLOW "--version" SF_COLOR_RESET
        "  - Display compiler information.\n"

        "   " SF_COLOR_YELLOW "--help" SF_COLOR_RESET
        "     - Display this help message.\n"
    );
}

static void command_help(int argc, char **argv) {
    (void)argv;

    if (argc != 2) {
        cmd_print_usage();
        return;
    }

    cmd_print_usage();
}

static void command_version(int argc, char **argv) {
    (void)argv;

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
    static const char *main_file_name = ENTRY_FILE_NAME;
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
        "\n"
        "[build]\n"
        "emit = \"bin\"\n"
        "slfr_flags = [\"-m src\"]\n"
        "asm_flags = []\n"
        "link_flags = []\n",

        project_name
    );

    snprintf(
        main_file_content,
        sizeof(main_file_content),

        "fn main() i32 {\n"
        "\treturn 0;\n"
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

    log_success("Created project", "'%s'", project_name);
}

static void command_build(int argc, char **argv) {
    (void)argc;
    (void)argv;

    static const char *asm_folder = "build/asm";
    static const char *obj_folder = "build/obj";
    static const char *bin_folder = "build/bin";
    static const char *lib_folder = "build/lib";
    static const char *toml_path = "sulfur.toml";

    sf_project_cfg cfg = {0};
    sf_files source_files = {0};
    char **object_files = NULL;
    char output_path[PATH_BUFFER_SIZE];
    const char *output_folder = NULL;
    bool has_main = false;

    sf_toml_parse_result parse_result = sf_toml_parse_project(toml_path, &cfg);

    switch (parse_result) {
        case SF_TOML_PARSE_RESULT_OK: break;

        case SF_TOML_PARSE_RESULT_FAILED_TO_OPEN: {
            log_failure("Build failed", "Failed to open '%s'", toml_path);
            goto cleanup;
        }
        case SF_TOML_PARSE_RESULT_NAME_INVALID: {
            log_failure(
                "Build failed",
                "Invalid '%s' in file '%s' (must be non-empty, not start with '.', no '/')",
                "project.name", toml_path
            );
            goto cleanup;
        }
        case SF_TOML_PARSE_RESULT_EMIT_INVALID: {
            log_failure(
                "Build failed",
                "Invalid '%s' in file '%s' (expected asm, obj, bin or lib)",
                "build.emit", toml_path
            );
            goto cleanup;
        }
        case SF_TOML_PARSE_RESULT_SLFR_FLAGS_INVALID: {
            log_failure(
                "Build failed", "Failed to read '%s' in file '%s'",
                "build.slfr_flags", toml_path
            );
            goto cleanup;
        }
        case SF_TOML_PARSE_RESULT_ASM_FLAGS_INVALID: {
            log_failure(
                "Build failed", "Failed to read '%s' in file '%s'",
                "build.asm_flags", toml_path
            );
            goto cleanup;
        }
        case SF_TOML_PARSE_RESULT_LINK_FLAGS_INVALID: {
            log_failure(
                "Build failed", "Failed to read '%s' in file '%s'",
                "build.link_flags", toml_path
            );
            goto cleanup;
        }
        case SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY: {
            log_failure("Build failed", "Failed to allocate memory");
            goto cleanup;
        }
    }

    if (!collect_slfr_files("src", &source_files)) {
        log_failure("Build failed", "Failed to collect .slfr files");
        goto cleanup;
    }

    if (source_files.count == 0) {
        log_failure("Build failed", "No source files found");
        goto cleanup;
    }

    for (size_t i = 0; i < source_files.count; i++) {
        const char *source = source_files.paths[i];
        const char *relative = strncmp(source, "src/", 4) == 0 ? source + 4 : source;

        if (strcmp(relative, ENTRY_FILE_NAME) == 0) {
            has_main = true;
            break;
        }
    }

    if (cfg.emit == SF_EMIT_BIN && !has_main) {
        log_failure("Build failed", "Entry file '%s' not found", ENTRY_FILE_NAME);
        goto cleanup;
    }

    if (cfg.emit == SF_EMIT_LIB && has_main) {
        log_failure("Build failed", "A library cannot contain '%s'", ENTRY_FILE_NAME);
        goto cleanup;
    }

    output_folder = cfg.emit == SF_EMIT_LIB ? lib_folder : bin_folder;

    if (
        !sf_create_folders(asm_folder) ||
        !sf_create_folders(obj_folder) ||
        !sf_create_folders(output_folder)
    ) {
        log_failure("Build failed", "Failed to create build directories");
        goto cleanup;
    }

    object_files = calloc(source_files.count, sizeof(*object_files));

    if (!object_files) {
        log_failure("Build failed", "Failed to allocate memory");
        goto cleanup;
    }

    for (size_t i = 0; i < source_files.count; i++) {
        const char *source = source_files.paths[i];
        const char *relative = strncmp(source, "src/", 4) == 0 ? source + 4 : source;

        char stem[PATH_BUFFER_SIZE];
        char asm_path[PATH_BUFFER_SIZE];
        char obj_path[PATH_BUFFER_SIZE];

        bool is_entry = strcmp(relative, ENTRY_FILE_NAME) == 0;

        if (snprintf(stem, sizeof(stem), "%s", relative) >= (int)sizeof(stem)) {
            log_failure("Build failed", "Path too long: '%s'", source);
            goto cleanup;
        }

        char *extension = strrchr(stem, '.');
        if (!extension || strcmp(extension, SOURCE_EXTENSION) != 0) {
            log_failure("Build failed", "Invalid source extension: '%s'", source);
            goto cleanup;
        }
        *extension = '\0';

        if (
            !format_path(asm_path, sizeof(asm_path), asm_folder, stem, ".asm") ||
            !format_path(obj_path, sizeof(obj_path), obj_folder, stem, ".o")
        ) {
            log_failure("Build failed", "Path too long: '%s'", source);
            goto cleanup;
        }

        if (!create_parent_folders(asm_path) || !create_parent_folders(obj_path)) {
            log_failure("Build failed", "Failed to create build directories for '%s'", source);
            goto cleanup;
        }

        if (!compile_file(&cfg, source, asm_path, !is_entry)) {
            log_failure("Build failed", "Failed to compile '%s'", source);
            goto cleanup;
        }

        log_action("Compiled", "'%s' -> '%s'", source, asm_path);

        if (cfg.emit == SF_EMIT_ASM) {
            continue;
        }

        if (!assemble_file(&cfg, asm_path, obj_path)) {
            log_failure("Build failed", "Failed to assemble '%s'", asm_path);
            goto cleanup;
        }

        log_action("Assembled", "'%s' -> '%s'", asm_path, obj_path);

        object_files[i] = strdup(obj_path);
        if (!object_files[i]) {
            log_failure("Build failed", "Failed to allocate memory");
            goto cleanup;
        }
    }

    switch (cfg.emit) {
        case SF_EMIT_ASM: {
            log_success("Build done", "Assembly emitted to '%s'", asm_folder);
            break;
        }
        case SF_EMIT_OBJ: {
            log_success("Build done", "Objects emitted to '%s'", obj_folder);
            break;
        }
        case SF_EMIT_LIB: {
            char file_name[PATH_BUFFER_SIZE];
            snprintf(file_name, sizeof(file_name), "lib%s", cfg.name);

            if (!format_path(output_path, sizeof(output_path), lib_folder, file_name, ".a")) {
                log_failure("Build failed", "Path too long for library '%s'", cfg.name);
                goto cleanup;
            }

            if (!archive_files(object_files, source_files.count, output_path)) {
                log_failure("Build failed", "Failed to archive '%s'", output_path);
                goto cleanup;
            }

            log_action("Archived", "'%s'", output_path);
            log_success("Build done", "Library emitted to '%s'", output_path);
            break;
        }
        case SF_EMIT_BIN: {
            if (!format_path(output_path, sizeof(output_path), bin_folder, cfg.name, "")) {
                log_failure("Build failed", "Path too long for binary '%s'", cfg.name);
                goto cleanup;
            }

            if (!link_files(&cfg, object_files, source_files.count, output_path)) {
                log_failure("Build failed", "Failed to link binary '%s'", cfg.name);
                goto cleanup;
            }

            log_action("Linked", "'%s'", output_path);
            log_success("Build done", "Successfully built project sources");
            break;
        }
    }

    cleanup: {
        if (object_files) {
            for (size_t i = 0; i < source_files.count; i++) {
                free(object_files[i]);
            }
            free(object_files);
        }

        sf_toml_free_project(&cfg);
    }

    free_slfr_files(&source_files);
}

static void command_clean(int argc, char **argv) {
    (void)argc;
    (void)argv;

    static const char *build_folder_path = "build";

    if (sf_delete_folder(build_folder_path)) {
        log_success("Clean done", "Successfully cleaned build artifacts");
    } else {
        log_failure("Clean failed", "No build artifacts found");
    }
}

static void command_run(int argc, char **argv) {
    (void)argc;
    (void)argv;

    static const char *toml_path = "sulfur.toml";
    static const char *bin_folder = "build/bin";

    sf_project_cfg cfg = {0};
    sf_args args = {0};
    char binary_path[PATH_BUFFER_SIZE];
    int status = -1;

    if (sf_toml_parse_project(toml_path, &cfg) != SF_TOML_PARSE_RESULT_OK) {
        log_failure("Run failed", "Failed to read '%s'", toml_path);
        return;
    }

    if (cfg.emit != SF_EMIT_BIN) {
        log_failure("Run failed", "Project does not emit a binary (build.emit)");
        goto cleanup;
    }

    if (!format_path(binary_path, sizeof(binary_path), bin_folder, cfg.name, "")) {
        log_failure("Run failed", "Path too long for binary '%s'", cfg.name);
        goto cleanup;
    }

    if (access(binary_path, X_OK) != 0) {
        log_failure("Run failed", "Failed to execute '%s'", binary_path);
        goto cleanup;
    }

    if (!args_push(&args, binary_path)) {
        log_failure("Run failed", "Failed to allocate memory");
        goto cleanup;
    }

    status = spawn(&args);

    if (status == -1) {
        log_failure("Run failed", "Failed to execute '%s'", binary_path);
        goto cleanup;
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);

        if (exit_code == EXEC_NOT_FOUND) {
            log_failure("Run failed", "Failed to execute '%s'", binary_path);
            goto cleanup;
        }

        log_success("Run done", "'%s' returned %d", binary_path, exit_code);
        goto cleanup;
    }

    if (WIFSIGNALED(status)) {
        log_failure(
            "Run failed", "'%s' terminated by signal %d",
            binary_path, WTERMSIG(status)
        );
        goto cleanup;
    }

    cleanup: {
        args_free(&args);
        sf_toml_free_project(&cfg);
    }
}

static bool compile_file(
    const sf_project_cfg *cfg, const char *input,
    const char *output, bool is_module
) {
    sf_args args = {0};

    bool ok =
        args_push(&args, "sulfur-compiler") &&
        args_push(&args, "-i") && args_push(&args, input) &&
        args_push(&args, "-o") && args_push(&args, output) &&
        args_push_list(&args, &cfg->slfr_flags) &&
        (!is_module || args_push(&args, "--module")) &&
        run_ok(&args);

    args_free(&args);
    return ok;
}

static bool assemble_file(
    const sf_project_cfg *cfg, const char *input, const char *output
) {
    sf_args args = {0};

    bool ok =
        args_push(&args, "nasm") &&
        args_push(&args, "-f") && args_push(&args, "elf64") &&
        args_push(&args, input) &&
        args_push(&args, "-o") && args_push(&args, output) &&
        args_push_list(&args, &cfg->asm_flags) &&
        run_ok(&args);

    args_free(&args);
    return ok;
}

static bool link_files(
    const sf_project_cfg *cfg, char **objects, size_t count, const char *output
) {
    sf_args args = {0};
    bool ok = args_push(&args, "ld");

    for (size_t i = 0; ok && i < count; i++) {
        ok = args_push(&args, objects[i]);
    }

    ok = ok &&
        args_push(&args, "-o") && args_push(&args, output) &&
        args_push_list(&args, &cfg->link_flags) &&
        run_ok(&args);

    args_free(&args);
    return ok;
}

static bool archive_files(char **objects, size_t count, const char *output) {
    remove(output);

    sf_args args = {0};

    bool ok =
        args_push(&args, "ar") &&
        args_push(&args, "rcs") &&
        args_push(&args, output);

    for (size_t i = 0; ok && i < count; i++) {
        ok = args_push(&args, objects[i]);
    }

    ok = ok && run_ok(&args);

    args_free(&args);
    return ok;
}

static bool format_path(
    char *out, size_t size,
    const char *folder, const char *name, const char *extension
) {
    int written = snprintf(out, size, "%s/%s%s", folder, name, extension);
    return written >= 0 && (size_t)written < size;
}

static bool create_parent_folders(const char *path) {
    char directory[PATH_BUFFER_SIZE];

    if (snprintf(directory, sizeof(directory), "%s", path) >= (int)sizeof(directory)) {
        return false;
    }

    char *slash = strrchr(directory, '/');

    if (!slash) {
        return true;
    }

    *slash = '\0';
    return sf_create_folders(directory);
}

static bool args_push_owned(sf_args *args, char *item) {
    if (args->count + 2 > args->capacity) {
        size_t capacity = args->capacity ? args->capacity * 2 : 16;
        char **items = realloc(args->items, capacity * sizeof(*items));

        if (!items) {
            return false;
        }

        args->items = items;
        args->capacity = capacity;
    }

    args->items[args->count++] = item;
    args->items[args->count] = NULL;
    return true;
}

static bool args_push(sf_args *args, const char *item) {
    char *copy = strdup(item);

    if (!copy) {
        return false;
    }

    if (!args_push_owned(args, copy)) {
        free(copy);
        return false;
    }

    return true;
}

static bool args_push_words(sf_args *args, const char *text) {
    const char *p = text;

    while (*p) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (!*p) {
            break;
        }

        const char *start = p;

        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }

        char *word = strndup(start, (size_t)(p - start));

        if (!word) {
            return false;
        }

        if (!args_push_owned(args, word)) {
            free(word);
            return false;
        }
    }

    return true;
}

static bool args_push_list(sf_args *args, const sf_string_list *list) {
    for (size_t i = 0; i < list->count; i++) {
        if (!args_push_words(args, list->items[i])) {
            return false;
        }
    }

    return true;
}

static void args_free(sf_args *args) {
    for (size_t i = 0; i < args->count; i++) {
        free(args->items[i]);
    }

    free(args->items);
    *args = (sf_args){0};
}

static int spawn(const sf_args *args) {
    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();

    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        execvp(args->items[0], args->items);
        _exit(EXEC_NOT_FOUND);
    }

    int status;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            return -1;
        }
    }

    return status;
}

static bool run_ok(const sf_args *args) {
    int status = spawn(args);
    return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static void log_failure(const char *title, const char *msg, ...) {
    printf(SF_COLOR_RED "%s\n" SF_COLOR_RESET "   ", title);

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    putchar('\n');
}

static void log_success(const char *title, const char *msg, ...) {
    printf(SF_COLOR_GREEN "%s\n" SF_COLOR_RESET "   ", title);

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    putchar('\n');
}

static void log_action(const char *action, const char *msg, ...) {
    printf(SF_COLOR_CYAN "[%s] " SF_COLOR_RESET, action);

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    putchar('\n');
}
