#include "sulfur/ast.h"
#include "sulfur/util/log.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

static void print_indent(int indent);
static void print_ast_node(sf_ast_node* node, int indent);

static const char* op_to_string(sf_operation_type op);
static const char* type_to_string(sf_value_type type);

static void free_var_assign(sf_ast_node* node);
static void free_var_decl(sf_ast_node* node);
static void free_literal(sf_ast_node* node);
static void free_ident(sf_ast_node* node);
static void free_program(sf_ast_node* node);
static void free_binary_expr(sf_ast_node* node);
static void free_unary_expr(sf_ast_node* node);
static void free_block(sf_ast_node* node);
static void free_cast_expr(sf_ast_node* node);

static void print_var_assign(const sf_ast_node* node, int indent);
static void print_var_decl(const sf_ast_node* node, int indent);
static void print_literal(const sf_ast_node* node, int indent);
static void print_ident(const sf_ast_node* node, int indent);
static void print_program(const sf_ast_node* node, int indent);
static void print_binary_expr(const sf_ast_node* node, int indent);
static void print_unary_expr(const sf_ast_node* node, int indent);
static void print_block(const sf_ast_node* node, int indent);
static void print_cast_expr(const sf_ast_node* node, int indent);

sf_program_node* sf_new_program() {
    sf_program_node* program = malloc(sizeof(sf_program_node));

    program->base.type = SF_NODE_PROGRAM;
    program->base.span = (sf_span){0};
    program->statements = NULL;
    program->statement_count = 0;
    program->statement_capacity = 0;

    return program;
}

sf_block_node* sf_new_block(sf_span span) {
    sf_block_node* block = malloc(sizeof(sf_block_node));

    block->base.type = SF_NODE_BLOCK;
    block->base.span = span;
    block->statements = NULL;
    block->statement_count = 0;
    block->statement_capacity = 0;

    return block;
}

void sf_program_add_statement(sf_program_node* program, sf_ast_node* stmt) {
    if (program->statement_count >= program->statement_capacity) {
        program->statement_capacity = program->statement_capacity == 0 ? 8 : program->statement_capacity * 2;
        
        sf_ast_node** new_statements = realloc(
            program->statements,
            program->statement_capacity * sizeof(sf_ast_node*)
        );

        if (!new_statements) {
            sf_log_helper(
                "Allocation Failed",
                "AST program memory reallocation failed.",
                "Make sure you have enough memory and try again.",
                NULL,
                SF_AST_REALLOC_FAILED,
                (sf_span){0},
                SF_SEV_FATAL,
                "N/A"
            );
        }

        program->statements = new_statements;
    }

    program->statements[program->statement_count++] = stmt;
}

void sf_block_add_statement(sf_block_node* block, sf_ast_node* stmt) {
    if (block->statement_count >= block->statement_capacity) {
        block->statement_capacity = block->statement_capacity == 0 ? 8 : block->statement_capacity * 2;
        
        sf_ast_node** new_statements = realloc(
            block->statements,
            block->statement_capacity * sizeof(sf_ast_node*)
        );

        if (!new_statements) {
            sf_log_helper(
                "Allocation Failed",
                "AST program memory reallocation failed.",
                "Make sure you have enough memory and try again.",
                NULL,
                SF_AST_REALLOC_FAILED,
                (sf_span){0},
                SF_SEV_FATAL,
                "N/A"
            );
        }

        block->statements = new_statements;
    }

    block->statements[block->statement_count++] = stmt;
}

sf_identifier_node* sf_new_identifier(const char* name, sf_span span) {
    sf_identifier_node* node = malloc(sizeof(sf_identifier_node));

    node->base.type = SF_NODE_IDENTIFIER;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->base.span = span;
    node->name = strdup(name);

    return node;
}

sf_literal_node* sf_new_literal(const char* value, sf_token_type token_type, sf_span span) {
    sf_literal_node* node = malloc(sizeof(sf_literal_node));

    node->base.type = SF_NODE_LITERAL;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->base.span = span;
    node->token_type = token_type;
    node->value = strdup(value);

    return node;
}

sf_binary_expr_node* sf_new_binary_expr(sf_ast_node* left, sf_ast_node* right, sf_operation_type op, sf_span span) {
    sf_binary_expr_node* node = malloc(sizeof(sf_binary_expr_node));

    node->base.type = SF_NODE_BINARY_EXPR;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->base.span = span;
    node->left = left;
    node->right = right;
    node->op = op;

    return node;
}

