#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/arena.h"
#include "sulfur/utils/span.h"

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
  SF_NODE_IF_STMT,
  SF_NODE_WHILE_STMT,
  SF_NODE_FUNC_DECL,
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

  SF_VAL_TYPE_BOOL,

  SF_VAL_TYPE_VOID,

  SF_VAL_TYPE_UNRESOLVED,
} sf_value_type;

typedef enum {
  // arithmetic
  SF_OP_TYPE_ADD,
  SF_OP_TYPE_SUB,
  SF_OP_TYPE_DIV,
  SF_OP_TYPE_MUL,
  SF_OP_TYPE_NEGATE,

  // bitwise
  SF_OP_TYPE_BITWISE_AND,
  SF_OP_TYPE_BITWISE_OR,
  SF_OP_TYPE_BITWISE_XOR,
  SF_OP_TYPE_BITWISE_NOT,
  SF_OP_TYPE_BITWISE_RSHIFT,
  SF_OP_TYPE_BITWISE_LSHIFT,

  // relational
  SF_OP_TYPE_RELATIONAL_EQUAL,
  SF_OP_TYPE_RELATIONAL_NOT_EQUAL,
  SF_OP_TYPE_RELATIONAL_LESS,
  SF_OP_TYPE_RELATIONAL_LESS_EQUAL,
  SF_OP_TYPE_RELATIONAL_GREATER,
  SF_OP_TYPE_RELATIONAL_GREATER_EQUAL,

  // logical
  SF_OP_TYPE_LOGICAL_OR,
  SF_OP_TYPE_LOGICAL_AND,
  SF_OP_TYPE_LOGICAL_NOT,

  // increment & decrement
  SF_OP_TYPE_PREFIX_INCREMENT,
  SF_OP_TYPE_POSTFIX_INCREMENT,
  SF_OP_TYPE_PREFIX_DECREMENT,
  SF_OP_TYPE_POSTFIX_DECREMENT,

  // fallback
  SF_OP_TYPE_UNRESOLVED
} sf_operation_type;

typedef struct {
  sf_value_type type;
  char *name;
} sf_parameter;

typedef struct {
  sf_node_type type;
  sf_value_type resolved;
  sf_span span;
} sf_ast_node;

typedef struct {
  sf_ast_node base;
  sf_token_type token_type;
  char *value;
} sf_literal_node;

typedef struct {
  sf_ast_node base;
  char *name;
  uint32_t depth;
  uint32_t id;
} sf_identifier_node;

typedef struct {
  sf_ast_node base;
  sf_ast_node *left;
  sf_ast_node *right;
  sf_operation_type op;
} sf_binary_expr_node;

typedef struct {
  sf_ast_node base;
  sf_ast_node *operand;
  sf_operation_type op;
} sf_unary_expr_node;

typedef struct {
  sf_ast_node base;
  char *name;
  sf_value_type var_type;
  sf_ast_node *value;
  uint32_t id;
} sf_var_decl_node;

typedef struct {
  sf_ast_node base;
  char *name;
  sf_ast_node *value;
  uint32_t id;
} sf_var_assign_node;

typedef struct {
  sf_ast_node base;
  sf_ast_node **statements;
  size_t statement_count;
  size_t statement_capacity;
} sf_program_node;

typedef struct {
  sf_ast_node base;
  sf_ast_node **statements;
  size_t statement_count;
  size_t statement_capacity;
} sf_block_node;

typedef struct {
  sf_ast_node base;
  sf_ast_node *operand;
  sf_value_type target_type;
} sf_cast_expr_node;

typedef struct {
  sf_ast_node base;
  sf_ast_node *condition;
  sf_ast_node *branch_then;
  sf_ast_node *branch_else;
} sf_if_stmt_node;

typedef struct {
  sf_ast_node base;
  sf_ast_node *condition;
  sf_ast_node *branch_do;
} sf_while_stmt_node;

typedef struct {
  sf_ast_node base;
  char *name;
  sf_parameter *parameters;
  size_t parameter_count;
  size_t parameter_capacity;
  sf_value_type return_type;
  sf_ast_node *body;
} sf_func_decl_node;

void sf_print_ast(sf_ast_node *root);

void sf_program_add_statement(sf_arena *arena, sf_program_node *program,
                              sf_ast_node *stmt);
void sf_block_add_statement(sf_arena *arena, sf_block_node *block,
                            sf_ast_node *stmt);
void sf_function_add_parameter(sf_arena *arena, sf_func_decl_node *function,
                               sf_parameter param);

sf_program_node *sf_new_program(sf_arena *arena);

sf_block_node *sf_new_block(sf_arena *arena, sf_span span);

sf_literal_node *sf_new_literal(sf_arena *arena, const char *value,
                                sf_token_type tokenType, sf_span span);

sf_identifier_node *sf_new_identifier(sf_arena *arena, const char *name,
                                      sf_span span);

sf_binary_expr_node *sf_new_binary_expr(sf_arena *arena, sf_ast_node *left,
                                        sf_ast_node *right,
                                        sf_operation_type op, sf_span span);

sf_var_decl_node *sf_new_var_decl(sf_arena *arena, const char *name,
                                  sf_value_type type, sf_ast_node *value,
                                  sf_span span);

sf_var_assign_node *sf_new_var_assign(sf_arena *arena, const char *name,
                                      sf_ast_node *value, sf_span span);

sf_unary_expr_node *sf_new_unary_expr(sf_arena *arena, sf_ast_node *operand,
                                      sf_operation_type op, sf_span span);

sf_cast_expr_node *sf_new_cast_expr(sf_arena *arena, sf_ast_node *operand,
                                    sf_value_type target_type, sf_span span);

sf_if_stmt_node *sf_new_if_stmt(sf_arena *arena, sf_ast_node *condition,
                                sf_ast_node *branch_then,
                                sf_ast_node *branch_else, sf_span span);

sf_while_stmt_node *sf_new_while_stmt(sf_arena *arena, sf_ast_node *condition,
                                      sf_ast_node *branch_do, sf_span span);

sf_func_decl_node *sf_new_func_decl(sf_arena *arena, const char *name,
                                    sf_value_type return_type, sf_span span);