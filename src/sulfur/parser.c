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

static sfASTNode* parse_declaration(sfTokenList list, size_t* current, const char* filename) {
    sfToken typeToken = advance(list, current);
    sfValueType type;
    switch (typeToken.type) {
        case SF_TOKEN_TYPE_KW_F32: type = SF_VAL_TYPE_F32; break;
        case SF_TOKEN_TYPE_KW_F64: type = SF_VAL_TYPE_F64; break;
        case SF_TOKEN_TYPE_KW_I8:  type = SF_VAL_TYPE_I8;  break;
        case SF_TOKEN_TYPE_KW_I16: type = SF_VAL_TYPE_I16; break;
        case SF_TOKEN_TYPE_KW_I32: type = SF_VAL_TYPE_I32; break;
        case SF_TOKEN_TYPE_KW_I64: type = SF_VAL_TYPE_I64; break;
        case SF_TOKEN_TYPE_KW_U8:  type = SF_VAL_TYPE_U8;  break;
        case SF_TOKEN_TYPE_KW_U16: type = SF_VAL_TYPE_U16; break;
        case SF_TOKEN_TYPE_KW_U32: type = SF_VAL_TYPE_U32; break;
        case SF_TOKEN_TYPE_KW_U64: type = SF_VAL_TYPE_U64; break;
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
            break;
    }

    sfToken nameToken = advance(list, current);
    char* name = nameToken.value;

    sfASTNode* val = NULL;

    if (match(list, current, SF_TOKEN_TYPE_EQUALS)) {
        sfToken valueToken = advance(list, current);
        val = (sfASTNode*)sfNewLiteral(valueToken.value);
    }

    expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename);

    return (sfASTNode*)sfNewVarDecl(name, type, val);
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

    return false;
}

static bool is_ident(sfToken token) {
    return (token.type == SF_TOKEN_TYPE_IDENTIFIER);
}

static sfASTNode* parse_assign(sfTokenList list, size_t* current, const char* filename) {
    sfToken nameToken = advance(list, current);
    char* name = nameToken.value;

    expect(list, current, SF_TOKEN_TYPE_EQUALS, filename);

    sfToken valueToken = advance(list, current);
    sfASTNode* val = (sfASTNode*)sfNewLiteral(valueToken.value);

    expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename);

    return (sfASTNode*)sfNewAssign(name, val);
}

static sfASTNode* parse_statement(sfTokenList list, size_t* current, const char* filename) {
    sfToken token = list.tokens[*current];

    if (is_type(token))  return parse_declaration(list, current, filename);
    if (is_ident(token)) return parse_assign(list, current, filename);

    return NULL;
}

sfProgramNode* parse(sfTokenList list, const char* filename) {
    sfProgramNode* program = sfNewProgram();

    size_t current = 0;
    while (list.tokens[current].type != SF_TOKEN_TYPE_EOF) {
        sfToken tok = list.tokens[current];

        sfASTNode* stmt = parse_statement(list, &current, filename);

        if (stmt == NULL) {
            sfLogHelper(
                "Unexpected Token",
                "Unexpected token '%s' at top level",
                "Follow the language syntax",
                filename,
                SF_PARSER_UNEXPECTED_TOKEN,
                tok.line,
                tok.column,
                SF_SEV_FATAL,
                tok.value
            );
            current++;
            continue;
        }

        sfProgramAddStatement(program, stmt);
    }

    return program;
}
