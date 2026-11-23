#pragma once

#include "sulfur/shared/qualifier.h"
#include "sulfur/shared/type.h"

#include <stddef.h>

typedef struct sf_parameter {
    const sf_type *type;
    const char *name;
    size_t id;
    sf_qualifiers qualifiers;
} sf_parameter;
