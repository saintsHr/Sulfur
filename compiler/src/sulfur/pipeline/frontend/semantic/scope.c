#include "sulfur/pipeline/frontend/semantic/scope.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/io/log.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/shared/string.h"

void sf_scope_init(sf_scope *scope) {
    scope->stack = NULL;
    scope->depth = 0;
    scope->capacity = 0;
    scope->next_id = 0;
    scope->failed_pushes = 0;
}

void sf_scope_push(sf_scope *scope) {
    if (scope->depth >= scope->capacity) {
        size_t new_capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;

        sf_symbol_var_table *new_stack =
            realloc(scope->stack, new_capacity * sizeof(sf_symbol_var_table));

        if (new_stack == NULL) {
            sf_log_out_of_memory("N/A");

            scope->failed_pushes++;

            return;
        }

        scope->stack = new_stack;
        scope->capacity = new_capacity;
    }

    sf_symbol_var_table table;
    sf_symbol_var_table_init(&table);

    scope->stack[scope->depth] = table;

    scope->depth++;
}

void sf_scope_pop(sf_scope *scope) {
    if (scope->failed_pushes > 0) {
        scope->failed_pushes--;
        return;
    }

    if (scope->depth == 0) {
        return;
    }

    sf_symbol_var_table_free(&scope->stack[scope->depth - 1]);
    scope->depth--;
}

sf_symbol_var *sf_scope_lookup(sf_scope *scope, const char *name) {
    for (size_t i = scope->depth; i > 0; i--) {
        sf_symbol_var *symbol = sf_symbol_var_table_lookup(&scope->stack[i - 1], name);

        if (symbol != NULL) {
            return symbol;
        }
    }

    return NULL;
}

void sf_scope_insert(sf_scope *scope, sf_symbol_var symbol, const char *filename) {
    if (scope->depth == 0) {
        return;
    }

    sf_symbol_var *sym =
        sf_symbol_var_table_lookup(&scope->stack[scope->depth - 1], symbol.name);

    if (sym != NULL) {
        sf_log(
            "Symbol Redefinition", "'%s' is already declared in this scope.",
            "Rename the new variable, or remove the duplicate declaration.", filename,
            SF_LOG_CODE_SEMANTIC_REDECLARATION, symbol.span, SF_LOG_SEVERITY_ERROR,
            symbol.name
        );

        return;
    }

    symbol.id = scope->next_id++;
    symbol.depth = scope->depth - 1;

    sf_symbol_var_table *table = &scope->stack[scope->depth - 1];
    sf_symbol_var_table_insert(table, symbol);
}

void sf_scope_free(sf_scope *scope) {
    scope->failed_pushes = 0;

    while (scope->depth > 0) {
        sf_scope_pop(scope);
    }

    free(scope->stack);

    scope->depth = 0;
    scope->capacity = 0;
    scope->stack = NULL;
}

const char *sf_scope_find_closest(sf_scope *scope, const char *name) {
    const char *best_candidate = NULL;
    size_t minimal_distance = UINT32_MAX;

    for (size_t i = scope->depth; i > 0; i--) {
        for (size_t j = 0; j < scope->stack[i - 1].count; j++) {
            sf_symbol_var *sym = &scope->stack[i - 1].symbols[j];
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

sf_scope_flow sf_scope_flow_save(const sf_scope *scope) {
    sf_scope_flow flow = {0};
    size_t total = 0;

    for (size_t i = 0; i < scope->depth; i++) {
        total += scope->stack[i].count;
    }

    if (total == 0) {
        return flow;
    }

    flow.flags = malloc(total * sizeof(bool));

    if (flow.flags == NULL) {
        return flow;
    }

    size_t k = 0;

    for (size_t i = 0; i < scope->depth; i++) {
        for (size_t j = 0; j < scope->stack[i].count; j++) {
            flow.flags[k++] = scope->stack[i].symbols[j].initialized;
        }
    }

    flow.count = total;

    return flow;
}

void sf_scope_flow_restore(sf_scope *scope, const sf_scope_flow *flow) {
    size_t k = 0;

    for (size_t i = 0; i < scope->depth; i++) {
        for (size_t j = 0; j < scope->stack[i].count; j++) {
            if (k >= flow->count) {
                return;
            }

            scope->stack[i].symbols[j].initialized = flow->flags[k++];
        }
    }
}

void sf_scope_flow_fill(sf_scope_flow *flow, bool value) {
    for (size_t i = 0; i < flow->count; i++) {
        flow->flags[i] = value;
    }
}

void sf_scope_flow_and(sf_scope_flow *flow_dst, const sf_scope_flow *flow_other) {
    size_t n = flow_dst->count < flow_other->count ? flow_dst->count : flow_other->count;

    for (size_t i = 0; i < n; i++) {
        flow_dst->flags[i] = flow_dst->flags[i] && flow_other->flags[i];
    }
}

void sf_scope_flow_free(sf_scope_flow *flow) {
    free(flow->flags);
    flow->flags = NULL;
    flow->count = 0;
}
