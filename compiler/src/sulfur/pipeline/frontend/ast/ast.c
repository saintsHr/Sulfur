#include "sulfur/pipeline/frontend/ast/ast.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sulfur/io/log.h"
#include "sulfur/shared/arena.h"
#include "sulfur/shared/operation.h"
#include "sulfur/shared/string.h"
#include "sulfur/shared/type.h"

static char *copy_string(sf_arena *arena, const char *str);
static void *new_node(sf_arena *arena, size_t size, sf_ast_node_type type, sf_span span);

static void print_indent(size_t indent);
static void print_node(const sf_ast_node *node, size_t indent);
static void print_path(const sf_path *path);

static void print_program_node(const sf_ast_node_program *node, size_t indent);
static void print_block_node(const sf_ast_node_block *node, size_t indent);
static void print_var_decl_node(const sf_ast_node_var_decl *node, size_t indent);
static void print_var_assign_node(const sf_ast_node_var_assign *node, size_t indent);
static void print_literal_node(const sf_ast_node_literal *node, size_t indent);
static void print_identifier_node(const sf_ast_node_identifier *node, size_t indent);
static void print_binary_node(const sf_ast_node_binary *node, size_t indent);
static void print_unary_node(const sf_ast_node_unary *node, size_t indent);
static void print_cast_node(const sf_ast_node_cast *node, size_t indent);
static void print_if_node(const sf_ast_node_if *node, size_t indent);
static void print_while_node(const sf_ast_node_while *node, size_t indent);
static void print_func_decl_node(const sf_ast_node_func_decl *node, size_t indent);
static void print_func_call_node(const sf_ast_node_func_call *node, size_t indent);
static void print_return_node(const sf_ast_node_return *node, size_t indent);
static void print_import_node(const sf_ast_node_import *node, size_t indent);
static void print_namespace_node(const sf_ast_node_namespace *node, size_t indent);

void sf_print_ast(const sf_ast_node *root) {
    print_node(root, 0);
}

void sf_program_add_statement(
    sf_arena *arena, sf_ast_node_program *program, sf_ast_node *stmt
) {
    sf_ast_node **statements = sf_arena_grow_one(
        arena, program->statements, program->statement_count,
        &program->statement_capacity, sizeof(sf_ast_node *)
    );

    if (statements == NULL) {
        return;
    }

    program->statements = statements;
    program->statements[program->statement_count++] = stmt;
}

void sf_program_add_import(
    sf_arena *arena, sf_ast_node_program *program, sf_ast_node_import *import
) {
    sf_ast_node_import **imports = sf_arena_grow_one(
        arena, program->imports, program->import_count, &program->import_capacity,
        sizeof(sf_ast_node_import *)
    );

    if (imports == NULL) {
        return;
    }

    program->imports = imports;
    program->imports[program->import_count++] = import;
}

void sf_block_add_statement(
    sf_arena *arena, sf_ast_node_block *block, sf_ast_node *stmt
) {
    sf_ast_node **statements = sf_arena_grow_one(
        arena, block->statements, block->statement_count, &block->statement_capacity,
        sizeof(sf_ast_node *)
    );

    if (statements == NULL) {
        return;
    }

    block->statements = statements;
    block->statements[block->statement_count++] = stmt;
}

void sf_function_add_parameter(
    sf_arena *arena, sf_ast_node_func_decl *function, sf_parameter param
) {
    sf_parameter *parameters = sf_arena_grow_one(
        arena, function->parameters, function->parameter_count,
        &function->parameter_capacity, sizeof(sf_parameter)
    );

    if (parameters == NULL) {
        return;
    }

    function->parameters = parameters;
    function->parameters[function->parameter_count++] = param;
}

void sf_call_add_argument(
    sf_arena *arena, sf_ast_node_func_call *call, sf_ast_node *argument
) {
    sf_ast_node **arguments = sf_arena_grow_one(
        arena, call->arguments, call->argument_count, &call->argument_capacity,
        sizeof(sf_ast_node *)
    );

    if (arguments == NULL) {
        return;
    }

    call->arguments = arguments;
    call->arguments[call->argument_count++] = argument;
}

void sf_namespace_add_item(
    sf_arena *arena, sf_ast_node_namespace *ns, sf_ast_node *item
) {
    sf_ast_node **items = sf_arena_grow_one(
        arena, ns->items, ns->item_count, &ns->item_capacity, sizeof(sf_ast_node *)
    );

    if (items == NULL) {
        return;
    }

    ns->items = items;
    ns->items[ns->item_count++] = item;
}

