#include "sulfur/pipeline/frontend/parser.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/string.h"
#include "sulfur/utils/token_utils.h"

static void recover_statement(sf_token_list list, size_t *current);
static void recover_expression(sf_token_list list, size_t *current);

static sf_token advance(sf_token_list list, size_t *current);
static bool match(sf_token_list list, size_t *current, sf_token_type type);
static bool expect(sf_token_list list, size_t *current, sf_token_type type,
                   const char *filename);

static sf_token peek_at(sf_token_list list, size_t idx);
static sf_token peek(sf_token_list list, size_t *current);
static sf_token peek_next(sf_token_list list, size_t *current);

static sf_ast_node *parse_statement(sf_arena *arena, sf_token_list list,
                                    size_t *current, const char *filename);
static sf_ast_node *parse_expr_stmt(sf_arena *arena, sf_token_list list,
                                    size_t *current, const char *filename);
static sf_ast_node *parse_declaration(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename);
static sf_ast_node *parse_assign(sf_arena *arena, sf_token_list list,
                                 size_t *current, const char *filename);
static sf_ast_node *parse_if_stmt(sf_arena *arena, sf_token_list list,
                                  size_t *current, const char *filename);
static sf_ast_node *parse_while_stmt(sf_arena *arena, sf_token_list list,
                                     size_t *current, const char *filename);
static sf_ast_node *parse_func_stmt(sf_arena *arena, sf_token_list list,
                                    size_t *current, const char *filename);
static sf_ast_node *parse_return_stmt(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename);

static sf_ast_node *parse_block(sf_arena *arena, sf_token_list list,
                                size_t *current, const char *filename);
static sf_ast_node *parse_primary(sf_arena *arena, sf_token_list list,
                                  size_t *current, const char *filename);
static sf_ast_node *parse_postfix(sf_arena *arena, sf_token_list list,
                                  size_t *current, const char *filename);
static sf_ast_node *parse_unary(sf_arena *arena, sf_token_list list,
                                size_t *current, const char *filename);
static sf_ast_node *parse_cast(sf_arena *arena, sf_token_list list,
                               size_t *current, const char *filename);
static sf_ast_node *parse_multiplicative(sf_arena *arena, sf_token_list list,
                                         size_t *current, const char *filename);
static sf_ast_node *parse_additive(sf_arena *arena, sf_token_list list,
                                   size_t *current, const char *filename);
static sf_ast_node *parse_shift(sf_arena *arena, sf_token_list list,
                                size_t *current, const char *filename);
static sf_ast_node *parse_bitwise_and(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename);
static sf_ast_node *parse_bitwise_xor(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename);
static sf_ast_node *parse_bitwise_or(sf_arena *arena, sf_token_list list,
                                     size_t *current, const char *filename);
static sf_ast_node *parse_relational(sf_arena *arena, sf_token_list list,
                                     size_t *current, const char *filename);
static sf_ast_node *parse_logical_and(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename);
static sf_ast_node *parse_logical_or(sf_arena *arena, sf_token_list list,
                                     size_t *current, const char *filename);

sf_program_node *sf_parse(sf_arena *arena, sf_token_list list,
                          const char *filename) {
  sf_program_node *program = sf_new_program(arena);

  if (list.count == 0) {
    return program;
  }

  size_t current = 0;

  while (peek_at(list, current).type != SF_TOKEN_TYPE_EOF) {
    sf_ast_node *stmt = parse_statement(arena, list, &current, filename);
    if (stmt)
      sf_program_add_statement(arena, program, stmt);
  }

  return program;
}

static sf_token advance(sf_token_list list, size_t *current) {
  sf_token t = peek(list, current);
  if (t.type != SF_TOKEN_TYPE_EOF)
    (*current)++;
  return t;
}

static bool match(sf_token_list list, size_t *current, sf_token_type type) {
  if (peek(list, current).type == type) {
    (*current)++;
    return true;
  }

  return false;
}

