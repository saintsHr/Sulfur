#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/arena.h"
#include "sulfur/utils/span.h"

typedef struct sf_var_symbol sf_var_symbol;

typedef enum {
    SF_NODE_PROGRAM,
    SF_NODE_VAR_DECL,
    SF_NODE_BINARY,
    SF_NODE_UNARY,
    SF_NODE_CAST,
    SF_NODE_LITERAL,
    SF_NODE_IDENTIFIER,
    SF_NODE_VAR_ASSIGN,
    SF_NODE_BLOCK,
    SF_NODE_IF,
    SF_NODE_WHILE,
    SF_NODE_FUNC_DECL,
    SF_NODE_FUNC_CALL,
    SF_NODE_RETURN,
    SF_NODE_IMPORT,
    SF_NODE_NAMESPACE
} sf_node_type;

typedef enum {
    SF_VAL_TYPE_UNRESOLVED,

    SF_VAL_TYPE_I8,
    SF_VAL_TYPE_I16,
    SF_VAL_TYPE_I32,
    SF_VAL_TYPE_I64,

    SF_VAL_TYPE_U8,
    SF_VAL_TYPE_U16,
    SF_VAL_TYPE_U32,
    SF_VAL_TYPE_U64,

    SF_VAL_TYPE_BOOL,

    SF_VAL_TYPE_VOID
} sf_value_type;

typedef enum {
    SF_OP_TYPE_UNRESOLVED,

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

    SF_OP_TYPE_RELATIONAL_EQUAL,
    SF_OP_TYPE_RELATIONAL_NOT_EQUAL,
    SF_OP_TYPE_RELATIONAL_LESS,
    SF_OP_TYPE_RELATIONAL_LESS_EQUAL,
    SF_OP_TYPE_RELATIONAL_GREATER,
    SF_OP_TYPE_RELATIONAL_GREATER_EQUAL,

    SF_OP_TYPE_LOGICAL_OR,
    SF_OP_TYPE_LOGICAL_AND,
    SF_OP_TYPE_LOGICAL_NOT,

    SF_OP_TYPE_PREFIX_INCREMENT,
    SF_OP_TYPE_POSTFIX_INCREMENT,
    SF_OP_TYPE_PREFIX_DECREMENT,
    SF_OP_TYPE_POSTFIX_DECREMENT
} sf_ir_operation_type;

typedef struct {
    sf_node_type type;
    sf_value_type resolved;
    sf_span span;
} sf_ast_node;

typedef struct {
    sf_span span;
    const char *name;
} sf_path_segment;

typedef struct {
    sf_path_segment *segments;
    size_t segment_count;
    size_t segment_capacity;
    bool is_global;
} sf_path;

typedef struct {
    sf_ast_node base;
    sf_token_type token_type;
    char *value;
} sf_literal_node;

typedef struct {
    sf_ast_node base;
    sf_path path;
    size_t depth;
    size_t id;
    sf_var_symbol *symbol;
} sf_identifier_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node *left;
    sf_ast_node *right;
    sf_ir_operation_type op;
} sf_binary_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node *operand;
    sf_ir_operation_type op;
} sf_unary_node;

typedef struct {
    bool is_mutable;
    bool is_public;
    bool is_extern;
} sf_qualifiers;

typedef struct {
    sf_ast_node base;
    sf_qualifiers qualifiers;
    sf_value_type var_type;
    char *name;
    size_t id;
    sf_ast_node *value;
} sf_var_decl_node;

typedef struct {
    sf_ast_node base;
    sf_path path;
    sf_ast_node *value;
    size_t id;
} sf_var_assign_node;

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
} sf_cast_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node *condition;
    sf_ast_node *branch_then;
    sf_ast_node *branch_else;
} sf_if_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node *condition;
    sf_ast_node *branch_do;
} sf_while_node;

typedef struct {
    sf_value_type type;
    const char *name;
    size_t id;
    sf_qualifiers qualifiers;
} sf_parameter;

typedef struct {
    sf_ast_node base;
    char *name;
    const char *link_name;
    sf_qualifiers qualifiers;
    sf_parameter *parameters;
    size_t parameter_count;
    size_t parameter_capacity;
    sf_value_type return_type;
    sf_ast_node *body;
} sf_func_decl_node;

