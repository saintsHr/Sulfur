#include "sulfur/pipeline/frontend/semantic/semantic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sulfur/pipeline/frontend/semantic/namespace.h"
#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/utils/arena.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/mangle.h"
#include "sulfur/utils/span.h"
#include "sulfur/utils/string.h"
#include "sulfur/utils/type_utils.h"

static void analyze_statement(sf_semantic *semantic, sf_ast_node *node);
static void analyze_var_decl(sf_semantic *semantic, sf_var_decl_node *var);
static void analyze_var_assign(sf_semantic *semantic, sf_var_assign_node *asg);
static void analyze_block(sf_semantic *semantic, sf_block_node *block);
static void analyze_if(sf_semantic *semantic, sf_if_node *if_stmt);
static void analyze_while(sf_semantic *semantic, sf_while_node *while_stmt);
static void analyze_func_decl(sf_semantic *semantic, sf_func_decl_node *func);
static void analyze_return(sf_semantic *semantic, sf_return_node *ret);
static void analyze_namespace(sf_semantic *semantic, sf_namespace_node *ns);

static bool analyze_var_sym_qual(sf_semantic *semantic, sf_var_symbol sym);
static bool analyze_func_sym_qual(sf_semantic *semantic, sf_func_symbol sym);

static bool analyze_program(
    sf_arena *arena, sf_module_list *modules, sf_program_node *program,
    const char *filename, sf_func_symbol_table *funcs,
    const sf_compiler_options *options, bool is_root,
    const char *module_name
);
static bool analyze_module(
    sf_arena *arena, sf_module_list *modules, size_t index,
    const sf_compiler_options *options
);
static void import_public_symbols(sf_semantic *semantic, const sf_module *dep);

static bool check_assignment_type(
    sf_value_type resolved, sf_value_type target,
    sf_span span, const char *filename
);
static bool check_return_type(
    sf_value_type resolved, sf_value_type target,
    const char *func_name, sf_span span, const char *filename
);
static void check_entry(sf_semantic *semantic);
static void check_entry_qualifiers(sf_semantic *semantic, const sf_func_symbol *entry);
static void check_entry_parameters(sf_semantic *semantic, const sf_func_symbol *entry);
static void check_entry_return(sf_semantic *semantic, const sf_func_symbol *entry);

static bool stmt_always_returns(sf_ast_node *node);
static bool has_no_effect(sf_ast_node *node);
static bool has_qualifiers(sf_func_symbol sym);
static bool shadows_parameter(sf_semantic *semantic, const char *name);

bool sf_analyze(
    sf_arena *arena, sf_program_node *program,
    const char *filename, const sf_compiler_options *options,
    sf_module_list *modules
) {
    sf_func_symbol_table funcs;
    sf_func_symbol_table_init(&funcs);

    const char *module_name = options->is_module
        ? sf_module_name_from_path(
            filename, options->module_paths, options->module_path_count
        )
        : NULL;

    bool ok = analyze_program(
        arena, modules, program, filename, &funcs,
        options, !options->is_module, module_name
    );

    sf_func_symbol_table_free(&funcs);

    for (size_t i = 0; i < modules->module_count; i++) {
        if (modules->modules[i].analysis != SF_MODULE_ANALYSIS_PENDING) {
            sf_func_symbol_table_free(&modules->modules[i].funcs);
        }
    }

    return ok;
}

void sf_semantic_report_qualified_unsupported(sf_semantic *s, sf_span span) {
    sf_log(
        "unsupported", "qualified names are not supported yet",
        "use a simple name for now", s->filename,
        SF_LOG_PARSER_UNEXPECTED_TOKEN, span,
        SF_LOG_SEVERITY_ERROR
    );
}

