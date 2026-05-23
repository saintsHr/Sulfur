#include "sulfur/ast.h"
#include "sulfur/util/log.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

static void sfPrintIndent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static const char* sfOpToString(sfOperationType op) {
    switch(op) {
        case SF_OP_TYPE_ADD: return "+";
        case SF_OP_TYPE_SUB: return "-";
        case SF_OP_TYPE_MUL: return "*";
        case SF_OP_TYPE_DIV: return "/";
        default: return "?";
    }
}

static const char* sfTypeToString(sfValueType type) {
    switch(type) {
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

        case SF_VAL_TYPE_STRING: return "string";

        default: return "?";
    }
}

static void sfPrintASTNode(sfASTNode* node, int indent) {
    if (!node) return;

    switch (node->type) {

        case SF_NODE_PROGRAM: {
            sfProgramNode* prog = (sfProgramNode*)node;

            sfPrintIndent(indent);
            printf("Program\n");

            for (size_t i = 0; i < prog->statement_count; i++) {
                sfPrintASTNode(prog->statements[i], indent + 1);
            }
            break;
        }

        case SF_NODE_VAR_DECL: {
            sfVarDeclNode* var = (sfVarDeclNode*)node;

            sfPrintIndent(indent);
            printf("VarDecl %s : %s\n", var->name, sfTypeToString(var->var_type));

            sfPrintASTNode(var->value, indent + 1);
            break;
        }

        case SF_NODE_ASSIGN: {
            sfAssignNode* asg = (sfAssignNode*)node;

            sfPrintIndent(indent);
            printf("Assign %s\n", asg->name);

            sfPrintASTNode(asg->value, indent + 1);
            break;
        }

        case SF_NODE_BINARY_EXPR: {
            sfBinaryExprNode* bin = (sfBinaryExprNode*)node;

            sfPrintIndent(indent);
            printf("Binary %s\n", sfOpToString(bin->op));

            sfPrintASTNode(bin->left, indent + 1);
            sfPrintASTNode(bin->right, indent + 1);
            break;
        }

        case SF_NODE_IDENTIFIER: {
            sfIdentifierNode* id = (sfIdentifierNode*)node;

            sfPrintIndent(indent);
            printf("Identifier %s\n", id->name);
            break;
        }

        case SF_NODE_LITERAL: {
            sfLiteralNode* lit = (sfLiteralNode*)node;
            sfPrintIndent(indent);
            printf("Literal %s\n", lit->value);
            break;
        }
    }
}

void sfPrintAST(sfASTNode* root) {
    sfPrintASTNode(root, 0);
}

sfProgramNode* sfNewProgram() {
    sfProgramNode* program = malloc(sizeof(sfProgramNode));

    program->base.type = SF_NODE_PROGRAM;
    program->statements = NULL;
    program->statement_count = 0;
    program->statement_capacity = 0;

    return program;
}

void sfProgramAddStatement(sfProgramNode* program, sfASTNode* stmt) {
    if (program->statement_count >= program->statement_capacity) {
        program->statement_capacity = program->statement_capacity == 0 ? 8 : program->statement_capacity * 2;
        
        sfASTNode** newStatements = realloc(
            program->statements,
            program->statement_capacity * sizeof(sfASTNode*)
        );

        if (!newStatements) {
            sfLogHelper(
                "Allocation Failed",
                "AST program memory reallocation failed.",
                "Make sure you have enough memory and try again.",
                NULL,
                SF_AST_REALLOC_FAILED,
                0,
                0,
                SF_SEV_FATAL,
                "N/A"
            );
        }

        program->statements = newStatements;
    }

    program->statements[program->statement_count++] = stmt;
}

sfIdentifierNode* sfNewIdentifier(const char* name) {
    sfIdentifierNode* node = malloc(sizeof(sfIdentifierNode));
    node->base.type = SF_NODE_IDENTIFIER;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->name = strdup(name);
    return node;
}

sfLiteralNode* sfNewLiteral(const char* value, sfTokenType tokenType) {
    sfLiteralNode* node = malloc(sizeof(sfLiteralNode));
    node->base.type = SF_NODE_LITERAL;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->token_type = tokenType;
    node->value = strdup(value);
    return node;
}

sfBinaryExprNode* sfNewBinary(sfASTNode* left, sfASTNode* right, sfOperationType op) {
    sfBinaryExprNode* node = malloc(sizeof(sfBinaryExprNode));
    node->base.type = SF_NODE_BINARY_EXPR;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->left = left;
    node->right = right;
    node->op = op;
    return node;
}

sfVarDeclNode* sfNewVarDecl(const char* name, sfValueType type, sfASTNode* value) {
    sfVarDeclNode* node = malloc(sizeof(sfVarDeclNode));
    node->base.type = SF_NODE_VAR_DECL;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->name = strdup(name);
    node->var_type = type;
    node->value = value;
    return node;
}

sfAssignNode* sfNewAssign(const char* name, sfASTNode* value) {
    sfAssignNode* node = malloc(sizeof(sfAssignNode));
    node->base.type = SF_NODE_ASSIGN;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->name = strdup(name);
    node->value = value;
    return node;
}

void sfFreeAST(sfASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case SF_NODE_LITERAL: {
            sfLiteralNode* lit = (sfLiteralNode*)node;
            free(lit->value);
            free(node);
            break;
        }

        case SF_NODE_IDENTIFIER: {
            sfIdentifierNode* id = (sfIdentifierNode*)node;
            free(id->name);
            free(id);
            break;
        }

        case SF_NODE_BINARY_EXPR: {
            sfBinaryExprNode* bin = (sfBinaryExprNode*)node;
            sfFreeAST(bin->left);
            sfFreeAST(bin->right);
            free(bin);
            break;
        }

        case SF_NODE_VAR_DECL: {
            sfVarDeclNode* var = (sfVarDeclNode*)node;
            free(var->name);
            sfFreeAST(var->value);
            free(var);
            break;
        }

        case SF_NODE_ASSIGN: {
            sfAssignNode* asg = (sfAssignNode*)node;
            free(asg->name);
            sfFreeAST(asg->value);
            free(asg);
            break;
        }

        case SF_NODE_PROGRAM: {
            sfProgramNode* prog = (sfProgramNode*)node;

            for (size_t i = 0; i < prog->statement_count; i++)
                sfFreeAST(prog->statements[i]);

            free(prog->statements);
            free(prog);
            break;
        }
    }
}
