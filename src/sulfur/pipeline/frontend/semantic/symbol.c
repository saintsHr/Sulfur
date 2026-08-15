#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/utils/log.h"

#include <string.h>

void sf_var_symbol_table_init(sf_var_symbol_table *table) {
  table->symbols = NULL;
  table->count = 0;
  table->capacity = 0;
}

sf_var_symbol *sf_var_symbol_table_lookup(sf_var_symbol_table *table,
                                          const char *name) {
  for (uint64_t i = 0; i < table->count; i++) {
    if (strcmp(table->symbols[i].name, name) == 0)
      return &table->symbols[i];
  }
  return NULL;
}

void sf_var_symbol_table_free(sf_var_symbol_table *table) {
  free(table->symbols);

  table->symbols = NULL;
  table->count = 0;
  table->capacity = 0;
}

void sf_var_symbol_table_insert(sf_var_symbol_table *table,
                                sf_var_symbol symbol, const char *filename) {
  if (table->count >= table->capacity) {
    table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    table->symbols =
        realloc(table->symbols, table->capacity * sizeof(sf_var_symbol));
  }

  table->symbols[table->count++] = symbol;
}

void sf_func_symbol_table_init(sf_func_symbol_table *table) {
  table->functions = NULL;
  table->count = 0;
  table->capacity = 0;
  table->next_id = 0;
}

sf_func_symbol *sf_func_symbol_table_lookup(sf_func_symbol_table *table,
                                            const char *name) {
  for (uint64_t i = 0; i < table->count; i++) {
    if (strcmp(table->functions[i].name, name) == 0)
      return &table->functions[i];
  }
  return NULL;
}

void sf_func_symbol_table_free(sf_func_symbol_table *table) {
  free(table->functions);
  table->functions = NULL;
  table->count = 0;
  table->capacity = 0;
}

void sf_func_symbol_table_insert(sf_func_symbol_table *table,
                                 sf_func_symbol func, const char *filename) {
  sf_func_symbol *existing = sf_func_symbol_table_lookup(table, func.name);
  if (existing != NULL) {
    sf_log("symbol redefinition", "'%s' is already declared",
           "rename the new function, or remove the duplicate declaration",
           filename, SF_SEMANTIC_REDECLARATION, func.span, SF_SEV_ERROR,
           func.name);
    return;
  }

  func.id = table->next_id++;

  if (table->count >= table->capacity) {
    table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    table->functions =
        realloc(table->functions, table->capacity * sizeof(sf_func_symbol));
  }

  table->functions[table->count++] = func;
}