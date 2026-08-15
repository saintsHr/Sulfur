#include "sulfur/utils/token_utils.h"

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"

bool token_is_type(sf_token token) {
  if (token.type == SF_TOKEN_TYPE_KW_I8)
    return true;
  if (token.type == SF_TOKEN_TYPE_KW_I16)
    return true;
  if (token.type == SF_TOKEN_TYPE_KW_I32)
    return true;
  if (token.type == SF_TOKEN_TYPE_KW_I64)
    return true;

  if (token.type == SF_TOKEN_TYPE_KW_U8)
    return true;
  if (token.type == SF_TOKEN_TYPE_KW_U16)
    return true;
  if (token.type == SF_TOKEN_TYPE_KW_U32)
    return true;
  if (token.type == SF_TOKEN_TYPE_KW_U64)
    return true;

  if (token.type == SF_TOKEN_TYPE_KW_BOOL)
    return true;

  if (token.type == SF_TOKEN_TYPE_KW_VOID)
    return true;

  return false;
}

bool token_is_ident(sf_token token) {
  return (token.type == SF_TOKEN_TYPE_IDENTIFIER);
}

bool token_is_block(sf_token token) {
  return (token.type == SF_TOKEN_TYPE_LBRACE);
}

bool token_is_if(sf_token token) { return (token.type == SF_TOKEN_TYPE_KW_IF); }

bool token_is_while(sf_token token) {
  return (token.type == SF_TOKEN_TYPE_KW_WHILE);
}

bool token_is_fn(sf_token token) { return (token.type == SF_TOKEN_TYPE_KW_FN); }

bool token_is_return(sf_token token) {
  return (token.type == SF_TOKEN_TYPE_KW_RETURN);
}

bool token_is_assignment_op(sf_token_type t) {
  switch (t) {
  case SF_TOKEN_TYPE_EQUAL:
  case SF_TOKEN_TYPE_PLUS_EQUAL:
  case SF_TOKEN_TYPE_MINUS_EQUAL:
  case SF_TOKEN_TYPE_STAR_EQUAL:
  case SF_TOKEN_TYPE_SLASH_EQUAL:
  case SF_TOKEN_TYPE_AMP_EQUAL:
  case SF_TOKEN_TYPE_PIPE_EQUAL:
  case SF_TOKEN_TYPE_CARET_EQUAL:
  case SF_TOKEN_TYPE_LEFT_SHIFT_EQUAL:
  case SF_TOKEN_TYPE_RIGHT_SHIFT_EQUAL:
    return true;
  default:
    return false;
  }
}

sf_value_type token_to_type(sf_token token) {
  switch (token.type) {
  case SF_TOKEN_TYPE_KW_I8:
    return SF_VAL_TYPE_I8;
  case SF_TOKEN_TYPE_KW_I16:
    return SF_VAL_TYPE_I16;
  case SF_TOKEN_TYPE_KW_I32:
    return SF_VAL_TYPE_I32;
  case SF_TOKEN_TYPE_KW_I64:
    return SF_VAL_TYPE_I64;

  case SF_TOKEN_TYPE_KW_U8:
    return SF_VAL_TYPE_U8;
  case SF_TOKEN_TYPE_KW_U16:
    return SF_VAL_TYPE_U16;
  case SF_TOKEN_TYPE_KW_U32:
    return SF_VAL_TYPE_U32;
  case SF_TOKEN_TYPE_KW_U64:
    return SF_VAL_TYPE_U64;

  case SF_TOKEN_TYPE_KW_BOOL:
    return SF_VAL_TYPE_BOOL;

  case SF_TOKEN_TYPE_KW_VOID:
    return SF_VAL_TYPE_VOID;

  default:
    return SF_VAL_TYPE_UNRESOLVED;
  }
}

