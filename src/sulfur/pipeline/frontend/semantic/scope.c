#include "sulfur/pipeline/frontend/semantic/scope.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/utils/log.h"

#define SF_LEVENSHTEIN_MAX 256

static size_t levenshtein_distance(const char *a, const char *b);

void scope_init(sf_scope *scope) {
    scope->stack = NULL;
    scope->depth = 0;
    scope->capacity = 0;
    scope->next_id = 0;
    scope->failed_pushes = 0;
}

void scope_push(sf_scope *scope) {
    if (scope->depth >= scope->capacity) {
        size_t new_capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;

        sf_var_symbol_table *new_stack = realloc(
            scope->stack, new_capacity * sizeof(sf_var_symbol_table)
        );

        if (new_stack == NULL) {
            sf_log(
                "Insufficient Memory.", "Cannot allocate memory for compiling.",
                "Free some memory and try again.", NULL,
                SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0},
                SF_LOG_SEVERITY_FATAL
            );

            scope->failed_pushes++;
            return;
        }

        scope->stack = new_stack;
        scope->capacity = new_capacity;
    }

    sf_var_symbol_table table;
    sf_var_symbol_table_init(&table);

    scope->stack[scope->depth] = table;

    scope->depth++;
}

void scope_pop(sf_scope *scope) {
    if (scope->failed_pushes > 0) {
        scope->failed_pushes--;
        return;
    }

    if (scope->depth == 0) {
        return;
    }

    sf_var_symbol_table_free(&scope->stack[scope->depth - 1]);
    scope->depth--;
}

sf_var_symbol *scope_lookup(sf_scope *scope, const char *name) {
    for (size_t i = scope->depth - 1; i >= 0; i--) {
        sf_var_symbol *symbol = sf_var_symbol_table_lookup(&scope->stack[i], name);

        if (symbol != NULL) {
            return symbol;
        }
    }

    return NULL;
}

void scope_insert(sf_scope *scope, sf_var_symbol symbol, const char *filename) {
    if (scope->depth == 0) {
        return;
    }

    sf_var_symbol *sym = sf_var_symbol_table_lookup(
        &scope->stack[scope->depth - 1], symbol.name
    );

    if (sym != NULL) {
        sf_log(
            "symbol redefinition", "'%s' is already declared in this scope",
            "rename the new variable, or remove the duplicate declaration",
            filename, SF_LOG_SEMANTIC_REDECLARATION, symbol.span,
            SF_LOG_SEVERITY_ERROR, symbol.name
        );

        return;
    }

    symbol.id = scope->next_id++;
    symbol.depth = scope->depth - 1;

    sf_var_symbol_table *table = &scope->stack[scope->depth - 1];
    sf_var_symbol_table_insert(table, symbol, filename);
}

void scope_free(sf_scope *scope) {
    scope->failed_pushes = 0;

    while (scope->depth > 0) {
        scope_pop(scope);
    }

    free(scope->stack);

    scope->depth = 0;
    scope->capacity = 0;
    scope->stack = NULL;
}

const char *scope_find_closest(sf_scope *scope, const char *name) {
    const char *best_candidate = NULL;
    size_t minimal_distance = UINT32_MAX;

    for (size_t i = scope->depth - 1; i >= 0; i--) {
        for (size_t j = 0; j < scope->stack[i].count; j++) {
            sf_var_symbol *sym = &scope->stack[i].symbols[j];
            size_t distance = levenshtein_distance(sym->name, name);

            if (distance < minimal_distance) {
                minimal_distance = distance;
                best_candidate = sym->name;
            }
        }
    }

    static const size_t threshold = 2;

    if (minimal_distance > strlen(name) / threshold) {
        return NULL;
    }

    return best_candidate;
}

static size_t levenshtein_distance(const char *a, const char *b) {
    size_t a_len = strlen(a);
    size_t b_len = strlen(b);

    if (b_len >= SF_LEVENSHTEIN_MAX) {
        return UINT32_MAX;
    }

    size_t previous[SF_LEVENSHTEIN_MAX];
    size_t current[SF_LEVENSHTEIN_MAX];

    for (size_t j = 0; j <= b_len; j++) {
        previous[j] = j;
    }

    for (size_t i = 1; i <= a_len; i++) {
        current[0] = i;

        for (size_t j = 1; j <= b_len; j++) {
            size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            size_t best = previous[j - 1] + cost;

            if (previous[j] + 1 < best) {
                best = previous[j] + 1;
            }

            if (current[j - 1] + 1 < best) {
                best = current[j - 1] + 1;
            }

            current[j] = best;
        }

        memcpy(previous, current, (b_len + 1) * sizeof(size_t));
    }

    return previous[b_len];
}
