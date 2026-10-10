#include "sulfur/pipeline/frontend/parser/parser_internal.h"

#include <stdbool.h>
#include <stddef.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/string.h"
#include "sulfur/utils/token_utils.h"
#include "sulfur/utils/log.h"

static sf_ast_node *parse_expr_stmt(sf_parser *p);
static sf_ast_node *parse_declaration(sf_parser *p);
static sf_ast_node *parse_assign(sf_parser *p);
static sf_ast_node *parse_if_stmt(sf_parser *p);
static sf_ast_node *parse_while_stmt(sf_parser *p);
static sf_ast_node *parse_func_stmt(sf_parser *p);
static sf_ast_node *parse_return_stmt(sf_parser *p);
static sf_ast_node *parse_block(sf_parser *p);

sf_ast_node *sf_parse_statement(sf_parser *p) {
    const sf_token *token = sf_parser_peek(p);

    if (sf_token_is_type(*token)) {
        return parse_declaration(p);
    }

    if (
        sf_token_is_ident(*token) &&
        sf_token_is_assignment_op(sf_parser_peek_next(p)->type)
    ) {
        return parse_assign(p);
    }

    if (sf_token_is_block(*token)) {
        return parse_block(p);
    }

    if (sf_token_is_if(*token)) {
        return parse_if_stmt(p);
    }

    if (sf_token_is_while(*token)) {
        return parse_while_stmt(p);
    }

    if (sf_token_is_fn(*token)) {
        return parse_func_stmt(p);
    }

    if (sf_token_is_return(*token)) {
        return parse_return_stmt(p);
    }

    return parse_expr_stmt(p);
}

static sf_ast_node *parse_expr_stmt(sf_parser *p) {
    sf_ast_node *expr = sf_parse_expression(p);

    if (expr == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_SEMICOLON)) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    return expr;
}

static sf_ast_node *parse_declaration(sf_parser *p) {
    sf_value_type type;

    if (!sf_parser_expect_type(p, &type, "a type keyword", "use any type keyword")) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    const sf_token *name_token = sf_parser_expect_identifier(
        p, "an identifier",
        "variable names cannot be reserved keywords, symbols, or numbers"
    );

    if (name_token == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_ast_node *value = NULL;

    if (sf_parser_match(p, SF_TOKEN_TYPE_EQUAL)) {
        value = sf_parse_expression(p);

        if (value == NULL) {
            sf_parser_recover_statement(p);
            return NULL;
        }
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_SEMICOLON)) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    return (sf_ast_node *)sf_new_var_decl(
        p->arena, name_token->value, type, value, name_token->span
    );
}

static sf_ast_node *parse_assign(sf_parser *p) {
    const sf_token *name_token = sf_parser_advance(p);
    const sf_token *op_token = sf_parser_advance(p);

    sf_ast_node *rhs = sf_parse_expression(p);

    if (rhs == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_ast_node *value = rhs;

    if (op_token->type != SF_TOKEN_TYPE_EQUAL) {
        sf_operation_type op = sf_token_assign_to_binary_op(op_token->type);

        if (op == SF_OP_TYPE_UNRESOLVED) {
            return NULL;
        }

        sf_ast_node *lhs_ref = (sf_ast_node *)sf_new_identifier(
            p->arena, name_token->value, name_token->span
        );

        if (lhs_ref == NULL) {
            return NULL;
        }

        value = (sf_ast_node *)sf_new_binary_expr(
            p->arena, lhs_ref, rhs, op, op_token->span
        );

        if (value == NULL) {
            return NULL;
        }
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_SEMICOLON)) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    return (sf_ast_node *)sf_new_var_assign(
        p->arena, name_token->value, value, name_token->span
    );
}

