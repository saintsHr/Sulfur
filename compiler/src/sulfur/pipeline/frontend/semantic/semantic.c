#include "sulfur/pipeline/frontend/semantic/semantic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/type_utils.h"

static void analyze_statement(sf_semantic *s, sf_ast_node *node);
static void analyze_var_decl(sf_semantic *s, sf_var_decl_node *var);
static void analyze_var_assign(sf_semantic *s, sf_var_assign_node *asg);
static void analyze_block(sf_semantic *s, sf_block_node *block);
static void analyze_if(sf_semantic *s, sf_if_stmt_node *if_stmt);
static void analyze_while(sf_semantic *s, sf_while_stmt_node *while_stmt);
static void analyze_func_decl(sf_semantic *s, sf_func_decl_node *func);
static void analyze_return(sf_semantic *s, sf_return_stmt_node *ret);

static bool check_assignment_type(
    sf_value_type resolved, sf_value_type target,
    sf_span span, const char *filename
);
static bool check_return_type(
    sf_value_type resolved, sf_value_type target,
    const char *func_name, sf_span span, const char *filename
);

static bool stmt_always_returns(sf_ast_node *node);

void sf_analyze(sf_program_node *program, const char *filename) {
    sf_scope scope;
    sf_func_symbol_table funcs;

    scope_init(&scope);
    scope_push(&scope);
    sf_func_symbol_table_init(&funcs);

    sf_semantic semantic = {
        .scope = &scope,
        .funcs = &funcs,
        .current_func = NULL,
        .filename = filename,
    };

    for (uint64_t i = 0; i < program->statement_count; i++) {
        analyze_statement(&semantic, program->statements[i]);
    }

    scope_pop(&scope);
    scope_free(&scope);
    sf_func_symbol_table_free(&funcs);
}

void sf_semantic_report_undeclared(
    const sf_semantic *s, const char *name, sf_span span
) {
    const char *closest = scope_find_closest(s->scope, name);

    if (closest != NULL) {
        sf_log(
            "undeclared symbol", "'%s' is not declared in this scope",
            "did you mean '%s'?", s->filename, SF_LOG_SEMANTIC_UNDECLARED, span,
            SF_LOG_SEVERITY_ERROR, name, closest
        );

        return;
    }

    sf_log(
        "undeclared symbol", "'%s' is not declared in this scope",
        "check for typos, or declare the variable before using it",
        s->filename, SF_LOG_SEMANTIC_UNDECLARED, span, SF_LOG_SEVERITY_ERROR,
        name
    );
}

static void analyze_statement(sf_semantic *s, sf_ast_node *node) {
    switch (node->type) {
        case SF_NODE_VAR_DECL: {
            analyze_var_decl(s, (sf_var_decl_node *)node);
            break;
        }
        case SF_NODE_VAR_ASSIGN: {
            analyze_var_assign(s, (sf_var_assign_node *)node);
            break;
        }
        case SF_NODE_BLOCK: {
            analyze_block(s, (sf_block_node *)node);
            break;
        }
        case SF_NODE_IF_STMT: {
            analyze_if(s, (sf_if_stmt_node *)node);
            break;
        }
        case SF_NODE_WHILE_STMT: {
            analyze_while(s, (sf_while_stmt_node *)node);
            break;
        }
        case SF_NODE_FUNC_DECL: {
            analyze_func_decl(s, (sf_func_decl_node *)node);
            break;
        }
        case SF_NODE_RETURN_STMT: {
            analyze_return(s, (sf_return_stmt_node *)node);
            break;
        }
        default: {
            sf_semantic_expr(s, node, SF_VAL_TYPE_UNRESOLVED);
            break;
        }
    }
}

static void analyze_var_decl(sf_semantic *s, sf_var_decl_node *var) {
    if (var->var_type == SF_VAL_TYPE_VOID) {
        sf_log(
            "invalid type", "variables cannot be 'void'",
            "give the variable '%s' a valid type", s->filename,
            SF_LOG_SEMANTIC_INVALID_TYPE, var->base.span, SF_LOG_SEVERITY_ERROR,
            var->name
        );

        return;
    }

    sf_var_symbol sym = {
        .name = var->name,
        .type = var->var_type,
        .initialized = var->value != NULL,
        .span = var->base.span,
    };

    if (var->value != NULL) {
        if (!sf_semantic_expr(s, var->value, var->var_type)) {
            return;
        }

        if (
            !check_assignment_type(
                var->value->resolved, var->var_type, var->base.span, s->filename
            )
        ) {
            return;
        }
    }

    scope_insert(s->scope, sym, s->filename);

    sf_var_symbol *inserted = scope_lookup(s->scope, var->name);

    if (inserted != NULL) {
        var->id = inserted->id;
    }
}

