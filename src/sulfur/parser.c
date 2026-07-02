#include "sulfur/parser.h"
#include "sulfur/ast.h"
#include "sulfur/lexer.h"
#include "sulfur/util/log.h"

#include <stdbool.h>

static void synchronize(sf_token_list list, size_t* current);

static sf_token advance(sf_token_list list, size_t* current);
static bool match(sf_token_list list, size_t* current, sf_token_type type);
static void expect(sf_token_list list, size_t* current, sf_token_type type, const char* filename);

static bool is_type(sf_token token);
static bool is_ident(sf_token token);
static bool is_block(sf_token token);

static sf_ast_node* parse_statement(sf_token_list list, size_t* current, const char* filename);
static sf_ast_node* parse_expression(sf_token_list list, size_t* current, const char* filename);

static sf_ast_node* parse_unary(sf_token_list list, size_t* current, const char* filename);
static sf_ast_node* parse_primary(sf_token_list list, size_t* current, const char* filename);
static sf_ast_node* parse_multiplicative(sf_token_list list, size_t* current, const char* filename);
static sf_ast_node* parse_cast(sf_token_list list, size_t* current, const char* filename);

static sf_ast_node* parse_declaration(sf_token_list list, size_t* current, const char* filename);
static sf_ast_node* parse_assign(sf_token_list list, size_t* current, const char* filename);
static sf_ast_node* parse_block(sf_token_list list, size_t* current, const char* filename);

sf_program_node* sf_parse(sf_token_list list, const char* filename) {
    sf_program_node* program = sf_new_program();

    size_t current = 0;

    while (list.tokens[current].type != SF_TOKEN_TYPE_EOF) {
        sf_ast_node* stmt = parse_statement(list, &current, filename);
        if (stmt) sf_program_add_statement(program, stmt);
    }

    return program;
}

static sf_token advance(sf_token_list list, size_t* current) {
    return list.tokens[(*current)++];
}

static bool match(sf_token_list list, size_t* current, sf_token_type type) {
    if (list.tokens[*current].type == type) {
        (*current)++;
        return true;
    }
    return false;
}

static void expect(sf_token_list list, size_t* current, sf_token_type type, const char* filename) {
    if (!match(list, current, type)) {
        sf_log(
            "unexpected token",
            "expected %s but found '%s'",
            "check for a missing or misplaced token nearby",
            filename,
            SF_PARSER_UNEXPECTED_TOKEN,
            list.tokens[*current].span,
            SF_SEV_ERROR,
            sf_token_type_name(type),
            list.tokens[*current].value
        );

        synchronize(list, current);
    }
}

static void synchronize(sf_token_list list, size_t* current) {
    while (list.tokens[*current].type != SF_TOKEN_TYPE_EOF) {
        if (list.tokens[*current].type == SF_TOKEN_TYPE_SEMICOLON) {
            (*current)++;
            return;
        }

        switch (list.tokens[*current].type) {
            case SF_TOKEN_TYPE_KW_I8:
            case SF_TOKEN_TYPE_KW_I16:
            case SF_TOKEN_TYPE_KW_I32:
            case SF_TOKEN_TYPE_KW_I64:
            case SF_TOKEN_TYPE_KW_U8:
            case SF_TOKEN_TYPE_KW_U16:
            case SF_TOKEN_TYPE_KW_U32:
            case SF_TOKEN_TYPE_KW_U64:
            case SF_TOKEN_TYPE_IDENTIFIER:
            case SF_TOKEN_TYPE_LBRACE:
                return;
            default:
                break;
        }

        (*current)++;
    }
}

static bool is_type(sf_token token) {
    if (token.type == SF_TOKEN_TYPE_KW_I8)  return true;
    if (token.type == SF_TOKEN_TYPE_KW_I16) return true;
    if (token.type == SF_TOKEN_TYPE_KW_I32) return true;
    if (token.type == SF_TOKEN_TYPE_KW_I64) return true;

    if (token.type == SF_TOKEN_TYPE_KW_U8)  return true;
    if (token.type == SF_TOKEN_TYPE_KW_U16) return true;
    if (token.type == SF_TOKEN_TYPE_KW_U32) return true;
    if (token.type == SF_TOKEN_TYPE_KW_U64) return true;

    return false;
}

static bool is_ident(sf_token token) {
    return (token.type == SF_TOKEN_TYPE_IDENTIFIER);
}

static bool is_block(sf_token token) {
    return (token.type == SF_TOKEN_TYPE_LBRACE);
}

