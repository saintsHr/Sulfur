#include "sulfur/parser.h"
#include "sulfur/ast.h"
#include "sulfur/lexer.h"
#include "sulfur/util/log.h"

#include <stdbool.h>
#include <stdlib.h>

static void recover_statement(sf_token_list list, size_t* current);
static void recover_expression(sf_token_list list, size_t* current);

static sf_token advance(sf_token_list list, size_t* current);
static bool match(sf_token_list list, size_t* current, sf_token_type type);
static bool expect(sf_token_list list, size_t* current, sf_token_type type, const char* filename);

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
    if (*current >= list.count) {
        return list.tokens[list.count - 1];
    }

    if (list.tokens[*current].type == SF_TOKEN_TYPE_EOF) {
        return list.tokens[*current];
    }

    if (list.count == 0) {
        abort();
    }

    return list.tokens[(*current)++];
}

static bool match(sf_token_list list, size_t* current, sf_token_type type) {
    if (list.count == 0) {
        abort();
    }

    if (list.tokens[*current].type == type) {
        (*current)++;
        return true;
    }

    return false;
}

static bool expect(sf_token_list list, size_t* current, sf_token_type type, const char* filename) {
    if (list.count == 0) {
        abort();
    }

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

        return false;
    }

    return true;
}

static void recover_expression(sf_token_list list, size_t* current) {
    while (list.tokens[*current].type != SF_TOKEN_TYPE_EOF) {
        switch (list.tokens[*current].type) {
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

static void recover_statement(sf_token_list list, size_t* current) {
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
            case SF_TOKEN_TYPE_RBRACE:
                return;

            default:
                (*current)++;
                break;
        }
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
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

    sf_span minus_span = list.tokens[*current].span;

    if (match(list, current, SF_TOKEN_TYPE_MINUS)) {
        sf_ast_node* operand = parse_unary(list, current, filename);
        if (operand == NULL) return NULL;

        return (sf_ast_node*)sf_new_unary_expr(operand, SF_OP_TYPE_NEGATE, minus_span);
    }

    return parse_primary(list, current, filename);
}

static sf_ast_node* parse_primary(sf_token_list list, size_t* current, const char* filename) {
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

    if (match(list, current, SF_TOKEN_TYPE_LPAREN)) {
        sf_ast_node* expr = parse_expression(list, current, filename);
        if (expr == NULL) return NULL;

        if (!expect(list, current, SF_TOKEN_TYPE_RPAREN, filename)) {
            recover_expression(list, current);
            return NULL;
        }
        
        return expr;
    }

    sf_token token = advance(list, current);

    if (token.type == SF_TOKEN_TYPE_INTEGER) {
        sf_ast_node* lit = (sf_ast_node*)sf_new_literal(token.value, token.type, token.span);
        if (lit == NULL) return NULL;

        return lit;
    }

    if (token.type == SF_TOKEN_TYPE_IDENTIFIER) {
        sf_ast_node* id = (sf_ast_node*)sf_new_identifier(token.value, token.span);
        if (id == NULL) return NULL;

        return id;
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

    recover_expression(list, current);

    return NULL;
}

static sf_ast_node* parse_multiplicative(sf_token_list list, size_t* current, const char* filename) {
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

    sf_ast_node* left = parse_cast(list, current, filename);
    if (left == NULL) return NULL;

    while (
        list.tokens[*current].type == SF_TOKEN_TYPE_MULT ||
        list.tokens[*current].type == SF_TOKEN_TYPE_DIV
    ) {
        sf_token op = advance(list, current);

        sf_ast_node* right = parse_cast(list, current, filename);
        if (right == NULL) return NULL;

        sf_operation_type op_type = (op.type == SF_TOKEN_TYPE_MULT)
            ? SF_OP_TYPE_MUL
            : SF_OP_TYPE_DIV;

        left = (sf_ast_node*)sf_new_binary_expr(left, right, op_type, op.span);
        if (left == NULL) return NULL;
    }

    return left;
}

static sf_ast_node* parse_expression(sf_token_list list, size_t* current, const char* filename) {
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

    sf_ast_node* left = parse_multiplicative(list, current, filename);
    if (left == NULL) return NULL;

    while (
        list.tokens[*current].type == SF_TOKEN_TYPE_PLUS ||
        list.tokens[*current].type == SF_TOKEN_TYPE_MINUS
    ) {
        sf_token op = advance(list, current);

        sf_ast_node* right = parse_multiplicative(list, current, filename);
        if (right == NULL) return NULL;

        sf_operation_type op_type = (op.type == SF_TOKEN_TYPE_PLUS)
            ? SF_OP_TYPE_ADD
            : SF_OP_TYPE_SUB;

        left = (sf_ast_node*)sf_new_binary_expr(left, right, op_type, op.span);
        if (left == NULL) return NULL;
    }

    return left;
}

static sf_ast_node* parse_declaration(sf_token_list list, size_t* current, const char* filename) {
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

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

            recover_statement(list, current);

            return NULL;
    }

    sf_token name_token = advance(list, current);

    if (name_token.type != SF_TOKEN_TYPE_IDENTIFIER) {
        sf_log(
            "unexpected token",
            "expected an identifier but found '%s'",
            "variable names cannot be reserved keywords, symbols, or numbers",
            filename,
            SF_PARSER_UNEXPECTED_TOKEN,
            name_token.span,
            SF_SEV_ERROR,
            name_token.value
        );

        recover_statement(list, current);
        return NULL;
    }

    char* name = name_token.value;
    sf_ast_node* val = NULL;

    if (match(list, current, SF_TOKEN_TYPE_EQUALS)) {
        val = parse_expression(list, current, filename);
        if (val == NULL) {
            recover_statement(list, current);
            return NULL;
        }
    }

    if (!expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename)) {
        recover_statement(list, current);
        return NULL;
    }

    sf_ast_node* dcl = (sf_ast_node*)sf_new_var_decl(name, type, val, name_token.span);
    if (dcl == NULL) return NULL;

    return dcl;
}

static sf_ast_node* parse_assign(sf_token_list list, size_t* current, const char* filename) {
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

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

        return NULL;
    }

    char* name = name_token.value;
    if (name == NULL) return NULL;

    if (!expect(list, current, SF_TOKEN_TYPE_EQUALS, filename)) {
        recover_statement(list, current);
        return NULL;
    }

    sf_ast_node* val = parse_expression(list, current, filename);

    if (val == NULL) {
        recover_statement(list, current);
        return NULL;
    }

    if (!expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename)) {
        recover_statement(list, current);
        return NULL;
    }

    sf_ast_node* as = (sf_ast_node*)sf_new_var_assign(name, val, name_token.span);
    if (as == NULL) return NULL;

    return as;
}

