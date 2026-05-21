#pragma once

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
} sfValueType;

typedef enum {
    SF_OP_TYPE_ADD,
    SF_OP_TYPE_SUB,
    SF_OP_TYPE_DIV,
    SF_OP_TYPE_MUL,
} sfOperationType;

typedef struct {
    sfNodeType type;
} sfASTNode;

typedef struct {
    sfASTNode base;
    sfValueType value_type;

    union {
        int8_t  i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;

        uint8_t  u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;

        float  f32;
        double f64;
    };

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

sfLiteralNode* sfNewLiteralF64(double value);
sfLiteralNode* sfNewLiteralF32(float value);

sfLiteralNode* sfNewLiteralI64(int64_t value);
sfLiteralNode* sfNewLiteralI32(int32_t value);
sfLiteralNode* sfNewLiteralI16(int16_t value);
sfLiteralNode* sfNewLiteralI8(int8_t value);

sfLiteralNode* sfNewLiteralU64(uint64_t value);
sfLiteralNode* sfNewLiteralU32(uint32_t value);
sfLiteralNode* sfNewLiteralU16(uint16_t value);
sfLiteralNode* sfNewLiteralU8(uint8_t value);

sfIdentifierNode* sfNewIdentifier(const char* name);
sfBinaryExprNode* sfNewBinary(sfASTNode* left, sfASTNode* right, sfOperationType op);
sfVarDeclNode* sfNewVarDecl(const char* name, sfValueType type, sfASTNode* value);
sfAssignNode* sfNewAssign(const char* name, sfASTNode* value);

void sfFreeAST(sfASTNode* node);
