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

static bool analyze_expr(sf_ast_node* node, sf_value_type expected, sf_symbol_table* table, const char* filename);
static void analyze_statement(sf_ast_node* node, sf_symbol_table* table, const char* filename);

static void symbol_table_init(sf_symbol_table* table);
static sf_symbol* symbol_table_lookup(sf_symbol_table* table, const char* name);
static void symbol_table_free(sf_symbol_table* table);
static void symbol_table_insert(sf_symbol_table* table, sf_symbol symbol, const char* filename);

void sf_analyze(sf_program_node* program, const char* filename) {
	sf_symbol_table table;
	symbol_table_init(&table);

	for (uint64_t i = 0; i < program->statement_count; i++) {
		analyze_statement(program->statements[i], &table, filename);
	}

	symbol_table_free(&table);
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

static bool analyze_expr(sf_ast_node* node, sf_value_type expected, sf_symbol_table* table, const char* filename) {
	switch (node->type) {
        case SF_NODE_BINARY_EXPR: {
		    sf_binary_expr_node* bin = (sf_binary_expr_node*)node;

		    bool ok = true;
		    ok &= analyze_expr(bin->left,  SF_VAL_TYPE_UNRESOLVED, table, filename);
		    ok &= analyze_expr(bin->right, SF_VAL_TYPE_UNRESOLVED, table, filename);

		    if (!ok) return false;

		    sf_value_type ltype = bin->left->resolved;
		    sf_value_type rtype = bin->right->resolved;

		    if (ltype == SF_VAL_TYPE_UNRESOLVED || rtype == SF_VAL_TYPE_UNRESOLVED) return false;

		    if (!is_types_same_group(ltype, rtype)) {
		        sf_log_helper(
		            "Type Mismatch",
		            "Cannot mix '%s' and '%s' in binary expression",
		            "cast both operands to the same type explicitly",
		            filename,
		            SF_SEMANTIC_TYPE_MISMATCH,
		            0, 0,
		            SF_SEV_FATAL,
		            value_type_name(ltype),
		            value_type_name(rtype)
		        );
		        return false;
		    }

		    node->resolved = type_promote(ltype, rtype);
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
        	sf_symbol* sym = symbol_table_lookup(table, id->name);

			if (sym == NULL) {
				sf_log_helper(
            		"Undeclared Symbol",
            		"Symbol '%s' is undeclared",
			        "make sure the variable is declared and also isnt a typo",
			        filename,
			        SF_SEMANTIC_UNDECLARED,
			        0, 0,
			        SF_SEV_FATAL,
			        id->name
			    );
			    return false;
			}

			if (!sym->initialized) {
		        sf_log_helper(
		            "Uninitialized Variable",
		            "Variable '%s' is used before being initialized",
		            "make sure the variable is assigned a value before using it",
		            filename,
		            SF_SEMANTIC_UNINITIALIZED,
		            0, 0,
		            SF_SEV_FATAL,
		            id->name
		        );
		        return false;
		    }

			node->resolved = sym->type;
			return true;
		}

        default: return true;
    }
}

static void analyze_statement(sf_ast_node* node, sf_symbol_table* table, const char* filename) {
	switch (node->type) {
        case SF_NODE_VAR_DECL: {
        	sf_var_decl_node* var = (sf_var_decl_node*)node;

        	sf_symbol sym = {
			    .name        = var->name,
			    .type        = var->var_type,
			    .initialized = var->value != NULL,
			};

			if (var->value != NULL) {
			    if (!analyze_expr(var->value, var->var_type, table, filename)) break;

			    sf_value_type resolved = var->value->resolved;

			    if (resolved != SF_VAL_TYPE_UNRESOLVED) {
			        if (!is_types_same_group(resolved, var->var_type)) {
			            sf_log_helper(
			                "Type Mismatch",
			                "Cannot assign '%s' expression to variable of type '%s'",
			                "make sure the expression type matches the variable type",
			                filename,
			                SF_SEMANTIC_TYPE_MISMATCH,
			                0, 0,
			                SF_SEV_FATAL,
			                value_type_name(resolved),
			                value_type_name(var->var_type)
			            );
			            break;
			        }

			        if (type_width(resolved) > type_width(var->var_type)) {
			            sf_log_helper(
			                "Narrowing Conversion",
			                "Cannot implicitly narrow '%s' to '%s'",
			                "explicitly cast the value or use a wider type",
			                filename,
			                SF_SEMANTIC_TYPE_MISMATCH,
			                0, 0,
			                SF_SEV_FATAL,
			                value_type_name(resolved),
			                value_type_name(var->var_type)
			            );
			            break;
			        }
			    }
			}

			symbol_table_insert(table, sym, filename);
        	break;
        }

        case SF_NODE_VAR_ASSIGN: {
        	sf_var_assign_node* asg = (sf_var_assign_node*)node;

        	sf_symbol* sym = symbol_table_lookup(table, asg->name);

        	if (sym == NULL) {
        		sf_log_helper(
            		"Undeclared Symbol",
            		"Symbol '%s' is undeclared",
			        "make sure the variable is declared and also isnt a typo",
			        filename,
			        SF_SEMANTIC_UNDECLARED,
			        0, 0,
			        SF_SEV_FATAL,
			        asg->name
			    );
			    break;
        	}

        	if (!analyze_expr(asg->value, sym->type, table, filename)) break;

        	sf_value_type resolved = asg->value->resolved;

        	if (resolved != SF_VAL_TYPE_UNRESOLVED) {
        	    if (!is_types_same_group(resolved, sym->type)) {
        	        sf_log_helper(
        	            "Type Mismatch",
        	            "Cannot assign '%s' expression to variable of type '%s'",
        	            "make sure the expression type matches the variable type",
        	            filename,
        	            SF_SEMANTIC_TYPE_MISMATCH,
        	            0, 0,
        	            SF_SEV_FATAL,
        	            value_type_name(resolved),
        	            value_type_name(sym->type)
        	        );
        	        break;
        	    }

        	    if (type_width(resolved) > type_width(sym->type)) {
        	        sf_log_helper(
        	            "Narrowing Conversion",
        	            "Cannot implicitly narrow '%s' to '%s'",
        	            "explicitly cast the value or use a wider type",
        	            filename,
        	            SF_SEMANTIC_TYPE_MISMATCH,
        	            0, 0,
        	            SF_SEV_FATAL,
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

        default: break;
    }
}

static void symbol_table_init(sf_symbol_table* table) {
    table->symbols  = NULL;
    table->count    = 0;
    table->capacity = 0;
}

static sf_symbol* symbol_table_lookup(sf_symbol_table* table, const char* name) {
    for (uint64_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) return &table->symbols[i];
    }
    return NULL;
}

static void symbol_table_insert(sf_symbol_table* table, sf_symbol symbol, const char* filename) {
    if (symbol_table_lookup(table, symbol.name) != NULL) {
        sf_log_helper(
            "Symbol Redefinition",
            "Redefinition of '%s' symbol",
            "Dont redefine variables",
            filename,
            SF_SEMANTIC_REDECLARATION,
            0,
            0,
            SF_SEV_FATAL,
            symbol.name
        );
    }

    if (table->count >= table->capacity) {
    	table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    	table->symbols  = realloc(table->symbols, table->capacity * sizeof(sf_symbol));
	}

	table->symbols[table->count++] = symbol;
}

static void symbol_table_free(sf_symbol_table* table) {
    free(table->symbols);

    table->symbols  = NULL;
    table->count    = 0;
    table->capacity = 0;
}