static sf_ast_node* parse_statement(sf_token_list list, size_t* current, const char* filename) {
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

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

    recover_statement(list, current);

    return NULL;
}

static sf_ast_node* parse_block(sf_token_list list, size_t* current, const char* filename) {
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

    sf_span block_span = list.tokens[*current].span;
    sf_block_node* block = sf_new_block(block_span);

    if (block == NULL) return NULL;

    if (!expect(list, current, SF_TOKEN_TYPE_LBRACE, filename)) {
        recover_statement(list, current);
        return NULL;
    }

    while (
        list.tokens[*current].type != SF_TOKEN_TYPE_RBRACE &&
        list.tokens[*current].type != SF_TOKEN_TYPE_EOF
    ) {
        sf_ast_node* stmt = parse_statement(list, current, filename);
        if (stmt) sf_block_add_statement(block, stmt);
    };

    if (!expect(list, current, SF_TOKEN_TYPE_RBRACE, filename)) {
        recover_statement(list, current);
        return NULL;
    }

    sf_ast_node* b = (sf_ast_node*)block;
    return b;
}

static sf_ast_node* parse_cast(sf_token_list list, size_t* current, const char* filename) {
    if (filename == NULL) return NULL;
    if (current == NULL) return NULL;

    sf_ast_node* expr = parse_unary(list, current, filename);
    if (expr == NULL) return NULL;

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

                recover_expression(list, current);

                return NULL;
        }

        expr = (sf_ast_node*)sf_new_cast_expr(expr, target_type, as_span);
        if (expr == NULL) return NULL;
    }

    return expr;
}