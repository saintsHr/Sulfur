#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/string.h"

#include <string.h>
#include <stdlib.h>

static bool has_same_name(sf_func_symbol a, sf_func_symbol b);
static bool has_same_parameters(sf_func_symbol a, sf_func_symbol b);
static bool has_same_return_type(sf_func_symbol a, sf_func_symbol b);
static bool has_same_signature(sf_func_symbol a, sf_func_symbol b);
static bool is_definition(sf_func_symbol func);

void sf_var_symbol_table_init(sf_var_symbol_table *table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

sf_var_symbol *sf_var_symbol_table_lookup(
    sf_var_symbol_table *table, const char *name
) {
    for (uint64_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }

    return NULL;
}

void sf_var_symbol_table_free(sf_var_symbol_table *table) {
    free(table->symbols);

    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

void sf_var_symbol_table_insert(
    sf_var_symbol_table *table, sf_var_symbol symbol,
    const char *filename
) {
    if (table->count >= table->capacity) {
        table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        table->symbols = realloc(table->symbols, table->capacity * sizeof(sf_var_symbol));
    }

    table->symbols[table->count++] = symbol;
}

void sf_func_symbol_table_init(sf_func_symbol_table *table) {
    table->functions = NULL;
    table->count = 0;
    table->capacity = 0;
    table->next_id = 0;
}

sf_func_symbol *sf_func_symbol_table_lookup(
    sf_func_symbol_table *table, const char *name
) {
    for (uint64_t i = 0; i < table->count; i++) {
        if (strcmp(table->functions[i].name, name) == 0) {
            return &table->functions[i];
        }
    }

    return NULL;
}

void sf_func_symbol_table_free(sf_func_symbol_table *table) {
    free(table->functions);

    table->functions = NULL;
    table->count = 0;
    table->capacity = 0;
}

bool sf_func_symbol_table_insert(
    sf_func_symbol_table *table,
    sf_func_symbol func, const char *filename
) {
    sf_func_symbol *existing = sf_func_symbol_table_lookup(table, func.name);

    if (existing != NULL) {
        if (!has_same_signature(func, *existing)) {
            sf_log(
                "conflicting declaration",
                "'%s' was previously declared with a different signature",
                "make the return type and parameter types match the previous declaration",
                filename, SF_LOG_SEMANTIC_CONFLICTING_DECLARATION, func.span,
                SF_LOG_SEVERITY_ERROR, func.name
            );

            return false;
        }

        if (is_definition(func) && is_definition(*existing)) {
            sf_log(
                "symbol redefinition", "'%s' is already defined",
                "rename the new function, or remove the duplicate definition",
                filename, SF_LOG_SEMANTIC_REDECLARATION, func.span,
                SF_LOG_SEVERITY_ERROR, func.name
            );

            return false;
        }

        if (is_definition(func)) existing->defined = true;

        return true;
    }

    func.id = table->next_id++;

    if (table->count >= table->capacity) {
        table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        table->functions = realloc(table->functions, table->capacity * sizeof(sf_func_symbol));
    }

    table->functions[table->count++] = func;

    return true;
}

const char *sf_func_symbol_table_find_closest(
    const sf_func_symbol_table *table, const char *name
) {
    const char *best_candidate = NULL;
    size_t minimal_distance = UINT32_MAX;

    for (size_t i = 0; i < table->count; i++) {
        const char *candidate = table->functions[i].name;
        size_t distance = sf_string_distance(candidate, name);

        if (distance < minimal_distance) {
            minimal_distance = distance;
            best_candidate = candidate;
        }
    }

    static const size_t threshold = 2;

    if (minimal_distance > strlen(name) / threshold) {
        return NULL;
    }

    return best_candidate;
}

static bool has_same_name(sf_func_symbol a, sf_func_symbol b) {
    if (strcmp(a.name, b.name) == 0) return true;
    return false;
}

static bool has_same_parameters(sf_func_symbol a, sf_func_symbol b) {
    if (a.parameter_count != b.parameter_count) return false;

    for (size_t i = 0; i < a.parameter_count; i++) {
        sf_parameter a_param = a.parameters[i];
        sf_parameter b_param = b.parameters[i];

        if (a_param.type != b_param.type) return false;
    }

    return true;
}

static bool has_same_return_type(sf_func_symbol a, sf_func_symbol b) {
    return a.return_type == b.return_type;
}

static bool has_same_signature(sf_func_symbol a, sf_func_symbol b) {
    if (
        has_same_return_type(a, b) &&
        has_same_parameters(a, b) &&
        has_same_name(a, b)
    ) {
        return true;
    }

    return false;
}

static bool is_definition(sf_func_symbol func) {
    return func.defined == true;
}