sf_operation_type token_assign_to_binary_op(sf_token_type t) {
  switch (t) {
  case SF_TOKEN_TYPE_PLUS_EQUAL:
    return SF_OP_TYPE_ADD;
  case SF_TOKEN_TYPE_MINUS_EQUAL:
    return SF_OP_TYPE_SUB;
  case SF_TOKEN_TYPE_STAR_EQUAL:
    return SF_OP_TYPE_MUL;
  case SF_TOKEN_TYPE_SLASH_EQUAL:
    return SF_OP_TYPE_DIV;
  case SF_TOKEN_TYPE_AMP_EQUAL:
    return SF_OP_TYPE_BITWISE_AND;
  case SF_TOKEN_TYPE_PIPE_EQUAL:
    return SF_OP_TYPE_BITWISE_OR;
  case SF_TOKEN_TYPE_CARET_EQUAL:
    return SF_OP_TYPE_BITWISE_XOR;
  case SF_TOKEN_TYPE_LEFT_SHIFT_EQUAL:
    return SF_OP_TYPE_BITWISE_LSHIFT;
  case SF_TOKEN_TYPE_RIGHT_SHIFT_EQUAL:
    return SF_OP_TYPE_BITWISE_RSHIFT;
  default:
    return SF_OP_TYPE_UNRESOLVED;
  }
}

sf_operation_type token_to_unary_op(sf_token token) {
  switch (token.type) {
  case SF_TOKEN_TYPE_MINUS:
    return SF_OP_TYPE_NEGATE;
  case SF_TOKEN_TYPE_TILDE:
    return SF_OP_TYPE_BITWISE_NOT;
  case SF_TOKEN_TYPE_BANG:
    return SF_OP_TYPE_LOGICAL_NOT;
  case SF_TOKEN_TYPE_PLUS_PLUS:
    return SF_OP_TYPE_PREFIX_INCREMENT;
  case SF_TOKEN_TYPE_MINUS_MINUS:
    return SF_OP_TYPE_PREFIX_DECREMENT;

  default:
    return SF_OP_TYPE_UNRESOLVED;
  }
}

sf_operation_type token_to_postfix_op(sf_token token) {
  switch (token.type) {
  case SF_TOKEN_TYPE_PLUS_PLUS:
    return SF_OP_TYPE_POSTFIX_INCREMENT;
  case SF_TOKEN_TYPE_MINUS_MINUS:
    return SF_OP_TYPE_POSTFIX_DECREMENT;

  default:
    return SF_OP_TYPE_UNRESOLVED;
  }
}

sf_operation_type token_to_binary_op(sf_token token) {
  switch (token.type) {
  // arithmetic
  case SF_TOKEN_TYPE_PLUS:
    return SF_OP_TYPE_ADD;
  case SF_TOKEN_TYPE_MINUS:
    return SF_OP_TYPE_SUB;
  case SF_TOKEN_TYPE_STAR:
    return SF_OP_TYPE_MUL;
  case SF_TOKEN_TYPE_SLASH:
    return SF_OP_TYPE_DIV;

  // bitwise
  case SF_TOKEN_TYPE_AMP:
    return SF_OP_TYPE_BITWISE_AND;
  case SF_TOKEN_TYPE_PIPE:
    return SF_OP_TYPE_BITWISE_OR;
  case SF_TOKEN_TYPE_CARET:
    return SF_OP_TYPE_BITWISE_XOR;
  case SF_TOKEN_TYPE_RIGHT_SHIFT:
    return SF_OP_TYPE_BITWISE_RSHIFT;
  case SF_TOKEN_TYPE_LEFT_SHIFT:
    return SF_OP_TYPE_BITWISE_LSHIFT;

  // relational
  case SF_TOKEN_TYPE_EQUAL_EQUAL:
    return SF_OP_TYPE_RELATIONAL_EQUAL;
  case SF_TOKEN_TYPE_BANG_EQUAL:
    return SF_OP_TYPE_RELATIONAL_NOT_EQUAL;
  case SF_TOKEN_TYPE_LESS:
    return SF_OP_TYPE_RELATIONAL_LESS;
  case SF_TOKEN_TYPE_LESS_EQUAL:
    return SF_OP_TYPE_RELATIONAL_LESS_EQUAL;
  case SF_TOKEN_TYPE_GREATER:
    return SF_OP_TYPE_RELATIONAL_GREATER;
  case SF_TOKEN_TYPE_GREATER_EQUAL:
    return SF_OP_TYPE_RELATIONAL_GREATER_EQUAL;

  // logical
  case SF_TOKEN_TYPE_AMP_AMP:
    return SF_OP_TYPE_LOGICAL_AND;
  case SF_TOKEN_TYPE_PIPE_PIPE:
    return SF_OP_TYPE_LOGICAL_OR;

  default:
    return SF_OP_TYPE_UNRESOLVED;
  }
}