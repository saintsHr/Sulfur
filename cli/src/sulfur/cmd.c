#define _DEFAULT_SOURCE

#include "sulfur/cmd.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

#include "sulfur/file.h"
#include "sulfur/toml.h"

#define SF_COLOR_RESET "\033[0m"
#define SF_COLOR_BOLD "\033[1m"

#define SF_COLOR_CYAN "\033[36m"
#define SF_COLOR_GREEN "\033[32m"
#define SF_COLOR_YELLOW "\033[33m"
#define SF_COLOR_RED "\033[31m"

#define SF_PRINTF(fmt, first) __attribute__((format(printf, fmt, first)))

#define CMD_STRING_HELP "--help"
#define CMD_STRING_VERSION "--version"

#define CMD_STRING_NEW "new"
#define CMD_STRING_BUILD "build"
#define CMD_STRING_CLEAN "clean"
#define CMD_STRING_RUN "run"

#define SOURCE_EXTENSION ".slfr"
#define ENTRY_FILE_NAME "main.slfr"

#define PATH_BUFFER_SIZE 1024
#define CONTENT_BUFFER_SIZE 1024

#define EXEC_NOT_FOUND 127

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

static bool command_help(int argc, char **argv);
static bool command_version(int argc, char **argv);
static bool command_new(int argc, char **argv);
static bool command_build(int argc, char **argv);
static bool command_clean(int argc, char **argv);
static bool command_run(int argc, char **argv);

static const char *compiler_path(void);

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

static void report_toml_error(
    const char *title, sf_toml_parse_result result, const char *path
);

static bool use_color(FILE *stream);
static const char *color(FILE *stream, const char *code);

static void log_block(
    FILE *stream, const char *code, const char *title,
    const char *msg, va_list args
) SF_PRINTF(4, 0);

static void log_failure(const char *title, const char *msg, ...) SF_PRINTF(2, 3);
static void log_warn(const char *title, const char *msg, ...) SF_PRINTF(2, 3);
static void log_success(const char *title, const char *msg, ...) SF_PRINTF(2, 3);
static void log_action(const char *action, const char *msg, ...) SF_PRINTF(2, 3);

bool cmd_parse(int argc, char **argv) {
    if (argc < 2) {
        return false;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, CMD_STRING_HELP) == 0) {
        return command_help(argc, argv);
    }

    if (strcmp(cmd, CMD_STRING_VERSION) == 0) {
        return command_version(argc, argv);
    }

    if (strcmp(cmd, CMD_STRING_NEW) == 0) {
        return command_new(argc, argv);
    }

    if (strcmp(cmd, CMD_STRING_BUILD) == 0) {
        return command_build(argc, argv);
    }

    if (strcmp(cmd, CMD_STRING_CLEAN) == 0) {
        return command_clean(argc, argv);
    }

    if (strcmp(cmd, CMD_STRING_RUN) == 0) {
        return command_run(argc, argv);
    }

    log_failure(
        "Unknown Command.",
        "Unknown command '%s'. Try 'sulfur --help'.",
        cmd
    );

    return false;
}

void cmd_print_usage(void) {
    const char *title = color(stdout, SF_COLOR_BOLD SF_COLOR_CYAN);
    const char *green = color(stdout, SF_COLOR_GREEN);
    const char *yellow = color(stdout, SF_COLOR_YELLOW);
    const char *reset = color(stdout, SF_COLOR_RESET);

    printf(
        "%sUsage:%s\n"
        "   sulfur <command> [arguments]\n"
        "\n"
        "%sCommands:%s\n"
        "   %snew%s <name> - Creates a new project folder.\n"
        "   %sbuild%s      - Builds the project.\n"
        "   %srun%s        - Runs the built project.\n"
        "   %sclean%s      - Cleans build artifacts.\n"
        "   %s--version%s  - Displays compiler information.\n"
        "   %s--help%s     - Displays this help message.\n",
        title, reset,
        title, reset,
        green, reset,
        green, reset,
        green, reset,
        green, reset,
        yellow, reset,
        yellow, reset
    );
}

