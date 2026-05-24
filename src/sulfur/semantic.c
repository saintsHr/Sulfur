#include "sulfur/ast.h"
#include "sulfur/util/log.h"
#include "sulfur/semantic.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void analyze_expr(sfASTNode* node, sfValueType expected, sfSymbolTable* table, const char* filename) {
	switch (node->type) {
        case SF_NODE_BINARY_EXPR: {
		    sfBinaryExprNode* bin = (sfBinaryExprNode*)node;
		    analyze_expr(bin->left,  expected, table, filename);
		    analyze_expr(bin->right, expected, table, filename);
		    node->resolved = expected;
		    break;
		}

        case SF_NODE_LITERAL: {
        	node->resolved = expected;
        	break;
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
			        0,
			        0,
			        SF_SEV_FATAL,
			        id->name
			    );
			}

			node->resolved = sym->type;

			break;
		}

        default: break;
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
			    analyze_expr(var->value, var->var_type, table, filename);
			}

			sfSymbolTableInsert(table, sym, filename);

        	break;
        }

        case SF_NODE_ASSIGN: {
        	sfAssignNode* asg = (sfAssignNode*)node;

        	sfSymbol* sym = (sfSymbol*)sfSymbolTableLookup(table, asg->name);
        	if (sym == NULL) {
        		sfLogHelper(
            		"Undeclared Symbol",
            		"Symbol '%s' is undeclared",
			        "make sure the variable is declared and also isnt a typo",
			        filename,
			        SF_SEMANTIC_UNDECLARED,
			        0,
			        0,
			        SF_SEV_FATAL,
			        asg->name
			    );
        	}

        	analyze_expr(asg->value, sym->type, table, filename);

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
