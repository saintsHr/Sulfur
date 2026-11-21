#pragma once

#include "stddef.h"

#include "sulfur/shared/attribute.h"
#include "sulfur/shared/parameter.h"
#include "sulfur/shared/qualifier.h"
#include "sulfur/shared/span.h"
#include "sulfur/shared/type.h"

typedef struct sf_symbol_var {
    const char *name;
    sf_type_kind type;
    bool initialized;
    size_t depth;
    size_t id;
    sf_span span;
    sf_qualifiers qualifiers;
    sf_attributes attributes;
} sf_symbol_var;

typedef struct sf_symbol_var_table {
    sf_symbol_var *symbols;
    size_t count;
    size_t capacity;
} sf_symbol_var_table;

typedef struct sf_symbol_func {
    char *name;
    const char *link_name;
    sf_type_kind return_type;
    sf_parameter *parameters;
    size_t parameter_count;
    bool defined;
    sf_span span;
    size_t id;
    sf_qualifiers qualifiers;
    sf_attributes attributes;
    bool imported;
} sf_symbol_func;

typedef struct sf_symbol_func_table {
    sf_symbol_func *functions;
    size_t count;
    size_t capacity;
    size_t next_id;
} sf_symbol_func_table;

void sf_symbol_var_table_init(sf_symbol_var_table *table);
sf_symbol_var *sf_symbol_var_table_lookup(sf_symbol_var_table *table, const char *name);
void sf_symbol_var_table_free(sf_symbol_var_table *table);
void sf_symbol_var_table_insert(sf_symbol_var_table *table, sf_symbol_var symbol);

void sf_symbol_func_table_init(sf_symbol_func_table *table);
sf_symbol_func *sf_symbol_func_table_lookup(
    sf_symbol_func_table *table, const char *name
);
void sf_symbol_func_table_free(sf_symbol_func_table *table);
bool sf_symbol_func_table_insert(
    sf_symbol_func_table *table, sf_symbol_func func, const char *filename
);

const char *sf_symbol_func_table_find_closest(
    const sf_symbol_func_table *table, const char *name
);
