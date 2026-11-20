#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sulfur/pipeline/frontend/lexer/lexer.h"
#include "sulfur/shared/arena.h"
#include "sulfur/shared/operation.h"
#include "sulfur/shared/path.h"
#include "sulfur/shared/span.h"
#include "sulfur/shared/type.h"

typedef struct sf_var_symbol sf_var_symbol;

typedef enum sf_ast_node_type {
    SF_AST_NODE_TYPE_PROGRAM,
    SF_AST_NODE_TYPE_VAR_DECL,
    SF_AST_NODE_TYPE_BINARY,
    SF_AST_NODE_TYPE_UNARY,
    SF_AST_NODE_TYPE_CAST,
    SF_AST_NODE_TYPE_LITERAL,
    SF_AST_NODE_TYPE_IDENTIFIER,
    SF_AST_NODE_TYPE_VAR_ASSIGN,
    SF_AST_NODE_TYPE_BLOCK,
    SF_AST_NODE_TYPE_IF,
    SF_AST_NODE_TYPE_WHILE,
    SF_AST_NODE_TYPE_FUNC_DECL,
    SF_AST_NODE_TYPE_FUNC_CALL,
    SF_AST_NODE_TYPE_RETURN,
    SF_AST_NODE_TYPE_IMPORT,
    SF_AST_NODE_TYPE_NAMESPACE
} sf_ast_node_type;

typedef struct sf_ast_node {
    sf_ast_node_type type;
    sf_span span;
    sf_type_kind resolved;
} sf_ast_node;

typedef struct sf_ast_node_literal {
    sf_ast_node base;
    sf_lexer_token_type token_type;
    const char *value;
} sf_ast_node_literal;

typedef struct sf_ast_node_identifier {
    sf_ast_node base;
    sf_path path;
    size_t depth;
    size_t id;
    sf_var_symbol *symbol;
} sf_ast_node_identifier;

typedef struct sf_ast_node_binary {
    sf_ast_node base;
    sf_ast_node *left;
    sf_ast_node *right;
    sf_operation_type op;
} sf_ast_node_binary;

typedef struct sf_ast_node_unary {
    sf_ast_node base;
    sf_ast_node *operand;
    sf_operation_type op;
} sf_ast_node_unary;

typedef struct sf_qualifiers {
    bool is_mutable;
    bool is_public;
    bool is_extern;
} sf_qualifiers;

typedef struct sf_attributes {
    bool no_mangle;
} sf_attributes;

typedef struct sf_ast_node_var_decl {
    sf_ast_node base;
    sf_qualifiers qualifiers;
    sf_attributes attributes;
    sf_type_kind var_type;
    const char *name;
    size_t id;
    sf_ast_node *value;
} sf_ast_node_var_decl;

typedef struct sf_ast_node_var_assign {
    sf_ast_node base;
    sf_path path;
    sf_ast_node *value;
    size_t id;
} sf_ast_node_var_assign;

typedef struct sf_ast_node_block {
    sf_ast_node base;
    sf_ast_node **statements;
    size_t statement_count;
    size_t statement_capacity;
} sf_ast_node_block;

typedef struct sf_ast_node_cast {
    sf_ast_node base;
    sf_ast_node *operand;
    sf_type_kind target_type;
} sf_ast_node_cast;

typedef struct sf_ast_node_if {
    sf_ast_node base;
    sf_ast_node *condition;
    sf_ast_node *branch_then;
    sf_ast_node *branch_else;
} sf_ast_node_if;

typedef struct sf_ast_node_while {
    sf_ast_node base;
    sf_ast_node *condition;
    sf_ast_node *branch_do;
} sf_ast_node_while;

typedef struct sf_parameter {
    sf_type_kind type;
    const char *name;
    size_t id;
    sf_qualifiers qualifiers;
} sf_parameter;

typedef struct sf_ast_node_func_decl {
    sf_ast_node base;
    const char *name;
    const char *link_name;
    sf_qualifiers qualifiers;
    sf_attributes attributes;
    sf_parameter *parameters;
    size_t parameter_count;
    size_t parameter_capacity;
    sf_type_kind return_type;
    sf_ast_node *body;
} sf_ast_node_func_decl;

