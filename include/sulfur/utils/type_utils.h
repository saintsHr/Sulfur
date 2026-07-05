#pragma once

#include "sulfur/pipeline/ast.h"
#include "sulfur/pipeline/ir.h"
#include <stdbool.h>

bool type_value_is_unsigned(sf_value_type type);
bool type_value_is_signed(sf_value_type type);
bool type_value_is_same_group(sf_value_type a, sf_value_type b);
bool type_value_is_castable(sf_value_type from, sf_value_type to);

const char* type_operation_name(sf_operation_type op);
const char* type_value_name(sf_value_type type);

uint8_t type_value_width_bits(sf_value_type type);
uint8_t type_value_width_bytes(sf_value_type type);

sf_value_type type_value_promote(sf_value_type a, sf_value_type b);

sf_opcode type_operation_to_opcode(sf_operation_type type);