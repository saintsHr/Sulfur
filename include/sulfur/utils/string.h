#pragma once

#include <stdint.h>

char* sf_strdup(const char* str);
void sf_strpush(const char* src, char** dst, uint64_t* len, uint64_t* capacity);