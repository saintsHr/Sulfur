#pragma once

#include "sulfur/lexer.h"
#include <stdlib.h>
#include <stdint.h>

typedef enum {
    SF_NODE_PROGRAM,
    SF_NODE_VAR_DECL,

    SF_NODE_BINARY_EXPR,
    SF_NODE_LITERAL,
    SF_NODE_IDENTIFIER,
    SF_NODE_ASSIGN,
} sfNodeType;

typedef enum {
    SF_VAL_TYPE_I8,
    SF_VAL_TYPE_I16,
    SF_VAL_TYPE_I32,
    SF_VAL_TYPE_I64,

    SF_VAL_TYPE_U8,
    SF_VAL_TYPE_U16,
    SF_VAL_TYPE_U32,
    SF_VAL_TYPE_U64,

    SF_VAL_TYPE_F32,
    SF_VAL_TYPE_F64,

    SF_VAL_TYPE_STRING,

    SF_VAL_TYPE_UNRESOLVED,
} sfValueType;

typedef enum {
    SF_OP_TYPE_ADD,
    SF_OP_TYPE_SUB,
    SF_OP_TYPE_DIV,
    SF_OP_TYPE_MUL,
} sfOperationType;

typedef struct {
    sfNodeType type;
    sfValueType resolved;
} sfASTNode;

typedef struct {
    sfASTNode base;
    sfTokenType token_type;
    char* value;
} sfLiteralNode;

typedef struct {
    sfASTNode base;
    char* name;
} sfIdentifierNode;

typedef struct {
    sfASTNode base;
    sfASTNode* left;
    sfASTNode* right;
    sfOperationType op;
} sfBinaryExprNode;

typedef struct {
    sfASTNode base;
    char* name;
    sfValueType var_type;
    sfASTNode* value;
} sfVarDeclNode;

typedef struct {
    sfASTNode base;
    char* name;
    sfASTNode* value;
} sfAssignNode;

typedef struct {
    sfASTNode base;
    sfASTNode** statements;
    size_t statement_count;
    size_t statement_capacity;
} sfProgramNode;

void sfPrintAST(sfASTNode* root);

sfProgramNode* sfNewProgram();
void sfProgramAddStatement(sfProgramNode* program, sfASTNode* stmt);

sfLiteralNode* sfNewLiteral(const char* value, sfTokenType tokenType);

sfIdentifierNode* sfNewIdentifier(const char* name);
sfBinaryExprNode* sfNewBinary(sfASTNode* left, sfASTNode* right, sfOperationType op);
sfVarDeclNode*    sfNewVarDecl(const char* name, sfValueType type, sfASTNode* value);
sfAssignNode*     sfNewAssign(const char* name, sfASTNode* value);

void sfFreeAST(sfASTNode* node);