static void analyze_var_assign(sf_semantic *s, sf_var_assign_node *asg) {
    sf_var_symbol *sym = scope_lookup(s->scope, asg->name);

    if (sym == NULL) {
        sf_semantic_report_undeclared(s, asg->name, asg->base.span);
        return;
    }

    asg->id = sym->id;

    if (!sf_semantic_expr(s, asg->value, sym->type)) {
        return;
    }

    if (
        !check_assignment_type(
            asg->value->resolved, sym->type, asg->base.span, s->filename
        )
    ) {
        return;
    }

    sym->initialized = true;
    asg->base.resolved = sym->type;
}

static void analyze_block(sf_semantic *s, sf_block_node *block) {
    scope_push(s->scope);

    for (uint32_t i = 0; i < block->statement_count; i++) {
        analyze_statement(s, block->statements[i]);
    }

    scope_pop(s->scope);
}

static void analyze_if(sf_semantic *s, sf_if_stmt_node *if_stmt) {
    if (!sf_semantic_expr(s, if_stmt->condition, SF_VAL_TYPE_BOOL)) {
        return;
    }

    if (if_stmt->condition->resolved != SF_VAL_TYPE_BOOL) {
        sf_log(
            "type mismatch",
            "if condition should always be a boolean expression, found %s",
            "check for typos and missing operands", s->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, if_stmt->condition->span,
            SF_LOG_SEVERITY_ERROR,
            sf_type_value_name(if_stmt->condition->resolved)
        );

        return;
    }

    analyze_statement(s, if_stmt->branch_then);

    if (if_stmt->branch_else != NULL) {
        analyze_statement(s, if_stmt->branch_else);
    }
}

static void analyze_while(sf_semantic *s, sf_while_stmt_node *while_stmt) {
    if (!sf_semantic_expr(s, while_stmt->condition, SF_VAL_TYPE_BOOL)) {
        return;
    }

    if (while_stmt->condition->resolved != SF_VAL_TYPE_BOOL) {
        sf_log(
            "type mismatch",
            "while condition should always be a boolean expression, found %s",
            "check for typos and missing operands", s->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, while_stmt->condition->span,
            SF_LOG_SEVERITY_ERROR,
            sf_type_value_name(while_stmt->condition->resolved)
        );

        return;
    }

    analyze_statement(s, while_stmt->branch_do);
}

static void analyze_func_decl(sf_semantic *s, sf_func_decl_node *func) {
    sf_func_symbol sym = {
        .name = func->name,
        .return_type = func->return_type,
        .parameter_count = func->parameter_count,
        .span = func->base.span,
        .parameters = func->parameters,
    };

    sf_func_symbol_table_insert(s->funcs, sym, s->filename);

    scope_push(s->scope);

    for (size_t i = 0; i < func->parameter_count; i++) {
        sf_parameter param = func->parameters[i];

        if (param.type == SF_VAL_TYPE_VOID) {
            sf_log(
                "invalid type", "parameters cannot be 'void'",
                "give the parameter '%s' a valid type", s->filename,
                SF_LOG_SEMANTIC_INVALID_TYPE, func->base.span,
                SF_LOG_SEVERITY_ERROR, param.name
            );

            continue;
        }

        sf_var_symbol param_sym = {
            .initialized = true,
            .name = param.name,
            .span = func->base.span,
            .type = param.type,
        };

        scope_insert(s->scope, param_sym, s->filename);

        size_t param_id = scope_lookup(s->scope, param.name)->id;
        func->parameters[i].id = param_id;
    }

    sf_func_decl_node *previous = s->current_func;
    s->current_func = func;

    analyze_statement(s, func->body);

    s->current_func = previous;

    if (func->return_type != SF_VAL_TYPE_VOID && !stmt_always_returns(func->body)) {
        sf_log(
            "missing return",
            "function '%s' does not return a value of type '%s' in all paths",
            "check all paths for missing returns", s->filename,
            SF_LOG_SEMANTIC_MISSING_RETURN, func->base.span,
            SF_LOG_SEVERITY_ERROR, func->name,
            sf_type_value_name(func->return_type)
        );
    }

    scope_pop(s->scope);
}

