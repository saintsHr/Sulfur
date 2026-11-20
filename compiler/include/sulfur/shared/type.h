#pragma once

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