static sf_ast_node *parse_if_stmt(sf_parser *p) {
    const sf_token *token = sf_parser_advance(p);

    sf_ast_node *condition = sf_parse_expression(p);

    if (condition == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_ast_node *branch_then = parse_block(p);

    if (branch_then == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_ast_node *branch_else = NULL;

    if (sf_parser_match(p, SF_TOKEN_TYPE_KW_ELSE)) {
        if (sf_parser_peek(p)->type == SF_TOKEN_TYPE_KW_IF) {
            branch_else = parse_if_stmt(p);
        } else {
            branch_else = parse_block(p);
        }

        if (branch_else == NULL) {
            sf_parser_recover_statement(p);
            return NULL;
        }
    }

    return (sf_ast_node *)sf_new_if_stmt(
        p->arena, condition, branch_then, branch_else, token->span
    );
}

static sf_ast_node *parse_while_stmt(sf_parser *p) {
    const sf_token *token = sf_parser_advance(p);

    sf_ast_node *condition = sf_parse_expression(p);

    if (condition == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_ast_node *branch_do = parse_block(p);

    if (branch_do == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    return (sf_ast_node *)sf_new_while_stmt(
        p->arena, condition, branch_do, token->span
    );
}

static sf_ast_node *parse_func_stmt(sf_parser *p) {
    const sf_token *fn_token = sf_parser_advance(p);

    const sf_token *name_token = sf_parser_expect_identifier(
        p, "a function name", "follow the language syntax"
    );

    if (name_token == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_func_decl_node *func = sf_new_func_decl(
        p->arena, name_token->value, SF_VAL_TYPE_UNRESOLVED, fn_token->span
    );

    if (func == NULL) {
        return NULL;
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_LPAREN)) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    if (sf_parser_peek(p)->type != SF_TOKEN_TYPE_RPAREN) {
        while (true) {
            sf_value_type param_type;

            if (
                !sf_parser_expect_type(
                    p, &param_type, "a type name", "follow the language syntax"
                )
            ) {
                sf_parser_recover_statement(p);
                return NULL;
            }

            const sf_token *param_name_token = sf_parser_expect_identifier(
                p, "an identifier", "follow the language syntax"
            );

            if (param_name_token == NULL) {
                sf_parser_recover_statement(p);
                return NULL;
            }

            char *param_name = sf_string_dup_arena(p->arena, param_name_token->value);

            if (param_name == NULL) {
                sf_log(
                    "Insufficient Memory.", "Cannot allocate memory for compiling.",
                    "Free some memory and try again.", NULL,
                    SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0},
                    SF_LOG_SEVERITY_FATAL
                );
                return NULL;
            }

            sf_parameter param = {.name = param_name, .type = param_type};
            sf_function_add_parameter(p->arena, func, param);

            if (!sf_parser_match(p, SF_TOKEN_TYPE_COMMA)) {
                break;
            }
        }
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_RPAREN)) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    const sf_token *ret_token = sf_parser_peek(p);

    if (ret_token->type == SF_TOKEN_TYPE_LBRACE) {
        func->return_type = SF_VAL_TYPE_VOID;
    } else if (sf_token_is_type(*ret_token)) {
        func->return_type = sf_token_to_type(*ret_token);
        sf_parser_advance(p);
    } else {
        sf_parser_report_unexpected(
            p, ret_token, "'{' or type name", "follow the language syntax"
        );

        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_ast_node *body = parse_block(p);

    if (body == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    func->body = body;

    return (sf_ast_node *)func;
}

static sf_ast_node *parse_return_stmt(sf_parser *p) {
    const sf_token *token = sf_parser_advance(p);
    sf_ast_node *value = NULL;

    if (sf_parser_peek(p)->type != SF_TOKEN_TYPE_SEMICOLON) {
        value = sf_parse_expression(p);

        if (value == NULL) {
            sf_parser_recover_statement(p);
            return NULL;
        }
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_SEMICOLON)) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    return (sf_ast_node *)sf_new_return_stmt(p->arena, value, token->span);
}

static sf_ast_node *parse_block(sf_parser *p) {
    sf_block_node *block = sf_new_block(p->arena, sf_parser_peek(p)->span);

    if (block == NULL) {
        return NULL;
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_LBRACE)) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    while (
        sf_parser_peek(p)->type != SF_TOKEN_TYPE_RBRACE &&
        sf_parser_peek(p)->type != SF_TOKEN_TYPE_EOF
    ) {
        size_t start = p->current;
        sf_ast_node *stmt = sf_parse_statement(p);

        if (stmt != NULL) {
            sf_block_add_statement(p->arena, block, stmt);
        } else if (p->current == start) {
            sf_parser_advance(p);
        }
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_RBRACE)) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    return (sf_ast_node *)block;
}
