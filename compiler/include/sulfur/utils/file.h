#pragma once

#include <stdbool.h>
#include <stddef.h>

char *sf_file_read(const char *filename, size_t *size);
bool sf_file_write(const char *filename, const char *content);
