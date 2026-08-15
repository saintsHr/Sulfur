#include "sulfur/pipeline/frontend/semantic/semantic.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/type_utils.h"

static bool analyze_expr(sf_ast_node *node, sf_value_type expected,
                         sf_func_symbol_table *funcs, sf_scope *scope,
                         const char *filename);
static void analyze_statement(sf_ast_node *node, sf_scope *scope,
                              sf_func_symbol_table *funcs,
                              sf_func_decl_node *current_func,
                              const char *filename);

static bool try_eval_const_uint(sf_ast_node *node, uint64_t *out_value);
static bool try_eval_const_int(sf_ast_node *node, int64_t *out_value);
static bool try_eval_const_bool(sf_ast_node *node, bool *out_value);

static bool analyze_const_overflow(sf_ast_node *node, sf_value_type resolved,
                                   const char *filename);

static void report_undeclared(const char *name, sf_scope *scope, sf_span span,
                              const char *filename);

static bool check_assignment_type(sf_value_type resolved, sf_value_type target,
                                  sf_span span, const char *filename);
static bool check_return_type(sf_value_type resolved, sf_value_type target,
                              const char *func_name, sf_span span,
                              const char *filename);

static bool stmt_always_returns(sf_ast_node *node);

void sf_analyze(sf_program_node *program, const char *filename) {
  sf_scope scope;
  sf_func_symbol_table funcs;

  scope_init(&scope);
  scope_push(&scope);
  sf_func_symbol_table_init(&funcs);

  for (uint64_t i = 0; i < program->statement_count; i++) {
    analyze_statement(program->statements[i], &scope, &funcs, NULL, filename);
  }

  scope_pop(&scope);
  scope_free(&scope);
  sf_func_symbol_table_free(&funcs);
}