static bool expect(sf_token_list list, size_t *current, sf_token_type type,
                   const char *filename) {
  if (!match(list, current, type)) {
    sf_token got = peek(list, current);

    sf_log("unexpected token", "expected '%s' but found '%s'",
           "check for a missing or misplaced token nearby", filename,
           SF_PARSER_UNEXPECTED_TOKEN, got.span, SF_SEV_ERROR,
           sf_token_type_name(type), got.value);

    return false;
  }

  return true;
}

static sf_token peek_at(sf_token_list list, size_t idx) {
  if (list.count == 0)
    abort();
  if (idx >= list.count)
    return list.tokens[list.count - 1];
  return list.tokens[idx];
}

static sf_token peek(sf_token_list list, size_t *current) {
  return peek_at(list, *current);
}

static sf_token peek_next(sf_token_list list, size_t *current) {
  return peek_at(list, *current + 1);
}

static void recover_expression(sf_token_list list, size_t *current) {
  while (peek(list, current).type != SF_TOKEN_TYPE_EOF) {
    switch (peek(list, current).type) {
    case SF_TOKEN_TYPE_SEMICOLON:
    case SF_TOKEN_TYPE_RPAREN:
    case SF_TOKEN_TYPE_RBRACE:
    case SF_TOKEN_TYPE_KW_AS:
      return;

    default:
      (*current)++;
      break;
    }
  }
}

static void recover_statement(sf_token_list list, size_t *current) {
  while (peek(list, current).type != SF_TOKEN_TYPE_EOF) {
    if (peek(list, current).type == SF_TOKEN_TYPE_SEMICOLON) {
      (*current)++;
      return;
    }

    switch (peek(list, current).type) {
    case SF_TOKEN_TYPE_KW_I8:
    case SF_TOKEN_TYPE_KW_I16:
    case SF_TOKEN_TYPE_KW_I32:
    case SF_TOKEN_TYPE_KW_I64:

    case SF_TOKEN_TYPE_KW_U8:
    case SF_TOKEN_TYPE_KW_U16:
    case SF_TOKEN_TYPE_KW_U32:
    case SF_TOKEN_TYPE_KW_U64:

    case SF_TOKEN_TYPE_KW_IF:
    case SF_TOKEN_TYPE_KW_WHILE:

    case SF_TOKEN_TYPE_IDENTIFIER:
    case SF_TOKEN_TYPE_LBRACE:
    case SF_TOKEN_TYPE_RBRACE:
      return;

    default:
      (*current)++;
      break;
    }
  }
}

static sf_ast_node *parse_unary(sf_arena *arena, sf_token_list list,
                                size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_token token = peek(list, current);
  sf_operation_type op_type = token_to_unary_op(token);

  if (op_type != SF_OP_TYPE_UNRESOLVED) {
    advance(list, current);

    sf_ast_node *operand = parse_unary(arena, list, current, filename);
    if (operand == NULL)
      return NULL;

    return (sf_ast_node *)sf_new_unary_expr(arena, operand, op_type,
                                            token.span);
  }

  return parse_postfix(arena, list, current, filename);
}

static sf_ast_node *parse_postfix(sf_arena *arena, sf_token_list list,
                                  size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *operand = parse_primary(arena, list, current, filename);
  if (operand == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_PLUS_PLUS ||
         peek(list, current).type == SF_TOKEN_TYPE_MINUS_MINUS) {
    sf_token op = advance(list, current);

    sf_operation_type op_type = token_to_postfix_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    operand =
        (sf_ast_node *)sf_new_unary_expr(arena, operand, op_type, op.span);
    if (operand == NULL)
      return NULL;
  }

  return operand;
}

