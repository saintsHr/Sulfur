#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/utils/log.h"
#include <string.h>

static uint32_t levenshtein_distance(const char* a, const char* b);

void scope_init(sf_scope* scope) {
	scope->stack = NULL;
    scope->depth = 0;
    scope->capacity = 0;
    scope->next_id = 0;
}

void scope_push(sf_scope* scope) {
	if (scope->depth >= scope->capacity) {
    	scope->capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;
    	scope->stack  = realloc(scope->stack, scope->capacity * sizeof(sf_symbol_table));
	}

	sf_symbol_table table;
	symbol_table_init(&table);

	scope->stack[scope->depth] = table;

	scope->depth++;
}

void scope_pop(sf_scope* scope) {
	symbol_table_free(&scope->stack[scope->depth - 1]);
	scope->depth--;
}

sf_symbol* scope_lookup(sf_scope* scope, const char* name) {
	for (int32_t i = scope->depth - 1; i >= 0; i--) {
		sf_symbol* symbol = symbol_table_lookup(&scope->stack[i], name);
		if (symbol != NULL) return symbol;
	}

	return NULL;
}

void scope_insert(sf_scope* scope, sf_symbol symbol, const char* filename) {
	sf_symbol* sym = symbol_table_lookup(&scope->stack[scope->depth - 1], symbol.name);

	if (sym != NULL) {
        sf_log(
		    "symbol redefinition",
		    "'%s' is already declared in this scope",
		    "rename the new variable, or remove the duplicate declaration",
		    filename,
		    SF_SEMANTIC_REDECLARATION,
		    symbol.span,
		    SF_SEV_ERROR,
		    symbol.name
		);

		return;
    }

    symbol.id    = scope->next_id++;
    symbol.depth = scope->depth - 1;

    sf_symbol_table* table = &scope->stack[scope->depth - 1];
    symbol_table_insert(table, symbol, filename);
}

void scope_free(sf_scope* scope) {
	while (scope->depth > 0) scope_pop(scope);
	free(scope->stack);

	scope->depth = 0;
	scope->capacity = 0;
	scope->stack = NULL;
}

const char* scope_find_closest(sf_scope* scope, const char* name) {
	const char* best_candidate = NULL;
	uint32_t minimal_distance = UINT32_MAX;

	for (int32_t i = scope->depth - 1; i >= 0; i--) {
		for (uint32_t j = 0; j < scope->stack[i].count; j++) {
			sf_symbol* sym = &scope->stack[i].symbols[j];
			uint32_t distance = levenshtein_distance(sym->name, name);

			if (distance < minimal_distance) {
				minimal_distance = distance;
				best_candidate = sym->name;
			}
		}
	}

	static const uint32_t threshold = 2;
	if (minimal_distance > strlen(name) / threshold) return NULL;

	return best_candidate;
}

static uint32_t levenshtein_distance(const char* a, const char* b) {
	uint32_t a_len = strlen(a);
	uint32_t b_len = strlen(b);
	uint32_t dp[a_len + 1][b_len + 1];

	for (uint32_t i = 0; i <= b_len; i++) {
		dp[0][i] = i;
	}
	for (uint32_t i = 0; i <= a_len; i++) {
		dp[i][0] = i;
	}

	for (uint32_t i = 1; i <= a_len; i++) {
		for (uint32_t j = 1; j <= b_len; j++) {
			if (a[i - 1] == b[j - 1]) {
				dp[i][j] = dp[i - 1][j - 1];
			} else {
				uint32_t sub = dp[i - 1][j - 1];
				uint32_t del = dp[i - 1][j];
				uint32_t ins = dp[i][j - 1];

				uint32_t min = sub;
				if (del < min) min = del;
				if (ins < min) min = ins;

				dp[i][j] = 1 + min;
			}
		}
	}

	return dp[a_len][b_len];
}