static bool analyze_expr(sf_ast_node *node, sf_value_type expected,
                         sf_func_symbol_table *funcs, sf_scope *scope,
                         const char *filename) {
  switch (node->type) {
  case SF_NODE_BINARY_EXPR: {
    sf_binary_expr_node *bin = (sf_binary_expr_node *)node;

    bool is_logical =
        (bin->op == SF_OP_TYPE_LOGICAL_AND || bin->op == SF_OP_TYPE_LOGICAL_OR);

    bool is_shift = (bin->op == SF_OP_TYPE_BITWISE_LSHIFT ||
                     bin->op == SF_OP_TYPE_BITWISE_RSHIFT);

    bool is_relational = (bin->op == SF_OP_TYPE_RELATIONAL_EQUAL ||
                          bin->op == SF_OP_TYPE_RELATIONAL_NOT_EQUAL ||
                          bin->op == SF_OP_TYPE_RELATIONAL_LESS ||
                          bin->op == SF_OP_TYPE_RELATIONAL_LESS_EQUAL ||
                          bin->op == SF_OP_TYPE_RELATIONAL_GREATER ||
                          bin->op == SF_OP_TYPE_RELATIONAL_GREATER_EQUAL);

    sf_value_type operand_expected = expected;

    if (is_logical) {
      operand_expected = SF_VAL_TYPE_BOOL;
    } else if (is_relational) {
      operand_expected = SF_VAL_TYPE_UNRESOLVED;
    } else if (!type_value_is_integer(operand_expected)) {
      operand_expected = SF_VAL_TYPE_UNRESOLVED;
    }

    bool left_is_literal = bin->left->type == SF_NODE_LITERAL;
    bool right_is_literal = bin->right->type == SF_NODE_LITERAL;

    bool swap_order = is_relational && left_is_literal && !right_is_literal;

    if (swap_order) {
      if (!analyze_expr(bin->right, operand_expected, funcs, scope, filename))
        return false;

      sf_value_type left_expected = operand_expected;

      if (bin->right->resolved != SF_VAL_TYPE_UNRESOLVED) {
        left_expected = bin->right->resolved;
      }

      if (!analyze_expr(bin->left, left_expected, funcs, scope, filename))
        return false;
    } else {
      if (!analyze_expr(bin->left, operand_expected, funcs, scope, filename))
        return false;

      sf_value_type right_expected = operand_expected;

      if (!is_shift && bin->left->resolved != SF_VAL_TYPE_UNRESOLVED) {
        right_expected = bin->left->resolved;
      }

      if (!analyze_expr(bin->right, right_expected, funcs, scope, filename))
        return false;
    }

    sf_value_type ltype = bin->left->resolved;
    sf_value_type rtype = bin->right->resolved;

    if (ltype == SF_VAL_TYPE_UNRESOLVED || rtype == SF_VAL_TYPE_UNRESOLVED)
      return false;

    if (is_logical) {
      if (ltype != SF_VAL_TYPE_BOOL || rtype != SF_VAL_TYPE_BOOL) {
        sf_log("type mismatch",
               "cannot perform a logical operation between %s and %s",
               "only use bool type literas/identifiers on logical "
               "operations",
               filename, SF_SEMANTIC_TYPE_MISMATCH, node->span, SF_SEV_ERROR,
               type_value_name(ltype), type_value_name(rtype));

        return false;
      }

      node->resolved = SF_VAL_TYPE_BOOL;

      bool const_result;
      if (try_eval_const_bool(node, &const_result)) {
        sf_log("constant condition",
               "this logical expression always evaluates to '%s'",
               "check the operands and operators", filename,
               SF_SEMANTIC_CONSTANT_EXPR, node->span, SF_SEV_WARNING,
               const_result ? "true" : "false");
      }

      return true;
    }

    if (is_shift) {
      node->resolved = ltype;

      int64_t width = type_value_width_bits(ltype);
      int64_t shift_amount;

      if (try_eval_const_int(bin->right, &shift_amount)) {
        if (shift_amount < 0 || shift_amount >= width) {
          sf_log("shift amount out of range",
                 "shift amount '%lld' is out of range for type '%s' "
                 "(width %lld)",
                 "use a shift amount between 0 and the type's bit "
                 "width minus one",
                 filename, SF_SEMANTIC_LITERAL_OVERFLOW, node->span,
                 SF_SEV_FATAL, (long long)shift_amount, type_value_name(ltype),
                 (long long)width);
          return false;
        }
      }

      return true;
    }

    if (is_relational) {
      if (!type_value_is_same_group(ltype, rtype)) {
        sf_log("type mismatch", "cannot compare '%s' and '%s'",
               "cast one of the operands to match the other's type", filename,
               SF_SEMANTIC_TYPE_MISMATCH, node->span, SF_SEV_ERROR,
               type_value_name(ltype), type_value_name(rtype));
        return false;
      }

      if (type_value_width_bits(ltype) != type_value_width_bits(rtype)) {
        sf_log("type mismatch",
               "cannot compare '%s' and '%s' due to differing widths",
               "cast one of the operands to match the other's width", filename,
               SF_SEMANTIC_TYPE_MISMATCH, node->span, SF_SEV_ERROR,
               type_value_name(ltype), type_value_name(rtype));
        return false;
      }

      node->resolved = SF_VAL_TYPE_BOOL;

      bool const_result;
      if (try_eval_const_bool(node, &const_result)) {
        sf_log("constant condition", "this comparison always evaluates to '%s'",
               "check the operands and operators", filename,
               SF_SEMANTIC_CONSTANT_EXPR, node->span, SF_SEV_WARNING,
               const_result ? "true" : "false");
      }

      return true;
    }

    if (!type_value_is_same_group(ltype, rtype)) {
      sf_log("type mismatch", "cannot mix '%s' and '%s' in the same expression",
             "cast one of the operands to match the other's type", filename,
             SF_SEMANTIC_TYPE_MISMATCH, node->span, SF_SEV_ERROR,
             type_value_name(ltype), type_value_name(rtype));
      return false;
    }

    node->resolved = type_value_promote(ltype, rtype);

    if (bin->op == SF_OP_TYPE_DIV) {
      uint64_t u;
      int64_t s;

      bool is_signed = type_value_is_signed(node->resolved);

      bool signed_div_by_zero =
          is_signed && try_eval_const_int(bin->right, &s) && s == 0;
      bool unsigned_div_by_zero =
          !is_signed && try_eval_const_uint(bin->right, &u) && u == 0;

      bool zero = signed_div_by_zero || unsigned_div_by_zero;

      if (zero) {
        sf_log("division by zero", "constant expression divides by zero",
               "ensure the divisor is non-zero", filename,
               SF_SEMANTIC_DIVISION_BY_ZERO, node->span, SF_SEV_ERROR);

        return false;
      }
    }

    if (!analyze_const_overflow(node, node->resolved, filename)) {
      return false;
    }

    return true;
  }

  case SF_NODE_UNARY_EXPR: {
    sf_unary_expr_node *un = (sf_unary_expr_node *)node;

    switch (un->op) {
    case SF_OP_TYPE_NEGATE: {
      bool is_negate = true;

      sf_ast_node *operand = un->operand;
      bool is_literal = operand->type == SF_NODE_LITERAL;

      sf_token_type operand_token =
          is_literal ? ((sf_literal_node *)operand)->token_type
                     : SF_TOKEN_TYPE_INTEGER;

      bool is_int_literal =
          is_literal && operand_token == SF_TOKEN_TYPE_INTEGER;

      if (is_negate && is_int_literal) {
        sf_literal_node *lit = (sf_literal_node *)un->operand;

        sf_value_type target = expected;

        if (target != SF_VAL_TYPE_UNRESOLVED && type_value_is_integer(target) &&
            !type_value_is_signed(target)) {
          sf_log("negative literal in unsigned context",
                 "cannot assign a negative literal to a "
                 "variable of "
                 "unsigned type '%s'",
                 "use a signed type, or remove the negation", filename,
                 SF_SEMANTIC_TYPE_MISMATCH, node->span, SF_SEV_ERROR,
                 type_value_name(target));
          return false;
        }

        if (target == SF_VAL_TYPE_UNRESOLVED ||
            !type_value_is_integer(target) || !type_value_is_signed(target)) {
          target = SF_VAL_TYPE_I64;
        }

        errno = 0;
        uint64_t magnitude = strtoull(lit->value, NULL, 10);

        if (errno == ERANGE ||
            !type_value_signed_literal_fits_negated(target, magnitude)) {
          sf_log("integer literal out of range",
                 "literal '-%s' does not fit in type '%s'",
                 "use a wider type, or cast explicitly if "
                 "truncation is "
                 "intended",
                 filename, SF_SEMANTIC_LITERAL_OVERFLOW, node->span,
                 SF_SEV_ERROR, lit->value, type_value_name(target));
          return false;
        }

        lit->base.resolved = target;
        node->resolved = target;
        return true;
      }

      sf_value_type pass_down = expected;
      if (!type_value_is_signed(expected))
        pass_down = SF_VAL_TYPE_I64;

      if (!analyze_expr(un->operand, pass_down, funcs, scope, filename))
        return false;

      sf_value_type child_type = un->operand->resolved;
      if (child_type == SF_VAL_TYPE_UNRESOLVED)
        return false;

      if (!type_value_is_signed(child_type)) {
        sf_log("invalid operand to unary '-'",
               "operator '-' requires a signed integer operand, "
               "got '%s'",
               NULL, filename, SF_SEMANTIC_TYPE_MISMATCH, node->span,
               SF_SEV_ERROR, type_value_name(child_type));
        return false;
      }

      node->resolved = child_type;
      return true;
    }

    case SF_OP_TYPE_BITWISE_NOT: {
      if (!analyze_expr(un->operand, expected, funcs, scope, filename))
        return false;

      sf_value_type child_type = un->operand->resolved;

      if (!type_value_is_integer(child_type)) {
        sf_log("invalid operand to '~'",
               "operator '~' requires an integer operand, got "
               "'%s'",
               NULL, filename, SF_SEMANTIC_TYPE_MISMATCH, node->span,
               SF_SEV_ERROR, type_value_name(child_type));
        return false;
      }

      node->resolved = child_type;
      return true;
    }

    case SF_OP_TYPE_POSTFIX_DECREMENT:
    case SF_OP_TYPE_POSTFIX_INCREMENT:
    case SF_OP_TYPE_PREFIX_DECREMENT:
    case SF_OP_TYPE_PREFIX_INCREMENT: {
      if (un->operand->type != SF_NODE_IDENTIFIER) {
        sf_log("invalid operand", "operand of '++'/'--' must be a variable",
               "only variables can be incremented or decremented", filename,
               SF_SEMANTIC_TYPE_MISMATCH, node->span, SF_SEV_ERROR);
        return false;
      }

      if (!analyze_expr(un->operand, expected, funcs, scope, filename))
        return false;

      sf_value_type child_type = un->operand->resolved;

      if (!type_value_is_integer(child_type)) {
        sf_log("invalid operand type",
               "operator '++'/'--' requires an integer operand, got '%s'", NULL,
               filename, SF_SEMANTIC_TYPE_MISMATCH, node->span, SF_SEV_ERROR,
               type_value_name(child_type));
        return false;
      }

      node->resolved = child_type;
      return true;
    }

    case SF_OP_TYPE_LOGICAL_NOT: {
      if (!analyze_expr(un->operand, SF_VAL_TYPE_BOOL, funcs, scope, filename))
        return false;

      if (un->operand->resolved != SF_VAL_TYPE_BOOL) {
        sf_log("invalid operand to '!'",
               "operator '!' requires a boolean operand, got '%s'", NULL,
               filename, SF_SEMANTIC_TYPE_MISMATCH, node->span, SF_SEV_ERROR,
               type_value_name(un->operand->resolved));
        return false;
      }

      bool const_result;
      if (try_eval_const_bool(node, &const_result)) {
        sf_log("constant condition", "this expression always evaluates to '%s'",
               "check the operands and operators", filename,
               SF_SEMANTIC_CONSTANT_EXPR, node->span, SF_SEV_WARNING,
               const_result ? "true" : "false");
      }

      node->resolved = SF_VAL_TYPE_BOOL;
      return true;
    }

    default:
      return false;
    }
  }

  case SF_NODE_CAST_EXPR: {
    sf_cast_expr_node *cast = (sf_cast_expr_node *)node;

    if (!analyze_expr(cast->operand, SF_VAL_TYPE_UNRESOLVED, funcs, scope,
                      filename))
      return false;

    sf_value_type from_type = cast->operand->resolved;
    sf_value_type to_type = cast->target_type;

    if (from_type == SF_VAL_TYPE_UNRESOLVED)
      return false;

    if (type_value_is_castable(from_type, to_type)) {
      node->resolved = cast->target_type;
    } else {
      sf_log("invalid cast", "cannot cast '%s' to '%s'",
             "these types are not compatible for casting", filename,
             SF_SEMANTIC_INVALID_EXPLICIT_CAST, node->span, SF_SEV_ERROR,
             type_value_name(from_type), type_value_name(to_type));

      return false;
    }

    return true;
  }

  case SF_NODE_LITERAL: {
    sf_literal_node *lit = (sf_literal_node *)node;

    if (lit->token_type == SF_TOKEN_TYPE_INTEGER) {
      sf_value_type target = expected;

      if (target == SF_VAL_TYPE_UNRESOLVED || !type_value_is_integer(target)) {
        target = SF_VAL_TYPE_I64;
      }

      errno = 0;
      char *endptr = NULL;
      uint64_t raw = strtoull(lit->value, &endptr, 10);

      if (errno == ERANGE) {
        sf_log("integer literal out of range",
               "literal '%s' is too large to represent",
               "this value does not fit in any integer type", filename,
               SF_SEMANTIC_LITERAL_OVERFLOW, node->span, SF_SEV_ERROR,
               lit->value);
        return false;
      }

      if (!type_value_uint_literal_fits(target, raw)) {
        sf_log("integer literal out of range",
               "literal '%s' does not fit in type '%s'",
               "use a wider type, or cast explicitly if truncation is "
               "intended",
               filename, SF_SEMANTIC_LITERAL_OVERFLOW, node->span, SF_SEV_ERROR,
               lit->value, type_value_name(target));
        return false;
      }

      node->resolved = target;
      return true;
    }

    if (lit->token_type == SF_TOKEN_TYPE_KW_TRUE ||
        lit->token_type == SF_TOKEN_TYPE_KW_FALSE) {
      node->resolved = SF_VAL_TYPE_BOOL;
      return true;
    }

    return true;
  }

  case SF_NODE_IDENTIFIER: {
    sf_identifier_node *id = (sf_identifier_node *)node;
    sf_var_symbol *sym = scope_lookup(scope, id->name);

    if (sym == NULL) {
      report_undeclared(id->name, scope, node->span, filename);
      return false;
    }

    if (!sym->initialized) {
      sf_log("uninitialized variable", "'%s' is used before being initialized",
             "assign a value to the variable before using it", filename,
             SF_SEMANTIC_UNINITIALIZED, node->span, SF_SEV_ERROR, id->name);
      return false;
    }

    node->resolved = sym->type;
    id->depth = sym->depth;
    id->id = sym->id;
    return true;
  }

  default:
    return true;
  }
}

