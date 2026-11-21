#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/io/log.h"
#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/shared/string.h"

#include <stdlib.h>
#include <string.h>

static bool has_same_name(sf_symbol_func a, sf_symbol_func b);
static bool has_same_parameters(sf_symbol_func a, sf_symbol_func b);
static bool has_same_return_type(sf_symbol_func a, sf_symbol_func b);
static bool has_same_signature(sf_symbol_func a, sf_symbol_func b);
static bool is_definition(sf_symbol_func func);

void sf_symbol_var_table_init(sf_symbol_var_table *table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

sf_symbol_var *sf_symbol_var_table_lookup(sf_symbol_var_table *table, const char *name) {
    for (uint64_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }

    return NULL;
}

void sf_symbol_var_table_free(sf_symbol_var_table *table) {
    free(table->symbols);

    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

void sf_symbol_var_table_insert(sf_symbol_var_table *table, sf_symbol_var symbol) {
    if (table->count >= table->capacity) {
        table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        table->symbols = realloc(table->symbols, table->capacity * sizeof(sf_symbol_var));
    }

    table->symbols[table->count++] = symbol;
}

void sf_symbol_func_table_init(sf_symbol_func_table *table) {
    table->functions = NULL;
    table->count = 0;
    table->capacity = 0;
    table->next_id = 0;
}

sf_symbol_func *sf_symbol_func_table_lookup(
    sf_symbol_func_table *table, const char *name
) {
    for (uint64_t i = 0; i < table->count; i++) {
        if (strcmp(table->functions[i].name, name) == 0) {
            return &table->functions[i];
        }
    }

    return NULL;
}

void sf_symbol_func_table_free(sf_symbol_func_table *table) {
    free(table->functions);

    table->functions = NULL;
    table->count = 0;
    table->capacity = 0;
}

bool sf_symbol_func_table_insert(
    sf_symbol_func_table *table, sf_symbol_func func, const char *filename
) {
    if (func.qualifiers.is_extern && is_definition(func)) {
        sf_log(
            "Invalid Definition", "Extern function '%s' cannot be defined here.",
            "Do not define extern functions where it is declared.", filename,
            SF_LOG_CODE_SEMANTIC_INVALID_DEFINITION, func.span, SF_LOG_SEVERITY_ERROR,
            func.name
        );

        return false;
    }

    sf_symbol_func *existing = sf_symbol_func_table_lookup(table, func.name);

    if (existing != NULL) {
        if (!is_definition(func)) {
            sf_log(
                "Redeclaration", "'%s' is already declared.",
                "Remove the duplicate declaration.", filename,
                SF_LOG_CODE_SEMANTIC_REDECLARATION, func.span, SF_LOG_SEVERITY_WARNING,
                func.name
            );
        }

        if (!has_same_signature(func, *existing)) {
            sf_log(
                "Conflicting Declaration",
                "'%s' was previously declared with a different signature.",
                "Make the return type and parameters match the previous "
                "declaration.",
                filename, SF_LOG_CODE_SEMANTIC_CONFLICTING_DECLARATION, func.span,
                SF_LOG_SEVERITY_ERROR, func.name
            );

            return false;
        }

        if (is_definition(func) && is_definition(*existing)) {
            sf_log(
                "Redefinition", "'%s' is already defined.",
                "Rename the new function, or remove the duplicate definition.", filename,
                SF_LOG_CODE_SEMANTIC_REDEFINITION, func.span, SF_LOG_SEVERITY_ERROR,
                func.name
            );

            return false;
        }

        if (is_definition(func)) {
            existing->defined = true;
        }

        return true;
    }

    func.id = table->next_id++;

    if (table->count >= table->capacity) {
        table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        table->functions =
            realloc(table->functions, table->capacity * sizeof(sf_symbol_func));
    }

    table->functions[table->count++] = func;

    return true;
}

const char *sf_symbol_func_table_find_closest(
    const sf_symbol_func_table *table, const char *name
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

static bool has_same_name(sf_symbol_func a, sf_symbol_func b) {
    if (strcmp(a.name, b.name) == 0) {
        return true;
    }
    return false;
}

static bool has_same_parameters(sf_symbol_func a, sf_symbol_func b) {
    if (a.parameter_count != b.parameter_count) {
        return false;
    }

    for (size_t i = 0; i < a.parameter_count; i++) {
        sf_parameter a_param = a.parameters[i];
        sf_parameter b_param = b.parameters[i];

        if (a_param.type != b_param.type) {
            return false;
        }
    }

    return true;
}

static bool has_same_return_type(sf_symbol_func a, sf_symbol_func b) {
    return a.return_type == b.return_type;
}

static bool has_same_signature(sf_symbol_func a, sf_symbol_func b) {
    if (has_same_return_type(a, b) && has_same_parameters(a, b) && has_same_name(a, b)) {
        return true;
    }

    return false;
}

static bool is_definition(sf_symbol_func func) {
    return func.defined == true;
}