static bool analyze_program(
    sf_arena *arena, sf_module_list *modules, sf_program_node *program,
    const char *filename, sf_func_symbol_table *funcs,
    const sf_compiler_options *options, bool is_root,
    const char *module_name
) {
    sf_scope scope;
    scope_init(&scope);
    scope_push(&scope);

    sf_semantic semantic = {
        .scope = &scope,
        .funcs = funcs,
        .current_func = NULL,
        .filename = filename,
        .arena = arena,
    };

    bool ok = true;

    for (size_t i = 0; i < program->import_count; i++) {
        sf_import_node *imp = program->imports[i];

        if (!imp->resolved) { ok = false; continue; }

        if (!analyze_module(arena, modules, imp->module_index, options)) {
            ok = false;
            continue;
        }

        import_public_symbols(&semantic, &modules->modules[imp->module_index]);
    }

    for (size_t i = 0; i < program->statement_count; i++) {
        analyze_statement(&semantic, program->statements[i]);
    }

    if (is_root) {
        check_entry(&semantic);
    }

    scope_pop(&scope);
    scope_free(&scope);

    return ok;
}

static bool analyze_module(
    sf_arena *arena, sf_module_list *modules, size_t index,
    const sf_compiler_options *options
) {
    sf_module *mod = &modules->modules[index];

    switch (mod->analysis) {
        case SF_MODULE_ANALYSIS_DONE: {
            return true;
        }
        case SF_MODULE_ANALYSIS_FAILED: {
            return false;
        }
        case SF_MODULE_ANALYSIS_IN_PROGRESS: {
            sf_log(
                "cyclic import", "module '%s' is part of an import cycle",
                "remove the circular import", mod->filename,
                SF_LOG_SEMANTIC_CYCLIC_IMPORT, (sf_span){0},
                SF_LOG_SEVERITY_ERROR, mod->filename
            );
            return false;
        }

        default: {
            break;
        }
    }

    if (mod->state != SF_MODULE_STATE_LOADED || mod->ast == NULL) {
        mod->analysis = SF_MODULE_ANALYSIS_FAILED;
        return false;
    }

    mod->analysis = SF_MODULE_ANALYSIS_IN_PROGRESS;
    sf_func_symbol_table_init(&mod->funcs);

    bool ok = analyze_program(
        arena, modules, mod->ast, mod->filename, &mod->funcs, options, false, mod->name
    );

    mod->analysis = ok ? SF_MODULE_ANALYSIS_DONE : SF_MODULE_ANALYSIS_FAILED;
    return ok;
}

static void import_public_symbols(sf_semantic *semantic, const sf_module *dep) {
    for (size_t i = 0; i < dep->funcs.count; i++) {
        const sf_func_symbol *src = &dep->funcs.functions[i];

        if (!src->qualifiers.is_public || src->imported) continue;

        sf_func_symbol sym = *src;
        sym.imported = true;
        sym.defined = true;

        sf_func_symbol_table_insert(semantic->funcs, sym, semantic->filename);
    }
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
        case SF_NODE_IF: {
            analyze_if(semantic, (sf_if_node *)node);
            break;
        }
        case SF_NODE_WHILE: {
            analyze_while(semantic, (sf_while_node *)node);
            break;
        }
        case SF_NODE_FUNC_DECL: {
            analyze_func_decl(semantic, (sf_func_decl_node *)node);
            break;
        }
        case SF_NODE_FUNC_CALL: {
            sf_semantic_expr(semantic, node, SF_VAL_TYPE_UNRESOLVED);
            break;
        }
        case SF_NODE_RETURN: {
            analyze_return(semantic, (sf_return_node *)node);
            break;
        }
        case SF_NODE_NAMESPACE: {
            analyze_namespace(semantic, (sf_namespace_node *)node);
            break;
        }
        default: {
            sf_semantic_expr(semantic, node, SF_VAL_TYPE_UNRESOLVED);

            if (has_no_effect(node)) {
                sf_log(
                    "unused expression", "the result of this expression is discarded",
                    "assign the result to a variable, or remove the statement", semantic->filename,
                    SF_LOG_SEMANTIC_UNUSED_EXPRESSION, node->span, SF_LOG_SEVERITY_WARNING
                );
            }

            break;
        }
    }
}