static void analyze_statement(sf_ast_node *node, sf_scope *scope,
                              sf_func_symbol_table *funcs,
                              sf_func_decl_node *current_func,
                              const char *filename) {
  switch (node->type) {
  case SF_NODE_VAR_DECL: {
    sf_var_decl_node *var = (sf_var_decl_node *)node;

    sf_var_symbol sym = {
        .name = var->name,
        .type = var->var_type,
        .initialized = var->value != NULL,
        .span = var->base.span,
    };

    if (var->value != NULL) {
      if (!analyze_expr(var->value, var->var_type, funcs, scope, filename))
        break;
      if (!check_assignment_type(var->value->resolved, var->var_type,
                                 var->base.span, filename))
        break;
    }

    scope_insert(scope, sym, filename);

    var->id = scope_lookup(scope, var->name)->id;

    break;
  }

  case SF_NODE_VAR_ASSIGN: {
    sf_var_assign_node *asg = (sf_var_assign_node *)node;

    sf_var_symbol *sym = scope_lookup(scope, asg->name);

    if (sym == NULL) {
      report_undeclared(asg->name, scope, node->span, filename);
      break;
    }

    asg->id = sym->id;

    if (!analyze_expr(asg->value, sym->type, funcs, scope, filename))
      break;
    if (!check_assignment_type(asg->value->resolved, sym->type, asg->base.span,
                               filename))
      break;

    sym->initialized = true;
    asg->base.resolved = sym->type;

    break;
  }

  case SF_NODE_BLOCK: {
    scope_push(scope);

    sf_block_node *block = (sf_block_node *)node;

    for (uint32_t i = 0; i < block->statement_count; i++) {
      analyze_statement(block->statements[i], scope, funcs, current_func,
                        filename);
    }

    scope_pop(scope);

    break;
  }

  case SF_NODE_IF_STMT: {
    sf_if_stmt_node *if_stmt = (sf_if_stmt_node *)node;

    if (!analyze_expr(if_stmt->condition, SF_VAL_TYPE_BOOL, funcs, scope,
                      filename)) {
      break;
    }

    if (if_stmt->condition->resolved != SF_VAL_TYPE_BOOL) {
      sf_log("type mismatch",
             "if condition should always be an boolean expression, found %s",
             "check for typos and missing operands", filename,
             SF_SEMANTIC_TYPE_MISMATCH, if_stmt->condition->span, SF_SEV_ERROR,
             type_value_name(if_stmt->condition->resolved));

      break;
    }

    analyze_statement(if_stmt->branch_then, scope, funcs, current_func,
                      filename);
    if (if_stmt->branch_else) {
      analyze_statement(if_stmt->branch_else, scope, funcs, current_func,
                        filename);
    }

    break;
  }

  case SF_NODE_WHILE_STMT: {
    sf_while_stmt_node *while_stmt = (sf_while_stmt_node *)node;

    if (!analyze_expr(while_stmt->condition, SF_VAL_TYPE_BOOL, funcs, scope,
                      filename)) {
      break;
    }

    if (while_stmt->condition->resolved != SF_VAL_TYPE_BOOL) {
      sf_log("type mismatch",
             "while condition should always be an boolean expression, found %s",
             "check for typos and missing operands", filename,
             SF_SEMANTIC_TYPE_MISMATCH, while_stmt->condition->span,
             SF_SEV_ERROR, type_value_name(while_stmt->condition->resolved));

      break;
    }

    analyze_statement(while_stmt->branch_do, scope, funcs, current_func,
                      filename);

    break;
  }

  case SF_NODE_FUNC_DECL: {
    sf_func_decl_node *func = (sf_func_decl_node *)node;

    sf_func_symbol sym = {
        .name = func->name,
        .return_type = func->return_type,
        .parameter_count = func->parameter_count,
        .span = func->base.span,
        .parameters = func->parameters,
    };

    sf_func_symbol_table_insert(funcs, sym, filename);

    scope_push(scope);

    for (size_t i = 0; i < func->parameter_count; i++) {
      sf_parameter param = func->parameters[i];

      sf_var_symbol param_sym = {.initialized = true,
                                 .name = param.name,
                                 .span = func->base.span,
                                 .type = param.type};

      scope_insert(scope, param_sym, filename);
    }

    analyze_statement(func->body, scope, funcs, func, filename);

    if (func->return_type != SF_VAL_TYPE_VOID) {
      if (!stmt_always_returns(func->body)) {
        sf_log(
            "missing return",
            "function '%s' does not return a value of type '%s' in all paths",
            "check all paths for missing returns", filename,
            SF_SEMANTIC_MISSING_RETURN, func->base.span, SF_SEV_ERROR,
            func->name, type_value_name(func->return_type));
      }
    }

    scope_pop(scope);

    break;
  }

  case SF_NODE_RETURN_STMT: {
    sf_return_stmt_node *ret = (sf_return_stmt_node *)node;

    if (current_func == NULL) {
      sf_log("return outside function",
             "'return' cannot be used outside of a function",
             "move this return inside a function body, or remove it", filename,
             SF_SEMANTIC_RETURN_OUTSIDE_FUNCTION, ret->base.span, SF_SEV_ERROR);
      break;
    }

    if (ret->value == NULL) {
      if (current_func->return_type != SF_VAL_TYPE_VOID) {
        sf_log("missing return value",
               "function '%s' expects a return value of type '%s', but this "
               "'return' provides none",
               "return a value of the expected type, or change the function's "
               "return type to 'void'",
               filename, SF_SEMANTIC_NO_VALUE_RETURN, ret->base.span,
               SF_SEV_ERROR, current_func->name,
               type_value_name(current_func->return_type));
      }

      break;
    }

    if (current_func->return_type == SF_VAL_TYPE_VOID) {
      if (!analyze_expr(ret->value, SF_VAL_TYPE_UNRESOLVED, funcs, scope,
                        filename))
        break;

      sf_log("unexpected return value",
             "function '%s' is 'void' and should not return a value, but this "
             "'return' provides a value of type '%s'",
             "remove the returned value, or give the function a non-void "
             "return type",
             filename, SF_SEMANTIC_VOID_RETURN_VALUE, ret->base.span,
             SF_SEV_ERROR, current_func->name,
             type_value_name(ret->value->resolved));

      break;
    }

    if (!analyze_expr(ret->value, current_func->return_type, funcs, scope,
                      filename))
      break;
    if (!check_return_type(ret->value->resolved, current_func->return_type,
                           current_func->name, ret->base.span, filename))
      break;

    break;
  }

  default: {
    analyze_expr(node, SF_VAL_TYPE_UNRESOLVED, funcs, scope, filename);
    break;
  }
  }
}

