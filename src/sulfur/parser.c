#include "sulfur/parser.h"
#include "sulfur/util/log.h"

#include <stdbool.h>

void initSymbolTable(sfSymbolTable* table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

void freeSymbolTable(sfSymbolTable* table) {
    for (size_t i = 0; i < table->count; i++) free(table->symbols[i].name);
    free(table->symbols);
}

void addSymbol(sfSymbolTable* table, const char* name, sfValueType type) {
    if (table->count == table->capacity) {
        table->capacity = table->capacity == 0 ? 4 : table->capacity * 2;
        table->symbols = realloc(table->symbols, table->capacity * sizeof(sfSymbol));
    }

    table->symbols[table->count].name = strdup(name);
    table->symbols[table->count].type = type;
    table->count++;
}

sfValueType getSymbolType(sfSymbolTable* table, const char* name) {
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) return table->symbols[i].type;
    }
    return -1;
}

static sfToken peek(sfTokenList list, size_t* current) {
    return list.tokens[*current];
}

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
        sfLogInfo l;
        l.code = SF_PARSER_UNEXPECTED_TOKEN;
        l.line = list.tokens[*current].line;
        l.col = list.tokens[*current].column;
        l.file = filename;
        l.sev = SF_SEV_FATAL;
        l.title = "Unexpected Token";
        l.desc = "Unexpected token '%s'.";
        l.hint = "Follow the language syntax.";
        sfLog(l, list.tokens[*current].value);
    }
}

static sfASTNode* parse_declaration(sfTokenList list, size_t* current, const char* filename, sfSymbolTable* table) {
    // gets type
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
            sfLogInfo l = {0}; 
            l.code = SF_PARSER_UNEXPECTED_TOKEN;
            l.line = typeToken.line;
            l.col = typeToken.column;
            l.file = filename;
            l.sev = SF_SEV_FATAL;
            l.title = "Unexpected Token";
            l.desc = "Unexpected token, expected a type keyword.";
            l.hint = "Follow the language syntax.";
            sfLog(l, typeToken.value);
    }

    // gets identifier
    sfToken nameToken = advance(list, current);
    char* name = nameToken.value;

    // gets value
    sfASTNode* val = NULL;

    if (match(list, current, SF_TOKEN_TYPE_EQUALS)) {
        sfToken valueToken = advance(list, current);

        switch(type) {
            case SF_VAL_TYPE_F32: val = (sfASTNode*)sfNewLiteralF32((float) atof(valueToken.value)); break;
            case SF_VAL_TYPE_F64: val = (sfASTNode*)sfNewLiteralF64((double)atof(valueToken.value)); break;

            case SF_VAL_TYPE_I8:  val = (sfASTNode*)sfNewLiteralI8 ((int8_t) strtoll(valueToken.value, NULL, 10)); break;
            case SF_VAL_TYPE_I16: val = (sfASTNode*)sfNewLiteralI16((int16_t)strtoll(valueToken.value, NULL, 10)); break;
            case SF_VAL_TYPE_I32: val = (sfASTNode*)sfNewLiteralI32((int32_t)strtoll(valueToken.value, NULL, 10)); break;
            case SF_VAL_TYPE_I64: val = (sfASTNode*)sfNewLiteralI64((int64_t)strtoll(valueToken.value, NULL, 10)); break;

            case SF_VAL_TYPE_U8:  val = (sfASTNode*)sfNewLiteralU8 ((uint8_t) strtoull(valueToken.value, NULL, 10)); break;
            case SF_VAL_TYPE_U16: val = (sfASTNode*)sfNewLiteralU16((uint16_t)strtoull(valueToken.value, NULL, 10)); break;
            case SF_VAL_TYPE_U32: val = (sfASTNode*)sfNewLiteralU32((uint32_t)strtoull(valueToken.value, NULL, 10)); break;
            case SF_VAL_TYPE_U64: val = (sfASTNode*)sfNewLiteralU64((uint64_t)strtoull(valueToken.value, NULL, 10)); break;
        }
    } else {
        // defaults
        switch(type) {
            case SF_VAL_TYPE_F32: val = (sfASTNode*)sfNewLiteralF32(0.0f); break;
            case SF_VAL_TYPE_F64: val = (sfASTNode*)sfNewLiteralF64(0.0);  break;

            case SF_VAL_TYPE_I8:  val = (sfASTNode*)sfNewLiteralI8 (0);    break;
            case SF_VAL_TYPE_I16: val = (sfASTNode*)sfNewLiteralI16(0);    break;
            case SF_VAL_TYPE_I32: val = (sfASTNode*)sfNewLiteralI32(0);    break;
            case SF_VAL_TYPE_I64: val = (sfASTNode*)sfNewLiteralI64(0);    break;

            case SF_VAL_TYPE_U8:  val = (sfASTNode*)sfNewLiteralU8 (0);    break;
            case SF_VAL_TYPE_U16: val = (sfASTNode*)sfNewLiteralU16(0);    break;
            case SF_VAL_TYPE_U32: val = (sfASTNode*)sfNewLiteralU32(0);    break;
            case SF_VAL_TYPE_U64: val = (sfASTNode*)sfNewLiteralU64(0);    break;
        }
    }

    // expect ;
    expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename);

    addSymbol(table, name, type);

    // creates & returns node
    return (sfASTNode*)sfNewVarDecl(name, type, val);
}

