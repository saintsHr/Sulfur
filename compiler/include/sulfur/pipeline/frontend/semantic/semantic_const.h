#pragma once

#include <stdbool.h>

#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"

bool sf_semantic_const_eval_uint(sf_ast_node *node, uint64_t *out_value);
bool sf_semantic_const_eval_int(sf_ast_node *node, int64_t *out_value);
bool sf_semantic_const_eval_bool(sf_ast_node *node, bool *out_value);
bool sf_semantic_const_check_overflow(
    sf_ast_node *node, sf_type_kind resolved, const char *filename
);
bool sf_semantic_const_is_zero(sf_ast_node *node, bool is_signed);
void sf_semantic_const_warn(sf_semantic *s, sf_ast_node *node, const char *desc);