static bool try_eval_const_uint(sf_ast_node *node, uint64_t *out_value) {
  if (node->type == SF_NODE_LITERAL) {
    sf_literal_node *lit = (sf_literal_node *)node;
    if (lit->token_type != SF_TOKEN_TYPE_INTEGER)
      return false;

    errno = 0;
    uint64_t v = strtoull(lit->value, NULL, 10);
    if (errno == ERANGE)
      return false;

    *out_value = v;
    return true;
  }

  if (node->type == SF_NODE_BINARY_EXPR) {
    sf_binary_expr_node *bin = (sf_binary_expr_node *)node;
    uint64_t l, r;
    if (!try_eval_const_uint(bin->left, &l))
      return false;
    if (!try_eval_const_uint(bin->right, &r))
      return false;

    switch (bin->op) {
    case SF_OP_TYPE_ADD:
      if (r > UINT64_MAX - l)
        return false;
      *out_value = l + r;
      return true;
    case SF_OP_TYPE_SUB: {
      if (l < r)
        return false;
      *out_value = l - r;
      return true;
    }
    case SF_OP_TYPE_MUL: {
      if (l != 0 && r > UINT64_MAX / l)
        return false;
      *out_value = l * r;
      return true;
    }
    case SF_OP_TYPE_DIV: {
      if (r == 0)
        return false;

      *out_value = l / r;
      return true;
    }
    case SF_OP_TYPE_BITWISE_LSHIFT:
      if (r >= 64)
        return false;
      *out_value = l << r;
      return true;
    case SF_OP_TYPE_BITWISE_RSHIFT:
      if (r >= 64)
        return false;
      *out_value = l >> r;
      return true;
    case SF_OP_TYPE_BITWISE_AND:
      *out_value = l & r;
      return true;
    case SF_OP_TYPE_BITWISE_OR:
      *out_value = l | r;
      return true;
    case SF_OP_TYPE_BITWISE_XOR:
      *out_value = l ^ r;
      return true;
    default:
      return false;
    }
  }

  return false;
}