bool sf_path_add_segment(sf_arena *arena, sf_path *path, const char *name, sf_span span) {
    char *name_copy = copy_string(arena, name);

    if (name_copy == NULL) {
        return false;
    }

    sf_path_segment *segments = sf_arena_grow_one(
        arena, path->segments, path->segment_count, &path->segment_capacity,
        sizeof(sf_path_segment)
    );

    if (segments == NULL) {
        return false;
    }

    path->segments = segments;
    path->segments[path->segment_count++] =
        (sf_path_segment){.name = name_copy, .span = span};

    return true;
}

sf_ast_node_program *sf_new_program_node(sf_arena *arena) {
    return new_node(
        arena, sizeof(sf_ast_node_program), SF_AST_NODE_TYPE_PROGRAM, (sf_span){0}
    );
}

sf_ast_node_block *sf_new_block_node(sf_arena *arena, sf_span span) {
    return new_node(arena, sizeof(sf_ast_node_block), SF_AST_NODE_TYPE_BLOCK, span);
}

sf_ast_node_literal *sf_new_literal_node(
    sf_arena *arena, const char *value, sf_token_type token_type, sf_span span
) {
    sf_ast_node_literal *node =
        new_node(arena, sizeof(sf_ast_node_literal), SF_AST_NODE_TYPE_LITERAL, span);

    if (node == NULL) {
        return NULL;
    }

    node->token_type = token_type;
    node->value = copy_string(arena, value);

    return node->value != NULL ? node : NULL;
}

sf_ast_node_identifier *sf_new_identifier_node(sf_arena *arena, sf_span span) {
    return new_node(
        arena, sizeof(sf_ast_node_identifier), SF_AST_NODE_TYPE_IDENTIFIER, span
    );
}

sf_ast_node_binary *sf_new_binary_node(
    sf_arena *arena, sf_ast_node *left, sf_ast_node *right, sf_operation_type op,
    sf_span span
) {
    sf_ast_node_binary *node =
        new_node(arena, sizeof(sf_ast_node_binary), SF_AST_NODE_TYPE_BINARY, span);

    if (node == NULL) {
        return NULL;
    }

    node->left = left;
    node->right = right;
    node->op = op;

    return node;
}

sf_ast_node_unary *sf_new_unary_node(
    sf_arena *arena, sf_ast_node *operand, sf_operation_type op, sf_span span
) {
    sf_ast_node_unary *node =
        new_node(arena, sizeof(sf_ast_node_unary), SF_AST_NODE_TYPE_UNARY, span);

    if (node == NULL) {
        return NULL;
    }

    node->op = op;
    node->operand = operand;

    return node;
}

sf_ast_node_cast *sf_new_cast_node(
    sf_arena *arena, sf_ast_node *operand, sf_type_kind target_type, sf_span span
) {
    sf_ast_node_cast *node =
        new_node(arena, sizeof(sf_ast_node_cast), SF_AST_NODE_TYPE_CAST, span);

    if (node == NULL) {
        return NULL;
    }

    node->target_type = target_type;
    node->operand = operand;

    return node;
}

sf_ast_node_var_decl *sf_new_var_decl_node(
    sf_arena *arena, const char *name, sf_type_kind type, sf_ast_node *value,
    sf_qualifiers qualifiers, sf_attributes attributes, sf_span span
) {
    sf_ast_node_var_decl *node =
        new_node(arena, sizeof(sf_ast_node_var_decl), SF_AST_NODE_TYPE_VAR_DECL, span);

    if (node == NULL) {
        return NULL;
    }

    node->name = copy_string(arena, name);
    node->var_type = type;
    node->value = value;
    node->qualifiers = qualifiers;
    node->attributes = attributes;

    return node->name != NULL ? node : NULL;
}

sf_ast_node_var_assign *sf_new_var_assign_node(
    sf_arena *arena, sf_ast_node *value, sf_span span
) {
    sf_ast_node_var_assign *node = new_node(
        arena, sizeof(sf_ast_node_var_assign), SF_AST_NODE_TYPE_VAR_ASSIGN, span
    );

    if (node == NULL) {
        return NULL;
    }

    node->value = value;

    return node;
}

sf_ast_node_if *sf_new_if_node(
    sf_arena *arena, sf_ast_node *condition, sf_ast_node *branch_then,
    sf_ast_node *branch_else, sf_span span
) {
    sf_ast_node_if *node =
        new_node(arena, sizeof(sf_ast_node_if), SF_AST_NODE_TYPE_IF, span);

    if (node == NULL) {
        return NULL;
    }

    node->condition = condition;
    node->branch_then = branch_then;
    node->branch_else = branch_else;

    return node;
}

