#include "sulfur/pipeline/frontend/ast.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sulfur/utils/arena.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/string.h"
#include "sulfur/utils/type_utils.h"

static void print_indent(int indent);
static void print_ast_node(sf_ast_node *node, int indent);
static void print_var_assign(const sf_ast_node *node, int indent);
static void print_var_decl(const sf_ast_node *node, int indent);
static void print_literal(const sf_ast_node *node, int indent);
static void print_ident(const sf_ast_node *node, int indent);
static void print_program(const sf_ast_node *node, int indent);
static void print_binary_expr(const sf_ast_node *node, int indent);
static void print_unary_expr(const sf_ast_node *node, int indent);
static void print_block(const sf_ast_node *node, int indent);
static void print_cast_expr(const sf_ast_node *node, int indent);
static void print_if_stmt(const sf_ast_node *node, int indent);
static void print_while_stmt(const sf_ast_node *node, int indent);
static void print_func_decl(const sf_ast_node *node, int indent);

sf_program_node *sf_new_program(sf_arena *arena) {
  sf_program_node *program = sf_arena_alloc(arena, sizeof(sf_program_node));

  program->base.type = SF_NODE_PROGRAM;
  program->base.span = (sf_span){0};
  program->statements = NULL;
  program->statement_count = 0;
  program->statement_capacity = 0;

  return program;
}

sf_block_node *sf_new_block(sf_arena *arena, sf_span span) {
  sf_block_node *block = sf_arena_alloc(arena, sizeof(sf_block_node));

  block->base.type = SF_NODE_BLOCK;
  block->base.span = span;
  block->statements = NULL;
  block->statement_count = 0;
  block->statement_capacity = 0;

  return block;
}

void sf_program_add_statement(sf_arena *arena, sf_program_node *program,
                              sf_ast_node *stmt) {
  if (program->statement_count >= program->statement_capacity) {
    program->statement_capacity =
        program->statement_capacity == 0 ? 8 : program->statement_capacity * 2;

    sf_ast_node **new_statements = sf_arena_grow_array(
        arena, program->statements, program->statement_count,
        program->statement_capacity, sizeof(sf_ast_node *));

    if (!new_statements) {
      sf_log("Insufficient Memory.", "Cannot allocate memory for compiling.",
             "Free some memory and try again.", NULL,
             SF_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_SEV_FATAL);
    }

    program->statements = new_statements;
  }

  program->statements[program->statement_count++] = stmt;
}

void sf_block_add_statement(sf_arena *arena, sf_block_node *block,
                            sf_ast_node *stmt) {
  if (block->statement_count >= block->statement_capacity) {
    block->statement_capacity =
        block->statement_capacity == 0 ? 8 : block->statement_capacity * 2;

    sf_ast_node **new_statements =
        sf_arena_grow_array(arena, block->statements, block->statement_count,
                            block->statement_capacity, sizeof(sf_ast_node *));

    if (!new_statements) {
      sf_log("Insufficient Memory.", "Cannot allocate memory for compiling.",
             "Free some memory and try again.", NULL,
             SF_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_SEV_FATAL);
    }

    block->statements = new_statements;
  }

  block->statements[block->statement_count++] = stmt;
}

void sf_function_add_parameter(sf_arena *arena, sf_func_decl_node *function,
                               sf_parameter param) {
  if (function->parameter_count >= function->parameter_capacity) {
    function->parameter_capacity = function->parameter_capacity == 0
                                       ? 8
                                       : function->parameter_capacity * 2;

    sf_parameter *new_parameters = sf_arena_grow_array(
        arena, function->parameters, function->parameter_count,
        function->parameter_capacity, sizeof(sf_parameter));

    if (!new_parameters) {
      sf_log("Insufficient Memory.", "Cannot allocate memory for compiling.",
             "Free some memory and try again.", NULL,
             SF_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_SEV_FATAL);
    }

    function->parameters = new_parameters;
  }

  function->parameters[function->parameter_count++] = param;
}

sf_identifier_node *sf_new_identifier(sf_arena *arena, const char *name,
                                      sf_span span) {
  sf_identifier_node *node = sf_arena_alloc(arena, sizeof(sf_identifier_node));

  node->base.type = SF_NODE_IDENTIFIER;
  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.span = span;
  node->name = sf_strdup_arena(arena, name);

  return node;
}

sf_literal_node *sf_new_literal(sf_arena *arena, const char *value,
                                sf_token_type token_type, sf_span span) {
  sf_literal_node *node = sf_arena_alloc(arena, sizeof(sf_literal_node));

  node->base.type = SF_NODE_LITERAL;
  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.span = span;
  node->token_type = token_type;
  node->value = sf_strdup_arena(arena, value);

  return node;
}