typedef struct {
    sf_ast_node base;

    sf_ast_node *callee;

    const char *link_name;

    sf_ast_node **arguments;
    size_t argument_count;
    size_t argument_capacity;
} sf_func_call_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node *value;
} sf_return_node;

typedef struct {
    sf_ast_node base;
    char *path;
    size_t module_index;
    bool resolved;
} sf_import_node;

typedef struct {
    sf_ast_node base;
    sf_path path;
    sf_ast_node **items;
    size_t item_count;
    size_t item_capacity;
} sf_namespace_node;

typedef struct {
    sf_ast_node base;
    sf_ast_node **statements;
    size_t statement_count;
    size_t statement_capacity;

    sf_import_node **imports;
    size_t import_count;
    size_t import_capacity;
} sf_program_node;

const char *sf_path_simple_name(const sf_path *path);

void sf_print_ast(const sf_ast_node *root);

void sf_program_add_statement(
    sf_arena *arena,
    sf_program_node *program,
    sf_ast_node *stmt
);
void sf_program_add_import(
    sf_arena *arena,
    sf_program_node *program,
    sf_import_node *import
);
void sf_block_add_statement(
    sf_arena *arena,
    sf_block_node *block,
    sf_ast_node *stmt
);
void sf_function_add_parameter(
    sf_arena *arena,
    sf_func_decl_node *function,
    sf_parameter param
);
void sf_call_add_argument(
    sf_arena *arena,
    sf_func_call_node *call,
    sf_ast_node *argument
);
void sf_namespace_add_item(
    sf_arena *arena,
    sf_namespace_node *namespace,
    sf_ast_node *item
);
bool sf_path_add_segment(
    sf_arena *arena,
    sf_path *path,
    const char *name,
    sf_span span
);

sf_program_node *sf_new_program_node(sf_arena *arena);
sf_block_node *sf_new_block_node(
    sf_arena *arena,
    sf_span span
);
sf_literal_node *sf_new_literal_node(
    sf_arena *arena,
    const char *value,
    sf_token_type tokenType,
    sf_span span
);
sf_identifier_node *sf_new_identifier_node(
    sf_arena *arena,
    sf_span span
);
sf_binary_node *sf_new_binary_node(
    sf_arena *arena,
    sf_ast_node *left,
    sf_ast_node *right,
    sf_ir_operation_type op,
    sf_span span
);
sf_var_decl_node *sf_new_var_decl_node(
    sf_arena *arena,
    const char *name,
    sf_value_type type,
    sf_ast_node *value,
    sf_qualifiers qualifiers,
    sf_span span
);
sf_var_assign_node *sf_new_var_assign_node(
    sf_arena *arena,
    sf_ast_node *value,
    sf_span span
);
sf_unary_node *sf_new_unary_node(
    sf_arena *arena,
    sf_ast_node *operand,
    sf_ir_operation_type op,
    sf_span span
);
sf_cast_node *sf_new_cast_node(
    sf_arena *arena,
    sf_ast_node *operand,
    sf_value_type target_type,
    sf_span span
);
sf_if_node *sf_new_if_node(
    sf_arena *arena,
    sf_ast_node *condition,
    sf_ast_node *branch_then,
    sf_ast_node *branch_else,
    sf_span span
);
sf_while_node *sf_new_while_node(
    sf_arena *arena,
    sf_ast_node *condition,
    sf_ast_node *branch_do,
    sf_span span
);
sf_func_decl_node *sf_new_func_decl_node(
    sf_arena *arena,
    const char *name,
    sf_value_type return_type,
    sf_qualifiers qualifiers,
    sf_span span
);
sf_return_node *sf_new_return_node(
    sf_arena *arena,
    sf_ast_node *value,
    sf_span span
);
sf_func_call_node *sf_new_func_call_node(
    sf_arena *arena,
    sf_ast_node *callee,
    sf_span span
);
sf_import_node *sf_new_import_node(
    sf_arena *arena,
    const char *path,
    sf_span span
);
sf_namespace_node *sf_new_namespace_node(
    sf_arena *arena,
    sf_span span
);
