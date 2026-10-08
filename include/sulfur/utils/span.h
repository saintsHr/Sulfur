#pragma once

#include <stddef.h>

typedef struct {
  size_t line;
  size_t col;
  size_t len;
} sf_span;
