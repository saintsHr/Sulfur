#pragma once

#include <stdint.h>

typedef struct {
	uint32_t line;
	uint32_t col;
	uint8_t len;
} sf_span;