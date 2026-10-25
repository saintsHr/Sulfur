#include "sulfur/pipeline/frontend/semantic/semantic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/span.h"
#include "sulfur/utils/type_utils.h"

static void analyze_statement(sf_semantic *semantic, sf_ast_node *node);
static void analyze_var_decl(sf_semantic *semantic, sf_var_decl_node *var);
static void analyze_var_assign(sf_semantic *semantic, sf_var_assign_node *asg);
static void analyze_block(sf_semantic *semantic, sf_block_node *block);
static void analyze_if(sf_semantic *semantic, sf_if_stmt_node *if_stmt);
static void analyze_while(sf_semantic *semantic, sf_while_stmt_node *while_stmt);
static void analyze_func_decl(sf_semantic *semantic, sf_func_decl_node *func);
static void analyze_return(sf_semantic *semantic, sf_return_stmt_node *ret);

static bool check_assignment_type(
    sf_value_type resolved, sf_value_type target,
    sf_span span, const char *filename
);
static bool check_return_type(
    sf_value_type resolved, sf_value_type target,
    const char *func_name, sf_span span, const char *filename
);
static void check_main_function(sf_semantic *semantic);

static bool stmt_always_returns(sf_ast_node *node);

void sf_analyze(sf_program_node *program, const char *filename, const sf_compiler_options *options) {
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

    for (size_t i = 0; i < program->statement_count; i++) {
        analyze_statement(&semantic, program->statements[i]);
    }

    if (!options->no_entry) {
        check_main_function(&semantic);
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

static void analyze_statement(sf_semantic *semantic, sf_ast_node *node) {
    switch (node->type) {
        case SF_NODE_VAR_DECL: {
            analyze_var_decl(semantic, (sf_var_decl_node *)node);
            break;
        }
        case SF_NODE_VAR_ASSIGN: {
            analyze_var_assign(semantic, (sf_var_assign_node *)node);
            break;
        }
        case SF_NODE_BLOCK: {
            analyze_block(semantic, (sf_block_node *)node);
            break;
        }
        case SF_NODE_IF_STMT: {
            analyze_if(semantic, (sf_if_stmt_node *)node);
            break;
        }
        case SF_NODE_WHILE_STMT: {
            analyze_while(semantic, (sf_while_stmt_node *)node);
            break;
        }
        case SF_NODE_FUNC_DECL: {
            analyze_func_decl(semantic, (sf_func_decl_node *)node);
            break;
        }
        case SF_NODE_RETURN_STMT: {
            analyze_return(semantic, (sf_return_stmt_node *)node);
            break;
        }
        default: {
            sf_semantic_expr(semantic, node, SF_VAL_TYPE_UNRESOLVED);
            break;
        }
    }
}

static void analyze_var_decl(sf_semantic *semantic, sf_var_decl_node *var) {
    if (var->var_type == SF_VAL_TYPE_VOID) {
        sf_log(
            "invalid type", "variables cannot be 'void'",
            "give the variable '%s' a valid type", semantic->filename,
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
        if (!sf_semantic_expr(semantic, var->value, var->var_type)) {
            return;
        }

        if (
            !check_assignment_type(
                var->value->resolved, var->var_type, var->base.span, semantic->filename
            )
        ) {
            return;
        }
    }

    scope_insert(semantic->scope, sym, semantic->filename);

    sf_var_symbol *inserted = scope_lookup(semantic->scope, var->name);

    if (inserted != NULL) {
        var->id = inserted->id;
    }
}

static void analyze_var_assign(sf_semantic *semantic, sf_var_assign_node *asg) {
    sf_var_symbol *sym = scope_lookup(semantic->scope, asg->name);

    if (sym == NULL) {
        sf_semantic_report_undeclared(semantic, asg->name, asg->base.span);
        return;
    }

    asg->id = sym->id;

    if (!sf_semantic_expr(semantic, asg->value, sym->type)) {
        return;
    }

    if (
        !check_assignment_type(
            asg->value->resolved, sym->type, asg->base.span, semantic->filename
        )
    ) {
        return;
    }

    sym->initialized = true;
    asg->base.resolved = sym->type;
}

static void analyze_block(sf_semantic *semantic, sf_block_node *block) {
    scope_push(semantic->scope);

    for (uint32_t i = 0; i < block->statement_count; i++) {
        analyze_statement(semantic, block->statements[i]);
    }

    scope_pop(semantic->scope);
}

static void analyze_if(sf_semantic *semantic, sf_if_stmt_node *if_stmt) {
    if (!sf_semantic_expr(semantic, if_stmt->condition, SF_VAL_TYPE_BOOL)) {
        return;
    }

    if (if_stmt->condition->resolved != SF_VAL_TYPE_BOOL) {
        sf_log(
            "type mismatch",
            "if condition should always be a boolean expression, found %s",
            "check for typos and missing operands", semantic->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, if_stmt->condition->span,
            SF_LOG_SEVERITY_ERROR,
            sf_type_value_name(if_stmt->condition->resolved)
        );

        return;
    }

    analyze_statement(semantic, if_stmt->branch_then);

    if (if_stmt->branch_else != NULL) {
        analyze_statement(semantic, if_stmt->branch_else);
    }
}

static void analyze_while(sf_semantic *semantic, sf_while_stmt_node *while_stmt) {
    if (!sf_semantic_expr(semantic, while_stmt->condition, SF_VAL_TYPE_BOOL)) {
        return;
    }

    if (while_stmt->condition->resolved != SF_VAL_TYPE_BOOL) {
        sf_log(
            "type mismatch",
            "while condition should always be a boolean expression, found %s",
            "check for typos and missing operands", semantic->filename,
            SF_LOG_SEMANTIC_TYPE_MISMATCH, while_stmt->condition->span,
            SF_LOG_SEVERITY_ERROR,
            sf_type_value_name(while_stmt->condition->resolved)
        );

        return;
    }

    analyze_statement(semantic, while_stmt->branch_do);
}

static void analyze_func_decl(sf_semantic *semantic, sf_func_decl_node *func) {
    sf_func_symbol sym = {
        .name = func->name,
        .return_type = func->return_type,
        .parameter_count = func->parameter_count,
        .defined = func->body != NULL,
        .span = func->base.span,
        .parameters = func->parameters,
    };

    for (size_t i = 0; i < func->parameter_count; i++) {
        sf_parameter param = func->parameters[i];

        if (param.type == SF_VAL_TYPE_VOID) {
            sf_log(
                "invalid type", "parameters cannot be 'void'",
                "give the parameter '%s' a valid type", semantic->filename,
                SF_LOG_SEMANTIC_INVALID_TYPE, func->base.span,
                SF_LOG_SEVERITY_ERROR, param.name
            );
        }
    }

    bool insert_result = sf_func_symbol_table_insert(semantic->funcs, sym, semantic->filename);
    if (insert_result == false) return;

    if (func->body == NULL) return;

    scope_push(semantic->scope);

    for (size_t i = 0; i < func->parameter_count; i++) {
        sf_parameter param = func->parameters[i];

        if (param.type == SF_VAL_TYPE_VOID) continue;

        sf_var_symbol param_sym = {
            .initialized = true,
            .name = param.name,
            .span = func->base.span,
            .type = param.type,
        };

        scope_insert(semantic->scope, param_sym, semantic->filename);

        size_t param_id = scope_lookup(semantic->scope, param.name)->id;
        func->parameters[i].id = param_id;
    }

    sf_func_decl_node *previous = semantic->current_func;
    semantic->current_func = func;

    analyze_statement(semantic, func->body);

    semantic->current_func = previous;

    if (func->return_type != SF_VAL_TYPE_VOID && !stmt_always_returns(func->body)) {
        sf_log(
            "missing return",
            "function '%s' does not return a value of type '%s' in all paths",
            "check all paths for missing returns", semantic->filename,
            SF_LOG_SEMANTIC_MISSING_RETURN, func->base.span,
            SF_LOG_SEVERITY_ERROR, func->name,
            sf_type_value_name(func->return_type)
        );
    }

    scope_pop(semantic->scope);
}

static void analyze_return(sf_semantic *semantic, sf_return_stmt_node *ret) {
    sf_func_decl_node *func = semantic->current_func;

    if (func == NULL) {
        sf_log(
            "return outside function",
            "'return' cannot be used outside of a function",
            "move this return inside a function body, or remove it",
            semantic->filename, SF_LOG_SEMANTIC_RETURN_OUTSIDE_FUNCTION,
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
                semantic->filename, SF_LOG_SEMANTIC_NO_VALUE_RETURN, ret->base.span,
                SF_LOG_SEVERITY_ERROR, func->name,
                sf_type_value_name(func->return_type)
            );
        }

        return;
    }

    if (func->return_type == SF_VAL_TYPE_VOID) {
        if (!sf_semantic_expr(semantic, ret->value, SF_VAL_TYPE_UNRESOLVED)) {
            return;
        }

        sf_log(
            "unexpected return value",
            "function '%s' is 'void' and should not return a value, but this "
            "'return' provides a value of type '%s'",
            "remove the returned value, or give the function a non-void "
            "return type",
            semantic->filename, SF_LOG_SEMANTIC_VOID_RETURN_VALUE, ret->base.span,
            SF_LOG_SEVERITY_ERROR, func->name,
            sf_type_value_name(ret->value->resolved)
        );

        return;
    }

    if (!sf_semantic_expr(semantic, ret->value, func->return_type)) {
        return;
    }

    check_return_type(
        ret->value->resolved, func->return_type, func->name, ret->base.span,
        semantic->filename
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

static void check_main_function(sf_semantic *semantic) {
    const sf_func_symbol *main_func = NULL;

    for (size_t i = 0; i < semantic->funcs->count; i++) {
        const sf_func_symbol *f = &semantic->funcs->functions[i];

        if (strcmp(f->name, "main") == 0) {
            main_func = f;
        }
    }

    if (main_func == NULL) {
        sf_log(
            "missing entry point function 'main'.",
            "the program does not define a 'main' function.",
            "define a 'main' function as the program entry point.",
            "%s", SF_LOG_SEMANTIC_NO_MAIN_FUNCTION,
            (sf_span){0}, SF_LOG_SEVERITY_ERROR,
            semantic->filename
        );

        return;
    }

    if (main_func->return_type != SF_VAL_TYPE_I32) {
        sf_log(
            "invalid return type for 'main'.",
            "the 'main' function must return 'i32'",
            "change the return type of 'main' to 'i32'.",
            "%s", SF_LOG_SEMANTIC_INVALID_MAIN_RETURN_TYPE,
            main_func->span, SF_LOG_SEVERITY_ERROR, semantic->filename
        );
    }

    if (main_func->parameter_count > 0) {
        sf_log(
            "invalid parameters for 'main'.",
            "the 'main' function must not have any parameters.",
            "remove all parameters from the 'main' function.",
            "%s", SF_LOG_SEMANTIC_INVALID_MAIN_PARAMETERS,
            main_func->span, SF_LOG_SEVERITY_ERROR, semantic->filename
        );
    }
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