static bool try_eval_const_int(sf_ast_node *node, int64_t *out_value) {
  if (node->type == SF_NODE_LITERAL) {
    sf_literal_node *lit = (sf_literal_node *)node;
    if (lit->token_type != SF_TOKEN_TYPE_INTEGER)
      return false;

    errno = 0;
    uint64_t v = strtoull(lit->value, NULL, 10);
    if (errno == ERANGE)
      return false;
    if (v > (uint64_t)INT64_MAX)
      return false;

    *out_value = (int64_t)v;
    return true;
  }

  if (node->type == SF_NODE_UNARY_EXPR) {
    sf_unary_expr_node *un = (sf_unary_expr_node *)node;
    if (un->op != SF_OP_TYPE_NEGATE)
      return false;

    if (un->operand->type == SF_NODE_LITERAL) {
      sf_literal_node *lit = (sf_literal_node *)un->operand;
      if (lit->token_type != SF_TOKEN_TYPE_INTEGER)
        return false;

      errno = 0;
      uint64_t magnitude = strtoull(lit->value, NULL, 10);
      if (errno == ERANGE)
        return false;
      if (magnitude > (uint64_t)INT64_MAX + 1)
        return false;

      if (magnitude == (uint64_t)INT64_MAX + 1) {
        *out_value = INT64_MIN;
      } else {
        *out_value = -(int64_t)magnitude;
      }
      return true;
    }

    int64_t child;

    if (!try_eval_const_int(un->operand, &child))
      return false;
    if (child == INT64_MIN)
      return false;

    *out_value = -child;
    return true;
  }

  if (node->type == SF_NODE_BINARY_EXPR) {
    sf_binary_expr_node *bin = (sf_binary_expr_node *)node;

    int64_t l, r;

    if (!try_eval_const_int(bin->left, &l))
      return false;
    if (!try_eval_const_int(bin->right, &r))
      return false;

    switch (bin->op) {
    case SF_OP_TYPE_ADD: {
      if (r > 0 && l > INT64_MAX - r)
        return false;
      if (r < 0 && l < INT64_MIN - r)
        return false;

      *out_value = l + r;
      return true;
    }
    case SF_OP_TYPE_SUB: {
      if (r < 0 && l > INT64_MAX + r)
        return false;
      if (r > 0 && l < INT64_MIN + r)
        return false;

      *out_value = l - r;
      return true;
    }
    case SF_OP_TYPE_MUL: {
      if (l != 0 && r != 0) {
        int64_t result = l * r;
        if (result / l != r)
          return false;
        *out_value = result;
      } else {
        *out_value = 0;
      }
      return true;
    }
    case SF_OP_TYPE_DIV: {
      if (r == 0)
        return false;
      if (l == INT64_MIN && r == -1)
        return false;

      *out_value = l / r;
      return true;
    }
    case SF_OP_TYPE_BITWISE_LSHIFT:
      if (r < 0 || r >= 64)
        return false;

      *out_value = l << r;
      return true;
    case SF_OP_TYPE_BITWISE_RSHIFT:
      if (r < 0 || r >= 64)
        return false;

      *out_value = l >> r;
      return true;
    case SF_OP_TYPE_BITWISE_AND:
      *out_value = l & r;
      return true;
    case SF_OP_TYPE_BITWISE_OR:
      *out_value = l | r;
      return true;
    case SF_OP_TYPE_BITWISE_XOR:
      *out_value = l ^ r;
      return true;
    default:
      return false;
    }
  }

  return false;
}

