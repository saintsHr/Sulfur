#include "sulfur/shared/type.h"
#include "sulfur/shared/arena.h"
#include <string.h>

static const sf_type k_types[] = {
    [SF_TYPE_KIND_UNRESOLVED] = {.kind = SF_TYPE_KIND_UNRESOLVED, .pointee = NULL},
    [SF_TYPE_KIND_I8] = {.kind = SF_TYPE_KIND_I8,         .pointee = NULL},
    [SF_TYPE_KIND_I16] = {.kind = SF_TYPE_KIND_I16,        .pointee = NULL},
    [SF_TYPE_KIND_I32] = {.kind = SF_TYPE_KIND_I32,        .pointee = NULL},
    [SF_TYPE_KIND_I64] = {.kind = SF_TYPE_KIND_I64,        .pointee = NULL},
    [SF_TYPE_KIND_U8] = {.kind = SF_TYPE_KIND_U8,         .pointee = NULL},
    [SF_TYPE_KIND_U16] = {.kind = SF_TYPE_KIND_U16,        .pointee = NULL},
    [SF_TYPE_KIND_U32] = {.kind = SF_TYPE_KIND_U32,        .pointee = NULL},
    [SF_TYPE_KIND_U64] = {.kind = SF_TYPE_KIND_U64,        .pointee = NULL},
    [SF_TYPE_KIND_BOOL] = {.kind = SF_TYPE_KIND_BOOL,       .pointee = NULL},
    [SF_TYPE_KIND_VOID] = {.kind = SF_TYPE_KIND_VOID,       .pointee = NULL},
};

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

    if ((from == SF_TYPE_KIND_POINTER && to != SF_TYPE_KIND_POINTER) ||
        (to == SF_TYPE_KIND_POINTER && from != SF_TYPE_KIND_POINTER)) {
        castable = false;
    }

    return castable;
}

uint8_t sf_type_kind_width_bits(sf_type_kind type) {
    switch (type) {
        case SF_TYPE_KIND_I8:
        case SF_TYPE_KIND_U8:
        case SF_TYPE_KIND_BOOL: {
            return 8;
        }
        case SF_TYPE_KIND_I16:
        case SF_TYPE_KIND_U16: {
            return 16;
        }
        case SF_TYPE_KIND_I32:
        case SF_TYPE_KIND_U32: {
            return 32;
        }
        case SF_TYPE_KIND_I64:
        case SF_TYPE_KIND_U64:
        case SF_TYPE_KIND_POINTER: {
            return 64;
        }
        case SF_TYPE_KIND_VOID: {
            return 0;
        }

        default: {
            return 64;
        }
    }
}

uint8_t sf_type_kind_width_bytes(sf_type_kind type) {
    return sf_type_kind_width_bits(type) / 8;
}

sf_type_kind sf_type_kind_promote(sf_type_kind a, sf_type_kind b) {
    return sf_type_kind_width_bits(a) >= sf_type_kind_width_bits(b) ? a : b;
}

const sf_type *sf_type_primitive(sf_type_kind kind) {
    if (kind == SF_TYPE_KIND_POINTER) {
        return NULL;
    }

    return &k_types[kind];
}

const sf_type *sf_type_pointer_to(sf_arena *arena, const sf_type *pointee) {
    sf_type *type = sf_arena_alloc(arena, sizeof(sf_type));

    if (type == NULL) {
        return NULL;
    }

    type->kind = SF_TYPE_KIND_POINTER;
    type->pointee = pointee;

    return type;
}

bool sf_type_equals(const sf_type *a, const sf_type *b) {
    if (a == b) {
        return true;
    }

    if (a == NULL || b == NULL) {
        return false;
    }

    if (a->kind != b->kind) {
        return false;
    }

    if (a->kind == SF_TYPE_KIND_POINTER && b->kind == SF_TYPE_KIND_POINTER) {
        return sf_type_equals(a->pointee, b->pointee);
    }

    return true;
}

const char *sf_type_name(sf_arena *arena, const sf_type *type) {
    size_t stars = 0;

    while (type != NULL && type->kind == SF_TYPE_KIND_POINTER) {
        stars++;
        type = type->pointee;
    }

    const char *base = type != NULL ? sf_type_kind_name(type->kind) : "?";
    size_t base_len = strlen(base);

    char *name = sf_arena_alloc(arena, base_len + stars + 1);

    if (name == NULL) {
        return NULL;
    }

    memcpy(name, base, base_len);
    memset(name + base_len, '*', stars);
    name[base_len + stars] = '\0';

    return name;
}
