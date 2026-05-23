#include "sulfur/parser.h"
#include "sulfur/lexer.h"
#include "sulfur/util/log.h"

#include <stdbool.h>

static sfToken advance(sfTokenList list, size_t* current) {
    return list.tokens[(*current)++];
}

static bool match(sfTokenList list, size_t* current, sfTokenType type) {
    if (list.tokens[*current].type == type) {
        (*current)++;
        return true;
    }
    return false;
}

static void expect(sfTokenList list, size_t* current, sfTokenType type, const char* filename) {
    if (!match(list, current, type)) {
        sfLogHelper(
            "Unexpected Token",
            "Unexpected token '%s'",
            "Follow the language syntax",
            filename,
            SF_PARSER_UNEXPECTED_TOKEN,
            list.tokens[*current].line,
            list.tokens[*current].column,
            SF_SEV_FATAL,
            list.tokens[*current].value
        );
    }
}

static bool is_type(sfToken token) {
    if (token.type == SF_TOKEN_TYPE_KW_I8)  return true;
    if (token.type == SF_TOKEN_TYPE_KW_I16) return true;
    if (token.type == SF_TOKEN_TYPE_KW_I32) return true;
    if (token.type == SF_TOKEN_TYPE_KW_I64) return true;

    if (token.type == SF_TOKEN_TYPE_KW_U8)  return true;
    if (token.type == SF_TOKEN_TYPE_KW_U16) return true;
    if (token.type == SF_TOKEN_TYPE_KW_U32) return true;
    if (token.type == SF_TOKEN_TYPE_KW_U64) return true;

    if (token.type == SF_TOKEN_TYPE_KW_F32) return true;
    if (token.type == SF_TOKEN_TYPE_KW_F64) return true;

    if (token.type == SF_TOKEN_TYPE_KW_STRING) return true;

    return false;
}

static bool is_ident(sfToken token) {
    return (token.type == SF_TOKEN_TYPE_IDENTIFIER);
}

static sfASTNode* parse_primary(sfTokenList list, size_t* current, const char* filename) {
    sfToken token = advance(list, current);

    if (token.type == SF_TOKEN_TYPE_NUMBER || token.type == SF_TOKEN_TYPE_STRING) {
        return (sfASTNode*)sfNewLiteral(token.value, token.type);
    }

    if (token.type == SF_TOKEN_TYPE_IDENTIFIER) {
        return (sfASTNode*)sfNewIdentifier(token.value);
    }

    sfLogHelper(
        "Unexpected Token",
        "Expected a value, got '%s'",
        "Follow the language syntax",
        filename,
        SF_PARSER_UNEXPECTED_TOKEN,
        token.line,
        token.column,
        SF_SEV_FATAL,
        token.value
    );

    return NULL;
}

static sfASTNode* parse_multiplicative(sfTokenList list, size_t* current, const char* filename) {
    sfASTNode* left = parse_primary(list, current, filename);

    while (
        list.tokens[*current].type == SF_TOKEN_TYPE_MULT ||
        list.tokens[*current].type == SF_TOKEN_TYPE_DIV
    ) {
        sfToken op = advance(list, current);
        sfASTNode* right = parse_primary(list, current, filename);

        sfOperationType op_type = (op.type == SF_TOKEN_TYPE_MULT)
            ? SF_OP_TYPE_MUL
            : SF_OP_TYPE_DIV;

        left = (sfASTNode*)sfNewBinary(left, right, op_type);
    }

    return left;
}

static sfASTNode* parse_expression(sfTokenList list, size_t* current, const char* filename) {
    sfASTNode* left = parse_multiplicative(list, current, filename);

    while (
        list.tokens[*current].type == SF_TOKEN_TYPE_PLUS ||
        list.tokens[*current].type == SF_TOKEN_TYPE_MINUS
    ) {
        sfToken op = advance(list, current);
        sfASTNode* right = parse_multiplicative(list, current, filename);

        sfOperationType op_type = (op.type == SF_TOKEN_TYPE_PLUS)
            ? SF_OP_TYPE_ADD
            : SF_OP_TYPE_SUB;

        left = (sfASTNode*)sfNewBinary(left, right, op_type);
    }

    return left;
}