static bool try_eval_const_bool(sf_ast_node *node, bool *out_value) {
  if (node->type == SF_NODE_LITERAL) {
    sf_literal_node *lit = (sf_literal_node *)node;

    if (lit->token_type == SF_TOKEN_TYPE_KW_TRUE) {
      *out_value = true;
      return true;
    }

    if (lit->token_type == SF_TOKEN_TYPE_KW_FALSE) {
      *out_value = false;
      return true;
    }

    return false;
  }

  if (node->type == SF_NODE_UNARY_EXPR) {
    sf_unary_expr_node *un = (sf_unary_expr_node *)node;

    if (un->op != SF_OP_TYPE_LOGICAL_NOT)
      return false;

    bool operand_value;
    if (!try_eval_const_bool(un->operand, &operand_value))
      return false;

    *out_value = !operand_value;
    return true;
  }

  if (node->type == SF_NODE_BINARY_EXPR) {
    sf_binary_expr_node *bin = (sf_binary_expr_node *)node;

    bool is_relational = (bin->op == SF_OP_TYPE_RELATIONAL_EQUAL ||
                          bin->op == SF_OP_TYPE_RELATIONAL_NOT_EQUAL ||
                          bin->op == SF_OP_TYPE_RELATIONAL_LESS ||
                          bin->op == SF_OP_TYPE_RELATIONAL_LESS_EQUAL ||
                          bin->op == SF_OP_TYPE_RELATIONAL_GREATER ||
                          bin->op == SF_OP_TYPE_RELATIONAL_GREATER_EQUAL);

    bool is_logical =
        (bin->op == SF_OP_TYPE_LOGICAL_AND || bin->op == SF_OP_TYPE_LOGICAL_OR);

    if (is_logical) {
      bool l, r;

      if (!try_eval_const_bool(bin->left, &l))
        return false;
      if (!try_eval_const_bool(bin->right, &r))
        return false;

      *out_value = (bin->op == SF_OP_TYPE_LOGICAL_AND) ? (l && r) : (l || r);

      return true;
    }

    if (is_relational) {
      bool is_signed = type_value_is_signed(bin->left->resolved);
      bool is_bool = bin->left->resolved == SF_VAL_TYPE_BOOL;

      if (is_bool) {
        if (bin->op != SF_OP_TYPE_RELATIONAL_EQUAL &&
            bin->op != SF_OP_TYPE_RELATIONAL_NOT_EQUAL) {
          return false;
        }

        bool l, r;

        if (!try_eval_const_bool(bin->left, &l))
          return false;
        if (!try_eval_const_bool(bin->right, &r))
          return false;

        switch (bin->op) {
        case SF_OP_TYPE_RELATIONAL_EQUAL:
          *out_value = l == r;
          break;
        case SF_OP_TYPE_RELATIONAL_NOT_EQUAL:
          *out_value = l != r;
          break;

        default:
          return false;
        }

        return true;
      }

      if (is_signed) {
        int64_t l, r;

        if (!try_eval_const_int(bin->left, &l))
          return false;
        if (!try_eval_const_int(bin->right, &r))
          return false;

        switch (bin->op) {
        case SF_OP_TYPE_RELATIONAL_EQUAL:
          *out_value = l == r;
          break;
        case SF_OP_TYPE_RELATIONAL_NOT_EQUAL:
          *out_value = l != r;
          break;
        case SF_OP_TYPE_RELATIONAL_LESS:
          *out_value = l < r;
          break;
        case SF_OP_TYPE_RELATIONAL_LESS_EQUAL:
          *out_value = l <= r;
          break;
        case SF_OP_TYPE_RELATIONAL_GREATER:
          *out_value = l > r;
          break;
        case SF_OP_TYPE_RELATIONAL_GREATER_EQUAL:
          *out_value = l >= r;
          break;
        default:
          return false;
        }
      } else {
        uint64_t l, r;

        if (!try_eval_const_uint(bin->left, &l))
          return false;
        if (!try_eval_const_uint(bin->right, &r))
          return false;

        switch (bin->op) {
        case SF_OP_TYPE_RELATIONAL_EQUAL:
          *out_value = l == r;
          break;
        case SF_OP_TYPE_RELATIONAL_NOT_EQUAL:
          *out_value = l != r;
          break;
        case SF_OP_TYPE_RELATIONAL_LESS:
          *out_value = l < r;
          break;
        case SF_OP_TYPE_RELATIONAL_LESS_EQUAL:
          *out_value = l <= r;
          break;
        case SF_OP_TYPE_RELATIONAL_GREATER:
          *out_value = l > r;
          break;
        case SF_OP_TYPE_RELATIONAL_GREATER_EQUAL:
          *out_value = l >= r;
          break;

        default:
          return false;
        }
      }

      return true;
    }

    return false;
  }

  return false;
}