static sf_ast_node *parse_primary(sf_arena *arena, sf_token_list list,
                                  size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  if (match(list, current, SF_TOKEN_TYPE_LPAREN)) {
    sf_ast_node *expr = parse_logical_or(arena, list, current, filename);
    if (expr == NULL)
      return NULL;

    if (!expect(list, current, SF_TOKEN_TYPE_RPAREN, filename)) {
      recover_expression(list, current);
      return NULL;
    }

    return expr;
  }

  sf_token token = advance(list, current);

  if (token.type == SF_TOKEN_TYPE_INTEGER ||
      token.type == SF_TOKEN_TYPE_KW_TRUE ||
      token.type == SF_TOKEN_TYPE_KW_FALSE) {
    sf_ast_node *lit = (sf_ast_node *)sf_new_literal(arena, token.value,
                                                     token.type, token.span);
    if (lit == NULL)
      return NULL;

    return lit;
  }

  if (token.type == SF_TOKEN_TYPE_IDENTIFIER) {
    sf_ast_node *id =
        (sf_ast_node *)sf_new_identifier(arena, token.value, token.span);
    if (id == NULL)
      return NULL;

    return id;
  }

  sf_log("unexpected token", "expected a literal or identifier but found '%s'",
         "check for a missing or misplaced token nearby", filename,
         SF_PARSER_UNEXPECTED_TOKEN, token.span, SF_SEV_ERROR, token.value);

  recover_expression(list, current);

  return NULL;
}

