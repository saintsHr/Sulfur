#include "sulfur/ast.h"
#include "sulfur/lexer.h"
#include "sulfur/util/log.h"
#include "sulfur/semantic.h"

#include <stdint.h>
#include <string.h>

static const char* sfValueTypeName(sfValueType type) {
    switch (type) {
        case SF_VAL_TYPE_I8:  return "i8";
        case SF_VAL_TYPE_I16: return "i16";
        case SF_VAL_TYPE_I32: return "i32";
        case SF_VAL_TYPE_I64: return "i64";
        case SF_VAL_TYPE_U8:  return "u8";
        case SF_VAL_TYPE_U16: return "u16";
        case SF_VAL_TYPE_U32: return "u32";
        case SF_VAL_TYPE_U64: return "u64";
        case SF_VAL_TYPE_F32: return "f32";
        case SF_VAL_TYPE_F64: return "f64";
        default:              return "unknown";
    }
}

static bool sfTypeIsFloat(sfValueType type) {
    switch (type) {
        case SF_VAL_TYPE_F32:
        case SF_VAL_TYPE_F64:
            return true;

        default:
        	return false;
    }
}

static bool sfTypeIsInt(sfValueType type) {
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

static bool sfTypeIsUnsigned(sfValueType type) {
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

static bool sfTypeSameGroup(sfValueType a, sfValueType b) {
    if (sfTypeIsInt(a)      && sfTypeIsInt(b))      return true;
    if (sfTypeIsFloat(a)    && sfTypeIsFloat(b))    return true;
    if (sfTypeIsUnsigned(a) && sfTypeIsUnsigned(b)) return true;
    return false;
}

static int sfTypeWidth(sfValueType type) {
    switch (type) {
        case SF_VAL_TYPE_I8:  case SF_VAL_TYPE_U8:  return 8;
        case SF_VAL_TYPE_I16: case SF_VAL_TYPE_U16: return 16;
        case SF_VAL_TYPE_I32: case SF_VAL_TYPE_U32: return 32;
        case SF_VAL_TYPE_I64: case SF_VAL_TYPE_U64: return 64;
        case SF_VAL_TYPE_F32: return 32;
        case SF_VAL_TYPE_F64: return 64;
        default: return 0;
    }
}

static sfValueType sfTypePromote(sfValueType a, sfValueType b) {
    return sfTypeWidth(a) >= sfTypeWidth(b) ? a : b;
}

static bool analyze_expr(sfASTNode* node, sfValueType expected, sfSymbolTable* table, const char* filename) {
	switch (node->type) {
        case SF_NODE_BINARY_EXPR: {
		    sfBinaryExprNode* bin = (sfBinaryExprNode*)node;

		    bool ok = true;
		    ok &= analyze_expr(bin->left,  SF_VAL_TYPE_UNRESOLVED, table, filename);
		    ok &= analyze_expr(bin->right, SF_VAL_TYPE_UNRESOLVED, table, filename);

		    if (!ok) return false;

		    sfValueType ltype = bin->left->resolved;
		    sfValueType rtype = bin->right->resolved;

		    if (ltype == SF_VAL_TYPE_UNRESOLVED || rtype == SF_VAL_TYPE_UNRESOLVED) return false;

		    if (!sfTypeSameGroup(ltype, rtype)) {
		        sfLogHelper(
		            "Type Mismatch",
		            "Cannot mix '%s' and '%s' in binary expression",
		            "cast both operands to the same type explicitly",
		            filename,
		            SF_SEMANTIC_TYPE_MISMATCH,
		            0, 0,
		            SF_SEV_FATAL,
		            sfValueTypeName(ltype),
		            sfValueTypeName(rtype)
		        );
		        return false;
		    }

		    node->resolved = sfTypePromote(ltype, rtype);
		    return true;
		}

        case SF_NODE_LITERAL: {
        	sfLiteralNode* lit = (sfLiteralNode*)node;

		    if (expected == SF_VAL_TYPE_UNRESOLVED) {
		        if (lit->token_type == SF_TOKEN_TYPE_INTEGER) node->resolved = SF_VAL_TYPE_I64;
		        if (lit->token_type == SF_TOKEN_TYPE_FLOAT)   node->resolved = SF_VAL_TYPE_F64;
		        return true;
		    }

		    bool mismatch = false;

		    if (
		    	lit->token_type == SF_TOKEN_TYPE_FLOAT &&
		    	(sfTypeIsInt(expected) || sfTypeIsUnsigned(expected))
		    ) {
		    	mismatch = true;
			}

		    if (
		    	lit->token_type == SF_TOKEN_TYPE_INTEGER &&
		    	sfTypeIsFloat(expected)
		    ) {
		    	mismatch = true;
			}

		    if (mismatch) {
		        sfLogHelper(
		            "Type Mismatch",
		            "Cannot assign %s literal to variable of type '%s'",
		            "make sure the literal matches the variable type",
		            filename,
		            SF_SEMANTIC_TYPE_MISMATCH,
		            0, 0,
		            SF_SEV_FATAL,
		            lit->token_type == SF_TOKEN_TYPE_FLOAT ? "float" : "integer",
		            sfValueTypeName(expected)
		        );
		        return false;
		    }

		    node->resolved = expected;
		    return true;
        }

        case SF_NODE_IDENTIFIER: {
        	sfIdentifierNode* id = (sfIdentifierNode*)node;
        	sfSymbol* sym = sfSymbolTableLookup(table, id->name);

			if (sym == NULL) {
				sfLogHelper(
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
		        sfLogHelper(
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

static void analyze_statement(sfASTNode* node, sfSymbolTable* table, const char* filename) {
	switch (node->type) {
        case SF_NODE_VAR_DECL: {
        	sfVarDeclNode* var = (sfVarDeclNode*)node;

        	sfSymbol sym = {
			    .name        = var->name,
			    .type        = var->var_type,
			    .initialized = var->value != NULL,
			};

			if (var->value != NULL) {
			    if (!analyze_expr(var->value, var->var_type, table, filename)) break;

			    sfValueType resolved = var->value->resolved;

			    if (resolved != SF_VAL_TYPE_UNRESOLVED) {
			        if (!sfTypeSameGroup(resolved, var->var_type)) {
			            sfLogHelper(
			                "Type Mismatch",
			                "Cannot assign '%s' expression to variable of type '%s'",
			                "make sure the expression type matches the variable type",
			                filename,
			                SF_SEMANTIC_TYPE_MISMATCH,
			                0, 0,
			                SF_SEV_FATAL,
			                sfValueTypeName(resolved),
			                sfValueTypeName(var->var_type)
			            );
			            break;
			        }

			        if (sfTypeWidth(resolved) > sfTypeWidth(var->var_type)) {
			            sfLogHelper(
			                "Narrowing Conversion",
			                "Cannot implicitly narrow '%s' to '%s'",
			                "explicitly cast the value or use a wider type",
			                filename,
			                SF_SEMANTIC_TYPE_MISMATCH,
			                0, 0,
			                SF_SEV_FATAL,
			                sfValueTypeName(resolved),
			                sfValueTypeName(var->var_type)
			            );
			            break;
			        }
			    }
			}

			sfSymbolTableInsert(table, sym, filename);
        	break;
        }

        case SF_NODE_ASSIGN: {
        	sfAssignNode* asg = (sfAssignNode*)node;

        	sfSymbol* sym = sfSymbolTableLookup(table, asg->name);
        	if (sym == NULL) {
        		sfLogHelper(
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

        	sfValueType resolved = asg->value->resolved;

        	if (resolved != SF_VAL_TYPE_UNRESOLVED) {
        	    if (!sfTypeSameGroup(resolved, sym->type)) {
        	        sfLogHelper(
        	            "Type Mismatch",
        	            "Cannot assign '%s' expression to variable of type '%s'",
        	            "make sure the expression type matches the variable type",
        	            filename,
        	            SF_SEMANTIC_TYPE_MISMATCH,
        	            0, 0,
        	            SF_SEV_FATAL,
        	            sfValueTypeName(resolved),
        	            sfValueTypeName(sym->type)
        	        );
        	        break;
        	    }

        	    if (sfTypeWidth(resolved) > sfTypeWidth(sym->type)) {
        	        sfLogHelper(
        	            "Narrowing Conversion",
        	            "Cannot implicitly narrow '%s' to '%s'",
        	            "explicitly cast the value or use a wider type",
        	            filename,
        	            SF_SEMANTIC_TYPE_MISMATCH,
        	            0, 0,
        	            SF_SEV_FATAL,
        	            sfValueTypeName(resolved),
        	            sfValueTypeName(sym->type)
        	        );
        	        break;
        	    }
        	}

        	sym->initialized = true;
        	break;
        }

        default: break;
    }
}

void sfSymbolTableInit(sfSymbolTable* table) {
    table->symbols  = NULL;
    table->count    = 0;
    table->capacity = 0;
}

sfSymbol* sfSymbolTableLookup(sfSymbolTable* table, const char* name) {
    for (uint64_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) return &table->symbols[i];
    }
    return NULL;
}

void sfSymbolTableInsert(sfSymbolTable* table, sfSymbol symbol, const char* filename) {
    if (sfSymbolTableLookup(table, symbol.name) != NULL) {
        sfLogHelper(
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
    	table->symbols  = realloc(table->symbols, table->capacity * sizeof(sfSymbol));
	}

	table->symbols[table->count++] = symbol;
}

void sfSymbolTableFree(sfSymbolTable* table) {
    free(table->symbols);
    table->symbols  = NULL;
    table->count    = 0;
    table->capacity = 0;
}

void sfAnalyze(sfProgramNode* program, const char* filename) {
	sfSymbolTable table;
	sfSymbolTableInit(&table);

	for (uint64_t i = 0; i < program->statement_count; i++) {
		analyze_statement(program->statements[i], &table, filename);
	}

	sfSymbolTableFree(&table);
}