static bool command_help(int argc, char **argv) {
    if (argc != 2) {
        log_failure(
            "Help Failed.",
            "Unexpected argument '%s'. Usage: sulfur --help.",
            argv[2]
        );

        return false;
    }

    cmd_print_usage();

    return true;
}

static bool command_version(int argc, char **argv) {
    if (argc != 2) {
        log_failure(
            "Version Failed.",
            "Unexpected argument '%s'. Usage: sulfur --version.",
            argv[2]
        );

        return false;
    }

    const char *cyan = color(stdout, SF_COLOR_CYAN);
    const char *green = color(stdout, SF_COLOR_GREEN);
    const char *yellow = color(stdout, SF_COLOR_YELLOW);
    const char *reset = color(stdout, SF_COLOR_RESET);

    printf(
        "%sVersion v0.1.0%s\n"
        "Copyright (C) 2026 %ssaintsHr%s\n"
        "Licensed under the %sMIT license%s\n",
        cyan, reset,
        green, reset,
        yellow, reset
    );

    return true;
}

static bool command_new(int argc, char **argv) {
    if (argc < 3) {
        log_failure(
            "New Failed.",
            "Missing project name. Usage: sulfur new <name>."
        );

        return false;
    }

    if (argc > 3) {
        log_failure(
            "New Failed.",
            "Unexpected argument '%s'. Usage: sulfur new <name>.",
            argv[3]
        );

        return false;
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

    if (project_name[0] == '\0' || project_name[0] == '.' || strchr(project_name, '/')) {
        log_failure(
            "New Failed.",
            "Invalid project name '%s'. The name must be non-empty, must not start with '.', and must not contain '/'.",
            project_name
        );

        return false;
    }

    int written = snprintf(
        toml_file_content,
        sizeof(toml_file_content),

        "[project]\n"
        "name = \"%s\"\n"
        "\n"
        "[build]\n"
        "emit = \"bin\"\n"
        "slfr_flags = [\n"
            "\t\"-m src /usr/local/share/sulfur\"\n"
        "]\n"
        "asm_flags = [\n"
            "\t\n"
        "]\n"
        "link_flags = [\n"
            "\t\"/usr/local/lib/sulfur/libsulfurrt.a\",\n"
            "\t\"/usr/local/lib/sulfur/libsulfurstd.a\"\n"
        "]\n",

        project_name
    );

    if (written < 0 || (size_t)written >= sizeof(toml_file_content)) {
        log_failure(
            "New Failed.",
            "Project name '%s' is too long to generate the configuration file.",
            project_name
        );

        return false;
    }

    written = snprintf(
        main_file_content,
        sizeof(main_file_content),

        "fn main() i32 {\n"
        "\treturn 0;\n"
        "}\n"
    );

    if (written < 0 || (size_t)written >= sizeof(main_file_content)) {
        log_failure(
            "New Failed.",
            "Failed to prepare the contents of '%s'.",
            main_file_name
        );

        return false;
    }

    if (
        snprintf(project_folder_path, sizeof(project_folder_path), "%s", project_name)
            >= (int)sizeof(project_folder_path) ||
        !format_path(src_folder_path, sizeof(src_folder_path), project_folder_path, src_folder_name, "") ||
        !format_path(main_file_path, sizeof(main_file_path), src_folder_path, main_file_name, "") ||
        !format_path(toml_file_path, sizeof(toml_file_path), project_folder_path, toml_file_name, "")
    ) {
        log_failure(
            "New Failed.",
            "The path for project '%s' exceeds the maximum supported length.",
            project_name
        );

        return false;
    }

    if (access(project_folder_path, F_OK) == 0) {
        log_failure(
            "New Failed.",
            "Project folder '%s' already exists.",
            project_folder_path
        );

        return false;
    }

    if (!sf_create_folder(project_folder_path)) {
        log_failure(
            "New Failed.",
            "Failed to create project folder '%s'.",
            project_folder_path
        );

        return false;
    }

    if (!sf_create_folder(src_folder_path)) {
        log_failure(
            "New Failed.",
            "Failed to create source folder '%s'.",
            src_folder_path
        );

        return false;
    }

    if (!sf_create_file(main_file_path)) {
        log_failure(
            "New Failed.",
            "Failed to create entry file '%s'.",
            main_file_path
        );

        return false;
    }

    if (!sf_create_file(toml_file_path)) {
        log_failure(
            "New Failed.",
            "Failed to create configuration file '%s'.",
            toml_file_path
        );

        return false;
    }

    if (!sf_write_file(main_file_path, main_file_content)) {
        log_failure(
            "New Failed.",
            "Failed to write entry file '%s'.",
            main_file_path
        );

        return false;
    }

    if (!sf_write_file(toml_file_path, toml_file_content)) {
        log_failure(
            "New Failed.",
            "Failed to write configuration file '%s'.",
            toml_file_path
        );

        return false;
    }

    log_success(
        "Created Project.",
        "Project '%s' was created successfully.",
        project_name
    );

    return true;
}

static bool command_build(int argc, char **argv) {
    if (argc != 2) {
        log_failure(
            "Build Failed.",
            "Unexpected argument '%s'. Usage: sulfur build.",
            argv[2]
        );

        return false;
    }

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
    bool success = false;

    sf_toml_parse_result parse_result = sf_toml_parse_project(toml_path, &cfg);

    if (parse_result != SF_TOML_PARSE_RESULT_OK) {
        report_toml_error("Build Failed.", parse_result, toml_path);
        goto cleanup;
    }

    if (!collect_slfr_files("src", &source_files)) {
        log_failure(
            "Build Failed.",
            "Failed to collect '%s' source files from 'src/'.",
            SOURCE_EXTENSION
        );

        goto cleanup;
    }

    if (source_files.count == 0) {
        log_failure(
            "Build Failed.",
            "No '%s' source files were found in 'src/'.",
            SOURCE_EXTENSION
        );

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
        log_failure(
            "Build Failed.",
            "Entry file 'src/%s' was not found. It is required when build.emit = \"bin\".",
            ENTRY_FILE_NAME
        );

        goto cleanup;
    }

    if (cfg.emit == SF_EMIT_LIB && has_main) {
        log_failure(
            "Build Failed.",
            "A library cannot contain the entry file 'src/%s'. Remove it or set build.emit = \"bin\".",
            ENTRY_FILE_NAME
        );

        goto cleanup;
    }

    output_folder = cfg.emit == SF_EMIT_LIB ? lib_folder : bin_folder;

    if (
        !sf_create_folders(asm_folder) ||
        !sf_create_folders(obj_folder) ||
        !sf_create_folders(output_folder)
    ) {
        log_failure(
            "Build Failed.",
            "Failed to create the required build directories."
        );

        goto cleanup;
    }

    object_files = calloc(source_files.count, sizeof(*object_files));

    if (!object_files) {
        log_failure(
            "Build Failed.",
            "Failed to allocate memory."
        );

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
            log_failure(
                "Build Failed.",
                "Source path '%s' exceeds the maximum supported length.",
                source
            );

            goto cleanup;
        }

        char *extension = strrchr(stem, '.');

        if (!extension || strcmp(extension, SOURCE_EXTENSION) != 0) {
            log_failure(
                "Build Failed.",
                "Source file '%s' has an invalid extension. Expected '%s'.",
                source, SOURCE_EXTENSION
            );

            goto cleanup;
        }

        *extension = '\0';

        if (
            !format_path(asm_path, sizeof(asm_path), asm_folder, stem, ".asm") ||
            !format_path(obj_path, sizeof(obj_path), obj_folder, stem, ".o")
        ) {
            log_failure(
                "Build Failed.",
                "Output path for source file '%s' exceeds the maximum supported length.",
                source
            );

            goto cleanup;
        }

        if (!create_parent_folders(asm_path) || !create_parent_folders(obj_path)) {
            log_failure(
                "Build Failed.",
                "Failed to create the required output directories for '%s'.",
                source
            );

            goto cleanup;
        }

        log_action(
            "Compiling", "[%zu/%zu] '%s' -> '%s'",
            i + 1, source_files.count, source, asm_path
        );

        if (!compile_file(&cfg, source, asm_path, !is_entry)) {
            log_failure(
                "Build Failed.",
                "Failed to compile source file '%s'.",
                source
            );

            goto cleanup;
        }

        if (cfg.emit == SF_EMIT_ASM) {
            continue;
        }

        if (!assemble_file(&cfg, asm_path, obj_path)) {
            log_failure(
                "Build Failed.",
                "Failed to assemble '%s'.",
                asm_path
            );

            goto cleanup;
        }

        object_files[i] = strdup(obj_path);

        if (!object_files[i]) {
            log_failure(
                "Build Failed.",
                "Failed to allocate memory."
            );

            goto cleanup;
        }
    }

    switch (cfg.emit) {
        case SF_EMIT_ASM: {
            log_success(
                "Build Done.",
                "Assembly files were emitted to '%s'.",
                asm_folder
            );

            success = true;

            break;
        }
        case SF_EMIT_OBJ: {
            log_success(
                "Build Done.",
                "Object files were emitted to '%s'.",
                obj_folder
            );

            success = true;
            break;
        }
        case SF_EMIT_LIB: {
            char file_name[PATH_BUFFER_SIZE];

            if (
                snprintf(
                    file_name, sizeof(file_name),
                    "lib%s", cfg.name
                ) >= (int)sizeof(file_name)
            ) {
                log_failure(
                    "Build Failed.",
                    "Library name '%s' exceeds the maximum supported length.",
                    cfg.name
                );

                goto cleanup;
            }

            if (
                !format_path(
                    output_path, sizeof(output_path),
                    lib_folder, file_name, ".a"
                )
            ) {
                log_failure(
                    "Build Failed.",
                    "Output path for library '%s' exceeds the maximum supported length.",
                    cfg.name
                );

                goto cleanup;
            }

            if (!archive_files(object_files, source_files.count, output_path)) {
                log_failure(
                    "Build Failed.",
                    "Failed to create library archive '%s'.",
                    output_path
                );

                goto cleanup;
            }

            log_success(
                "Build Done.",
                "Library archive was emitted to '%s'.",
                output_path
            );

            success = true;
            break;
        }
        case SF_EMIT_BIN: {
            if (
                !format_path(
                    output_path, sizeof(output_path),
                    bin_folder, cfg.name, ""
                )
            ) {
                log_failure(
                    "Build Failed.",
                    "Output path for binary '%s' exceeds the maximum supported length.",
                    cfg.name
                );

                goto cleanup;
            }

            if (
                !link_files(
                    &cfg, object_files,
                    source_files.count,
                    output_path
                )
            ) {
                log_failure(
                    "Build Failed.",
                    "Failed to link binary '%s'.",
                    cfg.name
                );

                goto cleanup;
            }

            log_success(
                "Build Done.",
                "Binary was emitted to '%s'.",
                output_path
            );

            success = true;

            break;
        }

        default: {
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
        free_slfr_files(&source_files);
    }

    return success;
}

static bool command_clean(int argc, char **argv) {
    if (argc != 2) {
        log_failure(
            "Clean Failed.",
            "Unexpected argument '%s'. Usage: sulfur clean.",
            argv[2]
        );

        return false;
    }

    static const char *build_folder_path = "build";

    if (access(build_folder_path, F_OK) != 0) {
        log_warn(
            "Clean.",
            "Nothing to clean. Directory '%s' does not exist.",
            build_folder_path
        );

        return true;
    }

    if (!sf_delete_folder(build_folder_path)) {
        log_failure(
            "Clean Failed.",
            "Failed to delete directory '%s'.",
            build_folder_path
        );

        return false;
    }

    log_success(
        "Clean Done.",
        "Removed build directory '%s'.",
        build_folder_path
    );

    return true;
}

static bool command_run(int argc, char **argv) {
    if (argc != 2) {
        log_failure(
            "Run Failed.",
            "Unexpected argument '%s'. Usage: sulfur run.",
            argv[2]
        );

        return false;
    }

    static const char *toml_path = "sulfur.toml";
    static const char *bin_folder = "build/bin";

    sf_project_cfg cfg = {0};
    sf_args args = {0};
    char binary_path[PATH_BUFFER_SIZE];
    bool success = false;
    int status = -1;

    sf_toml_parse_result parse_result = sf_toml_parse_project(toml_path, &cfg);

    if (parse_result != SF_TOML_PARSE_RESULT_OK) {
        report_toml_error("Run Failed.", parse_result, toml_path);
        goto cleanup;
    }

    if (cfg.emit != SF_EMIT_BIN) {
        log_failure(
            "Run Failed.",
            "The project does not emit a binary. Set build.emit = \"bin\" to run it."
        );

        goto cleanup;
    }

    if (!format_path(binary_path, sizeof(binary_path), bin_folder, cfg.name, "")) {
        log_failure(
            "Run Failed.",
            "Output path for binary '%s' exceeds the maximum supported length.",
            cfg.name
        );

        goto cleanup;
    }

    if (access(binary_path, X_OK) != 0) {
        log_failure(
            "Run Failed.",
            "Cannot execute '%s'. Make sure you have run 'sulfur build'.",
            binary_path
        );

        goto cleanup;
    }

    if (!args_push(&args, binary_path)) {
        log_failure(
            "Run Failed.",
            "Failed to allocate memory."
        );

        goto cleanup;
    }

    log_action("Running", "'%s'", binary_path);

    status = spawn(&args);

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);

        if (exit_code == EXEC_NOT_FOUND) {
            log_failure(
                "Run Failed.",
                "Failed to execute binary '%s'.",
                binary_path
            );

            goto cleanup;
        }

        if (exit_code != 0) {
            log_warn(
                "Run Done.",
                "Program '%s' exited with status %d.",
                binary_path, exit_code
            );
        } else {
            log_success(
                "Run Done.",
                "Program '%s' exited with status %d.",
                binary_path, exit_code
            );
        }

        success = true;

        goto cleanup;
    }

    if (WIFSIGNALED(status)) {
        log_failure(
            "Run Failed.",
            "Program '%s' was terminated by signal %d (%s).",
            binary_path, WTERMSIG(status), strsignal(WTERMSIG(status))
        );

        goto cleanup;
    }

    log_failure(
        "Run Failed.",
        "Program '%s' ended in an unknown execution state.",
        binary_path
    );

    cleanup: {
        args_free(&args);
        sf_toml_free_project(&cfg);

        return success;
    }
}

