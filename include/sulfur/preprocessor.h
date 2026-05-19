#pragma once

#include <stddef.h>

#define SF_MAX_DEFINES             512
#define SF_MAX_DEFINE_NAME_LENGTH  64
#define SF_MAX_DEFINE_VALUE_LENGTH 128

typedef struct {
    char name[SF_MAX_DEFINE_NAME_LENGTH];
    char value[SF_MAX_DEFINE_VALUE_LENGTH];
    size_t nameLen;
    size_t valueLen;
} sfDefine;

char* preprocess(const char* source, long srcSize, const char* filename);