static void analyze_var_decl(sf_semantic *semantic, sf_var_decl_node *var) {
    sf_var_symbol sym = {
        .name = var->name,
        .type = var->var_type,
        .initialized = var->value != NULL,
        .span = var->base.span,
        .qualifiers = var->qualifiers
    };

    if (sym.type == SF_VAL_TYPE_VOID) {
        sf_log(
            "invalid type", "variables cannot be 'void'",
            "give the variable '%s' a valid type", semantic->filename,
            SF_LOG_SEMANTIC_INVALID_TYPE, var->base.span, SF_LOG_SEVERITY_ERROR,
            var->name
        );

        return;
    }

    if (shadows_parameter(semantic, sym.name)) {
        sf_log(
            "redeclaration", "'%s' shadows a parameter of function '%s'",
            "rename the variable, or remove the declaration",
            semantic->filename, SF_LOG_SEMANTIC_REDECLARATION,
            var->base.span, SF_LOG_SEVERITY_ERROR,
            var->name, semantic->current_func->name
        );

        return;
    }

    if (analyze_var_sym_qual(semantic, sym) == false) {
        return;
    }

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
    const char *name = sf_path_simple_name(&asg->path);

    if (name == NULL) {
        sf_semantic_report_qualified_unsupported(semantic, asg->base.span);
        return;
    }

    sf_var_symbol *sym = scope_lookup(semantic->scope, name);

    if (sym == NULL) {
        sf_semantic_report_undeclared(semantic, name, asg->base.span);
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

    if (sym->qualifiers.is_mutable == false) {
        sf_log(
            "invalid assign", "variable '%s' is not mutable",
            "make the variable mutable or dont assign values to it",
            semantic->filename, SF_LOG_SEMANTIC_INVALID_ASSIGN,
            asg->base.span, SF_LOG_SEVERITY_ERROR, name
        );
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

static void analyze_if(sf_semantic *semantic, sf_if_node *if_stmt) {
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

    sf_flow_state before = scope_flow_save(semantic->scope);

    analyze_statement(semantic, if_stmt->branch_then);

    sf_flow_state after_then = scope_flow_save(semantic->scope);

    if (stmt_always_returns(if_stmt->branch_then)) {
        scope_flow_fill(&after_then, true);
    }

    scope_flow_restore(semantic->scope, &before);

    if (if_stmt->branch_else != NULL) {
        analyze_statement(semantic, if_stmt->branch_else);
    }

    sf_flow_state after_else = scope_flow_save(semantic->scope);

    if (stmt_always_returns(if_stmt->branch_else)) {
        scope_flow_fill(&after_else, true);
    }

    scope_flow_and(&after_then, &after_else);
    scope_flow_restore(semantic->scope, &after_then);

    scope_flow_free(&before);
    scope_flow_free(&after_then);
    scope_flow_free(&after_else);
}

static void analyze_while(sf_semantic *semantic, sf_while_node *while_stmt) {
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

    sf_flow_state before = scope_flow_save(semantic->scope);

    analyze_statement(semantic, while_stmt->branch_do);

    scope_flow_restore(semantic->scope, &before);
    scope_flow_free(&before);
}

static void analyze_func_decl(sf_semantic *semantic, sf_func_decl_node *func) {
    char qname[SF_QNAME_MAX];
    sf_path_segment self = {
        .span = func->base.span,
        .name = func->name
    };

    if (
        !sf_namespace_format(
            qname, sizeof qname, &semantic->ns,
            semantic->ns.depth, &self, 1
        )
    ) {
        sf_log(
            "name too long", "the qualified name of function '%s' is too long",
            "shorten the function or namespace names", semantic->filename,
            SF_LOG_SEMANTIC_NAME_TOO_LONG, func->base.span,
            SF_LOG_SEVERITY_ERROR, func->name
        );

        return;
    }

    char *qualified = sf_string_dup_arena(semantic->arena, qname);

    sf_func_symbol sym = {
        .name = qualified,
        .return_type = func->return_type,
        .parameter_count = func->parameter_count,
        .defined = func->body != NULL,
        .span = func->base.span,
        .parameters = func->parameters,
        .qualifiers = func->qualifiers
    };

    sym.link_name = sf_mangle_function(
        semantic->arena, qualified, func->qualifiers.is_extern
    );
    func->link_name = sym.link_name;

    if (analyze_func_sym_qual(semantic, sym) == false) {
        return;
    }

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

        for (size_t j = 0; j < i; j++) {
            if (strcmp(func->parameters[j].name, param.name) == 0) {
                sf_log(
                    "redeclaration", "parameter '%s' is already declared",
                    "rename one of the parameters", semantic->filename,
                    SF_LOG_SEMANTIC_REDECLARATION, func->base.span,
                    SF_LOG_SEVERITY_ERROR, param.name
                );
                break;
            }
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
            .qualifiers = param.qualifiers
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

static void analyze_return(sf_semantic *semantic, sf_return_node *ret) {
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

static bool analyze_var_sym_qual(sf_semantic *semantic, sf_var_symbol sym) {
    if (sym.qualifiers.is_public) {
        sf_log(
            "invalid qualifier", "variables cannot be public",
            "remove the invalid qualifier", semantic->filename,
            SF_LOG_SEMANTIC_INVALID_QUALIFIER, sym.span,
            SF_LOG_SEVERITY_ERROR
        );

        return false;
    }

    if (sym.qualifiers.is_extern) {
        sf_log(
            "invalid qualifier", "variables cannot be extern",
            "remove the invalid qualifier", semantic->filename,
            SF_LOG_SEMANTIC_INVALID_QUALIFIER, sym.span,
            SF_LOG_SEVERITY_ERROR
        );

        return false;
    }

    return true;
}

static bool analyze_func_sym_qual(sf_semantic *semantic, sf_func_symbol sym) {
    if (sym.qualifiers.is_mutable == true) {
        sf_log(
            "invalid qualifier", "function '%s' cannot be mutable",
            "remove the invalid qualifier", semantic->filename,
            SF_LOG_SEMANTIC_INVALID_QUALIFIER, sym.span,
            SF_LOG_SEVERITY_ERROR, sym.name
        );

        return false;
    }

    if (sym.qualifiers.is_extern && sym.qualifiers.is_public) {
        sf_log(
            "invalid qualifier", "function '%s' cannot be both public and extern",
            "remove one of the qualifiers", semantic->filename,
            SF_LOG_SEMANTIC_INVALID_QUALIFIER, sym.span,
            SF_LOG_SEVERITY_ERROR, sym.name
        );

        return false;
    }

    if (sym.qualifiers.is_extern && semantic->ns.depth > 0) {
        sf_log(
            "invalid qualifier", "extern function '%s' cannot be declared inside a namespace",
            "declare the extern function at the top level",
            semantic->filename, SF_LOG_SEMANTIC_INVALID_QUALIFIER,
            sym.span, SF_LOG_SEVERITY_ERROR, sym.name
        );

        return false;
    }

    return true;
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

static void check_entry(sf_semantic *semantic) {
    const sf_func_symbol *entry = sf_func_symbol_table_lookup(
        semantic->funcs, SF_SEMANTIC_ENTRY_NAME
    );

    if (entry == NULL) {
        sf_log(
            "missing entry point function",
            "the program does not define a '%s' function",
            "define a valid function as the program entry point",
            semantic->filename, SF_LOG_SEMANTIC_NO_MAIN_FUNCTION,
            (sf_span){0}, SF_LOG_SEVERITY_ERROR, SF_SEMANTIC_ENTRY_NAME
        );

        return;
    }

    check_entry_return(semantic, entry);
    check_entry_parameters(semantic, entry);
    check_entry_qualifiers(semantic, entry);
}

static void check_entry_qualifiers(sf_semantic *semantic, const sf_func_symbol *entry) {
    if (!entry->qualifiers.is_public) {
        sf_log(
            "invalid qualifiers",
            "the entry function '%s' must be public",
            "make the '%s' function public",
            semantic->filename, SF_LOG_SEMANTIC_INVALID_RETURN_TYPE,
            entry->span, SF_LOG_SEVERITY_ERROR, entry->name, entry->name
        );
    }

    if (entry->qualifiers.is_extern) {
        sf_log(
            "invalid qualifiers",
            "the entry function '%s' must not be extern",
            "remove the extern qualifier",
            semantic->filename, SF_LOG_SEMANTIC_INVALID_RETURN_TYPE,
            entry->span, SF_LOG_SEVERITY_ERROR, entry->name
        );
    }
}

static void check_entry_parameters(sf_semantic *semantic, const sf_func_symbol *entry) {
    if (entry->parameter_count > 0) {
        sf_log(
            "invalid parameters",
            "the '%s' function must not have any parameters",
            "remove all parameters from the function",
            semantic->filename, SF_LOG_SEMANTIC_INVALID_PARAMETERS,
            entry->span, SF_LOG_SEVERITY_ERROR, entry->name
        );
    }
}

static void check_entry_return(sf_semantic *semantic, const sf_func_symbol *entry) {
    if (entry->return_type != SF_VAL_TYPE_I32) {
        sf_log(
            "invalid return type",
            "the '%s' function must return '%s'",
            "change the funtion return type",
            semantic->filename, SF_LOG_SEMANTIC_INVALID_RETURN_TYPE,
            entry->span, SF_LOG_SEVERITY_ERROR,
            entry->name, sf_type_value_name(SF_VAL_TYPE_I32)
        );
    }
}

static bool stmt_always_returns(sf_ast_node *node) {
    if (node == NULL) {
        return false;
    }

    switch (node->type) {
        case SF_NODE_RETURN: {
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
        case SF_NODE_IF: {
            sf_if_node *if_stmt = (sf_if_node *)node;

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

static bool has_no_effect(sf_ast_node *node) {
    if (node == NULL) {
        return false;
    }

    switch (node->type) {
        case SF_NODE_LITERAL:
        case SF_NODE_IDENTIFIER: {
            return true;
        }
        case SF_NODE_BINARY: {
            sf_binary_node *ex = (sf_binary_node *)node;
            return has_no_effect(ex->left) && has_no_effect(ex->right);
        }
        case SF_NODE_UNARY: {
            sf_unary_node *un = (sf_unary_node *)node;

            if (sf_type_is_increment_or_decrement(un->op)) {
                return false;
            }

            return has_no_effect(un->operand);
        }
        case SF_NODE_CAST: {
            sf_cast_node *cast = (sf_cast_node *)node;
            return has_no_effect(cast->operand);
        }
        default: {
            return false;
        }
    }
}

static bool has_qualifiers(sf_func_symbol sym) {
    if (
        sym.qualifiers.is_mutable ||
        sym.qualifiers.is_public ||
        sym.qualifiers.is_extern
    ) {
        return true;
    }

    return false;
}

static bool shadows_parameter(sf_semantic *semantic, const char *name) {
    sf_func_decl_node *func = semantic->current_func;

    if (func == NULL) {
        return false;
    }

    for (size_t i = 0; i < func->parameter_count; i++) {
        if (strcmp(func->parameters[i].name, name) == 0) {
            return true;
        }
    }

    return false;
}

static void analyze_namespace(sf_semantic *semantic, sf_namespace_node *ns) {
    if (semantic->scope->depth > 1) {
        sf_log(
            "invalid namespace",
            "namespaces can only be declared at the top level or inside another namespace",
            "move the namespace out of the function or block",
            semantic->filename, SF_LOG_SEMANTIC_INVALID_NAMESPACE,
            ns->base.span, SF_LOG_SEVERITY_ERROR
        );

        return;
    }

    if (ns->path.segment_count == 0) {
        return;
    }

    size_t saved_depth = semantic->ns.depth;

    if (!sf_namespace_push_path(&semantic->ns, &ns->path)) {
        sf_log(
            "namespace too deep", "namespace nesting exceeds the maximum depth",
            "flatten the namespace hierarchy", semantic->filename,
            SF_LOG_SEMANTIC_INVALID_NAMESPACE, ns->base.span,
            SF_LOG_SEVERITY_ERROR
        );

        return;
    }

    for (size_t i = 0; i < ns->item_count; i++) {
        sf_ast_node *item = ns->items[i];

        switch (item->type) {
            case SF_NODE_FUNC_DECL:
            case SF_NODE_NAMESPACE: {
                analyze_statement(semantic, item);
                break;
            }
            case SF_NODE_VAR_DECL: {
                sf_log(
                    "unsupported", "variables inside namespaces are not supported yet",
                    "declare the variable outside the namespace",
                    semantic->filename, SF_LOG_SEMANTIC_INVALID_NAMESPACE,
                    item->span, SF_LOG_SEVERITY_ERROR
                );
                break;
            }
            default: {
                sf_log(
                    "invalid namespace item",
                    "only top-level statements can be inside a namespace",
                    "move this statement out of the namespace",
                    semantic->filename, SF_LOG_SEMANTIC_INVALID_NAMESPACE,
                    item->span, SF_LOG_SEVERITY_ERROR
                );
                break;
            }
        }
    }

    semantic->ns.depth = saved_depth;
}