static sf_ast_node* parse_unary(sf_token_list list, size_t* current, const char* filename) {
    sf_span minus_span = list.tokens[*current].span;

    if (match(list, current, SF_TOKEN_TYPE_MINUS)) {
        sf_ast_node* operand = parse_unary(list, current, filename);
        return (sf_ast_node*)sf_new_unary_expr(operand, SF_OP_TYPE_NEGATE, minus_span);
    }

    return parse_primary(list, current, filename);
}

static sf_ast_node* parse_primary(sf_token_list list, size_t* current, const char* filename) {
    if (match(list, current, SF_TOKEN_TYPE_LPAREN)) {
        sf_ast_node* expr = parse_expression(list, current, filename);

        expect(list, current, SF_TOKEN_TYPE_RPAREN, filename);
        
        return expr;
    }

    sf_token token = advance(list, current);

    if (token.type == SF_TOKEN_TYPE_INTEGER) {
        return (sf_ast_node*)sf_new_literal(token.value, token.type, token.span);
    }

    if (token.type == SF_TOKEN_TYPE_IDENTIFIER) {
        return (sf_ast_node*)sf_new_identifier(token.value, token.span);
    }

    sf_log(
        "unexpected token",
        "expected a literal or identifier but found '%s'",
        "check for a missing or misplaced token nearby",
        filename,
        SF_PARSER_UNEXPECTED_TOKEN,
        token.span,
        SF_SEV_ERROR,
        token.value
    );

    synchronize(list, current);

    return NULL;
}

static sf_ast_node* parse_multiplicative(sf_token_list list, size_t* current, const char* filename) {
    sf_ast_node* left = parse_cast(list, current, filename);

    while (
        list.tokens[*current].type == SF_TOKEN_TYPE_MULT ||
        list.tokens[*current].type == SF_TOKEN_TYPE_DIV
    ) {
        sf_token op = advance(list, current);
        sf_ast_node* right = parse_cast(list, current, filename);

        sf_operation_type op_type = (op.type == SF_TOKEN_TYPE_MULT)
            ? SF_OP_TYPE_MUL
            : SF_OP_TYPE_DIV;

        left = (sf_ast_node*)sf_new_binary_expr(left, right, op_type, op.span);
    }

    return left;
}

static sf_ast_node* parse_expression(sf_token_list list, size_t* current, const char* filename) {
    sf_ast_node* left = parse_multiplicative(list, current, filename);

    while (
        list.tokens[*current].type == SF_TOKEN_TYPE_PLUS ||
        list.tokens[*current].type == SF_TOKEN_TYPE_MINUS
    ) {
        sf_token op = advance(list, current);
        sf_ast_node* right = parse_multiplicative(list, current, filename);

        sf_operation_type op_type = (op.type == SF_TOKEN_TYPE_PLUS)
            ? SF_OP_TYPE_ADD
            : SF_OP_TYPE_SUB;

        left = (sf_ast_node*)sf_new_binary_expr(left, right, op_type, op.span);
    }

    return left;
}

static sf_ast_node* parse_declaration(sf_token_list list, size_t* current, const char* filename) {
    sf_token type_token = advance(list, current);
    sf_value_type type;

    switch (type_token.type) {
        case SF_TOKEN_TYPE_KW_I8:     type = SF_VAL_TYPE_I8;     break;
        case SF_TOKEN_TYPE_KW_I16:    type = SF_VAL_TYPE_I16;    break;
        case SF_TOKEN_TYPE_KW_I32:    type = SF_VAL_TYPE_I32;    break;
        case SF_TOKEN_TYPE_KW_I64:    type = SF_VAL_TYPE_I64;    break;
            
        case SF_TOKEN_TYPE_KW_U8:     type = SF_VAL_TYPE_U8;     break;
        case SF_TOKEN_TYPE_KW_U16:    type = SF_VAL_TYPE_U16;    break;
        case SF_TOKEN_TYPE_KW_U32:    type = SF_VAL_TYPE_U32;    break;
        case SF_TOKEN_TYPE_KW_U64:    type = SF_VAL_TYPE_U64;    break;
            
        default: 
            sf_log(
                "unexpected token",
                "expected a type keyword but found '%s'",
                "use any type keyword",
                filename,
                SF_PARSER_UNEXPECTED_TOKEN,
                type_token.span,
                SF_SEV_ERROR,
                type_token.value
            );

            synchronize(list, current);

            return NULL;
            break;
    }

    sf_token name_token = advance(list, current);
    char* name = name_token.value;

    sf_ast_node* val = NULL;

    if (match(list, current, SF_TOKEN_TYPE_EQUALS)) {
        val = parse_expression(list, current, filename);
    }

    expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename);

    return (sf_ast_node*)sf_new_var_decl(name, type, val, name_token.span);
}