static void analyze_return(sf_semantic *s, sf_return_stmt_node *ret) {
    sf_func_decl_node *func = s->current_func;

    if (func == NULL) {
        sf_log(
            "return outside function",
            "'return' cannot be used outside of a function",
            "move this return inside a function body, or remove it",
            s->filename, SF_LOG_SEMANTIC_RETURN_OUTSIDE_FUNCTION,
            ret->base.span, SF_LOG_SEVERITY_ERROR
        );

        return;
    }

    if (ret->value == NULL) {
        if (func->return_type != SF_VAL_TYPE_VOID) {
            sf_log(
                "missing return value",
                "function '%s' expects a return value of type '%s', but this "
                "'return' provides none",
                "return a value of the expected type, or change the "
                "function's return type to 'void'",
                s->filename, SF_LOG_SEMANTIC_NO_VALUE_RETURN, ret->base.span,
                SF_LOG_SEVERITY_ERROR, func->name,
                sf_type_value_name(func->return_type)
            );
        }

        return;
    }

    if (func->return_type == SF_VAL_TYPE_VOID) {
        if (!sf_semantic_expr(s, ret->value, SF_VAL_TYPE_UNRESOLVED)) {
            return;
        }

        sf_log(
            "unexpected return value",
            "function '%s' is 'void' and should not return a value, but this "
            "'return' provides a value of type '%s'",
            "remove the returned value, or give the function a non-void "
            "return type",
            s->filename, SF_LOG_SEMANTIC_VOID_RETURN_VALUE, ret->base.span,
            SF_LOG_SEVERITY_ERROR, func->name,
            sf_type_value_name(ret->value->resolved)
        );

        return;
    }

    if (!sf_semantic_expr(s, ret->value, func->return_type)) {
        return;
    }

    check_return_type(
        ret->value->resolved, func->return_type, func->name, ret->base.span,
        s->filename
    );
}

static bool check_assignment_type(
    sf_value_type resolved, sf_value_type target,
    sf_span span, const char *filename
) {
    if (resolved == SF_VAL_TYPE_UNRESOLVED) {
        return true;
    }

    if (!sf_type_value_is_same_group(resolved, target)) {
        sf_log(
            "type mismatch", "cannot assign '%s' to a variable of type '%s'",
            "make sure the expression type matches the variable type, or cast "
            "it",
            filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(resolved),
            sf_type_value_name(target)
        );

        return false;
    }

    if (sf_type_value_width_bits(resolved) > sf_type_value_width_bits(target)) {
        sf_log(
            "narrowing conversion", "cannot implicitly narrow '%s' to '%s'",
            "cast the value explicitly, or use a wider variable type",
            filename, SF_LOG_SEMANTIC_INVALID_IMPLICIT_CAST, span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(resolved),
            sf_type_value_name(target)
        );

        return false;
    }

    return true;
}

static bool check_return_type(
    sf_value_type resolved, sf_value_type target,
    const char *func_name, sf_span span, const char *filename
) {
    if (resolved == SF_VAL_TYPE_UNRESOLVED) {
        return true;
    }

    if (!sf_type_value_is_same_group(resolved, target)) {
        sf_log(
            "type mismatch",
            "function '%s' expects to return '%s', but this expression has "
            "type '%s'",
            "make sure the returned expression matches the function's return "
            "type, or cast it",
            filename, SF_LOG_SEMANTIC_TYPE_MISMATCH, span,
            SF_LOG_SEVERITY_ERROR, func_name, sf_type_value_name(target),
            sf_type_value_name(resolved)
        );

        return false;
    }

    if (sf_type_value_width_bits(resolved) > sf_type_value_width_bits(target)) {
        sf_log(
            "narrowing conversion",
            "cannot implicitly narrow '%s' to '%s' when returning from "
            "function '%s'",
            "cast the value explicitly, or change the function's return type",
            filename, SF_LOG_SEMANTIC_INVALID_IMPLICIT_CAST, span,
            SF_LOG_SEVERITY_ERROR, sf_type_value_name(resolved),
            sf_type_value_name(target), func_name
        );

        return false;
    }

    return true;
}

static bool stmt_always_returns(sf_ast_node *node) {
    if (node == NULL) {
        return false;
    }

    switch (node->type) {
        case SF_NODE_RETURN_STMT: {
            return true;
        }
        case SF_NODE_BLOCK: {
            sf_block_node *block = (sf_block_node *)node;

            for (size_t i = 0; i < block->statement_count; i++) {
                if (stmt_always_returns(block->statements[i])) {
                    return true;
                }
            }

            return false;
        }
        case SF_NODE_IF_STMT: {
            sf_if_stmt_node *if_stmt = (sf_if_stmt_node *)node;

            if (if_stmt->branch_else == NULL) {
                return false;
            }

            return stmt_always_returns(if_stmt->branch_then) &&
                   stmt_always_returns(if_stmt->branch_else);
        }
        default: {
            return false;
        }
    }
}