static sfASTNode* parse_declaration(sfTokenList list, size_t* current, const char* filename) {
    sfToken typeToken = advance(list, current);
    sfValueType type;
    switch (typeToken.type) {
        case SF_TOKEN_TYPE_KW_F32:    type = SF_VAL_TYPE_F32;    break;
        case SF_TOKEN_TYPE_KW_F64:    type = SF_VAL_TYPE_F64;    break;
        case SF_TOKEN_TYPE_KW_I8:     type = SF_VAL_TYPE_I8;     break;
        case SF_TOKEN_TYPE_KW_I16:    type = SF_VAL_TYPE_I16;    break;
        case SF_TOKEN_TYPE_KW_I32:    type = SF_VAL_TYPE_I32;    break;
        case SF_TOKEN_TYPE_KW_I64:    type = SF_VAL_TYPE_I64;    break;
        case SF_TOKEN_TYPE_KW_U8:     type = SF_VAL_TYPE_U8;     break;
        case SF_TOKEN_TYPE_KW_U16:    type = SF_VAL_TYPE_U16;    break;
        case SF_TOKEN_TYPE_KW_U32:    type = SF_VAL_TYPE_U32;    break;
        case SF_TOKEN_TYPE_KW_U64:    type = SF_VAL_TYPE_U64;    break;
        case SF_TOKEN_TYPE_KW_STRING: type = SF_VAL_TYPE_STRING; break;
        default: 
            sfLogHelper(
                "Unexpected Token",
                "Unexpected token, expected a type keyword.",
                "Follow the language syntax.",
                filename,
                SF_PARSER_UNEXPECTED_TOKEN,
                typeToken.line,
                typeToken.column,
                SF_SEV_FATAL,
                typeToken.value
            );
            return NULL;
            break;
    }

    sfToken nameToken = advance(list, current);
    char* name = nameToken.value;

    sfASTNode* val = NULL;

    if (match(list, current, SF_TOKEN_TYPE_EQUALS)) {
        val = parse_expression(list, current, filename);
    }

    expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename);

    return (sfASTNode*)sfNewVarDecl(name, type, val);
}

static sfASTNode* parse_assign(sfTokenList list, size_t* current, const char* filename) {
    sfToken nameToken = advance(list, current);

    if (nameToken.type != SF_TOKEN_TYPE_IDENTIFIER) {
        sfLogHelper(
            "Unexpected Token",
            "Expected an identifier, got '%s'",
            "Follow the language syntax",
            filename,
            SF_PARSER_UNEXPECTED_TOKEN,
            nameToken.line,
            nameToken.column,
            SF_SEV_FATAL,
            nameToken.value
        );
    }

    char* name = nameToken.value;

    expect(list, current, SF_TOKEN_TYPE_EQUALS, filename);

    sfASTNode* val = parse_expression(list, current, filename);

    expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename);

    return (sfASTNode*)sfNewAssign(name, val);
}

static sfASTNode* parse_statement(sfTokenList list, size_t* current, const char* filename) {
    sfToken token = list.tokens[*current];

    if (is_type(token))  return parse_declaration(list, current, filename);
    if (is_ident(token)) return parse_assign(list, current, filename);

    sfLogHelper(
        "Unexpected Token",
        "Unexpected token '%s' at statement level",
        "Follow the language syntax",
        filename,
        SF_PARSER_UNEXPECTED_TOKEN,
        token.line,
        token.column,
        SF_SEV_FATAL,
        token.value
    );

    return NULL;
}

sfProgramNode* parse(sfTokenList list, const char* filename) {
    sfProgramNode* program = sfNewProgram();

    size_t current = 0;
    while (list.tokens[current].type != SF_TOKEN_TYPE_EOF) {
        sfASTNode* stmt = parse_statement(list, &current, filename);
        sfProgramAddStatement(program, stmt);
    }

    return program;
}
