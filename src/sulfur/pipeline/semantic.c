#include "sulfur/pipeline/ast.h"
#include "sulfur/pipeline/lexer.h"
#include "sulfur/pipeline/semantic.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/type_utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool analyze_expr(sf_ast_node* node, sf_value_type expected, sf_scope* scope, const char* filename);
static void analyze_statement(sf_ast_node* node, sf_scope* scope, const char* filename);

static void symbol_table_init(sf_symbol_table* table);
static sf_symbol* symbol_table_lookup(sf_symbol_table* table, const char* name);
static void symbol_table_free(sf_symbol_table* table);
static void symbol_table_insert(sf_symbol_table* table, sf_symbol symbol, const char* filename);

static void scope_init(sf_scope* scope);
static void scope_push(sf_scope* scope);
static void scope_pop(sf_scope* scope);
static sf_symbol* scope_lookup(sf_scope* scope, const char* name);
static void scope_insert(sf_scope* scope, sf_symbol symbol, const char* filename);
static void scope_free(sf_scope* scope);
static const char* scope_find_closest(sf_scope* scope, const char* name);

static uint32_t levenshtein_distance(const char* a, const char* b);

static bool try_eval_const_uint(sf_ast_node* node, uint64_t* out_value);
static bool try_eval_const_int(sf_ast_node* node, int64_t* out_value);
static bool analyze_constant_overflow(sf_ast_node* node, sf_value_type resolved, const char* filename);

