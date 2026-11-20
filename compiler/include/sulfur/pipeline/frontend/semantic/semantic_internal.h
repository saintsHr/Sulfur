#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/driver/main.h"
#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/semantic/namespace.h"
#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/shared/arena.h"
#include "sulfur/shared/span.h"

#define SF_SEMANTIC_ENTRY_NAME "main"

typedef struct {
    sf_scope *scope;
    sf_namespace_stack ns;
    sf_func_symbol_table *funcs;
    sf_ast_node_func_decl *current_func;
    const char *filename;
    sf_arena *arena;
    const sf_compilation_options *options;
} sf_semantic;

bool sf_semantic_expr(sf_semantic *s, sf_ast_node *node, sf_type_kind expected);
const sf_func_symbol *sf_semantic_resolve_func(sf_semantic *s, const sf_path *path);
void sf_semantic_report_undeclared(const sf_semantic *s, const char *name, sf_span span);
bool sf_const_eval_uint(sf_ast_node *node, uint64_t *out_value);
bool sf_const_eval_int(sf_ast_node *node, int64_t *out_value);
bool sf_const_eval_bool(sf_ast_node *node, bool *out_value);
bool sf_const_check_overflow(
    sf_ast_node *node, sf_type_kind resolved, const char *filename
);
bool sf_const_is_zero(sf_ast_node *node, bool is_signed);
void sf_const_warn(sf_semantic *s, sf_ast_node *node, const char *desc);
const char *sf_semantic_suggest_func(sf_semantic *s, const sf_path *path);

static inline bool sf_op_is_logical(sf_operation_type op) {
    return op == SF_OPERATION_TYPE_LOGICAL_AND || op == SF_OPERATION_TYPE_LOGICAL_OR;
}

static inline bool sf_op_is_shift(sf_operation_type op) {
    return op == SF_OPERATION_TYPE_BITWISE_LSHIFT ||
           op == SF_OPERATION_TYPE_BITWISE_RSHIFT;
}

static inline bool sf_op_is_relational(sf_operation_type op) {
    return op == SF_OPERATION_TYPE_RELATIONAL_EQUAL ||
           op == SF_OPERATION_TYPE_RELATIONAL_NOT_EQUAL ||
           op == SF_OPERATION_TYPE_RELATIONAL_LESS ||
           op == SF_OPERATION_TYPE_RELATIONAL_LESS_EQUAL ||
           op == SF_OPERATION_TYPE_RELATIONAL_GREATER ||
           op == SF_OPERATION_TYPE_RELATIONAL_GREATER_EQUAL;
}