static bool analyze_const_overflow(sf_ast_node *node, sf_value_type resolved,
                                   const char *filename) {
  if (type_value_is_signed(resolved)) {
    int64_t value;
    if (!try_eval_const_int(node, &value))
      return true;

    bool fits;
    switch (resolved) {
    case SF_VAL_TYPE_I8:
      fits = value >= INT8_MIN && value <= INT8_MAX;
      break;
    case SF_VAL_TYPE_I16:
      fits = value >= INT16_MIN && value <= INT16_MAX;
      break;
    case SF_VAL_TYPE_I32:
      fits = value >= INT32_MIN && value <= INT32_MAX;
      break;
    case SF_VAL_TYPE_I64:
      fits = true;
      break;
    default:
      fits = true;
      break;
    }

    if (!fits) {
      sf_log("constant expression overflow",
             "the constant expression evaluates to '%lld', which does not "
             "fit in type '%s'",
             "the expression overflows at compile time; use a wider type or "
             "restructure the expression",
             filename, SF_SEMANTIC_LITERAL_OVERFLOW, node->span, SF_SEV_ERROR,
             (long long)value, type_value_name(resolved));
      return false;
    }

    return true;
  } else {
    uint64_t value;
    if (!try_eval_const_uint(node, &value)) {
      if (node->type == SF_NODE_BINARY_EXPR) {
        sf_binary_expr_node *bin = (sf_binary_expr_node *)node;
        if (bin->op == SF_OP_TYPE_SUB) {
          uint64_t l, r;
          if (try_eval_const_uint(bin->left, &l) &&
              try_eval_const_uint(bin->right, &r) && l < r) {
            sf_log("constant expression underflow",
                   "the constant expression '%llu - %llu' underflows "
                   "for an unsigned type",
                   "unsigned subtraction cannot produce a negative "
                   "result; check operand order or use a signed type",
                   filename, SF_SEMANTIC_LITERAL_OVERFLOW, node->span,
                   SF_SEV_ERROR, (unsigned long long)l, (unsigned long long)r);
            return false;
          }
        }
      }
      return true;
    }

    if (!type_value_uint_literal_fits(resolved, value)) {
      sf_log("constant expression overflow",
             "the constant expression evaluates to '%llu', which does not "
             "fit in type '%s'",
             "the expression overflows at compile time; use a wider type or "
             "restructure the expression",
             filename, SF_SEMANTIC_LITERAL_OVERFLOW, node->span, SF_SEV_ERROR,
             (unsigned long long)value, type_value_name(resolved));
      return false;
    }

    return true;
  }
}

