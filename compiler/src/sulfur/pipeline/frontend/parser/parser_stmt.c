#include "sulfur/pipeline/frontend/parser/parser_internal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/lexer.h"
#include "sulfur/utils/string.h"
#include "sulfur/utils/token_utils.h"
#include "sulfur/utils/log.h"

static sf_ast_node *parse_expr_stmt(sf_parser *p);
static sf_ast_node *parse_decl(sf_parser *p);
static sf_ast_node *parse_var_decl(sf_parser *p, sf_qualifiers qualifiers);
static sf_ast_node *parse_func_decl(sf_parser *p, sf_qualifiers qualifiers);
static sf_ast_node *parse_var_assign(sf_parser *p);
static sf_ast_node *parse_if(sf_parser *p);
static sf_ast_node *parse_while(sf_parser *p);
static sf_ast_node *parse_return(sf_parser *p);
static sf_ast_node *parse_block(sf_parser *p);
static sf_ast_node *parse_namespace(sf_parser *p);

static sf_qualifiers parse_qualifiers(sf_parser *p);
static sf_ast_node *parse_import(sf_parser *p);

static void report_repeated_qual(const sf_parser *p, sf_token token);

static void skip_invalid_import(sf_parser *p);

sf_ast_node *sf_parse_top_level(sf_parser *p) {
    const sf_token *token = sf_parser_peek(p);

    if (sf_token_is_fn(*token) || sf_token_is_qual(*token)) {
        sf_ast_node *node = parse_decl(p);

        if (node == NULL) {
            sf_parser_recover_top_level(p);
        };

        return node;
    }

    if (sf_token_is_import(*token)) {
        sf_ast_node *node = parse_import(p);

        if (node == NULL) {
            sf_parser_recover_top_level(p);
        };

        return node;
    }

    if (sf_token_is_namespace(*token)) {
        sf_ast_node *node = parse_namespace(p);
        return node;
    }

    sf_log(
        "unexpected token", "token not allowed in top-level",
        "follow the language grammar", p->filename,
        SF_LOG_PARSER_UNEXPECTED_TOKEN,
        token->span, SF_LOG_SEVERITY_ERROR
    );

    sf_parser_recover_top_level(p);

    return NULL;
}

sf_ast_node *sf_parse_statement(sf_parser *p) {
    const sf_token *token = sf_parser_peek(p);

    if (
        sf_token_is_type(*token) ||
        sf_token_is_fn(*token) ||
        sf_token_is_qual(*token)
    ) {
        return parse_decl(p);
    }

    if (
        sf_token_is_ident(*token) &&
        sf_token_is_assign(*sf_parser_peek_next(p))
    ) {
        return parse_var_assign(p);
    }

    if (sf_token_is_block(*token)) {
        return parse_block(p);
    }

    if (sf_token_is_if(*token)) {
        return parse_if(p);
    }

    if (sf_token_is_while(*token)) {
        return parse_while(p);
    }

    if (sf_token_is_return(*token)) {
        return parse_return(p);
    }

    return parse_expr_stmt(p);
}