sf_binary_expr_node *sf_new_binary_expr(sf_arena *arena, sf_ast_node *left,
                                        sf_ast_node *right,
                                        sf_operation_type op, sf_span span) {
  sf_binary_expr_node *node =
      sf_arena_alloc(arena, sizeof(sf_binary_expr_node));

  node->base.type = SF_NODE_BINARY_EXPR;
  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.span = span;
  node->left = left;
  node->right = right;
  node->op = op;

  return node;
}

sf_unary_expr_node *sf_new_unary_expr(sf_arena *arena, sf_ast_node *operand,
                                      sf_operation_type op, sf_span span) {
  sf_unary_expr_node *node = sf_arena_alloc(arena, sizeof(sf_unary_expr_node));

  node->base.type = SF_NODE_UNARY_EXPR;
  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.span = span;
  node->op = op;
  node->operand = operand;

  return node;
}

sf_cast_expr_node *sf_new_cast_expr(sf_arena *arena, sf_ast_node *operand,
                                    sf_value_type target_type, sf_span span) {
  sf_cast_expr_node *node = sf_arena_alloc(arena, sizeof(sf_cast_expr_node));

  node->base.type = SF_NODE_CAST_EXPR;
  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.span = span;
  node->target_type = target_type;
  node->operand = operand;

  return node;
}

sf_var_decl_node *sf_new_var_decl(sf_arena *arena, const char *name,
                                  sf_value_type type, sf_ast_node *value,
                                  sf_span span) {
  sf_var_decl_node *node = sf_arena_alloc(arena, sizeof(sf_var_decl_node));

  node->base.type = SF_NODE_VAR_DECL;
  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.span = span;
  node->name = sf_strdup_arena(arena, name);
  node->var_type = type;
  node->value = value;

  return node;
}

sf_var_assign_node *sf_new_var_assign(sf_arena *arena, const char *name,
                                      sf_ast_node *value, sf_span span) {
  sf_var_assign_node *node = sf_arena_alloc(arena, sizeof(sf_var_assign_node));

  node->base.type = SF_NODE_VAR_ASSIGN;
  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.span = span;
  node->name = sf_strdup_arena(arena, name);
  node->value = value;

  return node;
}

sf_if_stmt_node *sf_new_if_stmt(sf_arena *arena, sf_ast_node *condition,
                                sf_ast_node *branch_then,
                                sf_ast_node *branch_else, sf_span span) {
  sf_if_stmt_node *node = sf_arena_alloc(arena, sizeof(sf_if_stmt_node));

  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.type = SF_NODE_IF_STMT;
  node->base.span = span;
  node->branch_then = branch_then;
  node->branch_else = branch_else;
  node->condition = condition;

  return node;
}

sf_while_stmt_node *sf_new_while_stmt(sf_arena *arena, sf_ast_node *condition,
                                      sf_ast_node *branch_do, sf_span span) {
  sf_while_stmt_node *node = sf_arena_alloc(arena, sizeof(sf_while_stmt_node));

  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.type = SF_NODE_WHILE_STMT;
  node->base.span = span;
  node->branch_do = branch_do;
  node->condition = condition;

  return node;
}

sf_func_decl_node *sf_new_func_decl(sf_arena *arena, const char *name,
                                    sf_value_type return_type, sf_span span) {
  sf_func_decl_node *node = sf_arena_alloc(arena, sizeof(sf_func_decl_node));

  node->base.type = SF_NODE_FUNC_DECL;
  node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
  node->base.span = span;

  node->name = sf_strdup_arena(arena, name);
  node->return_type = return_type;
  node->parameters = NULL;
  node->parameter_capacity = 0;
  node->parameter_count = 0;
  node->body = NULL;

  return node;
}

void sf_print_ast(sf_ast_node *root) { print_ast_node(root, 0); }

static void print_indent(int indent) {
  for (int i = 0; i < indent; i++)
    printf("  ");
}

static void print_ast_node(sf_ast_node *node, int indent) {
  if (!node)
    return;

  switch (node->type) {
  case SF_NODE_PROGRAM:
    print_program(node, indent);
    break;
  case SF_NODE_VAR_DECL:
    print_var_decl(node, indent);
    break;
  case SF_NODE_VAR_ASSIGN:
    print_var_assign(node, indent);
    break;
  case SF_NODE_BINARY_EXPR:
    print_binary_expr(node, indent);
    break;
  case SF_NODE_UNARY_EXPR:
    print_unary_expr(node, indent);
    break;
  case SF_NODE_IDENTIFIER:
    print_ident(node, indent);
    break;
  case SF_NODE_LITERAL:
    print_literal(node, indent);
    break;
  case SF_NODE_BLOCK:
    print_block(node, indent);
    break;
  case SF_NODE_CAST_EXPR:
    print_cast_expr(node, indent);
    break;
  case SF_NODE_IF_STMT:
    print_if_stmt(node, indent);
    break;
  case SF_NODE_WHILE_STMT:
    print_while_stmt(node, indent);
    break;
  case SF_NODE_FUNC_DECL:
    print_func_decl(node, indent);
    break;
  default:
    break;
  }
}

