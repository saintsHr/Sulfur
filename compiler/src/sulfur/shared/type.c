#include "sulfur/shared/type.h"

#include "sulfur/pipeline/midend/ir/ir.h"
#include "sulfur/shared/operation.h"

bool sf_type_kind_unsigned_literal_fits(sf_type_kind type, uint64_t value) {
    switch (type) {
        case SF_TYPE_KIND_I8: {
            return value <= (uint64_t)INT8_MAX;
        }
        case SF_TYPE_KIND_I16: {
            return value <= (uint64_t)INT16_MAX;
        }
        case SF_TYPE_KIND_I32: {
            return value <= (uint64_t)INT32_MAX;
        }
        case SF_TYPE_KIND_I64: {
            return value <= (uint64_t)INT64_MAX;
        }
        case SF_TYPE_KIND_U8: {
            return value <= UINT8_MAX;
        }
        case SF_TYPE_KIND_U16: {
            return value <= UINT16_MAX;
        }
        case SF_TYPE_KIND_U32: {
            return value <= UINT32_MAX;
        }
        case SF_TYPE_KIND_U64: {
            return true;
        }
        default: {
            return false;
        }
    }
}

bool sf_type_kind_signed_literal_fits_negated(sf_type_kind type, uint64_t magnitude) {
    switch (type) {
        case SF_TYPE_KIND_I8: {
            return magnitude <= (uint64_t)INT8_MAX + 1;
        }
        case SF_TYPE_KIND_I16: {
            return magnitude <= (uint64_t)INT16_MAX + 1;
        }
        case SF_TYPE_KIND_I32: {
            return magnitude <= (uint64_t)INT32_MAX + 1;
        }
        case SF_TYPE_KIND_I64: {
            return magnitude <= (uint64_t)INT64_MAX + 1;
        }
        default: {
            return false;
        }
    }
}

const char *sf_type_kind_name(sf_type_kind type) {
    switch (type) {
        case SF_TYPE_KIND_I8: {
            return "i8";
        }
        case SF_TYPE_KIND_I16: {
            return "i16";
        }
        case SF_TYPE_KIND_I32: {
            return "i32";
        }
        case SF_TYPE_KIND_I64: {
            return "i64";
        }

        case SF_TYPE_KIND_U8: {
            return "u8";
        }
        case SF_TYPE_KIND_U16: {
            return "u16";
        }
        case SF_TYPE_KIND_U32: {
            return "u32";
        }
        case SF_TYPE_KIND_U64: {
            return "u64";
        }

        case SF_TYPE_KIND_BOOL: {
            return "bool";
        }

        case SF_TYPE_KIND_VOID: {
            return "void";
        }

        default: {
            return "?";
        }
    }
}

bool sf_type_kind_is_unsigned(sf_type_kind type) {
    switch (type) {
        case SF_TYPE_KIND_U8:
        case SF_TYPE_KIND_U16:
        case SF_TYPE_KIND_U32:
        case SF_TYPE_KIND_U64:
            return true;
        default:
            return false;
    }
}

bool sf_type_kind_is_signed(sf_type_kind type) {
    switch (type) {
        case SF_TYPE_KIND_I8:
        case SF_TYPE_KIND_I16:
        case SF_TYPE_KIND_I32:
        case SF_TYPE_KIND_I64:
            return true;
        default:
            return false;
    }
}

bool sf_type_kind_is_integer(sf_type_kind type) {
    return sf_type_kind_is_signed(type) || sf_type_kind_is_unsigned(type);
}

bool sf_type_kind_is_same_group(sf_type_kind a, sf_type_kind b) {
    if (sf_type_kind_is_unsigned(a) && sf_type_kind_is_unsigned(b)) {
        return true;
    }

    if (sf_type_kind_is_signed(a) && sf_type_kind_is_signed(b)) {
        return true;
    }

    if (a == SF_TYPE_KIND_BOOL && b == SF_TYPE_KIND_BOOL) {
        return true;
    }

    return false;
}

bool sf_type_kind_is_castable(sf_type_kind from, sf_type_kind to) {
    if ((from == SF_TYPE_KIND_UNRESOLVED) || (to == SF_TYPE_KIND_UNRESOLVED)) {
        return false;
    }

    if (from == to) {
        return true;
    }

    bool castable = true;

    if (((from == SF_TYPE_KIND_BOOL) || (to == SF_TYPE_KIND_BOOL)) ||
        ((from == SF_TYPE_KIND_VOID) || (to == SF_TYPE_KIND_VOID))) {
        castable = false;
    }

    return castable;
}

uint8_t sf_type_kind_width_bits(sf_type_kind type) {
    switch (type) {
        case SF_TYPE_KIND_I8:
        case SF_TYPE_KIND_U8:
        case SF_TYPE_KIND_BOOL:
            return 8;
        case SF_TYPE_KIND_I16:
        case SF_TYPE_KIND_U16:
            return 16;
        case SF_TYPE_KIND_I32:
        case SF_TYPE_KIND_U32:
            return 32;
        case SF_TYPE_KIND_I64:
        case SF_TYPE_KIND_U64:
            return 64;
        case SF_TYPE_KIND_VOID:
            return 0;
        default:
            return 64;
    }
}

uint8_t sf_type_kind_width_bytes(sf_type_kind type) {
    return sf_type_kind_width_bits(type) / 8;
}

sf_type_kind sf_type_kind_promote(sf_type_kind a, sf_type_kind b) {
    return sf_type_kind_width_bits(a) >= sf_type_kind_width_bits(b) ? a : b;
}