sf_ast_node_while *sf_new_while_node(
    sf_arena *arena, sf_ast_node *condition, sf_ast_node *branch_do, sf_span span
) {
    sf_ast_node_while *node =
        new_node(arena, sizeof(sf_ast_node_while), SF_AST_NODE_TYPE_WHILE, span);

    if (node == NULL) {
        return NULL;
    }

    node->condition = condition;
    node->branch_do = branch_do;

    return node;
}

sf_ast_node_func_decl *sf_new_func_decl_node(
    sf_arena *arena, const char *name, sf_type_kind return_type, sf_qualifiers qualifiers,
    sf_attributes attributes, sf_span span
) {
    sf_ast_node_func_decl *node =
        new_node(arena, sizeof(sf_ast_node_func_decl), SF_AST_NODE_TYPE_FUNC_DECL, span);

    if (node == NULL) {
        return NULL;
    }

    node->body = NULL;
    node->name = copy_string(arena, name);
    node->return_type = return_type;
    node->qualifiers = qualifiers;
    node->attributes = attributes;
    node->link_name = NULL;

    return node->name != NULL ? node : NULL;
}

sf_ast_node_func_call *sf_new_func_call_node(
    sf_arena *arena, sf_ast_node *callee, sf_span span
) {
    sf_ast_node_func_call *node =
        new_node(arena, sizeof(sf_ast_node_func_call), SF_AST_NODE_TYPE_FUNC_CALL, span);

    if (node == NULL) {
        return NULL;
    }

    node->callee = callee;
    node->link_name = NULL;

    return node;
}

sf_ast_node_return *sf_new_return_node(
    sf_arena *arena, sf_ast_node *value, sf_span span
) {
    sf_ast_node_return *node =
        new_node(arena, sizeof(sf_ast_node_return), SF_AST_NODE_TYPE_RETURN, span);

    if (node == NULL) {
        return NULL;
    }

    node->value = value;

    return node;
}

sf_ast_node_import *sf_new_import_node(sf_arena *arena, const char *path, sf_span span) {
    sf_ast_node_import *node =
        new_node(arena, sizeof(sf_ast_node_import), SF_AST_NODE_TYPE_IMPORT, span);

    if (node == NULL) {
        return NULL;
    }

    node->path = copy_string(arena, path);
    node->module_index = 0;
    node->resolved = false;

    return node;
}

