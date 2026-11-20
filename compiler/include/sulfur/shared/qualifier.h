#pragma once

#include "stdbool.h"

typedef struct sf_qualifiers {
    bool is_mutable;
    bool is_public;
    bool is_extern;
} sf_qualifiers;