static void print_var_assign(const sf_ast_node *node, int indent) {
  sf_var_assign_node *asg = (sf_var_assign_node *)node;

  print_indent(indent);
  printf("Assign %s\n", asg->name);

  print_ast_node(asg->value, indent + 1);
}

static void print_var_decl(const sf_ast_node *node, int indent) {
  sf_var_decl_node *var = (sf_var_decl_node *)node;

  print_indent(indent);
  printf("VarDecl %s : %s\n", var->name, type_value_name(var->var_type));

  print_ast_node(var->value, indent + 1);
}

static void print_literal(const sf_ast_node *node, int indent) {
  sf_literal_node *lit = (sf_literal_node *)node;

  print_indent(indent);
  printf("Literal %s\n", lit->value);
}

static void print_ident(const sf_ast_node *node, int indent) {
  sf_identifier_node *id = (sf_identifier_node *)node;

  print_indent(indent);
  printf("Identifier %s\n", id->name);
}

static void print_program(const sf_ast_node *node, int indent) {
  sf_program_node *prog = (sf_program_node *)node;

  print_indent(indent);
  printf("Program\n");

  for (size_t i = 0; i < prog->statement_count; i++) {
    print_ast_node(prog->statements[i], indent + 1);
  }
}

static void print_binary_expr(const sf_ast_node *node, int indent) {
  sf_binary_expr_node *bin = (sf_binary_expr_node *)node;

  print_indent(indent);
  printf("Binary %s\n", type_operation_name(bin->op));

  print_ast_node(bin->left, indent + 1);
  print_ast_node(bin->right, indent + 1);
}

static void print_unary_expr(const sf_ast_node *node, int indent) {
  sf_unary_expr_node *un = (sf_unary_expr_node *)node;

  print_indent(indent);
  printf("Unary %s\n", type_operation_name(un->op));

  print_ast_node(un->operand, indent + 1);
}

static void print_block(const sf_ast_node *node, int indent) {
  sf_block_node *block = (sf_block_node *)node;

  print_indent(indent);
  printf("Block\n");

  for (size_t i = 0; i < block->statement_count; i++) {
    print_ast_node(block->statements[i], indent + 1);
  }
}

static void print_cast_expr(const sf_ast_node *node, int indent) {
  sf_cast_expr_node *cast = (sf_cast_expr_node *)node;

  print_indent(indent);
  printf("Cast : %s\n", type_value_name(cast->target_type));

  print_ast_node(cast->operand, indent + 1);
}

static void print_if_stmt(const sf_ast_node *node, int indent) {
  sf_if_stmt_node *if_stmt = (sf_if_stmt_node *)node;

  print_indent(indent);
  printf("If\n");

  print_indent(indent + 1);
  printf("Condition\n");
  print_ast_node(if_stmt->condition, indent + 2);

  print_indent(indent + 1);
  printf("Then\n");
  print_ast_node(if_stmt->branch_then, indent + 2);

  if (if_stmt->branch_else != NULL) {
    print_indent(indent + 1);
    printf("Else\n");
    print_ast_node(if_stmt->branch_else, indent + 2);
  }
}

static void print_while_stmt(const sf_ast_node *node, int indent) {
  sf_while_stmt_node *while_stmt = (sf_while_stmt_node *)node;

  print_indent(indent);
  printf("While\n");

  print_indent(indent + 1);
  printf("Condition\n");
  print_ast_node(while_stmt->condition, indent + 2);

  print_indent(indent + 1);
  printf("Do\n");
  print_ast_node(while_stmt->branch_do, indent + 2);
}

static void print_func_decl(const sf_ast_node *node, int indent) {
  sf_func_decl_node *func_decl = (sf_func_decl_node *)node;

  print_indent(indent);
  printf("Function %s : %s\n", func_decl->name,
         type_value_name(func_decl->return_type));

  print_indent(indent + 1);
  printf("Parameters\n");
  for (size_t i = 0; i < func_decl->parameter_count; i++) {
    print_indent(indent + 2);
    printf("%s : %s\n", func_decl->parameters[i].name,
           type_value_name(func_decl->parameters[i].type));
  }

  print_indent(indent + 1);
  printf("Body\n");
  print_ast_node(func_decl->body, indent + 2);
}