sf_unary_expr_node* sf_new_unary_expr(sf_ast_node* operand, sf_operation_type op, sf_span span) {
    sf_unary_expr_node* node = malloc(sizeof(sf_unary_expr_node));

    node->base.type = SF_NODE_UNARY_EXPR;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->base.span = span;
    node->op = op;
    node->operand = operand;

    return node;
}

sf_cast_expr_node* sf_new_cast_expr(sf_ast_node* operand, sf_value_type target_type, sf_span span) {
    sf_cast_expr_node* node = malloc(sizeof(sf_cast_expr_node));

    node->base.type = SF_NODE_CAST_EXPR;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->base.span = span;
    node->target_type = target_type;
    node->operand = operand;

    return node;
}

sf_var_decl_node* sf_new_var_decl(const char* name, sf_value_type type, sf_ast_node* value, sf_span span) {
    sf_var_decl_node* node = malloc(sizeof(sf_var_decl_node));

    node->base.type = SF_NODE_VAR_DECL;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->base.span = span;
    node->name = strdup(name);
    node->var_type = type;
    node->value = value;

    return node;
}

sf_var_assign_node* sf_new_var_assign(const char* name, sf_ast_node* value, sf_span span) {
    sf_var_assign_node* node = malloc(sizeof(sf_var_assign_node));

    node->base.type = SF_NODE_VAR_ASSIGN;
    node->base.resolved = SF_VAL_TYPE_UNRESOLVED;
    node->base.span = span;
    node->name = strdup(name);
    node->value = value;

    return node;
}

void sf_free_ast(sf_ast_node* node) {
    if (!node) return;

    switch (node->type) {
        case SF_NODE_LITERAL: free_literal(node); break;
        case SF_NODE_IDENTIFIER: free_ident(node); break;
        case SF_NODE_BINARY_EXPR: free_binary_expr(node); break;
        case SF_NODE_UNARY_EXPR: free_unary_expr(node); break;
        case SF_NODE_VAR_DECL: free_var_decl(node); break;
        case SF_NODE_VAR_ASSIGN: free_var_assign(node); break;
        case SF_NODE_PROGRAM: free_program(node); break;
        case SF_NODE_BLOCK: free_block(node); break;
        case SF_NODE_CAST_EXPR: free_cast_expr(node); break;
    }
}