void sf_parser_recover_top_level(sf_parser *p) {
    if (sf_parser_peek(p)->type != SF_TOKEN_TYPE_EOF) {
        p->current++;
    }

    int depth = 0;

    while (sf_parser_peek(p)->type != SF_TOKEN_TYPE_EOF) {
        switch (sf_parser_peek(p)->type) {
            case SF_TOKEN_TYPE_LBRACE: {
                depth++;
                p->current++;
                break;
            }

            case SF_TOKEN_TYPE_RBRACE: {
                if (depth > 0) {
                    depth--;
                }
                p->current++;
                break;
            }

            case SF_TOKEN_TYPE_SEMICOLON: {
                p->current++;
                if (depth == 0) {
                    return;
                }
                break;
            }

            case SF_TOKEN_TYPE_KW_FN:
            case SF_TOKEN_TYPE_KW_I8:
            case SF_TOKEN_TYPE_KW_I16:
            case SF_TOKEN_TYPE_KW_I32:
            case SF_TOKEN_TYPE_KW_I64:
            case SF_TOKEN_TYPE_KW_U8:
            case SF_TOKEN_TYPE_KW_U16:
            case SF_TOKEN_TYPE_KW_U32:
            case SF_TOKEN_TYPE_KW_U64:
            case SF_TOKEN_TYPE_KW_BOOL: {
                if (depth == 0) {
                    return;
                }
                p->current++;
                break;
            }

            default: {
                p->current++;
                break;
            }
        }
    }
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

static sf_ast_node *parse_decl(sf_parser *p) {
    sf_qualifiers quals = parse_qualifiers(p);
    const sf_token *token = sf_parser_peek(p);

    if (sf_token_is_fn(*token)) {
        return parse_func_decl(p, quals);
    }

    if (sf_token_is_type(*token)) {
        return parse_var_decl(p, quals);
    }

    sf_parser_report_unexpected(
        p, token, "a declaration",
        "expected a variable declaration"
    );

    sf_parser_recover_statement(p);
    return NULL;
}

static sf_ast_node *parse_var_decl(sf_parser *p, sf_qualifiers qualifiers) {
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

    return (sf_ast_node *)sf_new_var_decl_node(
        p->arena, name_token->value, type,
        value, qualifiers, name_token->span
    );
}

static sf_ast_node *parse_var_assign(sf_parser *p) {
    const sf_token *name_token = sf_parser_advance(p);
    const sf_token *op_token = sf_parser_advance(p);

    sf_ast_node *rhs = sf_parse_expression(p);

    if (rhs == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_ast_node *value = rhs;

    if (op_token->type != SF_TOKEN_TYPE_EQUAL) {
        sf_ir_operation_type op = sf_token_to_binary_op(*op_token);

        if (op == SF_OP_TYPE_UNRESOLVED) {
            return NULL;
        }

        sf_ast_node *lhs_ref = (sf_ast_node *)sf_parser_make_identifier(p, name_token);

        if (lhs_ref == NULL) {
            return NULL;
        }

        value = (sf_ast_node *)sf_new_binary_node(
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

    sf_var_assign_node *assign = sf_new_var_assign_node(p->arena, value, name_token->span);

    if (assign == NULL) {
        return NULL;
    }

    if (!sf_path_add_segment(p->arena, &assign->path, name_token->value, name_token->span)) {
        return NULL;
    }

    return (sf_ast_node *)assign;
}

static sf_ast_node *parse_if(sf_parser *p) {
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
            branch_else = parse_if(p);
        } else {
            branch_else = parse_block(p);
        }

        if (branch_else == NULL) {
            sf_parser_recover_statement(p);
            return NULL;
        }
    }

    return (sf_ast_node *)sf_new_if_node(
        p->arena, condition, branch_then, branch_else, token->span
    );
}

static sf_ast_node *parse_while(sf_parser *p) {
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

    return (sf_ast_node *)sf_new_while_node(
        p->arena, condition, branch_do, token->span
    );
}

static sf_ast_node *parse_func_decl(sf_parser *p, sf_qualifiers qualifiers) {
    const sf_token *fn_token = sf_parser_advance(p);

    const sf_token *name_token = sf_parser_expect_identifier(
        p, "a function name", "follow the language syntax"
    );

    if (name_token == NULL) {
        sf_parser_recover_statement(p);
        return NULL;
    }

    sf_func_decl_node *func = sf_new_func_decl_node(
        p->arena, name_token->value, SF_VAL_TYPE_UNRESOLVED, qualifiers, fn_token->span
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
            sf_qualifiers param_qualifiers = parse_qualifiers(p);

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

            char *param_name = sf_string_dup_arena(
                p->arena, param_name_token->value
            );

            if (param_name == NULL) {
                sf_log(
                    "insufficient memory", "cannot allocate memory for compiling.",
                    "free some memory and try again", NULL,
                    SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0},
                    SF_LOG_SEVERITY_FATAL
                );
                return NULL;
            }

            sf_parameter param = {
                .name = param_name,
                .type = param_type,
                .qualifiers = param_qualifiers,
                .id = 0
            };

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

    if (sf_token_is_type(*ret_token)) {
        func->return_type = sf_token_to_type(*ret_token);
        sf_parser_advance(p);
    } else if (
        ret_token->type == SF_TOKEN_TYPE_LBRACE ||
        ret_token->type == SF_TOKEN_TYPE_SEMICOLON
    ) {
        func->return_type = SF_VAL_TYPE_VOID;
    } else {
        sf_parser_report_unexpected(
            p, ret_token, "'{', ';', or type name",
            "follow the language syntax"
        );

        sf_parser_recover_statement(p);
        return NULL;
    }

    const sf_token *end_token = sf_parser_peek(p);

    if (end_token->type == SF_TOKEN_TYPE_LBRACE) {
        sf_ast_node *block = parse_block(p);

        if (block == NULL) {
            sf_parser_recover_statement(p);
            return NULL;
        }

        func->body = block;
    } else if (end_token->type == SF_TOKEN_TYPE_SEMICOLON) {
        func->body = NULL;
        sf_parser_advance(p);
    } else {
        sf_parser_report_unexpected(
            p, end_token, "'{' or ';'",
            "follow the language syntax"
        );

        sf_parser_recover_statement(p);
        return NULL;
    }

    return (sf_ast_node *)func;
}

static sf_ast_node *parse_return(sf_parser *p) {
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

    return (sf_ast_node *)sf_new_return_node(p->arena, value, token->span);
}

static sf_ast_node *parse_block(sf_parser *p) {
    sf_block_node *block = sf_new_block_node(p->arena, sf_parser_peek(p)->span);

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

static sf_ast_node *parse_namespace(sf_parser *p) {
    const sf_token *token_ns = sf_parser_advance(p);
    const sf_token *token_seg = sf_parser_expect_identifier(
        p, "a namespace name", "namespace names are identifiers separated by '::'"
    );

    if (token_seg == NULL) {
        return NULL;
    }

    sf_namespace_node *ns = sf_new_namespace_node(p->arena, token_ns->span);

    if (ns == NULL) {
        return NULL;
    }

    sf_path_add_segment(p->arena, &ns->path, token_seg->value, token_seg->span);

    while (sf_parser_match(p, SF_TOKEN_TYPE_COLON_COLON)) {
        const sf_token *token = sf_parser_expect_identifier(
            p, "a name after '::'", "namespace names are identifiers separated by '::'"
        );

        if (token == NULL) {
            return NULL;
        }

        sf_path_add_segment(
            p->arena, &ns->path, token->value, token->span
        );
    }

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_LBRACE)) {
        return NULL;
    }

    while (
        sf_parser_peek(p)->type != SF_TOKEN_TYPE_RBRACE &&
        sf_parser_peek(p)->type != SF_TOKEN_TYPE_EOF
    ) {
        const sf_token *token = sf_parser_peek(p);

        size_t start = p->current;

        sf_ast_node *item = NULL;
        if (token->type != SF_TOKEN_TYPE_KW_IMPORT) {
            item = sf_parse_top_level(p);
        } else {
            sf_log(
                "invalid import", "imports are not allowed inside namespaces",
                "move the import somewhere else or remove it", p->filename,
                SF_LOG_PARSER_INVALID_IMPORT, token->span,
                SF_LOG_SEVERITY_ERROR
            );

            skip_invalid_import(p);
        }

        if (item != NULL) {
            sf_namespace_add_item(p->arena, ns, item);
        } else if (p->current == start) {
            sf_parser_advance(p);
        }
    }

    sf_parser_expect(p, SF_TOKEN_TYPE_RBRACE);

    return (sf_ast_node *)ns;
}

static sf_qualifiers parse_qualifiers(sf_parser *p) {
    sf_qualifiers quals = {0};

    while (true) {
        const sf_token *token = sf_parser_peek(p);

        switch (token->type) {
            case SF_TOKEN_TYPE_KW_MUT: {
                if (quals.is_mutable == true) {
                    report_repeated_qual(p, *token);
                } else {
                    quals.is_mutable = true;
                }
                break;
            }

            case SF_TOKEN_TYPE_KW_PUB: {
                if (quals.is_public == true) {
                    report_repeated_qual(p, *token);
                } else {
                    quals.is_public = true;
                }
                break;
            }

            case SF_TOKEN_TYPE_KW_EXTERN: {
                if (quals.is_extern == true) {
                    report_repeated_qual(p, *token);
                } else {
                    quals.is_extern = true;
                }
                break;
            }

            default: {
                return quals;
            }
        }

        sf_parser_advance(p);
    }
}

#define IMPORT_PATH_SIZE 512

static sf_ast_node *parse_import(sf_parser *p) {
    const sf_token *token_import = sf_parser_peek(p);

    if (!sf_parser_expect(p, SF_TOKEN_TYPE_KW_IMPORT)) {
        return NULL;
    }

    char path[IMPORT_PATH_SIZE] = {0};
    size_t path_len = 0;

    bool expect_identifier = true;

    while (true) {
        const sf_token *token = sf_parser_peek(p);

        if (token->type == SF_TOKEN_TYPE_SEMICOLON) {
            if (expect_identifier) {
                sf_parser_report_unexpected(
                    p, token, "identifier",
                    "import paths must contain valid identifiers separated by dots, ending with ';'"
                );
                return NULL;
            }

            sf_parser_advance(p);
            break;
        }

        if (expect_identifier) {
            if (token->type != SF_TOKEN_TYPE_IDENTIFIER) {
                sf_parser_report_unexpected(
                    p, token, "identifier",
                    "import paths must contain valid identifiers separated by dots, ending with ';'"
                );
                return NULL;
            }

            token = sf_parser_advance(p);

            size_t len = strlen(token->value);

            if (path_len + len >= IMPORT_PATH_SIZE) {
                return NULL;
            }

            memcpy(path + path_len, token->value, len);
            path_len += len;

            expect_identifier = false;
        } else {
            if (token->type != SF_TOKEN_TYPE_DOT) {
                sf_parser_report_unexpected(
                    p, token, "'.' or ';'",
                    "import paths must contain valid identifiers separated by dots, ending with ';'"
                );

                return NULL;
            }

            token = sf_parser_advance(p);

            if (path_len + 1 >= IMPORT_PATH_SIZE) {
                return NULL;
            }

            path[path_len++] = '/';
            expect_identifier = true;
        }
    }

    static const char extension[] = ".slfr";
    const size_t extension_len = sizeof(extension) - 1;

    if (path_len + extension_len >= IMPORT_PATH_SIZE) {
        return NULL;
    }

    memcpy(path + path_len, extension, extension_len + 1);

    return (sf_ast_node *)sf_new_import_node(
        p->arena,
        path,
        token_import->span
    );
}

static void report_repeated_qual(const sf_parser *p, sf_token token) {
    sf_log(
        "repeated qualifier",
        "declaration has 2 or more occurencies of the '%s' qualifier",
        "remove all duplicated qualifiers", p->filename,
        SF_LOG_PARSER_REPEATED_QUALIFIER, token.span,
        SF_LOG_SEVERITY_WARNING, sf_token_type_name(token.type)
    );
}

static void skip_invalid_import(sf_parser *p) {
    sf_parser_advance(p);

    while (
        sf_parser_peek(p)->type != SF_TOKEN_TYPE_SEMICOLON &&
        sf_parser_peek(p)->type != SF_TOKEN_TYPE_RBRACE &&
        sf_parser_peek(p)->type != SF_TOKEN_TYPE_EOF
    ) {
        sf_parser_advance(p);
    }

    if (sf_parser_peek(p)->type == SF_TOKEN_TYPE_SEMICOLON) {
        sf_parser_advance(p);
    }
}

const char *sf_path_simple_name(const sf_path *path) {
    if (path->is_global || path->segment_count != 1) {
        return NULL;
    }

    return path->segments[0].name;
}
