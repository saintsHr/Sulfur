#include "sulfur/pipeline/frontend/ast.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sulfur/utils/arena.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/string.h"
#include "sulfur/utils/type_utils.h"

static void report_out_of_memory(void);
static char *copy_string(sf_arena *arena, const char *str);
static void *new_node(
    sf_arena *arena, size_t size,
    sf_node_type type, sf_span span
);
static void *grow_for_one(
    sf_arena *arena, void *items, size_t count,
    size_t *capacity, size_t elem_size
);

static void print_indent(size_t indent);
static void print_node(const sf_ast_node *node, size_t indent);
static void print_program(const sf_program_node *node, size_t indent);
static void print_block(const sf_block_node *node, size_t indent);
static void print_var_decl(const sf_var_decl_node *node, size_t indent);
static void print_var_assign(const sf_var_assign_node *node, size_t indent);
static void print_literal(const sf_literal_node *node, size_t indent);
static void print_identifier(const sf_identifier_node *node, size_t indent);
static void print_binary_expr(const sf_binary_expr_node *node, size_t indent);
static void print_unary_expr(const sf_unary_expr_node *node, size_t indent);
static void print_cast_expr(const sf_cast_expr_node *node, size_t indent);
static void print_if_stmt(const sf_if_stmt_node *node, size_t indent);
static void print_while_stmt(const sf_while_stmt_node *node, size_t indent);
static void print_func_decl(const sf_func_decl_node *node, size_t indent);
static void print_return_stmt(const sf_return_stmt_node *node, size_t indent);

void sf_program_add_statement(sf_arena *arena, sf_program_node *program, sf_ast_node *stmt) {
    sf_ast_node **statements = grow_for_one(
        arena, program->statements, program->statement_count,
        &program->statement_capacity, sizeof(sf_ast_node *)
    );

    if (statements == NULL) {
        return;
    }

    program->statements = statements;
    program->statements[program->statement_count++] = stmt;
}

void sf_block_add_statement(sf_arena *arena, sf_block_node *block, sf_ast_node *stmt) {
    sf_ast_node **statements = grow_for_one(
        arena, block->statements, block->statement_count,
        &block->statement_capacity, sizeof(sf_ast_node *)
    );

    if (statements == NULL) {
        return;
    }

    block->statements = statements;
    block->statements[block->statement_count++] = stmt;
}

void sf_function_add_parameter(sf_arena *arena, sf_func_decl_node *function, sf_parameter param) {
    sf_parameter *parameters = grow_for_one(
        arena, function->parameters, function->parameter_count,
        &function->parameter_capacity, sizeof(sf_parameter)
    );

    if (parameters == NULL) {
        return;
    }

    function->parameters = parameters;
    function->parameters[function->parameter_count++] = param;
}

sf_program_node *sf_new_program(sf_arena *arena) {
    return new_node(arena, sizeof(sf_program_node), SF_NODE_PROGRAM, (sf_span){0});
}

sf_block_node *sf_new_block(sf_arena *arena, sf_span span) {
    return new_node(arena, sizeof(sf_block_node), SF_NODE_BLOCK, span);
}

sf_literal_node *sf_new_literal(
    sf_arena *arena, const char *value,
    sf_token_type token_type, sf_span span
) {
    sf_literal_node *node = new_node(arena, sizeof(sf_literal_node), SF_NODE_LITERAL, span);

    if (node == NULL) {
        return NULL;
    }

    node->token_type = token_type;
    node->value = copy_string(arena, value);

    return node->value != NULL ? node : NULL;
}

