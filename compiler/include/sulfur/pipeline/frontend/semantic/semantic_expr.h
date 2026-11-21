#pragma once

#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"

bool sf_semantic_run_expr(sf_semantic *s, sf_ast_node *node, sf_type_kind expected);