static sf_ast_node *parse_multiplicative(sf_arena *arena, sf_token_list list,
                                         size_t *current,
                                         const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_cast(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_STAR ||
         peek(list, current).type == SF_TOKEN_TYPE_SLASH) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_cast(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}

static sf_ast_node *parse_additive(sf_arena *arena, sf_token_list list,
                                   size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_multiplicative(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_PLUS ||
         peek(list, current).type == SF_TOKEN_TYPE_MINUS) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_multiplicative(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}

static sf_ast_node *parse_declaration(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_token type_token = advance(list, current);
  sf_value_type type = token_to_type(type_token);

  if (type == SF_VAL_TYPE_UNRESOLVED) {
    sf_log("unexpected token", "expected a type keyword but found '%s'",
           "use any type keyword", filename, SF_PARSER_UNEXPECTED_TOKEN,
           type_token.span, SF_SEV_ERROR, type_token.value);

    recover_statement(list, current);
    return NULL;
  }

  sf_token name_token = advance(list, current);

  if (name_token.type != SF_TOKEN_TYPE_IDENTIFIER) {
    sf_log("unexpected token", "expected an identifier but found '%s'",
           "variable names cannot be reserved keywords, symbols, or numbers",
           filename, SF_PARSER_UNEXPECTED_TOKEN, name_token.span, SF_SEV_ERROR,
           name_token.value);

    recover_statement(list, current);
    return NULL;
  }

  char *name = name_token.value;
  sf_ast_node *val = NULL;

  if (match(list, current, SF_TOKEN_TYPE_EQUAL)) {
    val = parse_logical_or(arena, list, current, filename);

    if (val == NULL) {
      recover_statement(list, current);
      return NULL;
    }
  }

  if (!expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename)) {
    recover_statement(list, current);
    return NULL;
  }

  sf_ast_node *dcl =
      (sf_ast_node *)sf_new_var_decl(arena, name, type, val, name_token.span);
  if (dcl == NULL)
    return NULL;

  return dcl;
}

static sf_ast_node *parse_assign(sf_arena *arena, sf_token_list list,
                                 size_t *current, const char *filename) {
  if (!arena || !current || !filename)
    return NULL;

  sf_token name_token = advance(list, current);
  if (name_token.type != SF_TOKEN_TYPE_IDENTIFIER)
    return NULL;

  sf_token op_token = advance(list, current);
  if (!token_is_assignment_op(op_token.type)) {
    sf_log("unexpected token", "expected assignment operator but found '%s'",
           "use '=' or compound assignment like '+='", filename,
           SF_PARSER_UNEXPECTED_TOKEN, op_token.span, SF_SEV_ERROR,
           op_token.value);
    recover_statement(list, current);
    return NULL;
  }

  sf_ast_node *rhs = parse_logical_or(arena, list, current, filename);
  if (!rhs) {
    recover_statement(list, current);
    return NULL;
  }

  sf_ast_node *value = rhs;

  if (op_token.type != SF_TOKEN_TYPE_EQUAL) {
    sf_operation_type bop = token_assign_to_binary_op(op_token.type);
    if (bop == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    sf_ast_node *lhs_ref = (sf_ast_node *)sf_new_identifier(
        arena, name_token.value, name_token.span);
    if (!lhs_ref)
      return NULL;

    value = (sf_ast_node *)sf_new_binary_expr(arena, lhs_ref, rhs, bop,
                                              op_token.span);
    if (!value)
      return NULL;
  }

  if (!expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename)) {
    recover_statement(list, current);
    return NULL;
  }

  return (sf_ast_node *)sf_new_var_assign(arena, name_token.value, value,
                                          name_token.span);
}

static sf_ast_node *parse_if_stmt(sf_arena *arena, sf_token_list list,
                                  size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_token token = advance(list, current); // gets "if" keyword token

  // gets condition
  sf_ast_node *condition = parse_logical_or(arena, list, current, filename);
  if (condition == NULL) {
    recover_statement(list, current);
    return NULL;
  }

  // gets "then" body inside of "{ }"
  sf_ast_node *branch_then = parse_block(arena, list, current, filename);
  if (branch_then == NULL) {
    recover_statement(list, current);
    return NULL;
  }

  // gets "else" body inside "{ }", if present
  sf_ast_node *branch_else = NULL;
  if (match(list, current, SF_TOKEN_TYPE_KW_ELSE)) {
    if (peek(list, current).type == SF_TOKEN_TYPE_KW_IF) {
      branch_else = parse_if_stmt(arena, list, current, filename);
    } else {
      branch_else = parse_block(arena, list, current, filename);
    }

    if (branch_else == NULL) {
      recover_statement(list, current);
      return NULL;
    }
  }

  sf_if_stmt_node *if_stmt =
      sf_new_if_stmt(arena, condition, branch_then, branch_else, token.span);

  return (sf_ast_node *)if_stmt;
}

static sf_ast_node *parse_while_stmt(sf_arena *arena, sf_token_list list,
                                     size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_token token = advance(list, current); // gets "while" keyword token

  // gets condition
  sf_ast_node *condition = parse_logical_or(arena, list, current, filename);
  if (condition == NULL) {
    recover_statement(list, current);
    return NULL;
  }

  // gets "do" body inside "{ }"
  sf_ast_node *branch_do = parse_block(arena, list, current, filename);
  if (branch_do == NULL) {
    recover_statement(list, current);
    return NULL;
  }

  sf_while_stmt_node *while_stmt =
      sf_new_while_stmt(arena, condition, branch_do, token.span);

  return (sf_ast_node *)while_stmt;
}

static sf_ast_node *parse_func_stmt(sf_arena *arena, sf_token_list list,
                                    size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_token fn_token = advance(list, current); // gets "fn" keyword token

  // gets name token
  sf_token name_token = advance(list, current);
  if (name_token.type != SF_TOKEN_TYPE_IDENTIFIER) {
    sf_log("unexpected token", "expected a function name but found '%s'",
           "follow the language syntax", filename, SF_PARSER_UNEXPECTED_TOKEN,
           name_token.span, SF_SEV_ERROR, name_token.value);

    recover_statement(list, current);
    return NULL;
  }

  // creates node (without return type)
  sf_func_decl_node *func = sf_new_func_decl(
      arena, name_token.value, SF_VAL_TYPE_UNRESOLVED, fn_token.span);

  // expects "("
  if (!expect(list, current, SF_TOKEN_TYPE_LPAREN, filename)) {
    recover_statement(list, current);
    return NULL;
  }

  // gets parameters
  if (peek(list, current).type != SF_TOKEN_TYPE_RPAREN) {
    while (true) {
      sf_token type_token = advance(list, current);
      if (token_to_type(type_token) == SF_VAL_TYPE_UNRESOLVED) {
        sf_log("unexpected token", "expected a type name but found '%s'",
               "follow the language syntax", filename,
               SF_PARSER_UNEXPECTED_TOKEN, type_token.span, SF_SEV_ERROR,
               type_token.value);

        recover_statement(list, current);
        return NULL;
      }

      sf_token name_token = advance(list, current);
      if (name_token.type != SF_TOKEN_TYPE_IDENTIFIER) {
        sf_log("unexpected token", "expected a identifier but found '%s'",
               "follow the language syntax", filename,
               SF_PARSER_UNEXPECTED_TOKEN, name_token.span, SF_SEV_ERROR,
               name_token.value);

        recover_statement(list, current);
        return NULL;
      }

      sf_parameter param = {.name = sf_strdup_arena(arena, name_token.value),
                            .type = token_to_type(type_token)};

      sf_function_add_parameter(arena, func, param);

      if (!match(list, current, SF_TOKEN_TYPE_COMMA))
        break;
    }
  }

  // expects ")"
  if (!expect(list, current, SF_TOKEN_TYPE_RPAREN, filename)) {
    recover_statement(list, current);
    return NULL;
  }

  // gets the return type
  sf_token ret_token = peek(list, current);
  if (ret_token.type != SF_TOKEN_TYPE_LBRACE) {
    if (token_is_type(ret_token)) {
      func->return_type = token_to_type(ret_token);
      advance(list, current);
    } else {
      sf_log("unexpected token", "expected '{' or type name but found '%s'",
             "follow the language syntax", filename, SF_PARSER_UNEXPECTED_TOKEN,
             ret_token.span, SF_SEV_ERROR, ret_token.value);

      recover_statement(list, current);
      return NULL;
    }
  } else {
    func->return_type = SF_VAL_TYPE_VOID;
  }

  // gets body
  sf_ast_node *body = parse_block(arena, list, current, filename);
  if (body == NULL) {
    recover_statement(list, current);
    return NULL;
  }
  func->body = body;

  return (sf_ast_node *)func;
}

static sf_ast_node *parse_return_stmt(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_token token = advance(list, current); // get "return" token
  sf_ast_node *value = NULL;               // return value

  // gets return value if it exists
  if (peek(list, current).type != SF_TOKEN_TYPE_SEMICOLON) {
    value = parse_logical_or(arena, list, current, filename);
    if (value == NULL) {
      recover_statement(list, current);
      return NULL;
    }
  }

  // expects ";"
  if (!expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename)) {
    recover_statement(list, current);
    return NULL;
  }

  sf_return_stmt_node *ret = sf_new_return_stmt(arena, value, token.span);
  return (sf_ast_node *)ret;
}