sf_identifier_node *sf_new_identifier(sf_arena *arena, const char *name, sf_span span) {
    sf_identifier_node *node = new_node(
        arena, sizeof(sf_identifier_node),
        SF_NODE_IDENTIFIER, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->name = copy_string(arena, name);

    return node->name != NULL ? node : NULL;
}

sf_binary_expr_node *sf_new_binary_expr(
    sf_arena *arena, sf_ast_node *left,
    sf_ast_node *right, sf_operation_type op,
    sf_span span
) {
    sf_binary_expr_node *node = new_node(
        arena, sizeof(sf_binary_expr_node),
        SF_NODE_BINARY_EXPR, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->left = left;
    node->right = right;
    node->op = op;

    return node;
}

sf_unary_expr_node *sf_new_unary_expr(
    sf_arena *arena, sf_ast_node *operand,
    sf_operation_type op, sf_span span
) {
    sf_unary_expr_node *node = new_node(
        arena, sizeof(sf_unary_expr_node),
        SF_NODE_UNARY_EXPR, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->op = op;
    node->operand = operand;

    return node;
}

sf_cast_expr_node *sf_new_cast_expr(
    sf_arena *arena, sf_ast_node *operand,
    sf_value_type target_type, sf_span span
) {
    sf_cast_expr_node *node = new_node(
        arena, sizeof(sf_cast_expr_node),
        SF_NODE_CAST_EXPR, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->target_type = target_type;
    node->operand = operand;

    return node;
}

sf_var_decl_node *sf_new_var_decl(
    sf_arena *arena, const char *name,
    sf_value_type type, sf_ast_node *value,
    sf_span span
) {
    sf_var_decl_node *node = new_node(
        arena, sizeof(sf_var_decl_node),
        SF_NODE_VAR_DECL, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->name = copy_string(arena, name);
    node->var_type = type;
    node->value = value;

    return node->name != NULL ? node : NULL;
}

sf_var_assign_node *sf_new_var_assign(
    sf_arena *arena, const char *name,
    sf_ast_node *value, sf_span span
) {
    sf_var_assign_node *node = new_node(
        arena, sizeof(sf_var_assign_node),
        SF_NODE_VAR_ASSIGN, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->name = copy_string(arena, name);
    node->value = value;

    return node->name != NULL ? node : NULL;
}

sf_if_stmt_node *sf_new_if_stmt(
    sf_arena *arena, sf_ast_node *condition, sf_ast_node *branch_then,
    sf_ast_node *branch_else, sf_span span
) {
    sf_if_stmt_node *node = new_node(
        arena, sizeof(sf_if_stmt_node),
        SF_NODE_IF_STMT, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->condition = condition;
    node->branch_then = branch_then;
    node->branch_else = branch_else;

    return node;
}

sf_while_stmt_node *sf_new_while_stmt(
    sf_arena *arena, sf_ast_node *condition,
    sf_ast_node *branch_do, sf_span span
) {
    sf_while_stmt_node *node = new_node(
        arena, sizeof(sf_while_stmt_node),
        SF_NODE_WHILE_STMT, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->condition = condition;
    node->branch_do = branch_do;

    return node;
}

sf_func_decl_node *sf_new_func_decl(
    sf_arena *arena, const char *name,
    sf_value_type return_type, sf_span span
) {
    sf_func_decl_node *node = new_node(
        arena, sizeof(sf_func_decl_node),
        SF_NODE_FUNC_DECL, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->name = copy_string(arena, name);
    node->return_type = return_type;

    return node->name != NULL ? node : NULL;
}

sf_return_stmt_node *sf_new_return_stmt(
    sf_arena *arena, sf_ast_node *value, sf_span span
) {
    sf_return_stmt_node *node = new_node(
        arena, sizeof(sf_return_stmt_node),
        SF_NODE_RETURN_STMT, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->value = value;

    return node;
}

void sf_print_ast(const sf_ast_node *root) {
    print_node(root, 0);
}

static void print_indent(size_t indent) {
    for (size_t i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void print_node(const sf_ast_node *node, size_t indent) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case SF_NODE_PROGRAM: {
            print_program((const sf_program_node *)node, indent);
            break;
        }
        case SF_NODE_VAR_DECL: {
            print_var_decl((const sf_var_decl_node *)node, indent);
            break;
        }
        case SF_NODE_VAR_ASSIGN: {
            print_var_assign((const sf_var_assign_node *)node, indent);
            break;
        }
        case SF_NODE_BINARY_EXPR: {
            print_binary_expr((const sf_binary_expr_node *)node, indent);
            break;
        }
        case SF_NODE_UNARY_EXPR: {
            print_unary_expr((const sf_unary_expr_node *)node, indent);
            break;
        }
        case SF_NODE_IDENTIFIER: {
            print_identifier((const sf_identifier_node *)node, indent);
            break;
        }
        case SF_NODE_LITERAL: {
            print_literal((const sf_literal_node *)node, indent);
            break;
        }
        case SF_NODE_BLOCK: {
            print_block((const sf_block_node *)node, indent);
            break;
        }
        case SF_NODE_CAST_EXPR: {
            print_cast_expr((const sf_cast_expr_node *)node, indent);
            break;
        }
        case SF_NODE_IF_STMT: {
            print_if_stmt((const sf_if_stmt_node *)node, indent);
            break;
        }
        case SF_NODE_WHILE_STMT: {
            print_while_stmt((const sf_while_stmt_node *)node, indent);
            break;
        }
        case SF_NODE_FUNC_DECL: {
            print_func_decl((const sf_func_decl_node *)node, indent);
            break;
        }
        case SF_NODE_RETURN_STMT: {
            print_return_stmt((const sf_return_stmt_node *)node, indent);
            break;
        }
    }
}

static void print_program(const sf_program_node *node, size_t indent) {
    print_indent(indent);
    printf("Program\n");

    for (size_t i = 0; i < node->statement_count; i++) {
        print_node(node->statements[i], indent + 1);
    }
}

static void print_block(const sf_block_node *node, size_t indent) {
    print_indent(indent);
    printf("Block\n");

    for (size_t i = 0; i < node->statement_count; i++) {
        print_node(node->statements[i], indent + 1);
    }
}

static void print_var_decl(const sf_var_decl_node *node, size_t indent) {
    print_indent(indent);
    printf("VarDecl %s : %s\n", node->name, sf_type_value_name(node->var_type));

    print_node(node->value, indent + 1);
}

static void print_var_assign(const sf_var_assign_node *node, size_t indent) {
    print_indent(indent);
    printf("Assign %s\n", node->name);

    print_node(node->value, indent + 1);
}

static void print_literal(const sf_literal_node *node, size_t indent) {
    print_indent(indent);
    printf("Literal %s\n", node->value);
}

static void print_identifier(const sf_identifier_node *node, size_t indent) {
    print_indent(indent);
    printf("Identifier %s\n", node->name);
}

static void print_binary_expr(const sf_binary_expr_node *node, size_t indent) {
    print_indent(indent);
    printf("Binary %s\n", sf_type_operation_name(node->op));

    print_node(node->left, indent + 1);
    print_node(node->right, indent + 1);
}

static void print_unary_expr(const sf_unary_expr_node *node, size_t indent) {
    print_indent(indent);
    printf("Unary %s\n", sf_type_operation_name(node->op));

    print_node(node->operand, indent + 1);
}

static void print_cast_expr(const sf_cast_expr_node *node, size_t indent) {
    print_indent(indent);
    printf("Cast : %s\n", sf_type_value_name(node->target_type));

    print_node(node->operand, indent + 1);
}

static void print_if_stmt(const sf_if_stmt_node *node, size_t indent) {
    print_indent(indent);
    printf("If\n");

    print_indent(indent + 1);
    printf("Condition\n");
    print_node(node->condition, indent + 2);

    print_indent(indent + 1);
    printf("Then\n");
    print_node(node->branch_then, indent + 2);

    if (node->branch_else != NULL) {
        print_indent(indent + 1);
        printf("Else\n");
        print_node(node->branch_else, indent + 2);
    }
}

static void print_while_stmt(const sf_while_stmt_node *node, size_t indent) {
    print_indent(indent);
    printf("While\n");

    print_indent(indent + 1);
    printf("Condition\n");
    print_node(node->condition, indent + 2);

    print_indent(indent + 1);
    printf("Do\n");
    print_node(node->branch_do, indent + 2);
}

static void print_func_decl(const sf_func_decl_node *node, size_t indent) {
    print_indent(indent);
    printf("Function %s : %s\n", node->name, sf_type_value_name(node->return_type));

    print_indent(indent + 1);
    printf("Parameters\n");

    for (size_t i = 0; i < node->parameter_count; i++) {
        print_indent(indent + 2);
        printf("%s : %s\n", node->parameters[i].name, sf_type_value_name(node->parameters[i].type));
    }

    print_indent(indent + 1);
    printf("Body\n");
    print_node(node->body, indent + 2);
}

static void print_return_stmt(const sf_return_stmt_node *node, size_t indent) {
    print_indent(indent);
    printf("Return\n");

    if (node->value != NULL) {
        print_node(node->value, indent + 1);
    }
}

static void report_out_of_memory(void) {
    sf_log(
        "Insufficient Memory.", "Cannot allocate memory for compiling.",
        "Free some memory and try again.", NULL,
        SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
    );
}

static void *new_node(
    sf_arena *arena, size_t size,
    sf_node_type type, sf_span span
) {
    sf_ast_node *node = sf_arena_alloc(arena, size);

    if (node == NULL) {
        report_out_of_memory();
        return NULL;
    }

    memset(node, 0, size);

    node->type = type;
    node->resolved = SF_VAL_TYPE_UNRESOLVED;
    node->span = span;

    return node;
}

static char *copy_string(sf_arena *arena, const char *str) {
    char *copy = sf_string_dup_arena(arena, str);

    if (copy == NULL) {
        report_out_of_memory();
    }

    return copy;
}

static void *grow_for_one(
    sf_arena *arena, void *items, size_t count,
    size_t *capacity, size_t elem_size
) {
    if (count < *capacity) {
        return items;
    }

    size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;

    void *grown = sf_arena_grow_array(arena, items, count, new_capacity, elem_size);

    if (grown == NULL) {
        report_out_of_memory();
        return NULL;
    }

    *capacity = new_capacity;

    return grown;
}
