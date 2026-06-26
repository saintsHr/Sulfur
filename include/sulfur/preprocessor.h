#pragma once

#include <stddef.h>

#define SF_MAX_DEFINES             512
#define SF_MAX_DEFINE_NAME_LENGTH  64
#define SF_MAX_DEFINE_VALUE_LENGTH 128

#define STR2(x) #x
#define STR(x) STR2(x)

#define DEFINE_KEYWORD "define"

typedef struct {
    char name[SF_MAX_DEFINE_NAME_LENGTH];
    char value[SF_MAX_DEFINE_VALUE_LENGTH];
    size_t name_lenght;
    size_t value_lenght;
} sf_define;

typedef struct {
    sf_define defines[SF_MAX_DEFINES];
    int define_count;
} sf_preprocessor_context;

char* preprocess(const char* source, long srcSize, const char* filename);
