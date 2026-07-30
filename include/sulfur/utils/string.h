#pragma once

#include <stdint.h>

#include "sulfur/utils/arena.h"

char* sf_strdup(const char* str);
char* sf_strdup_arena(sf_arena* arena, const char* s);
void sf_strpush(const char* src, char** dst, uint64_t* len, uint64_t* capacity);