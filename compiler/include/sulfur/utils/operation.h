#pragma once

#include <stdbool.h>

#include "sulfur/shared/operation.h"

bool sf_operation_is_increment_or_decrement(sf_operation_type op);
const char *sf_operation_name(sf_operation_type op);
