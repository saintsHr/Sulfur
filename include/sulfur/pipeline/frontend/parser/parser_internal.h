#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/arena.h"

typedef struct {
    sf_arena *arena;
    sf_token_list tokens;
    size_t current;
    const char *filename;
} sf_parser;

const sf_token *sf_parser_peek(const sf_parser *p);
const sf_token *sf_parser_peek_next(const sf_parser *p);
const sf_token *sf_parser_advance(sf_parser *p);
bool sf_parser_match(sf_parser *p, sf_token_type type);
bool sf_parser_expect(sf_parser *p, sf_token_type type);

const sf_token *sf_parser_expect_identifier(
    sf_parser *p, const char *expected, const char *hint
);
bool sf_parser_expect_type(
    sf_parser *p, sf_value_type *type,
    const char *expected, const char *hint
);

void sf_parser_report_unexpected(
    const sf_parser *p, const sf_token *got,
    const char *expected, const char *hint
);

void sf_parser_recover_statement(sf_parser *p);
void sf_parser_recover_expression(sf_parser *p);

sf_ast_node *sf_parse_statement(sf_parser *p);
sf_ast_node *sf_parse_expression(sf_parser *p);
