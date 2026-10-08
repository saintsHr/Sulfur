#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/backend/ir/ir.h"
#include "sulfur/pipeline/frontend/ast.h"

bool sf_type_value_uint_literal_fits(sf_value_type type, uint64_t value);
bool sf_type_value_signed_literal_fits_negated(sf_value_type type, uint64_t magnitude);

bool sf_type_value_is_integer(sf_value_type type);
bool sf_type_value_is_unsigned(sf_value_type type);
bool sf_type_value_is_signed(sf_value_type type);
bool sf_type_value_is_same_group(sf_value_type a, sf_value_type b);
bool sf_type_value_is_castable(sf_value_type from, sf_value_type to);

const char *sf_type_operation_name(sf_operation_type op);
const char *sf_type_value_name(sf_value_type type);

uint8_t sf_type_value_width_bits(sf_value_type type);
uint8_t sf_type_value_width_bytes(sf_value_type type);

sf_value_type sf_type_value_promote(sf_value_type a, sf_value_type b);

sf_opcode sf_type_operation_to_opcode(sf_operation_type type);