static sf_ast_node *parse_statement(sf_arena *arena, sf_token_list list,
                                    size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_token token = peek(list, current);

  if (token_is_type(token)) {
    return parse_declaration(arena, list, current, filename);
  }

  if (token_is_ident(token) &&
      token_is_assignment_op(peek_next(list, current).type)) {
    return parse_assign(arena, list, current, filename);
  }

  if (token_is_block(token)) {
    return parse_block(arena, list, current, filename);
  }

  if (token_is_if(token)) {
    return parse_if_stmt(arena, list, current, filename);
  }

  if (token_is_while(token)) {
    return parse_while_stmt(arena, list, current, filename);
  }

  if (token_is_fn(token)) {
    return parse_func_stmt(arena, list, current, filename);
  }

  if (token_is_return(token)) {
    return parse_return_stmt(arena, list, current, filename);
  }

  return parse_expr_stmt(arena, list, current, filename);
}

static sf_ast_node *parse_expr_stmt(sf_arena *arena, sf_token_list list,
                                    size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *expr = parse_logical_or(arena, list, current, filename);
  if (expr == NULL)
    return NULL;

  if (!expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename)) {
    recover_statement(list, current);
    return NULL;
  }

  return expr;
}

static sf_ast_node *parse_block(sf_arena *arena, sf_token_list list,
                                size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_span block_span = peek(list, current).span;
  sf_block_node *block = sf_new_block(arena, block_span);

  if (block == NULL)
    return NULL;

  if (!expect(list, current, SF_TOKEN_TYPE_LBRACE, filename)) {
    recover_statement(list, current);
    return NULL;
  }

  while (peek(list, current).type != SF_TOKEN_TYPE_RBRACE &&
         peek(list, current).type != SF_TOKEN_TYPE_EOF) {
    sf_ast_node *stmt = parse_statement(arena, list, current, filename);
    if (stmt)
      sf_block_add_statement(arena, block, stmt);
  };

  if (!expect(list, current, SF_TOKEN_TYPE_RBRACE, filename)) {
    recover_statement(list, current);
    return NULL;
  }

  sf_ast_node *b = (sf_ast_node *)block;
  return b;
}

