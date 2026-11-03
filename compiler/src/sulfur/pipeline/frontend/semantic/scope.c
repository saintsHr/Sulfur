#include "sulfur/pipeline/frontend/semantic/scope.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/utils/log.h"
#include "sulfur/utils/string.h"

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
    for (size_t i = scope->depth; i > 0; i--) {
        sf_var_symbol *symbol = sf_var_symbol_table_lookup(&scope->stack[i - 1], name);

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

    for (size_t i = scope->depth; i > 0; i--) {
        for (size_t j = 0; j < scope->stack[i - 1].count; j++) {
            sf_var_symbol *sym = &scope->stack[i - 1].symbols[j];
            size_t distance = sf_string_distance(sym->name, name);

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

sf_flow_state scope_flow_save(const sf_scope *scope) {
    sf_flow_state state = {0};
    size_t total = 0;

    for (size_t i = 0; i < scope->depth; i++) {
        total += scope->stack[i].count;
    }

    if (total == 0) {
        return state;
    }

    state.flags = malloc(total * sizeof(bool));

    if (state.flags == NULL) {
        return state;
    }

    size_t k = 0;

    for (size_t i = 0; i < scope->depth; i++) {
        for (size_t j = 0; j < scope->stack[i].count; j++) {
            state.flags[k++] = scope->stack[i].symbols[j].initialized;
        }
    }

    state.count = total;

    return state;
}

void scope_flow_restore(sf_scope *scope, const sf_flow_state *state) {
    size_t k = 0;

    for (size_t i = 0; i < scope->depth; i++) {
        for (size_t j = 0; j < scope->stack[i].count; j++) {
            if (k >= state->count) {
                return;
            }

            scope->stack[i].symbols[j].initialized = state->flags[k++];
        }
    }
}

void scope_flow_fill(sf_flow_state *state, bool value) {
    for (size_t i = 0; i < state->count; i++) {
        state->flags[i] = value;
    }
}

void scope_flow_and(sf_flow_state *dst, const sf_flow_state *other) {
    size_t n = dst->count < other->count ? dst->count : other->count;

    for (size_t i = 0; i < n; i++) {
        dst->flags[i] = dst->flags[i] && other->flags[i];
    }
}

void scope_flow_free(sf_flow_state *state) {
    free(state->flags);
    state->flags = NULL;
    state->count = 0;
}