void sf_analyze(sf_program_node* program, const char* filename) {
	sf_scope scope;

	scope_init(&scope);
	scope_push(&scope);

	for (uint64_t i = 0; i < program->statement_count; i++) {
		analyze_statement(program->statements[i], &scope, filename);
	}

	scope_pop(&scope);
	scope_free(&scope);
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

static bool analyze_expr(sf_ast_node* node, sf_value_type expected, sf_scope* scope, const char* filename) {
	switch (node->type) {
        case SF_NODE_BINARY_EXPR: {
		    sf_binary_expr_node* bin = (sf_binary_expr_node*)node;

		    bool ok = true;
		    ok &= analyze_expr(bin->left,  expected, scope, filename);
		    ok &= analyze_expr(bin->right, expected, scope, filename);

		    if (!ok) return false;

		    sf_value_type ltype = bin->left->resolved;
		    sf_value_type rtype = bin->right->resolved;

		    if (ltype == SF_VAL_TYPE_UNRESOLVED || rtype == SF_VAL_TYPE_UNRESOLVED) return false;

		    bool is_shift = (
		    	bin->op == SF_OP_TYPE_BITWISE_LSHIFT ||
		    	bin->op == SF_OP_TYPE_BITWISE_RSHIFT
		    );

		    bool is_bitwise_logical = (
		    	bin->op == SF_OP_TYPE_BITWISE_AND || 
		        bin->op == SF_OP_TYPE_BITWISE_OR  || 
		        bin->op == SF_OP_TYPE_BITWISE_XOR
		    );

		    if (is_shift) {
		        node->resolved = ltype;
		        return true;
		    }

		    if (!type_value_is_same_group(ltype, rtype)) {
		        sf_log(
		            "type mismatch",
		            "cannot mix '%s' and '%s' in the same expression",
		            "cast one of the operands to match the other's type",
		            filename,
		            SF_SEMANTIC_TYPE_MISMATCH,
		            node->span,
		            SF_SEV_ERROR,
		            type_value_name(ltype),
		            type_value_name(rtype)
		        );
		        return false;
		    }

		    node->resolved = type_value_promote(ltype, rtype);

		    if (bin->op == SF_OP_TYPE_DIV) {
			    uint64_t u;
			    int64_t s;

			    bool zero =
			    	(type_value_is_signed(node->resolved) &&
			         try_eval_const_int(bin->right, &s) && s == 0) ||
			        (!type_value_is_signed(node->resolved) &&
			         try_eval_const_uint(bin->right, &u) && u == 0);

			    if (zero) {
			        sf_log(
			            "division by zero",
			            "constant expression divides by zero",
			            "ensure the divisor is non-zero",
			            filename,
			            SF_SEMANTIC_DIVISION_BY_ZERO,
			            node->span,
			            SF_SEV_ERROR
			        );

			        return false;
			    }
			}

			if (!analyze_constant_overflow(node, node->resolved, filename)) {
			    return false;
			}

		    return true;
		}

		case SF_NODE_UNARY_EXPR: {
		    sf_unary_expr_node* un = (sf_unary_expr_node*)node;

		    if (un->op == SF_OP_TYPE_NEGATE &&
		        un->operand->type == SF_NODE_LITERAL &&
		        ((sf_literal_node*)un->operand)->token_type == SF_TOKEN_TYPE_INTEGER) {

		        sf_literal_node* lit = (sf_literal_node*)un->operand;

		        sf_value_type target = expected;

		        if (target != SF_VAL_TYPE_UNRESOLVED && type_value_is_integer(target) && !type_value_is_signed(target)) {
			        sf_log(
			            "negative literal in unsigned context",
			            "cannot assign a negative literal to a variable of unsigned type '%s'",
			            "use a signed type, or remove the negation",
			            filename,
			            SF_SEMANTIC_TYPE_MISMATCH,
			            node->span,
			            SF_SEV_ERROR,
			            type_value_name(target)
			        );
			        return false;
			    }

			    if (target == SF_VAL_TYPE_UNRESOLVED || !type_value_is_integer(target) || !type_value_is_signed(target)) {
			        target = SF_VAL_TYPE_I64;
			    }

		        errno = 0;
		        uint64_t magnitude = strtoull(lit->value, NULL, 10);

		        if (errno == ERANGE || !type_value_signed_literal_fits_negated(target, magnitude)) {
		            sf_log(
		                "integer literal out of range",
		                "literal '-%s' does not fit in type '%s'",
		                "use a wider type, or cast explicitly if truncation is intended",
		                filename,
		                SF_SEMANTIC_LITERAL_OVERFLOW,
		                node->span,
		                SF_SEV_ERROR,
		                lit->value,
		                type_value_name(target)
		            );
		            return false;
		        }

		        lit->base.resolved = target;
		        node->resolved = target;
		        return true;
		    }

		    sf_value_type pass_down = expected;
		    if (un->op == SF_OP_TYPE_NEGATE && !type_value_is_signed(expected)) {
		        pass_down = SF_VAL_TYPE_I64;
		    }

		    if (!analyze_expr(un->operand, pass_down, scope, filename)) return false;

		    sf_value_type child_type = un->operand->resolved;
		    if (child_type == SF_VAL_TYPE_UNRESOLVED) return false;

		    node->resolved = child_type;
		    return true;
		}

        case SF_NODE_CAST_EXPR: {
		    sf_cast_expr_node* cast = (sf_cast_expr_node*)node;

		    if (!analyze_expr(cast->operand, SF_VAL_TYPE_UNRESOLVED, scope, filename)) return false;

		    sf_value_type from_type = cast->operand->resolved;
		    sf_value_type to_type = cast->target_type;

		    if (from_type == SF_VAL_TYPE_UNRESOLVED) return false;

		    if (type_value_is_castable(from_type, to_type)) {
		    	node->resolved = cast->target_type;
		    } else {
		    	sf_log(
				    "invalid cast",
				    "cannot cast '%s' to '%s'",
				    "these types are not compatible for casting",
				    filename,
				    SF_SEMANTIC_INVALID_EXPLICIT_CAST,
				    node->span,
				    SF_SEV_ERROR,
				    type_value_name(from_type),
				    type_value_name(to_type)
				);

		        return false;
		    }

		    return true;
		}

        case SF_NODE_LITERAL: {
		    sf_literal_node* lit = (sf_literal_node*)node;

		    if (lit->token_type == SF_TOKEN_TYPE_INTEGER) {
		        sf_value_type target = expected;

		        if (target == SF_VAL_TYPE_UNRESOLVED || !type_value_is_integer(target)) {
		            target = SF_VAL_TYPE_I64;
		        }

		        errno = 0;
		        char* endptr = NULL;
		        uint64_t raw = strtoull(lit->value, &endptr, 10);

		        if (errno == ERANGE) {
		            sf_log(
		                "integer literal out of range",
		                "literal '%s' is too large to represent",
		                "this value does not fit in any integer type",
		                filename,
		                SF_SEMANTIC_LITERAL_OVERFLOW,
		                node->span,
		                SF_SEV_ERROR,
		                lit->value
		            );
		            return false;
		        }

		        if (!type_value_uint_literal_fits(target, raw)) {
		            sf_log(
		                "integer literal out of range",
		                "literal '%s' does not fit in type '%s'",
		                "use a wider type, or cast explicitly if truncation is intended",
		                filename,
		                SF_SEMANTIC_LITERAL_OVERFLOW,
		                node->span,
		                SF_SEV_ERROR,
		                lit->value,
		                type_value_name(target)
		            );
		            return false;
		        }

		        node->resolved = target;
		        return true;
		    }

		    if (
		        lit->token_type == SF_TOKEN_TYPE_KW_TRUE ||
		        lit->token_type == SF_TOKEN_TYPE_KW_FALSE
		    ) {
		        node->resolved = SF_VAL_TYPE_BOOL;
		        return true;
		    }

		    return true;
		}

        case SF_NODE_IDENTIFIER: {
        	sf_identifier_node* id = (sf_identifier_node*)node;
        	sf_symbol* sym = scope_lookup(scope, id->name);

			if (sym == NULL) {
				const char* closest = scope_find_closest(scope, id->name);

				if (closest != NULL) {
					sf_log(
					    "undeclared symbol",
					    "'%s' is not declared in this scope",
					    "did u mean '%s'?",
					    filename,
					    SF_SEMANTIC_UNDECLARED,
					    node->span,
					    SF_SEV_ERROR,
					    id->name,
					    closest
					);
				} else {
					sf_log(
					    "undeclared symbol",
					    "'%s' is not declared in this scope",
					    "check for typos, or declare the variable before using it",
					    filename,
					    SF_SEMANTIC_UNDECLARED,
					    node->span,
					    SF_SEV_ERROR,
					    id->name
					);
				}

			    return false;
			}

			if (!sym->initialized) {
		        sf_log(
				    "uninitialized variable",
				    "'%s' is used before being initialized",
				    "assign a value to the variable before using it",
				    filename,
				    SF_SEMANTIC_UNINITIALIZED,
				    node->span,
				    SF_SEV_ERROR,
				    id->name
				);
		        return false;
		    }

			node->resolved = sym->type;
			id->depth = sym->depth;
			id->id = sym->id;
			return true;
		}

        default: return true;
    }
}

static void analyze_statement(sf_ast_node* node, sf_scope* scope, const char* filename) {
	switch (node->type) {
        case SF_NODE_VAR_DECL: {
        	sf_var_decl_node* var = (sf_var_decl_node*)node;

        	sf_symbol sym = {
			    .name        = var->name,
			    .type        = var->var_type,
			    .initialized = var->value != NULL,
			    .span        = var->base.span,
			};

			if (var->value != NULL) {
			    if (!analyze_expr(var->value, var->var_type, scope, filename)) break;

			    sf_value_type resolved = var->value->resolved;

			    if (resolved != SF_VAL_TYPE_UNRESOLVED) {
			        if (!type_value_is_same_group(resolved, var->var_type)) {
			            sf_log(
						    "type mismatch",
						    "cannot assign '%s' to a variable of type '%s'",
						    "make sure the expression type matches the variable type, or cast it",
						    filename,
						    SF_SEMANTIC_TYPE_MISMATCH,
						    var->base.span,
						    SF_SEV_ERROR,
						    type_value_name(resolved),
						    type_value_name(var->var_type)
						);

			            break;
			        }

			        if (type_value_width_bits(resolved) > type_value_width_bits(var->var_type)) {
			            sf_log(
						    "narrowing conversion",
						    "cannot implicitly narrow '%s' to '%s'",
						    "cast the value explicitly, or use a wider variable type",
						    filename,
						    SF_SEMANTIC_INVALID_IMPLICIT_CAST,
						    var->base.span,
						    SF_SEV_ERROR,
						    type_value_name(resolved),
						    type_value_name(var->var_type)
						);

			            break;
			        }
			    }
			}

			scope_insert(scope, sym, filename);

			var->id = scope_lookup(scope, var->name)->id;

        	break;
        }

        case SF_NODE_VAR_ASSIGN: {
        	sf_var_assign_node* asg = (sf_var_assign_node*)node;

        	sf_symbol* sym = scope_lookup(scope, asg->name);

        	if (sym == NULL) {
        		const char* closest = scope_find_closest(scope, asg->name);

				if (closest != NULL) {
					sf_log(
					    "undeclared symbol",
					    "'%s' is not declared in this scope",
					    "did u mean '%s'?",
					    filename,
					    SF_SEMANTIC_UNDECLARED,
					    node->span,
					    SF_SEV_ERROR,
					    asg->name,
					    closest
					);
				} else {
					sf_log(
					    "undeclared symbol",
					    "'%s' is not declared in this scope",
					    "check for typos, or declare the variable before assigning it",
					    filename,
					    SF_SEMANTIC_UNDECLARED,
					    node->span,
					    SF_SEV_ERROR,
					    asg->name
					);
				}

			    break;
        	}

        	asg->id = sym->id;

        	if (!analyze_expr(asg->value, sym->type, scope, filename)) break;

        	sf_value_type resolved = asg->value->resolved;

        	if (resolved != SF_VAL_TYPE_UNRESOLVED) {
        	    if (!type_value_is_same_group(resolved, sym->type)) {
        	        sf_log(
					    "type mismatch",
					    "cannot assign '%s' to a variable of type '%s'",
					    "make sure the expression type matches the variable type, or cast it",
					    filename,
					    SF_SEMANTIC_TYPE_MISMATCH,
					    asg->base.span,
					    SF_SEV_ERROR,
					    type_value_name(resolved),
					    type_value_name(sym->type)
					);

        	        break;
        	    }

        	    if (type_value_width_bits(resolved) > type_value_width_bits(sym->type)) {
        	        sf_log(
					    "narrowing conversion",
					    "cannot implicitly narrow '%s' to '%s'",
					    "cast the value explicitly, or use a wider variable type",
					    filename,
					    SF_SEMANTIC_TYPE_MISMATCH,
					    asg->base.span,
					    SF_SEV_ERROR,
					    type_value_name(resolved),
					    type_value_name(sym->type)
					);
					
        	        break;
        	    }
        	}

        	sym->initialized = true;
        	asg->base.resolved = sym->type;
        	
        	break;
        }

    	case SF_NODE_BLOCK: {
    		scope_push(scope);

			sf_block_node* block = (sf_block_node*)node;

			for (uint32_t i = 0; i < block->statement_count; i++) {
				analyze_statement(block->statements[i], scope, filename);
			}

    		scope_pop(scope);

    		break;
    	}

        default: break;
    }
}

static void symbol_table_init(sf_symbol_table* table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

static sf_symbol* symbol_table_lookup(sf_symbol_table* table, const char* name) {
    for (uint64_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) return &table->symbols[i];
    }
    return NULL;
}

static void symbol_table_insert(sf_symbol_table* table, sf_symbol symbol, const char* filename) {
    if (table->count >= table->capacity) {
    	table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    	table->symbols  = realloc(table->symbols, table->capacity * sizeof(sf_symbol));
	}

	table->symbols[table->count++] = symbol;
}

static void symbol_table_free(sf_symbol_table* table) {
    free(table->symbols);

    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

static void scope_init(sf_scope* scope) {
	scope->stack = NULL;
    scope->depth = 0;
    scope->capacity = 0;
    scope->next_id = 0;
}

static void scope_push(sf_scope* scope) {
	if (scope->depth >= scope->capacity) {
    	scope->capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;
    	scope->stack  = realloc(scope->stack, scope->capacity * sizeof(sf_symbol_table));
	}

	sf_symbol_table table;
	symbol_table_init(&table);

	scope->stack[scope->depth] = table;

	scope->depth++;
}

static void scope_pop(sf_scope* scope) {
	symbol_table_free(&scope->stack[scope->depth - 1]);
	scope->depth--;
}

static sf_symbol* scope_lookup(sf_scope* scope, const char* name) {
	for (int32_t i = scope->depth - 1; i >= 0; i--) {
		sf_symbol* symbol = symbol_table_lookup(&scope->stack[i], name);
		if (symbol != NULL) return symbol;
	}

	return NULL;
}

static void scope_insert(sf_scope* scope, sf_symbol symbol, const char* filename) {
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

static void scope_free(sf_scope* scope) {
	while (scope->depth > 0) scope_pop(scope);
	free(scope->stack);

	scope->depth = 0;
	scope->capacity = 0;
	scope->stack = NULL;
}

static const char* scope_find_closest(sf_scope* scope, const char* name) {
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

static bool try_eval_const_uint(sf_ast_node* node, uint64_t* out_value) {
    if (node->type == SF_NODE_LITERAL) {
        sf_literal_node* lit = (sf_literal_node*)node;
        if (lit->token_type != SF_TOKEN_TYPE_INTEGER) return false;

        errno = 0;
        uint64_t v = strtoull(lit->value, NULL, 10);
        if (errno == ERANGE) return false;

        *out_value = v;
        return true;
    }

    if (node->type == SF_NODE_BINARY_EXPR) {
        sf_binary_expr_node* bin = (sf_binary_expr_node*)node;
        uint64_t l, r;
        if (!try_eval_const_uint(bin->left, &l)) return false;
        if (!try_eval_const_uint(bin->right, &r)) return false;

        switch (bin->op) {
            case SF_OP_TYPE_ADD: 
                if (r > UINT64_MAX - l) return false;
                *out_value = l + r;
                return true;
            case SF_OP_TYPE_SUB: {
                if (l < r) return false;
                *out_value = l - r;
                return true;
            }
            case SF_OP_TYPE_MUL: {
                if (l != 0 && r > UINT64_MAX / l) return false;
                *out_value = l * r;
                return true;
            }
            case SF_OP_TYPE_DIV: {
			    if (r == 0) return false;
			    
			    *out_value = l / r;
			    return true;
			}
            case SF_OP_TYPE_BITWISE_LSHIFT:
                if (r >= 64) return false;
                *out_value = l << r;
                return true;
            case SF_OP_TYPE_BITWISE_RSHIFT:
                if (r >= 64) return false;
                *out_value = l >> r;
                return true;
            case SF_OP_TYPE_BITWISE_AND:
                *out_value = l & r;
                return true;
            case SF_OP_TYPE_BITWISE_OR:
                *out_value = l | r;
                return true;
            case SF_OP_TYPE_BITWISE_XOR:
                *out_value = l ^ r;
                return true;
            default:
                return false;
        }
    }

    return false;
}

static bool try_eval_const_int(sf_ast_node* node, int64_t* out_value) {
    if (node->type == SF_NODE_LITERAL) {
        sf_literal_node* lit = (sf_literal_node*)node;
        if (lit->token_type != SF_TOKEN_TYPE_INTEGER) return false;

        errno = 0;
        uint64_t v = strtoull(lit->value, NULL, 10);
        if (errno == ERANGE) return false;
        if (v > (uint64_t)INT64_MAX) return false;

        *out_value = (int64_t)v;
        return true;
    }

    if (node->type == SF_NODE_UNARY_EXPR) {
        sf_unary_expr_node* un = (sf_unary_expr_node*)node;
        if (un->op != SF_OP_TYPE_NEGATE) return false;

        if (un->operand->type == SF_NODE_LITERAL) {
            sf_literal_node* lit = (sf_literal_node*)un->operand;
            if (lit->token_type != SF_TOKEN_TYPE_INTEGER) return false;

            errno = 0;
            uint64_t magnitude = strtoull(lit->value, NULL, 10);
            if (errno == ERANGE) return false;
            if (magnitude > (uint64_t)INT64_MAX + 1) return false;

            if (magnitude == (uint64_t)INT64_MAX + 1) {
                *out_value = INT64_MIN;
            } else {
                *out_value = -(int64_t)magnitude;
            }
            return true;
        }

        int64_t child;

        if (!try_eval_const_int(un->operand, &child)) return false;
        if (child == INT64_MIN) return false;

        *out_value = -child;
        return true;
    }

    if (node->type == SF_NODE_BINARY_EXPR) {
        sf_binary_expr_node* bin = (sf_binary_expr_node*)node;

        int64_t l, r;

        if (!try_eval_const_int(bin->left, &l)) return false;
        if (!try_eval_const_int(bin->right, &r)) return false;

        switch (bin->op) {
            case SF_OP_TYPE_ADD: {
                if (r > 0 && l > INT64_MAX - r) return false;
                if (r < 0 && l < INT64_MIN - r) return false;

                *out_value = l + r;
                return true;
            }
            case SF_OP_TYPE_SUB: {
                if (r < 0 && l > INT64_MAX + r) return false;
                if (r > 0 && l < INT64_MIN + r) return false;

                *out_value = l - r;
                return true;
            }
            case SF_OP_TYPE_MUL: {
                if (l != 0 && r != 0) {
                    int64_t result = l * r;
                    if (result / l != r) return false;
                    *out_value = result;
                } else {
                    *out_value = 0;
                }
                return true;
            }
        	case SF_OP_TYPE_DIV: {
			    if (r == 0) return false;
			    if (l == INT64_MIN && r == -1) return false;

			    *out_value = l / r;
			    return true;
			}
            case SF_OP_TYPE_BITWISE_LSHIFT:
                if (r < 0 || r >= 64) return false;

                *out_value = l << r;
                return true;
            case SF_OP_TYPE_BITWISE_RSHIFT:
                if (r < 0 || r >= 64) return false;

                *out_value = l >> r;
                return true;
            case SF_OP_TYPE_BITWISE_AND:
                *out_value = l & r;
                return true;
            case SF_OP_TYPE_BITWISE_OR:
                *out_value = l | r;
                return true;
            case SF_OP_TYPE_BITWISE_XOR:
                *out_value = l ^ r;
                return true;
            default:
                return false;
        }
    }

    return false;
}

static bool analyze_constant_overflow(sf_ast_node* node, sf_value_type resolved, const char* filename) {
    if (type_value_is_signed(resolved)) {
        int64_t value;
        if (!try_eval_const_int(node, &value)) return true;

        bool fits;
        switch (resolved) {
            case SF_VAL_TYPE_I8:  fits = value >= INT8_MIN  && value <= INT8_MAX;  break;
            case SF_VAL_TYPE_I16: fits = value >= INT16_MIN && value <= INT16_MAX; break;
            case SF_VAL_TYPE_I32: fits = value >= INT32_MIN && value <= INT32_MAX; break;
            case SF_VAL_TYPE_I64: fits = true; break;
            default: fits = true; break;
        }

        if (!fits) {
            sf_log(
                "constant expression overflow",
                "the constant expression evaluates to '%lld', which does not fit in type '%s'",
                "the expression overflows at compile time; use a wider type or restructure the expression",
                filename,
                SF_SEMANTIC_LITERAL_OVERFLOW,
                node->span,
                SF_SEV_ERROR,
                (long long)value,
                type_value_name(resolved)
            );
            return false;
        }

        return true;
    } else {
        uint64_t value;
        if (!try_eval_const_uint(node, &value)) {
            if (node->type == SF_NODE_BINARY_EXPR) {
                sf_binary_expr_node* bin = (sf_binary_expr_node*)node;
                if (bin->op == SF_OP_TYPE_SUB) {
                    uint64_t l, r;
                    if (try_eval_const_uint(bin->left, &l) && try_eval_const_uint(bin->right, &r) && l < r) {
                        sf_log(
                            "constant expression underflow",
                            "the constant expression '%llu - %llu' underflows for an unsigned type",
                            "unsigned subtraction cannot produce a negative result; check operand order or use a signed type",
                            filename,
                            SF_SEMANTIC_LITERAL_OVERFLOW,
                            node->span,
                            SF_SEV_ERROR,
                            (unsigned long long)l,
                            (unsigned long long)r
                        );
                        return false;
                    }
                }
            }
            return true;
        }

        if (!type_value_uint_literal_fits(resolved, value)) {
            sf_log(
                "constant expression overflow",
                "the constant expression evaluates to '%llu', which does not fit in type '%s'",
                "the expression overflows at compile time; use a wider type or restructure the expression",
                filename,
                SF_SEMANTIC_LITERAL_OVERFLOW,
                node->span,
                SF_SEV_ERROR,
                (unsigned long long)value,
                type_value_name(resolved)
            );
            return false;
        }

        return true;
    }
}