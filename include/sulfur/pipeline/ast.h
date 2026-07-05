#pragma once

#include "sulfur/pipeline/lexer.h"
#include "sulfur/utils/span.h"

#include <stdlib.h>
#include <stdint.h>

typedef enum {
    SF_NODE_PROGRAM,
    SF_NODE_VAR_DECL,
    SF_NODE_BINARY_EXPR,
    SF_NODE_UNARY_EXPR,
    SF_NODE_CAST_EXPR,
    SF_NODE_LITERAL,
    SF_NODE_IDENTIFIER,
    SF_NODE_VAR_ASSIGN,
    SF_NODE_BLOCK,
} sf_node_type;

typedef enum {
    SF_VAL_TYPE_I8,
    SF_VAL_TYPE_I16,
    SF_VAL_TYPE_I32,
    SF_VAL_TYPE_I64,
    SF_VAL_TYPE_U8,
    SF_VAL_TYPE_U16,
    SF_VAL_TYPE_U32,
    SF_VAL_TYPE_U64,
    SF_VAL_TYPE_UNRESOLVED,
} sf_value_type;

typedef enum {
    SF_OP_TYPE_ADD,
    SF_OP_TYPE_SUB,
    SF_OP_TYPE_DIV,
    SF_OP_TYPE_MUL,

    SF_OP_TYPE_NEGATE,

    SF_OP_TYPE_BITWISE_AND,
    SF_OP_TYPE_BITWISE_OR,
    SF_OP_TYPE_BITWISE_XOR,
    SF_OP_TYPE_BITWISE_NOT,
    SF_OP_TYPE_BITWISE_RSHIFT,
    SF_OP_TYPE_BITWISE_LSHIFT,
} sf_operation_type;

typedef struct {
    sf_node_type type;
    sf_value_type resolved;
    sf_span span;
} sf_ast_node;

typedef struct {
    sf_ast_node base;
    sf_token_type token_type;
    char* value;
} sf_literal_node;

typedef struct {
    sf_ast_node base;
    char* name;
    uint32_t depth;
    uint32_t id;
} sf_identifier_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node* left;
    sf_ast_node* right;
    sf_operation_type op;
} sf_binary_expr_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node* operand;
    sf_operation_type op;
} sf_unary_expr_node;

typedef struct {
    sf_ast_node base;
    char* name;
    sf_value_type var_type;
    sf_ast_node* value;
    uint32_t id;
} sf_var_decl_node;

typedef struct {
    sf_ast_node base;
    char* name;
    sf_ast_node* value;
    uint32_t id;
} sf_var_assign_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node** statements;
    size_t statement_count;
    size_t statement_capacity;
} sf_program_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node** statements;
    size_t statement_count;
    size_t statement_capacity;
} sf_block_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node* operand;
    sf_value_type target_type;
} sf_cast_expr_node;

void sf_print_ast(sf_ast_node* root);

sf_program_node* sf_new_program();

void sf_program_add_statement(sf_program_node* program, sf_ast_node* stmt);
void sf_block_add_statement(sf_block_node* block, sf_ast_node* stmt);

sf_literal_node* sf_new_literal(const char* value, sf_token_type tokenType, sf_span span);
sf_identifier_node* sf_new_identifier(const char* name, sf_span span);
sf_binary_expr_node* sf_new_binary_expr(sf_ast_node* left, sf_ast_node* right, sf_operation_type op, sf_span span);
sf_var_decl_node* sf_new_var_decl(const char* name, sf_value_type type, sf_ast_node* value, sf_span span);
sf_var_assign_node* sf_new_var_assign(const char* name, sf_ast_node* value, sf_span span);
sf_block_node* sf_new_block(sf_span span);
sf_unary_expr_node* sf_new_unary_expr(sf_ast_node* operand, sf_operation_type op, sf_span span);
sf_cast_expr_node* sf_new_cast_expr(sf_ast_node* operand, sf_value_type target_type, sf_span span);

void sf_free_ast(sf_ast_node* node);