#include "sulfur/pipeline/frontend/parser/parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "sulfur/pipeline/frontend/parser/parser_internal.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/token_utils.h"

static const sf_token *peek_at(const sf_parser *p, size_t idx);
static void describe_token(const sf_token *token, char *buf, size_t size);

sf_program_node *sf_parse(sf_arena *arena, sf_token_list list, const char *filename) {
    sf_program_node *program = sf_new_program(arena);

    if (program == NULL || list.count == 0) {
        return program;
    }

    sf_parser parser = {
        .arena = arena,
        .tokens = list,
        .current = 0,
        .filename = filename,
    };

    while (sf_parser_peek(&parser)->type != SF_TOKEN_TYPE_EOF) {
        size_t start = parser.current;
        sf_ast_node *stmt = sf_parse_statement(&parser);

        if (stmt != NULL) {
            sf_program_add_statement(arena, program, stmt);
        } else if (parser.current == start) {
            sf_parser_advance(&parser);
        }
    }

    return program;
}

const sf_token *sf_parser_peek(const sf_parser *p) {
    return peek_at(p, p->current);
}

const sf_token *sf_parser_peek_next(const sf_parser *p) {
    return peek_at(p, p->current + 1);
}

const sf_token *sf_parser_advance(sf_parser *p) {
    const sf_token *token = sf_parser_peek(p);

    if (token->type != SF_TOKEN_TYPE_EOF) {
        p->current++;
    }

    return token;
}

bool sf_parser_match(sf_parser *p, sf_token_type type) {
    if (sf_parser_peek(p)->type == type) {
        p->current++;
        return true;
    }

    return false;
}

bool sf_parser_expect(sf_parser *p, sf_token_type type) {
    if (sf_parser_match(p, type)) {
        return true;
    }

    const sf_token *got = sf_parser_peek(p);

    if (got->type != SF_TOKEN_TYPE_UNDEFINED) {
        char found[SF_MAX_TOKEN_VALUE_SIZE + 3];
        describe_token(got, found, sizeof(found));

        sf_log(
            "unexpected token", "expected '%s' but found %s",
            "check for a missing or misplaced token nearby", p->filename,
            SF_LOG_PARSER_UNEXPECTED_TOKEN, got->span, SF_LOG_SEVERITY_ERROR,
            sf_token_type_name(type), found
        );
    }

    return false;
}

const sf_token *sf_parser_expect_identifier(
    sf_parser *p, const char *expected, const char *hint
) {
    const sf_token *token = sf_parser_peek(p);

    if (token->type != SF_TOKEN_TYPE_IDENTIFIER) {
        sf_parser_report_unexpected(p, token, expected, hint);
        return NULL;
    }

    return sf_parser_advance(p);
}

bool sf_parser_expect_type(
    sf_parser *p, sf_value_type *type,
    const char *expected, const char *hint
) {
    const sf_token *token = sf_parser_peek(p);
    sf_value_type value = sf_token_to_type(*token);

    if (value == SF_VAL_TYPE_UNRESOLVED) {
        sf_parser_report_unexpected(p, token, expected, hint);
        return false;
    }

    sf_parser_advance(p);
    *type = value;

    return true;
}

void sf_parser_report_unexpected(
    const sf_parser *p, const sf_token *got,
    const char *expected, const char *hint
) {
    if (got->type == SF_TOKEN_TYPE_UNDEFINED) {
        return;
    }

    char found[SF_MAX_TOKEN_VALUE_SIZE + 3];
    describe_token(got, found, sizeof(found));

    sf_log(
        "unexpected token", "expected %s but found %s", hint, p->filename,
        SF_LOG_PARSER_UNEXPECTED_TOKEN, got->span, SF_LOG_SEVERITY_ERROR,
        expected, found
    );
}

void sf_parser_recover_expression(sf_parser *p) {
    while (sf_parser_peek(p)->type != SF_TOKEN_TYPE_EOF) {
        switch (sf_parser_peek(p)->type) {
            case SF_TOKEN_TYPE_SEMICOLON:
            case SF_TOKEN_TYPE_RPAREN:
            case SF_TOKEN_TYPE_LBRACE:
            case SF_TOKEN_TYPE_RBRACE:
            case SF_TOKEN_TYPE_KW_AS: {
                return;
            }

            default: {
                p->current++;
                break;
            }
        }
    }
}

void sf_parser_recover_statement(sf_parser *p) {
    while (sf_parser_peek(p)->type != SF_TOKEN_TYPE_EOF) {
        if (sf_parser_peek(p)->type == SF_TOKEN_TYPE_SEMICOLON) {
            p->current++;
            return;
        }

        switch (sf_parser_peek(p)->type) {
            case SF_TOKEN_TYPE_KW_I8:
            case SF_TOKEN_TYPE_KW_I16:
            case SF_TOKEN_TYPE_KW_I32:
            case SF_TOKEN_TYPE_KW_I64:
            case SF_TOKEN_TYPE_KW_U8:
            case SF_TOKEN_TYPE_KW_U16:
            case SF_TOKEN_TYPE_KW_U32:
            case SF_TOKEN_TYPE_KW_U64:
            case SF_TOKEN_TYPE_KW_BOOL:
            case SF_TOKEN_TYPE_KW_IF:
            case SF_TOKEN_TYPE_KW_WHILE:
            case SF_TOKEN_TYPE_KW_FN:
            case SF_TOKEN_TYPE_KW_RETURN:
            case SF_TOKEN_TYPE_IDENTIFIER:
            case SF_TOKEN_TYPE_LBRACE:
            case SF_TOKEN_TYPE_RBRACE: {
                return;
            }

            default: {
                p->current++;
                break;
            }
        }
    }
}

static const sf_token *peek_at(const sf_parser *p, size_t idx) {
    if (idx >= p->tokens.count) {
        return &p->tokens.tokens[p->tokens.count - 1];
    }

    return &p->tokens.tokens[idx];
}

static void describe_token(const sf_token *token, char *buf, size_t size) {
    if (token->type == SF_TOKEN_TYPE_EOF) {
        snprintf(buf, size, "end of file");
        return;
    }

    snprintf(buf, size, "'%s'", token->value);
}