void sf_print_ast(sf_ast_node* root) {
    print_ast_node(root, 0);
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static const char* op_to_string(sf_operation_type op) {
    switch(op) {
        case SF_OP_TYPE_ADD: return "+";
        case SF_OP_TYPE_SUB: return "-";
        case SF_OP_TYPE_MUL: return "*";
        case SF_OP_TYPE_DIV: return "/";
        case SF_OP_TYPE_NEGATE: return "-";

        default: return "?";
    }
}

static const char* type_to_string(sf_value_type type) {
    switch(type) {
        case SF_VAL_TYPE_I8:  return "i8";
        case SF_VAL_TYPE_I16: return "i16";
        case SF_VAL_TYPE_I32: return "i32";
        case SF_VAL_TYPE_I64: return "i64";

        case SF_VAL_TYPE_U8:  return "u8";
        case SF_VAL_TYPE_U16: return "u16";
        case SF_VAL_TYPE_U32: return "u32";
        case SF_VAL_TYPE_U64: return "u64";

        default: return "?";
    }
}

static void print_ast_node(sf_ast_node* node, int indent) {
    if (!node) return;

    switch (node->type) {
        case SF_NODE_PROGRAM: print_program(node, indent); break;
        case SF_NODE_VAR_DECL: print_var_decl(node, indent); break;
        case SF_NODE_VAR_ASSIGN: print_var_assign(node, indent); break;
        case SF_NODE_BINARY_EXPR: print_binary_expr(node, indent); break;
        case SF_NODE_UNARY_EXPR: print_unary_expr(node, indent); break;
        case SF_NODE_IDENTIFIER: print_ident(node, indent); break;
        case SF_NODE_LITERAL: print_literal(node, indent); break;
        case SF_NODE_BLOCK: print_block(node, indent); break;
        case SF_NODE_CAST_EXPR: print_cast_expr(node, indent); break;
    }
}

static void free_var_assign(sf_ast_node* node) {
    sf_var_assign_node* asg = (sf_var_assign_node*)node;

    free(asg->name);
    sf_free_ast(asg->value);
    free(asg);
}

static void free_var_decl(sf_ast_node* node) {
    sf_var_decl_node* var = (sf_var_decl_node*)node;

    free(var->name);
    sf_free_ast(var->value);
    free(var);
}

static void free_literal(sf_ast_node* node) {
    sf_literal_node* lit = (sf_literal_node*)node;

    free(lit->value);
    free(node);
}

static void free_ident(sf_ast_node* node) {
    sf_identifier_node* id = (sf_identifier_node*)node;

    free(id->name);
    free(id);
}

static void free_program(sf_ast_node* node) {
    sf_program_node* prog = (sf_program_node*)node;

    for (size_t i = 0; i < prog->statement_count; i++) {
        sf_free_ast(prog->statements[i]);
    }

    free(prog->statements);
    free(prog);
}

static void free_binary_expr(sf_ast_node* node) {
    sf_binary_expr_node* bin = (sf_binary_expr_node*)node;

    sf_free_ast(bin->left);
    sf_free_ast(bin->right);
    free(bin);
}

static void free_unary_expr(sf_ast_node* node) {
    sf_unary_expr_node* un = (sf_unary_expr_node*)node;

    sf_free_ast(un->operand);
    free(un);
}

static void free_block(sf_ast_node* node) {
    sf_block_node* block = (sf_block_node*)node;

    for (size_t i = 0; i < block->statement_count; i++) {
        sf_free_ast(block->statements[i]);
    }

    free(block->statements);
    free(block);
}

static void free_cast_expr(sf_ast_node* node) {
    sf_cast_expr_node* cast = (sf_cast_expr_node*)node;

    sf_free_ast(cast->operand);

    free(cast);
}

static void print_var_assign(const sf_ast_node* node, int indent) {
    sf_var_assign_node* asg = (sf_var_assign_node*)node;

    print_indent(indent);
    printf("Assign %s\n", asg->name);

    print_ast_node(asg->value, indent + 1);
}

static void print_var_decl(const sf_ast_node* node, int indent) {
    sf_var_decl_node* var = (sf_var_decl_node*)node;

    print_indent(indent);
    printf("VarDecl %s : %s\n", var->name, type_to_string(var->var_type));

    print_ast_node(var->value, indent + 1);
}

static void print_literal(const sf_ast_node* node, int indent) {
    sf_literal_node* lit = (sf_literal_node*)node;

    print_indent(indent);
    printf("Literal %s\n", lit->value);
}

static void print_ident(const sf_ast_node* node, int indent) {
    sf_identifier_node* id = (sf_identifier_node*)node;

    print_indent(indent);
    printf("Identifier %s\n", id->name);
}

static void print_program(const sf_ast_node* node, int indent) {
    sf_program_node* prog = (sf_program_node*)node;

    print_indent(indent);
    printf("Program\n");

    for (size_t i = 0; i < prog->statement_count; i++) {
        print_ast_node(prog->statements[i], indent + 1);
    }
}

static void print_binary_expr(const sf_ast_node* node, int indent) {
    sf_binary_expr_node* bin = (sf_binary_expr_node*)node;

    print_indent(indent);
    printf("Binary %s\n", op_to_string(bin->op));

    print_ast_node(bin->left, indent + 1);
    print_ast_node(bin->right, indent + 1);
}

static void print_unary_expr(const sf_ast_node* node, int indent) {
    sf_unary_expr_node* un = (sf_unary_expr_node*)node;

    print_indent(indent);
    printf("Unary %s\n", op_to_string(un->op));

    print_ast_node(un->operand, indent + 1);
}

static void print_block(const sf_ast_node* node, int indent) {
    sf_block_node* block = (sf_block_node*)node;

    print_indent(indent);
    printf("Block\n");

    for (size_t i = 0; i < block->statement_count; i++) {
        print_ast_node(block->statements[i], indent + 1);
    }
}

static void print_cast_expr(const sf_ast_node* node, int indent) {
    sf_cast_expr_node* cast = (sf_cast_expr_node*)node;

    print_indent(indent);
    printf("Cast to %s\n", type_to_string(cast->target_type));

    print_ast_node(cast->operand, indent + 1);
}