typedef struct sf_ast_node_func_call {
    sf_ast_node base;
    sf_ast_node *callee;
    const char *link_name;
    sf_ast_node **arguments;
    size_t argument_count;
    size_t argument_capacity;
} sf_ast_node_func_call;

typedef struct sf_ast_node_return {
    sf_ast_node base;
    sf_ast_node *value;
} sf_ast_node_return;

typedef struct sf_ast_node_import {
    sf_ast_node base;
    const char *path;
    size_t module_index;
    bool resolved;
} sf_ast_node_import;

typedef struct sf_ast_node_namespace {
    sf_ast_node base;
    sf_path path;
    sf_ast_node **items;
    size_t item_count;
    size_t item_capacity;
} sf_ast_node_namespace;

typedef struct sf_ast_node_program {
    sf_ast_node base;
    sf_ast_node **statements;
    size_t statement_count;
    size_t statement_capacity;
    sf_ast_node_import **imports;
    size_t import_count;
    size_t import_capacity;
} sf_ast_node_program;

const char *sf_path_simple_name(const sf_path *path);

void sf_print_ast(const sf_ast_node *root);

void sf_program_add_statement(
    sf_arena *arena, sf_ast_node_program *program, sf_ast_node *stmt
);
void sf_program_add_import(
    sf_arena *arena, sf_ast_node_program *program, sf_ast_node_import *import
);
void sf_block_add_statement(sf_arena *arena, sf_ast_node_block *block, sf_ast_node *stmt);
void sf_function_add_parameter(
    sf_arena *arena, sf_ast_node_func_decl *function, sf_parameter param
);
void sf_call_add_argument(
    sf_arena *arena, sf_ast_node_func_call *call, sf_ast_node *argument
);
void sf_namespace_add_item(sf_arena *arena, sf_ast_node_namespace *ns, sf_ast_node *item);
bool sf_path_add_segment(sf_arena *arena, sf_path *path, const char *name, sf_span span);

sf_ast_node_program *sf_new_program_node(sf_arena *arena);
sf_ast_node_block *sf_new_block_node(sf_arena *arena, sf_span span);
sf_ast_node_literal *sf_new_literal_node(
    sf_arena *arena, const char *value, sf_lexer_token_type token_type, sf_span span
);
sf_ast_node_identifier *sf_new_identifier_node(sf_arena *arena, sf_span span);
sf_ast_node_binary *sf_new_binary_node(
    sf_arena *arena, sf_ast_node *left, sf_ast_node *right, sf_operation_type op,
    sf_span span
);
sf_ast_node_var_decl *sf_new_var_decl_node(
    sf_arena *arena, const char *name, sf_type_kind type, sf_ast_node *value,
    sf_qualifiers qualifiers, sf_attributes attributes, sf_span span
);
sf_ast_node_var_assign *sf_new_var_assign_node(
    sf_arena *arena, sf_ast_node *value, sf_span span
);
sf_ast_node_unary *sf_new_unary_node(
    sf_arena *arena, sf_ast_node *operand, sf_operation_type op, sf_span span
);
sf_ast_node_cast *sf_new_cast_node(
    sf_arena *arena, sf_ast_node *operand, sf_type_kind target_type, sf_span span
);
sf_ast_node_if *sf_new_if_node(
    sf_arena *arena, sf_ast_node *condition, sf_ast_node *branch_then,
    sf_ast_node *branch_else, sf_span span
);
sf_ast_node_while *sf_new_while_node(
    sf_arena *arena, sf_ast_node *condition, sf_ast_node *branch_do, sf_span span
);
sf_ast_node_func_decl *sf_new_func_decl_node(
    sf_arena *arena, const char *name, sf_type_kind return_type, sf_qualifiers qualifiers,
    sf_attributes attributes, sf_span span
);
sf_ast_node_return *sf_new_return_node(sf_arena *arena, sf_ast_node *value, sf_span span);
sf_ast_node_func_call *sf_new_func_call_node(
    sf_arena *arena, sf_ast_node *callee, sf_span span
);
sf_ast_node_import *sf_new_import_node(sf_arena *arena, const char *path, sf_span span);
sf_ast_node_namespace *sf_new_namespace_node(sf_arena *arena, sf_span span);
