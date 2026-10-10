#include "sulfur/pipeline/frontend/parser/parser_internal.h"

#include <stdbool.h>
#include <stddef.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/token_utils.h"

typedef struct {
    sf_token_type ops[6];
    size_t op_count;
    bool chainable;
} sf_binary_level;

static const sf_binary_level k_binary_levels[] = {
    {{SF_TOKEN_TYPE_PIPE_PIPE}, 1, true},
    {{SF_TOKEN_TYPE_AMP_AMP}, 1, true},
    {
        {
            SF_TOKEN_TYPE_EQUAL_EQUAL, SF_TOKEN_TYPE_BANG_EQUAL,
            SF_TOKEN_TYPE_LESS, SF_TOKEN_TYPE_LESS_EQUAL,
            SF_TOKEN_TYPE_GREATER, SF_TOKEN_TYPE_GREATER_EQUAL,
        },
        6, false
    },
    {{SF_TOKEN_TYPE_PIPE}, 1, true},
    {{SF_TOKEN_TYPE_CARET}, 1, true},
    {{SF_TOKEN_TYPE_AMP}, 1, true},
    {{SF_TOKEN_TYPE_LEFT_SHIFT, SF_TOKEN_TYPE_RIGHT_SHIFT}, 2, true},
    {{SF_TOKEN_TYPE_PLUS, SF_TOKEN_TYPE_MINUS}, 2, true},
    {{SF_TOKEN_TYPE_STAR, SF_TOKEN_TYPE_SLASH}, 2, true},
};

#define SF_BINARY_LEVEL_COUNT (sizeof(k_binary_levels) / sizeof(k_binary_levels[0]))

static bool level_has_operator(const sf_binary_level *level, sf_token_type type);

static sf_ast_node *parse_binary(sf_parser *p, size_t level);
static sf_ast_node *parse_cast(sf_parser *p);
static sf_ast_node *parse_unary(sf_parser *p);
static sf_ast_node *parse_postfix(sf_parser *p);
static sf_ast_node *parse_primary(sf_parser *p);

sf_ast_node *sf_parse_expression(sf_parser *p) {
    return parse_binary(p, 0);
}

static bool level_has_operator(const sf_binary_level *level, sf_token_type type) {
    for (size_t i = 0; i < level->op_count; i++) {
        if (level->ops[i] == type) {
            return true;
        }
    }

    return false;
}

static sf_ast_node *parse_binary(sf_parser *p, size_t level) {
    if (level >= SF_BINARY_LEVEL_COUNT) {
        return parse_cast(p);
    }

    const sf_binary_level *info = &k_binary_levels[level];

    sf_ast_node *left = parse_binary(p, level + 1);

    if (left == NULL) {
        return NULL;
    }

    while (level_has_operator(info, sf_parser_peek(p)->type)) {
        const sf_token *op_token = sf_parser_advance(p);

        sf_ast_node *right = parse_binary(p, level + 1);

        if (right == NULL) {
            return NULL;
        }

        sf_operation_type op = sf_token_to_binary_op(*op_token);

        if (op == SF_OP_TYPE_UNRESOLVED) {
            return NULL;
        }

        left = (sf_ast_node *)sf_new_binary_expr(
            p->arena, left, right, op, op_token->span
        );

        if (left == NULL) {
            return NULL;
        }

        if (!info->chainable) {
            break;
        }
    }

    return left;
}

static sf_ast_node *parse_cast(sf_parser *p) {
    sf_ast_node *expr = parse_unary(p);

    if (expr == NULL) {
        return NULL;
    }

    while (sf_parser_peek(p)->type == SF_TOKEN_TYPE_KW_AS) {
        sf_span as_span = sf_parser_advance(p)->span;
        sf_value_type target_type;

        if (
            !sf_parser_expect_type(
                p, &target_type, "a type keyword after 'as'",
                "use any type keyword"
            )
        ) {
            sf_parser_recover_expression(p);
            return NULL;
        }

        expr = (sf_ast_node *)sf_new_cast_expr(p->arena, expr, target_type, as_span);

        if (expr == NULL) {
            return NULL;
        }
    }

    return expr;
}

static sf_ast_node *parse_unary(sf_parser *p) {
    const sf_token *token = sf_parser_peek(p);
    sf_operation_type op = sf_token_to_unary_op(*token);

    if (op == SF_OP_TYPE_UNRESOLVED) {
        return parse_postfix(p);
    }

    sf_parser_advance(p);

    sf_ast_node *operand = parse_unary(p);

    if (operand == NULL) {
        return NULL;
    }

    return (sf_ast_node *)sf_new_unary_expr(p->arena, operand, op, token->span);
}

static sf_ast_node *parse_postfix(sf_parser *p) {
    sf_ast_node *operand = parse_primary(p);

    if (operand == NULL) {
        return NULL;
    }

    while (
        sf_parser_peek(p)->type == SF_TOKEN_TYPE_PLUS_PLUS ||
        sf_parser_peek(p)->type == SF_TOKEN_TYPE_MINUS_MINUS
    ) {
        const sf_token *op_token = sf_parser_advance(p);
        sf_operation_type op = sf_token_to_postfix_op(*op_token);

        if (op == SF_OP_TYPE_UNRESOLVED) {
            return NULL;
        }

        operand = (sf_ast_node *)sf_new_unary_expr(
            p->arena, operand, op, op_token->span
        );

        if (operand == NULL) {
            return NULL;
        }
    }

    return operand;
}

static sf_ast_node *parse_primary(sf_parser *p) {
    if (sf_parser_match(p, SF_TOKEN_TYPE_LPAREN)) {
        sf_ast_node *expr = sf_parse_expression(p);

        if (expr == NULL) {
            return NULL;
        }

        if (!sf_parser_expect(p, SF_TOKEN_TYPE_RPAREN)) {
            sf_parser_recover_expression(p);
            return NULL;
        }

        return expr;
    }

    const sf_token *token = sf_parser_peek(p);

    if (
        token->type == SF_TOKEN_TYPE_INTEGER ||
        token->type == SF_TOKEN_TYPE_KW_TRUE ||
        token->type == SF_TOKEN_TYPE_KW_FALSE
    ) {
        sf_parser_advance(p);

        return (sf_ast_node *)sf_new_literal(
            p->arena, token->value, token->type, token->span
        );
    }

    if (token->type == SF_TOKEN_TYPE_IDENTIFIER) {
        sf_parser_advance(p);

        return (sf_ast_node *)sf_new_identifier(p->arena, token->value, token->span);
    }

    sf_parser_report_unexpected(
        p, token, "a literal or identifier",
        "check for a missing or misplaced token nearby"
    );

    sf_parser_recover_expression(p);

    return NULL;
}