static void report_undeclared(const char *name, sf_scope *scope, sf_span span,
                              const char *filename) {
  const char *closest = scope_find_closest(scope, name);

  if (closest != NULL) {
    sf_log("undeclared symbol", "'%s' is not declared in this scope",
           "did u mean '%s'?", filename, SF_SEMANTIC_UNDECLARED, span,
           SF_SEV_ERROR, name, closest);
  } else {
    sf_log("undeclared symbol", "'%s' is not declared in this scope",
           "check for typos, or declare the variable before using it", filename,
           SF_SEMANTIC_UNDECLARED, span, SF_SEV_ERROR, name);
  }
}

static bool check_assignment_type(sf_value_type resolved, sf_value_type target,
                                  sf_span span, const char *filename) {
  if (resolved == SF_VAL_TYPE_UNRESOLVED)
    return true;

  if (!type_value_is_same_group(resolved, target)) {
    sf_log("type mismatch", "cannot assign '%s' to a variable of type '%s'",
           "make sure the expression type matches the variable type, or cast "
           "it",
           filename, SF_SEMANTIC_TYPE_MISMATCH, span, SF_SEV_ERROR,
           type_value_name(resolved), type_value_name(target));
    return false;
  }

  if (type_value_width_bits(resolved) > type_value_width_bits(target)) {
    sf_log("narrowing conversion", "cannot implicitly narrow '%s' to '%s'",
           "cast the value explicitly, or use a wider variable type", filename,
           SF_SEMANTIC_INVALID_IMPLICIT_CAST, span, SF_SEV_ERROR,
           type_value_name(resolved), type_value_name(target));
    return false;
  }

  return true;
}

static bool check_return_type(sf_value_type resolved, sf_value_type target,
                              const char *func_name, sf_span span,
                              const char *filename) {
  if (resolved == SF_VAL_TYPE_UNRESOLVED)
    return true;

  if (!type_value_is_same_group(resolved, target)) {
    sf_log("type mismatch",
           "function '%s' expects to return '%s', but this expression has "
           "type '%s'",
           "make sure the returned expression matches the function's return "
           "type, or cast it",
           filename, SF_SEMANTIC_TYPE_MISMATCH, span, SF_SEV_ERROR, func_name,
           type_value_name(target), type_value_name(resolved));
    return false;
  }

  if (type_value_width_bits(resolved) > type_value_width_bits(target)) {
    sf_log("narrowing conversion",
           "cannot implicitly narrow '%s' to '%s' when returning from "
           "function '%s'",
           "cast the value explicitly, or change the function's return type",
           filename, SF_SEMANTIC_INVALID_IMPLICIT_CAST, span, SF_SEV_ERROR,
           type_value_name(resolved), type_value_name(target), func_name);
    return false;
  }

  return true;
}

static bool stmt_always_returns(sf_ast_node *node) {
  if (node == NULL)
    return false;

  switch (node->type) {
  case SF_NODE_RETURN_STMT:
    return true;

  case SF_NODE_BLOCK: {
    sf_block_node *block = (sf_block_node *)node;

    if (block->statement_count == 0)
      return false;

    sf_ast_node *last = block->statements[block->statement_count - 1];
    return stmt_always_returns(last);
  }

  case SF_NODE_IF_STMT: {
    sf_if_stmt_node *if_stmt = (sf_if_stmt_node *)node;

    if (if_stmt->branch_else == NULL)
      return false;

    return stmt_always_returns(if_stmt->branch_then) &&
           stmt_always_returns(if_stmt->branch_else);
  }

  default:
    return false;
  }
}