sf_ast_node_namespace *sf_new_namespace_node(sf_arena *arena, sf_span span) {
    sf_ast_node_namespace *node =
        new_node(arena, sizeof(sf_ast_node_namespace), SF_AST_NODE_TYPE_NAMESPACE, span);

    if (node == NULL) {
        return NULL;
    }

    return node;
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
        case SF_AST_NODE_TYPE_PROGRAM: {
            print_program_node((const sf_ast_node_program *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_VAR_DECL: {
            print_var_decl_node((const sf_ast_node_var_decl *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_VAR_ASSIGN: {
            print_var_assign_node((const sf_ast_node_var_assign *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_BINARY: {
            print_binary_node((const sf_ast_node_binary *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_UNARY: {
            print_unary_node((const sf_ast_node_unary *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_IDENTIFIER: {
            print_identifier_node((const sf_ast_node_identifier *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_LITERAL: {
            print_literal_node((const sf_ast_node_literal *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_BLOCK: {
            print_block_node((const sf_ast_node_block *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_CAST: {
            print_cast_node((const sf_ast_node_cast *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_IF: {
            print_if_node((const sf_ast_node_if *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_WHILE: {
            print_while_node((const sf_ast_node_while *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_FUNC_DECL: {
            print_func_decl_node((const sf_ast_node_func_decl *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_FUNC_CALL: {
            print_func_call_node((const sf_ast_node_func_call *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_RETURN: {
            print_return_node((const sf_ast_node_return *)node, indent);
            break;
        }
        case SF_AST_NODE_TYPE_NAMESPACE: {
            print_namespace_node((const sf_ast_node_namespace *)node, indent);
            break;
        }

        default: {
            break;
        }
    }
}

static void print_program_node(const sf_ast_node_program *node, size_t indent) {
    print_indent(indent);
    printf("Program\n");

    if (node->import_count > 0) {
        print_indent(indent + 1);
        printf("Imports\n");

        for (size_t i = 0; i < node->import_count; i++) {
            print_import_node(node->imports[i], indent + 2);
        }
    }

    if (node->statement_count > 0) {
        print_indent(indent + 1);
        printf("Statements\n");

        for (size_t i = 0; i < node->statement_count; i++) {
            print_node(node->statements[i], indent + 2);
        }
    }
}

static void print_block_node(const sf_ast_node_block *node, size_t indent) {
    print_indent(indent);
    printf("Block\n");

    for (size_t i = 0; i < node->statement_count; i++) {
        print_node(node->statements[i], indent + 1);
    }
}

static void print_var_decl_node(const sf_ast_node_var_decl *node, size_t indent) {
    print_indent(indent);
    printf("VarDecl %s %s\n", sf_type_kind_name(node->var_type), node->name);

    print_node(node->value, indent + 1);
}

static void print_var_assign_node(const sf_ast_node_var_assign *node, size_t indent) {
    print_indent(indent);
    printf("Assign ");
    print_path(&node->path);
    printf("\n");

    print_node(node->value, indent + 1);
}

static void print_literal_node(const sf_ast_node_literal *node, size_t indent) {
    print_indent(indent);
    printf("Literal %s\n", node->value);
}

static void print_identifier_node(const sf_ast_node_identifier *node, size_t indent) {
    print_indent(indent);
    printf("Identifier ");
    print_path(&node->path);
    printf("\n");
}

static void print_binary_node(const sf_ast_node_binary *node, size_t indent) {
    print_indent(indent);
    printf("Binary %s\n", sf_operation_name(node->op));

    print_node(node->left, indent + 1);
    print_node(node->right, indent + 1);
}

static void print_unary_node(const sf_ast_node_unary *node, size_t indent) {
    print_indent(indent);
    printf("Unary %s\n", sf_operation_name(node->op));

    print_node(node->operand, indent + 1);
}

static void print_cast_node(const sf_ast_node_cast *node, size_t indent) {
    print_indent(indent);
    printf("Cast %s\n", sf_type_kind_name(node->target_type));

    print_node(node->operand, indent + 1);
}

static void print_if_node(const sf_ast_node_if *node, size_t indent) {
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

static void print_while_node(const sf_ast_node_while *node, size_t indent) {
    print_indent(indent);
    printf("While\n");

    print_indent(indent + 1);
    printf("Condition\n");
    print_node(node->condition, indent + 2);

    print_indent(indent + 1);
    printf("Do\n");
    print_node(node->branch_do, indent + 2);
}

static void print_func_decl_node(const sf_ast_node_func_decl *node, size_t indent) {
    print_indent(indent);
    printf("Function %s %s\n", node->name, sf_type_kind_name(node->return_type));

    print_indent(indent + 1);
    printf("Parameters\n");

    for (size_t i = 0; i < node->parameter_count; i++) {
        print_indent(indent + 2);
        printf(
            "%s %s\n", sf_type_kind_name(node->parameters[i].type),
            node->parameters[i].name
        );
    }

    print_indent(indent + 1);
    printf("Body\n");
    print_node(node->body, indent + 2);
}

static void print_func_call_node(const sf_ast_node_func_call *node, size_t indent) {
    print_indent(indent);
    printf("Call\n");

    print_indent(indent + 1);
    printf("Callee\n");

    print_node(node->callee, indent + 2);

    print_indent(indent + 1);
    printf("Arguments\n");

    for (size_t i = 0; i < node->argument_count; i++) {
        print_node(node->arguments[i], indent + 2);
    }
}

static void print_return_node(const sf_ast_node_return *node, size_t indent) {
    print_indent(indent);
    printf("Return\n");

    if (node->value != NULL) {
        print_node(node->value, indent + 1);
    }
}

static void print_import_node(const sf_ast_node_import *node, size_t indent) {
    if (node == NULL) {
        return;
    }

    print_indent(indent);
    printf("Import %s\n", node->path);
}

static void print_namespace_node(const sf_ast_node_namespace *ns, size_t indent) {
    print_indent(indent);
    printf("Namespace ");
    print_path(&ns->path);
    printf("\n");

    for (size_t i = 0; i < ns->item_count; i++) {
        print_node(ns->items[i], indent + 1);
    }
}

static void *new_node(sf_arena *arena, size_t size, sf_ast_node_type type, sf_span span) {
    sf_ast_node *node = sf_arena_alloc(arena, size);

    if (node == NULL) {
        sf_log_out_of_memory("N/A");
        return NULL;
    }

    memset(node, 0, size);

    node->type = type;
    node->resolved = SF_TYPE_KIND_UNRESOLVED;
    node->span = span;

    return node;
}

static char *copy_string(sf_arena *arena, const char *str) {
    char *copy = sf_string_dup_arena(arena, str);

    if (copy == NULL) {
        sf_log_out_of_memory("N/A");
    }

    return copy;
}

static void print_path(const sf_path *path) {
    if (path->is_global) {
        printf("::");
    }

    for (size_t i = 0; i < path->segment_count; i++) {
        if (i > 0) {
            printf("::");
        }

        printf("%s", path->segments[i].name);
    }
}
