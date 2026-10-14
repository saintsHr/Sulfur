#pragma once

#include <stdbool.h>

bool sf_create_folder(const char *path);
bool sf_delete_folder(const char *path);

bool sf_create_file(const char *path);
bool sf_delete_file(const char *path);

bool sf_write_file(const char *path, const char *content);
bool sf_read_file(const char *path, char **content);
