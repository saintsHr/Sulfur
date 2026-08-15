#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/frontend/ast.h"

typedef struct {
  char *name;
  sf_value_type type;
  bool initialized;
  uint32_t depth;
  uint32_t id;
  sf_span span;
} sf_var_symbol;

typedef struct {
  sf_var_symbol *symbols;
  uint32_t count;
  uint32_t capacity;
} sf_var_symbol_table;

typedef struct {
  char *name;
  sf_value_type return_type;
  sf_parameter *parameters;
  size_t parameter_count;
  sf_span span;
  uint32_t id;
} sf_func_symbol;

typedef struct {
  sf_func_symbol *functions;
  uint32_t count;
  uint32_t capacity;
  uint32_t next_id;
} sf_func_symbol_table;

void sf_var_symbol_table_init(sf_var_symbol_table *table);
sf_var_symbol *sf_var_symbol_table_lookup(sf_var_symbol_table *table,
                                          const char *name);
void sf_var_symbol_table_free(sf_var_symbol_table *table);
void sf_var_symbol_table_insert(sf_var_symbol_table *table,
                                sf_var_symbol symbol, const char *filename);

void sf_func_symbol_table_init(sf_func_symbol_table *table);
sf_func_symbol *sf_func_symbol_table_lookup(sf_func_symbol_table *table,
                                            const char *name);
void sf_func_symbol_table_free(sf_func_symbol_table *table);
void sf_func_symbol_table_insert(sf_func_symbol_table *table,
                                 sf_func_symbol func, const char *filename);