static const char *compiler_path(void) {
    static char path[PATH_MAX];
    static bool init = false;

    if (init) return path;

    char self[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", self, sizeof(self) - 1);

    if (n > 0) {
        self[n] = '\0';
        char *dir = dirname(self);

        if (snprintf(path, sizeof(path), "%s/sulfur-compiler", dir) < (int)sizeof(path)
            && access(path, X_OK) == 0) {
            init = true;
            return path;
        }
    }

    snprintf(path, sizeof(path), "sulfur-compiler");
    init = true;
    return path;
}

static bool compile_file(
    const sf_project_cfg *cfg, const char *input,
    const char *output, bool is_module
) {
    sf_args args = {0};

    bool ok =
        args_push(&args, compiler_path()) &&
        args_push(&args, "-i") && args_push(&args, input) &&
        args_push(&args, "-o") && args_push(&args, output) &&
        args_push_list(&args, &cfg->slfr_flags) &&
        (!is_module || args_push(&args, "--module"));

    if (!ok) {
        log_failure(
            "Command Failed.",
            "Failed to allocate memory."
        );

        args_free(&args);
        return false;
    }

    ok = run_ok(&args);

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
        args_push_list(&args, &cfg->asm_flags);

    if (!ok) {
        log_failure(
            "Command Failed.",
            "Failed to allocate memory."
        );

        args_free(&args);
        return false;
    }

    ok = run_ok(&args);

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
        args_push_list(&args, &cfg->link_flags);

    if (!ok) {
        log_failure(
            "Command Failed.",
            "Failed to allocate memory."
        );

        args_free(&args);
        return false;
    }

    ok = run_ok(&args);

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

    if (!ok) {
        log_failure(
            "Command Failed.",
            "Failed to allocate memory."
        );

        args_free(&args);
        return false;
    }

    ok = run_ok(&args);

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

static void report_toml_error(
    const char *title, sf_toml_parse_result result, const char *path
) {
    const char *detail = sf_toml_error_detail();

    switch (result) {
        case SF_TOML_PARSE_RESULT_OK: {
            break;
        }
        case SF_TOML_PARSE_RESULT_INVALID_ARGUMENT: {
            log_failure(
                title,
                "Internal error: Invalid arguments were provided while reading '%s'.",
                path
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_FAILED_TO_OPEN: {
            log_failure(
                title,
                "Failed to open configuration file '%s': %s. Run this command from the project root.",
                path, detail
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_SYNTAX_ERROR: {
            log_failure(
                title,
                "Invalid TOML syntax in '%s': %s.",
                path, detail
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_NAME_MISSING: {
            log_failure(
                title,
                "Missing required field 'project.name' in '%s'.",
                path
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_NAME_INVALID: {
            log_failure(
                title,
                "Invalid field 'project.name' in '%s': %s. The name must be non-empty, must not start with '.', and must not contain '/'.",
                path, detail
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_EMIT_INVALID: {
            log_failure(
                title,
                "Invalid field 'build.emit' in '%s': %s. Expected one of: asm, obj, bin or lib.",
                path, detail
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_SLFR_FLAGS_INVALID: {
            log_failure(
                title,
                "Invalid field 'build.slfr_flags' in '%s': %s.",
                path, detail
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_ASM_FLAGS_INVALID: {
            log_failure(
                title,
                "Invalid field 'build.asm_flags' in '%s': %s.",
                path, detail
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_LINK_FLAGS_INVALID: {
            log_failure(
                title,
                "Invalid field 'build.link_flags' in '%s': %s.",
                path, detail
            );

            break;
        }
        case SF_TOML_PARSE_RESULT_FAILED_TO_ALLOCATE_MEMORY: {
            log_failure(
                title,
                "Failed to allocate memory."
            );

            break;
        }
        default: {
            log_failure(
                title,
                "An unknown error occurred while reading configuration file '%s'.",
                path
            );

            break;
        }
    }
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

    int status = -1;

    while (waitpid(pid, &status, 0) < 0);

    return status;
}

static bool run_ok(const sf_args *args) {
    int status = spawn(args);

    if (status == -1) {
        return false;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);

        if (
            code == EXEC_NOT_FOUND ||
            code != 0
        ) {
            return false;
        }

        return true;
    }

    if (WIFSIGNALED(status)) {
        return false;
    }

    return false;
}

static bool use_color(FILE *stream) {
    const char *no_color = getenv("NO_COLOR");

    if (no_color && no_color[0] != '\0') {
        return false;
    }

    return isatty(fileno(stream));
}

static const char *color(FILE *stream, const char *code) {
    return use_color(stream) ? code : "";
}

static void log_block(
    FILE *stream, const char *code, const char *title,
    const char *msg, va_list args
) {
    fflush(stdout);

    fprintf(
        stream, "%s%s%s\n   ",
        color(stream, code), title, color(stream, SF_COLOR_RESET)
    );

    vfprintf(stream, msg, args);
    fputc('\n', stream);
}

static void log_failure(const char *title, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_block(stderr, SF_COLOR_RED, title, msg, args);
    va_end(args);
}

static void log_warn(const char *title, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_block(stderr, SF_COLOR_YELLOW, title, msg, args);
    va_end(args);
}

static void log_success(const char *title, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_block(stdout, SF_COLOR_GREEN, title, msg, args);
    va_end(args);
}

static void log_action(const char *action, const char *msg, ...) {
    printf(
        "%s[%s]%s ",
        color(stdout, SF_COLOR_CYAN), action, color(stdout, SF_COLOR_RESET)
    );

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    putchar('\n');
}