static sf_ast_node *parse_cast(sf_arena *arena, sf_token_list list,
                               size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *expr = parse_unary(arena, list, current, filename);
  if (expr == NULL)
    return NULL;

  while (true) {
    sf_span as_span = peek(list, current).span;
    if (!match(list, current, SF_TOKEN_TYPE_KW_AS))
      break;

    sf_token type_token = advance(list, current);
    sf_value_type target_type = token_to_type(type_token);

    if (target_type == SF_VAL_TYPE_UNRESOLVED) {
      sf_log("unexpected token",
             "expected a type keyword after 'as' but found '%s'",
             "use any type keyword", filename, SF_PARSER_UNEXPECTED_TOKEN,
             type_token.span, SF_SEV_ERROR, type_token.value);

      recover_expression(list, current);

      return NULL;
    }

    expr = (sf_ast_node *)sf_new_cast_expr(arena, expr, target_type, as_span);
    if (expr == NULL)
      return NULL;
  }

  return expr;
}

static sf_ast_node *parse_shift(sf_arena *arena, sf_token_list list,
                                size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_additive(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_RIGHT_SHIFT ||
         peek(list, current).type == SF_TOKEN_TYPE_LEFT_SHIFT) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_additive(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}

static sf_ast_node *parse_bitwise_and(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_shift(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_AMP) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_shift(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}

static sf_ast_node *parse_bitwise_xor(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_bitwise_and(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_CARET) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_bitwise_and(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}

static sf_ast_node *parse_bitwise_or(sf_arena *arena, sf_token_list list,
                                     size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_bitwise_xor(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_PIPE) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_bitwise_xor(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}

static sf_ast_node *parse_relational(sf_arena *arena, sf_token_list list,
                                     size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_bitwise_or(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  sf_token_type t = peek(list, current).type;

  if (t == SF_TOKEN_TYPE_EQUAL_EQUAL || t == SF_TOKEN_TYPE_BANG_EQUAL ||
      t == SF_TOKEN_TYPE_LESS || t == SF_TOKEN_TYPE_LESS_EQUAL ||
      t == SF_TOKEN_TYPE_GREATER || t == SF_TOKEN_TYPE_GREATER_EQUAL) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_bitwise_or(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}

static sf_ast_node *parse_logical_and(sf_arena *arena, sf_token_list list,
                                      size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_relational(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_AMP_AMP) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_relational(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}

static sf_ast_node *parse_logical_or(sf_arena *arena, sf_token_list list,
                                     size_t *current, const char *filename) {
  if (filename == NULL)
    return NULL;
  if (current == NULL)
    return NULL;
  if (arena == NULL)
    return NULL;

  sf_ast_node *left = parse_logical_and(arena, list, current, filename);
  if (left == NULL)
    return NULL;

  while (peek(list, current).type == SF_TOKEN_TYPE_PIPE_PIPE) {
    sf_token op = advance(list, current);

    sf_ast_node *right = parse_logical_and(arena, list, current, filename);
    if (right == NULL)
      return NULL;

    sf_operation_type op_type = token_to_binary_op(op);
    if (op_type == SF_OP_TYPE_UNRESOLVED)
      return NULL;

    left =
        (sf_ast_node *)sf_new_binary_expr(arena, left, right, op_type, op.span);
    if (left == NULL)
      return NULL;
  }

  return left;
}