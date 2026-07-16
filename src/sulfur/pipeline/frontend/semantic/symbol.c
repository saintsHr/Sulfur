#include "sulfur/pipeline/frontend/semantic/symbol.h"

#include <string.h>

void symbol_table_init(sf_symbol_table* table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

sf_symbol* symbol_table_lookup(sf_symbol_table* table, const char* name) {
    for (uint64_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0)
            return &table->symbols[i];
    }
    return NULL;
}

void symbol_table_insert(
    sf_symbol_table* table, sf_symbol symbol, const char* filename
) {
    if (table->count >= table->capacity) {
        table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        table->symbols =
            realloc(table->symbols, table->capacity * sizeof(sf_symbol));
    }

    table->symbols[table->count++] = symbol;
}

void symbol_table_free(sf_symbol_table* table) {
    free(table->symbols);

    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}