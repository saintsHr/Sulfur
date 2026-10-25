#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char **paths;
    size_t count;
} sf_files;

bool sf_create_folder(const char *path);
bool sf_create_folders(const char *path);
bool sf_delete_folder(const char *path);

bool sf_create_file(const char *path);
bool sf_delete_file(const char *path);

bool sf_write_file(const char *path, const char *content);
bool sf_read_file(const char *path, char **content);

bool collect_slfr_files(const char *path, sf_files *files);
void free_slfr_files(sf_files *files);
bool has_slfr_extension(const char *path);
