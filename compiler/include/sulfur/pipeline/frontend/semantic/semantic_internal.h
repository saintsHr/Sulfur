#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/utils/span.h"

typedef struct {
    sf_scope *scope;
    sf_func_symbol_table *funcs;
    sf_func_decl_node *current_func;
    const char *filename;
} sf_semantic;

bool sf_semantic_expr(sf_semantic *s, sf_ast_node *node, sf_value_type expected);

void sf_semantic_report_undeclared(
    const sf_semantic *s, const char *name, sf_span span
);

bool sf_const_eval_uint(sf_ast_node *node, uint64_t *out_value);
bool sf_const_eval_int(sf_ast_node *node, int64_t *out_value);
bool sf_const_eval_bool(sf_ast_node *node, bool *out_value);

bool sf_const_check_overflow(
    sf_ast_node *node, sf_value_type resolved, const char *filename
);

static inline bool sf_op_is_logical(sf_operation_type op) {
    return op == SF_OP_TYPE_LOGICAL_AND || op == SF_OP_TYPE_LOGICAL_OR;
}

static inline bool sf_op_is_shift(sf_operation_type op) {
    return op == SF_OP_TYPE_BITWISE_LSHIFT || op == SF_OP_TYPE_BITWISE_RSHIFT;
}

static inline bool sf_op_is_relational(sf_operation_type op) {
    return
        op == SF_OP_TYPE_RELATIONAL_EQUAL ||
        op == SF_OP_TYPE_RELATIONAL_NOT_EQUAL ||
        op == SF_OP_TYPE_RELATIONAL_LESS ||
        op == SF_OP_TYPE_RELATIONAL_LESS_EQUAL ||
        op == SF_OP_TYPE_RELATIONAL_GREATER ||
        op == SF_OP_TYPE_RELATIONAL_GREATER_EQUAL;
}
