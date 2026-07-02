#include "sulfur/ast.h"
#include "sulfur/lexer.h"
#include "sulfur/util/log.h"
#include "sulfur/semantic.h"

#include <stdint.h>
#include <string.h>

static const char* value_type_name(sf_value_type type);
static int type_width(sf_value_type type);
static sf_value_type type_promote(sf_value_type a, sf_value_type b);

static bool is_type_unsigned(sf_value_type type);
static bool is_type_signed(sf_value_type type);
static bool is_types_same_group(sf_value_type a, sf_value_type b);
static bool is_castable(sf_value_type from, sf_value_type to);

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

static const char* value_type_name(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:  return "i8";
        case SF_VAL_TYPE_I16: return "i16";
        case SF_VAL_TYPE_I32: return "i32";
        case SF_VAL_TYPE_I64: return "i64";
        case SF_VAL_TYPE_U8:  return "u8";
        case SF_VAL_TYPE_U16: return "u16";
        case SF_VAL_TYPE_U32: return "u32";
        case SF_VAL_TYPE_U64: return "u64";

        default:              return "unknown";
    }
}

static bool is_type_unsigned(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_U8:
        case SF_VAL_TYPE_U16:
        case SF_VAL_TYPE_U32:
        case SF_VAL_TYPE_U64:
            return true;

        default:
        	return false;
    }
}

static bool is_type_signed(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:
        case SF_VAL_TYPE_I16:
        case SF_VAL_TYPE_I32:
        case SF_VAL_TYPE_I64:
            return true;

        default:
        	return false;
    }
}

static bool is_types_same_group(sf_value_type a, sf_value_type b) {
    if (is_type_unsigned(a) && is_type_unsigned(b)) return true;
    if (is_type_signed(a) && is_type_signed(b)) return true;

    return false;
}

static bool is_castable(sf_value_type from, sf_value_type to) {
	return true;
}

static int type_width(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:  case SF_VAL_TYPE_U8:  return 8;
        case SF_VAL_TYPE_I16: case SF_VAL_TYPE_U16: return 16;
        case SF_VAL_TYPE_I32: case SF_VAL_TYPE_U32: return 32;
        case SF_VAL_TYPE_I64: case SF_VAL_TYPE_U64: return 64;
        default: return 0;
    }
}

static sf_value_type type_promote(sf_value_type a, sf_value_type b) {
    return type_width(a) >= type_width(b) ? a : b;
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

		    if (!is_types_same_group(ltype, rtype)) {
		        sf_log(
				    "type mismatch",
				    "cannot mix '%s' and '%s' in the same expression",
				    "cast one of the operands to match the other's type",
				    filename,
				    SF_SEMANTIC_TYPE_MISMATCH,
				    node->span,
				    SF_SEV_ERROR,
				    value_type_name(ltype),
				    value_type_name(rtype)
				);

		        return false;
		    }

		    node->resolved = type_promote(ltype, rtype);
		    return true;
		}

		case SF_NODE_UNARY_EXPR: {
            sf_unary_expr_node* un = (sf_unary_expr_node*)node;

            if (!analyze_expr(un->operand, expected, scope, filename)) return false;

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

		    if (is_castable(from_type, to_type)) {
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
				    value_type_name(from_type),
				    value_type_name(to_type)
				);

		        return false;
		    }

		    return true;
		}

        case SF_NODE_LITERAL: {
        	sf_literal_node* lit = (sf_literal_node*)node;

		    if (expected == SF_VAL_TYPE_UNRESOLVED) {
		        if (lit->token_type == SF_TOKEN_TYPE_INTEGER) node->resolved = SF_VAL_TYPE_I64;
		        return true;
		    }

		    node->resolved = expected;
		    return true;
        }

        case SF_NODE_IDENTIFIER: {
        	sf_identifier_node* id = (sf_identifier_node*)node;
        	sf_symbol* sym = scope_lookup(scope, id->name);

			if (sym == NULL) {
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
			        if (!is_types_same_group(resolved, var->var_type)) {
			            sf_log(
						    "type mismatch",
						    "cannot assign '%s' to a variable of type '%s'",
						    "make sure the expression type matches the variable type, or cast it",
						    filename,
						    SF_SEMANTIC_TYPE_MISMATCH,
						    var->base.span,
						    SF_SEV_ERROR,
						    value_type_name(resolved),
						    value_type_name(var->var_type)
						);

			            break;
			        }

			        if (type_width(resolved) > type_width(var->var_type)) {
			            sf_log(
						    "narrowing conversion",
						    "cannot implicitly narrow '%s' to '%s'",
						    "cast the value explicitly, or use a wider variable type",
						    filename,
						    SF_SEMANTIC_INVALID_IMPLICIT_CAST,
						    var->base.span,
						    SF_SEV_ERROR,
						    value_type_name(resolved),
						    value_type_name(var->var_type)
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
        		sf_log(
				    "undeclared symbol",
				    "'%s' is not declared in this scope",
				    "check for typos, or declare the variable before assigning to it",
				    filename,
				    SF_SEMANTIC_UNDECLARED,
				    asg->base.span,
				    SF_SEV_ERROR,
				    asg->name
				);

			    break;
        	}

        	asg->id = sym->id;

        	if (!analyze_expr(asg->value, sym->type, scope, filename)) break;

        	sf_value_type resolved = asg->value->resolved;

        	if (resolved != SF_VAL_TYPE_UNRESOLVED) {
        	    if (!is_types_same_group(resolved, sym->type)) {
        	        sf_log(
					    "type mismatch",
					    "cannot assign '%s' to a variable of type '%s'",
					    "make sure the expression type matches the variable type, or cast it",
					    filename,
					    SF_SEMANTIC_TYPE_MISMATCH,
					    asg->base.span,
					    SF_SEV_ERROR,
					    value_type_name(resolved),
					    value_type_name(sym->type)
					);

        	        break;
        	    }

        	    if (type_width(resolved) > type_width(sym->type)) {
        	        sf_log(
					    "narrowing conversion",
					    "cannot implicitly narrow '%s' to '%s'",
					    "cast the value explicitly, or use a wider variable type",
					    filename,
					    SF_SEMANTIC_TYPE_MISMATCH,
					    asg->base.span,
					    SF_SEV_ERROR,
					    value_type_name(resolved),
					    value_type_name(sym->type)
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