static sfASTNode* parse_assign(sfTokenList list, size_t* current, const char* filename, sfValueType type) {
    // gets identifier
    sfToken nameToken = advance(list, current);
    char* name = nameToken.value;

    // expects =
    expect(list, current, SF_TOKEN_TYPE_EQUALS, filename);

    // gets value
    sfToken valueToken = advance(list, current);
    sfASTNode* val = NULL;
    switch(type) {
        case SF_VAL_TYPE_F32: val = (sfASTNode*)sfNewLiteralF32((float)atof(valueToken.value)); break;
        case SF_VAL_TYPE_F64: val = (sfASTNode*)sfNewLiteralF64((double)atof(valueToken.value)); break;
        case SF_VAL_TYPE_I8:  val = (sfASTNode*)sfNewLiteralI8((int8_t)atoi(valueToken.value)); break;
        case SF_VAL_TYPE_I16: val = (sfASTNode*)sfNewLiteralI16((int16_t)atoi(valueToken.value)); break;
        case SF_VAL_TYPE_I32: val = (sfASTNode*)sfNewLiteralI32((int32_t)atoi(valueToken.value)); break;
        case SF_VAL_TYPE_I64: val = (sfASTNode*)sfNewLiteralI64((int64_t)atoll(valueToken.value)); break;
        case SF_VAL_TYPE_U8:  val = (sfASTNode*)sfNewLiteralU8((uint8_t)atoi(valueToken.value)); break;
        case SF_VAL_TYPE_U16: val = (sfASTNode*)sfNewLiteralU16((uint16_t)atoi(valueToken.value)); break;
        case SF_VAL_TYPE_U32: val = (sfASTNode*)sfNewLiteralU32((uint32_t)atoi(valueToken.value)); break;
        case SF_VAL_TYPE_U64: val = (sfASTNode*)sfNewLiteralU64((uint64_t)atoll(valueToken.value)); break;
    }

    // expect ;
    expect(list, current, SF_TOKEN_TYPE_SEMICOLON, filename);

    // creates & returns node
    return (sfASTNode*)sfNewAssign(name, val);
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
    if (token.type == SF_TOKEN_TYPE_IDENTIFIER)  return true;
    return false;
}

static sfASTNode* parse_statement(sfTokenList list, size_t* current, const char* filename, sfSymbolTable* table) {
    sfToken token = list.tokens[*current];

    if (is_type(token)) {
        sfASTNode* decl = parse_declaration(list, current, filename, table);
        return decl;
    }

    if (is_ident(token)) {
        sfValueType type = getSymbolType(table, token.value);
        if (type == -1) {
            sfLogInfo l = {0};
            l.code = SF_PARSER_UNDECLARED_VARIABLE;
            l.line = token.line;
            l.col = token.column;
            l.file = filename;
            l.sev = SF_SEV_FATAL;
            l.title = "Undeclared Variable";
            l.desc = "Variable '%s' used before declaration.";
            l.hint = "Declare the variable before using it.";
            sfLog(l, token.value);
        }
        return parse_assign(list, current, filename, type);
    }

    return NULL;
}

sfProgramNode* parse(sfTokenList list, const char* filename) {
    sfProgramNode* program = sfNewProgram();
    sfSymbolTable symTable;
    initSymbolTable(&symTable);

    size_t current = 0;
    while (list.tokens[current].type != SF_TOKEN_TYPE_EOF) {
        sfToken tok = list.tokens[current];

        sfASTNode* stmt = parse_statement(list, &current, filename, &symTable);

        if (stmt == NULL) {
            sfLogHelper(
                "Unexpected Token",
                "Unexpected token '%s' at top level.",
                "Follow the language syntax.",
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

    freeSymbolTable(&symTable);
    return program;
}
