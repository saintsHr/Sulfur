#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum sf_type_kind {
    SF_TYPE_KIND_UNRESOLVED,
    SF_TYPE_KIND_I8,
    SF_TYPE_KIND_I16,
    SF_TYPE_KIND_I32,
    SF_TYPE_KIND_I64,
    SF_TYPE_KIND_U8,
    SF_TYPE_KIND_U16,
    SF_TYPE_KIND_U32,
    SF_TYPE_KIND_U64,
    SF_TYPE_KIND_BOOL,
    SF_TYPE_KIND_VOID
} sf_type_kind;

bool sf_type_kind_unsigned_literal_fits(sf_type_kind type, uint64_t value);
bool sf_type_kind_signed_literal_fits_negated(sf_type_kind type, uint64_t magnitude);
bool sf_type_kind_is_integer(sf_type_kind type);
bool sf_type_kind_is_unsigned(sf_type_kind type);
bool sf_type_kind_is_signed(sf_type_kind type);
bool sf_type_kind_is_same_group(sf_type_kind a, sf_type_kind b);
bool sf_type_kind_is_castable(sf_type_kind from, sf_type_kind to);
const char *sf_type_kind_name(sf_type_kind type);
uint8_t sf_type_kind_width_bits(sf_type_kind type);
uint8_t sf_type_kind_width_bytes(sf_type_kind type);
sf_type_kind sf_type_kind_promote(sf_type_kind a, sf_type_kind b);