static sf_ast_node* parse_assign(sf_token_list list, size_t* current, const char* filename) {
    sf_token name_token = advance(list, current);

    if (name_token.type != SF_TOKEN_TYPE_IDENTIFIER) {
        sf_log(
            "unexpected token",
            "expected an identifier but found '%s'",
            "check for a missing or misplaced token nearby",
            filename,
            SF_PARSER_UNEXPECTED_TOKEN,
            name_token.span,
            SF_SEV_ERROR,
            name_token.value
        );

        synchronize(list, current);
    }

    char* name = name_token.value;

    expect(list, current, SF_TOKEN_TYPE_EQUALS, filename);

    sf_ast_node* val = parse_expression(list, current, filename);

    expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename);

    return (sf_ast_node*)sf_new_var_assign(name, val, name_token.span);
}

static sf_ast_node* parse_statement(sf_token_list list, size_t* current, const char* filename) {
    sf_token token = list.tokens[*current];

    if (is_type(token)) {
        return parse_declaration(list, current, filename);
    }

    if (is_ident(token)) {
        return parse_assign(list, current, filename);
    }

    if (is_block(token)) {
        return parse_block(list, current, filename);
    }

    sf_log(
        "unexpected token",
        "unexpected '%s' at the start of a statement",
        "a statement must start with a type, identifier, or '{'",
        filename,
        SF_PARSER_UNEXPECTED_TOKEN,
        token.span,
        SF_SEV_ERROR,
        token.value
    );

    synchronize(list, current);

    return NULL;
}

static sf_ast_node* parse_block(sf_token_list list, size_t* current, const char* filename) {
    sf_span block_span = list.tokens[*current].span;
    sf_block_node* block = sf_new_block(block_span);

    expect(list, current, SF_TOKEN_TYPE_LBRACE, filename);

    while (
        list.tokens[*current].type != SF_TOKEN_TYPE_RBRACE &&
        list.tokens[*current].type != SF_TOKEN_TYPE_EOF
    ) {
        sf_ast_node* stmt = parse_statement(list, current, filename);
        if (stmt) sf_block_add_statement(block, stmt);
    };

    expect(list, current, SF_TOKEN_TYPE_RBRACE, filename);

    return (sf_ast_node*)block;
}

static sf_ast_node* parse_cast(sf_token_list list, size_t* current, const char* filename) {
    sf_ast_node* expr = parse_unary(list, current, filename);

    while (true) {
        sf_span as_span = list.tokens[*current].span;
        if (!match(list, current, SF_TOKEN_TYPE_KW_AS)) break;

        sf_token type_token = advance(list, current);
        sf_value_type target_type;

        switch (type_token.type) {
            case SF_TOKEN_TYPE_KW_I8:  target_type = SF_VAL_TYPE_I8;     break;
            case SF_TOKEN_TYPE_KW_I16: target_type = SF_VAL_TYPE_I16;    break;
            case SF_TOKEN_TYPE_KW_I32: target_type = SF_VAL_TYPE_I32;    break;
            case SF_TOKEN_TYPE_KW_I64: target_type = SF_VAL_TYPE_I64;    break;
                
            case SF_TOKEN_TYPE_KW_U8:  target_type = SF_VAL_TYPE_U8;     break;
            case SF_TOKEN_TYPE_KW_U16: target_type = SF_VAL_TYPE_U16;    break;
            case SF_TOKEN_TYPE_KW_U32: target_type = SF_VAL_TYPE_U32;    break;
            case SF_TOKEN_TYPE_KW_U64: target_type = SF_VAL_TYPE_U64;    break;
                
            default: 
                sf_log(
                    "unexpected token",
                    "expected a type keyword after 'as' but found '%s'",
                    "use any type keyword",
                    filename,
                    SF_PARSER_UNEXPECTED_TOKEN,
                    type_token.span,
                    SF_SEV_ERROR,
                    type_token.value
                );

                synchronize(list, current);

                return NULL;
                break;
        }

        expr = (sf_ast_node*)sf_new_cast_expr(expr, target_type, as_span);
    }

    return expr;
}