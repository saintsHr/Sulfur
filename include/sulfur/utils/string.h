#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "sulfur/utils/arena.h"

char *sf_string_dup(const char *str);
char *sf_string_dup_arena(sf_arena *arena, const char *s);
bool sf_string_push(const char *src, char **dst, size_t *len, size_t *capacity);
