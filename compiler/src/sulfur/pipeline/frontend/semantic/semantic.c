#include "sulfur/pipeline/frontend/semantic/semantic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sulfur/io/log.h"
#include "sulfur/pipeline/frontend/ast/ast.h"
#include "sulfur/pipeline/frontend/semantic/mangle.h"
#include "sulfur/pipeline/frontend/semantic/namespace.h"
#include "sulfur/pipeline/frontend/semantic/scope.h"
#include "sulfur/pipeline/frontend/semantic/semantic_expr.h"
#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/shared/arena.h"
#include "sulfur/shared/operation.h"
#include "sulfur/shared/span.h"
#include "sulfur/shared/string.h"
#include "sulfur/shared/type.h"

static void analyze_statement(sf_semantic *semantic, sf_ast_node *node);
static void analyze_var_decl(sf_semantic *semantic, sf_ast_node_var_decl *var);
static void analyze_var_assign(sf_semantic *semantic, sf_ast_node_var_assign *asg);
static void analyze_block(sf_semantic *semantic, sf_ast_node_block *block);
static void analyze_if(sf_semantic *semantic, sf_ast_node_if *if_stmt);
static void analyze_while(sf_semantic *semantic, sf_ast_node_while *while_stmt);
static void analyze_func_decl(sf_semantic *semantic, sf_ast_node_func_decl *func);
static void analyze_return(sf_semantic *semantic, sf_ast_node_return *ret);
static void analyze_namespace(sf_semantic *semantic, sf_ast_node_namespace *ns);

static bool analyze_var_sym_qual(sf_semantic *semantic, sf_symbol_var sym);
static bool analyze_func_sym_qual(sf_semantic *semantic, sf_symbol_func sym);

static bool analyze_var_sym_attr(sf_semantic *semantic, sf_symbol_var sym);
static bool analyze_func_sym_attr(sf_semantic *semantic, sf_symbol_func sym);

static bool analyze_program(
    sf_arena *arena, sf_module_list *modules, sf_ast_node_program *program,
    const char *filename, sf_symbol_func_table *funcs,
    const sf_compilation_options *options, bool is_root
);
static bool analyze_module(
    sf_arena *arena, sf_module_list *modules, size_t index,
    const sf_compilation_options *options
);
static void import_public_symbols(sf_semantic *semantic, const sf_module *dep);

static bool check_assignment_type(
    sf_type_kind resolved, sf_type_kind target, sf_span span, const char *filename
);
static bool check_return_type(
    sf_type_kind resolved, sf_type_kind target, const char *func_name, sf_span span,
    const char *filename
);
static void check_entry(sf_semantic *semantic);
static void check_entry_qualifiers(sf_semantic *semantic, const sf_symbol_func *entry);
static void check_entry_parameters(sf_semantic *semantic, const sf_symbol_func *entry);
static void check_entry_return(sf_semantic *semantic, const sf_symbol_func *entry);

static bool stmt_always_returns(sf_ast_node *node);
static bool has_no_effect(sf_ast_node *node);
static bool shadows_parameter(sf_semantic *semantic, const char *name);

bool sf_semantic_run(
    sf_arena *arena, sf_ast_node_program *program, const char *filename,
    const sf_compilation_options *options, sf_module_list *modules
) {
    sf_symbol_func_table funcs;
    sf_symbol_func_table_init(&funcs);

    bool ok = analyze_program(
        arena, modules, program, filename, &funcs, options, !options->is_module
    );

    sf_symbol_func_table_free(&funcs);

    for (size_t i = 0; i < modules->module_count; i++) {
        if (modules->modules[i].analysis != SF_MODULE_ANALYSIS_PENDING) {
            sf_symbol_func_table_free(&modules->modules[i].funcs);
        }
    }

    return ok;
}

static bool analyze_program(
    sf_arena *arena, sf_module_list *modules, sf_ast_node_program *program,
    const char *filename, sf_symbol_func_table *funcs,
    const sf_compilation_options *options, bool is_root
) {
    sf_scope scope;
    sf_scope_init(&scope);
    sf_scope_push(&scope);

    sf_semantic semantic = {
        .scope = &scope,
        .funcs = funcs,
        .current_func = NULL,
        .filename = filename,
        .arena = arena,
        .options = options
    };

    bool ok = true;

    for (size_t i = 0; i < program->import_count; i++) {
        sf_ast_node_import *imp = program->imports[i];

        if (!imp->resolved) {
            ok = false;
            continue;
        }

        if (!analyze_module(arena, modules, imp->module_index, options)) {
            ok = false;
            continue;
        }

        import_public_symbols(&semantic, &modules->modules[imp->module_index]);
    }

    for (size_t i = 0; i < program->statement_count; i++) {
        analyze_statement(&semantic, program->statements[i]);
    }

    if (is_root && !options->no_entry) {
        check_entry(&semantic);
    }

    sf_scope_pop(&scope);
    sf_scope_free(&scope);

    return ok;
}

static bool analyze_module(
    sf_arena *arena, sf_module_list *modules, size_t index,
    const sf_compilation_options *options
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
                "Cyclic Import", "Module '%s' is part of an import cycle.",
                "Remove the circular import.", mod->filename,
                SF_LOG_CODE_SEMANTIC_CYCLIC_IMPORT, (sf_span){0}, SF_LOG_SEVERITY_ERROR,
                mod->filename
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
    sf_symbol_func_table_init(&mod->funcs);

    bool ok = analyze_program(
        arena, modules, mod->ast, mod->filename, &mod->funcs, options, false
    );

    mod->analysis = ok ? SF_MODULE_ANALYSIS_DONE : SF_MODULE_ANALYSIS_FAILED;
    return ok;
}

static void import_public_symbols(sf_semantic *semantic, const sf_module *dep) {
    for (size_t i = 0; i < dep->funcs.count; i++) {
        const sf_symbol_func *src = &dep->funcs.functions[i];

        if (!src->qualifiers.is_public || src->imported) {
            continue;
        }

        sf_symbol_func sym = *src;
        sym.imported = true;
        sym.defined = true;

        sf_symbol_func_table_insert(semantic->funcs, sym, semantic->filename);
    }
}

void sf_semantic_report_undeclared(const sf_semantic *s, const char *name, sf_span span) {
    const char *closest = sf_scope_find_closest(s->scope, name);

    if (closest != NULL) {
        sf_log(
            "Undeclared Symbol", "'%s' is not declared in this scope.",
            "Did you mean '%s'?", s->filename, SF_LOG_CODE_SEMANTIC_UNDECLARED, span,
            SF_LOG_SEVERITY_ERROR, name, closest
        );

        return;
    }

    sf_log(
        "Undeclared Symbol", "'%s' is not declared in this scope.",
        "Check for typos, or declare the variable before using it.", s->filename,
        SF_LOG_CODE_SEMANTIC_UNDECLARED, span, SF_LOG_SEVERITY_ERROR, name
    );
}

static void analyze_statement(sf_semantic *semantic, sf_ast_node *node) {
    switch (node->type) {
        case SF_AST_NODE_TYPE_VAR_DECL: {
            analyze_var_decl(semantic, (sf_ast_node_var_decl *)node);
            break;
        }
        case SF_AST_NODE_TYPE_VAR_ASSIGN: {
            analyze_var_assign(semantic, (sf_ast_node_var_assign *)node);
            break;
        }
        case SF_AST_NODE_TYPE_BLOCK: {
            analyze_block(semantic, (sf_ast_node_block *)node);
            break;
        }
        case SF_AST_NODE_TYPE_IF: {
            analyze_if(semantic, (sf_ast_node_if *)node);
            break;
        }
        case SF_AST_NODE_TYPE_WHILE: {
            analyze_while(semantic, (sf_ast_node_while *)node);
            break;
        }
        case SF_AST_NODE_TYPE_FUNC_DECL: {
            analyze_func_decl(semantic, (sf_ast_node_func_decl *)node);
            break;
        }
        case SF_AST_NODE_TYPE_FUNC_CALL: {
            sf_semantic_run_expr(semantic, node, SF_TYPE_KIND_UNRESOLVED);
            break;
        }
        case SF_AST_NODE_TYPE_RETURN: {
            analyze_return(semantic, (sf_ast_node_return *)node);
            break;
        }
        case SF_AST_NODE_TYPE_NAMESPACE: {
            analyze_namespace(semantic, (sf_ast_node_namespace *)node);
            break;
        }
        default: {
            sf_semantic_run_expr(semantic, node, SF_TYPE_KIND_UNRESOLVED);

            if (has_no_effect(node)) {
                sf_log(
                    "Unused Expression", "The result of this expression is discarded.",
                    "Assign the result to a variable, or remove the statement.",
                    semantic->filename, SF_LOG_CODE_SEMANTIC_UNUSED_EXPRESSION,
                    node->span, SF_LOG_SEVERITY_WARNING
                );
            }

            break;
        }
    }
}

static void analyze_var_decl(sf_semantic *semantic, sf_ast_node_var_decl *var) {
    sf_symbol_var sym = {
        .name = var->name,
        .type = var->var_type,
        .initialized = var->value != NULL,
        .span = var->base.span,
        .qualifiers = var->qualifiers,
        .attributes = var->attributes,
    };

    if (sym.type->kind == SF_TYPE_KIND_VOID) {
        sf_log(
            "Invalid Type", "Variables cannot be 'void'.",
            "Give the variable '%s' a valid type.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_TYPE, var->base.span, SF_LOG_SEVERITY_ERROR,
            var->name
        );

        return;
    }

    if (shadows_parameter(semantic, sym.name)) {
        sf_log(
            "Redeclaration", "'%s' shadows a parameter of function '%s'.",
            "Rename the variable, or remove the declaration.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_REDECLARATION, var->base.span, SF_LOG_SEVERITY_ERROR,
            var->name, semantic->current_func->name
        );

        return;
    }

    if (analyze_var_sym_qual(semantic, sym) == false) {
        return;
    }
    if (analyze_var_sym_attr(semantic, sym) == false) {
        return;
    }

    if (var->value != NULL) {
        if (!sf_semantic_run_expr(semantic, var->value, var->var_type->kind)) {
            return;
        }

        if (!check_assignment_type(
                var->value->resolved->kind, var->var_type->kind, var->base.span,
                semantic->filename
            )) {
            return;
        }
    }

    sf_scope_insert(semantic->scope, sym, semantic->filename);

    sf_symbol_var *inserted = sf_scope_lookup(semantic->scope, var->name);

    if (inserted != NULL) {
        var->id = inserted->id;
    }
}

static void analyze_var_assign(sf_semantic *semantic, sf_ast_node_var_assign *asg) {
    const char *name = sf_path_name(&asg->path);

    if (name == NULL) {
        return;
    }

    sf_symbol_var *sym = sf_scope_lookup(semantic->scope, name);

    if (sym == NULL) {
        sf_semantic_report_undeclared(semantic, name, asg->base.span);
        return;
    }

    asg->id = sym->id;

    if (!sf_semantic_run_expr(semantic, asg->value, sym->type->kind)) {
        return;
    }

    if (!check_assignment_type(
            asg->value->resolved->kind, sym->type->kind, asg->base.span,
            semantic->filename
        )) {
        return;
    }

    if (sym->qualifiers.is_mutable == false) {
        sf_log(
            "Invalid Assign", "Variable '%s' is not mutable.",
            "Make the variable mutable or dont assign values to it.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_ASSIGN, asg->base.span, SF_LOG_SEVERITY_ERROR,
            name
        );
    }

    sym->initialized = true;
    asg->base.resolved = sf_type_primitive(sym->type->kind);
}

static void analyze_block(sf_semantic *semantic, sf_ast_node_block *block) {
    sf_scope_push(semantic->scope);

    for (uint32_t i = 0; i < block->statement_count; i++) {
        analyze_statement(semantic, block->statements[i]);
    }

    sf_scope_pop(semantic->scope);
}

static void analyze_if(sf_semantic *semantic, sf_ast_node_if *if_stmt) {
    if (!sf_semantic_run_expr(semantic, if_stmt->condition, SF_TYPE_KIND_BOOL)) {
        return;
    }

    if (if_stmt->condition->resolved->kind != SF_TYPE_KIND_BOOL) {
        sf_log(
            "Type Mismatch",
            "'if' condition should always be a boolean expression, found '%s'.",
            "Check for typos and missing operands.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_TYPE_MISMATCH, if_stmt->condition->span,
            SF_LOG_SEVERITY_ERROR, sf_type_kind_name(if_stmt->condition->resolved->kind)
        );

        return;
    }

    sf_scope_flow before = sf_scope_flow_save(semantic->scope);

    analyze_statement(semantic, if_stmt->branch_then);

    sf_scope_flow after_then = sf_scope_flow_save(semantic->scope);

    if (stmt_always_returns(if_stmt->branch_then)) {
        sf_scope_flow_fill(&after_then, true);
    }

    sf_scope_flow_restore(semantic->scope, &before);

    if (if_stmt->branch_else != NULL) {
        analyze_statement(semantic, if_stmt->branch_else);
    }

    sf_scope_flow after_else = sf_scope_flow_save(semantic->scope);

    if (stmt_always_returns(if_stmt->branch_else)) {
        sf_scope_flow_fill(&after_else, true);
    }

    sf_scope_flow_and(&after_then, &after_else);
    sf_scope_flow_restore(semantic->scope, &after_then);

    sf_scope_flow_free(&before);
    sf_scope_flow_free(&after_then);
    sf_scope_flow_free(&after_else);
}

static void analyze_while(sf_semantic *semantic, sf_ast_node_while *while_stmt) {
    if (!sf_semantic_run_expr(semantic, while_stmt->condition, SF_TYPE_KIND_BOOL)) {
        return;
    }

    if (while_stmt->condition->resolved->kind != SF_TYPE_KIND_BOOL) {
        sf_log(
            "Type Mismatch",
            "While condition should always be a boolean expression, found "
            "'%s'.",
            "Check for typos and missing operands.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_TYPE_MISMATCH, while_stmt->condition->span,
            SF_LOG_SEVERITY_ERROR,
            sf_type_kind_name(while_stmt->condition->resolved->kind)
        );

        return;
    }

    sf_scope_flow before = sf_scope_flow_save(semantic->scope);

    analyze_statement(semantic, while_stmt->branch_do);

    sf_scope_flow_restore(semantic->scope, &before);
    sf_scope_flow_free(&before);
}

static void analyze_func_decl(sf_semantic *semantic, sf_ast_node_func_decl *func) {
    char qname[SF_NS_QNAME_MAX];
    sf_path_segment self = {.span = func->base.span, .name = func->name};

    if (!sf_namespace_format(
            qname, sizeof(qname), &semantic->ns, semantic->ns.depth, &self, 1
        )) {
        sf_log(
            "Name too Long", "The qualified name of function '%s' is too long.",
            "Shorten the function or namespace names.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_NAME_TOO_LONG, func->base.span, SF_LOG_SEVERITY_ERROR,
            func->name
        );

        return;
    }

    char *qualified = sf_string_dup_arena(semantic->arena, qname);

    sf_symbol_func sym = {
        .name = qualified,
        .return_type = func->return_type,
        .parameter_count = func->parameter_count,
        .defined = func->body != NULL,
        .span = func->base.span,
        .parameters = func->parameters,
        .qualifiers = func->qualifiers,
        .attributes = func->attributes,
    };

    bool is_main = strcmp(SF_SEMANTIC_ENTRY_NAME, func->name) == 0;
    bool keep_name = func->qualifiers.is_extern || func->attributes.no_mangle ||
                     (is_main && !semantic->options->no_entry);

    sym.link_name = sf_mangle_function(semantic->arena, qualified, keep_name);
    func->link_name = sym.link_name;

    if (analyze_func_sym_qual(semantic, sym) == false) {
        return;
    }
    if (analyze_func_sym_attr(semantic, sym) == false) {
        return;
    }

    for (size_t i = 0; i < func->parameter_count; i++) {
        sf_parameter param = func->parameters[i];

        if (param.type == sf_type_primitive(SF_TYPE_KIND_VOID)) {
            sf_log(
                "Invalid Type", "Parameters cannot be 'void'.",
                "Give the parameter '%s' a valid type.", semantic->filename,
                SF_LOG_CODE_SEMANTIC_INVALID_TYPE, func->base.span, SF_LOG_SEVERITY_ERROR,
                param.name
            );
        }

        for (size_t j = 0; j < i; j++) {
            if (strcmp(func->parameters[j].name, param.name) == 0) {
                sf_log(
                    "Redeclaration", "Parameter '%s' is already declared.",
                    "Rename one of the parameters.", semantic->filename,
                    SF_LOG_CODE_SEMANTIC_REDECLARATION, func->base.span,
                    SF_LOG_SEVERITY_ERROR, param.name
                );
                break;
            }
        }
    }

    bool insert_result =
        sf_symbol_func_table_insert(semantic->funcs, sym, semantic->filename);
    if (insert_result == false) {
        return;
    }

    if (func->body == NULL) {
        return;
    }

    sf_scope_push(semantic->scope);

    for (size_t i = 0; i < func->parameter_count; i++) {
        sf_parameter param = func->parameters[i];

        if (param.type == sf_type_primitive(SF_TYPE_KIND_VOID)) {
            continue;
        }

        sf_symbol_var param_sym = {
            .initialized = true,
            .name = param.name,
            .span = func->base.span,
            .type = param.type,
            .qualifiers = param.qualifiers
        };

        sf_scope_insert(semantic->scope, param_sym, semantic->filename);

        size_t param_id = sf_scope_lookup(semantic->scope, param.name)->id;
        func->parameters[i].id = param_id;
    }

    sf_ast_node_func_decl *previous = semantic->current_func;
    semantic->current_func = func;

    analyze_statement(semantic, func->body);

    semantic->current_func = previous;

    if (func->return_type != sf_type_primitive(SF_TYPE_KIND_VOID) &&
        !stmt_always_returns(func->body)) {
        sf_log(
            "Missing Return",
            "Function '%s' does not return a value of type '%s' in all paths.",
            "Check all paths for missing returns.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_MISSING_RETURN, func->base.span, SF_LOG_SEVERITY_ERROR,
            func->name, sf_type_name(func->return_type)
        );
    }

    sf_scope_pop(semantic->scope);
}

static void analyze_return(sf_semantic *semantic, sf_ast_node_return *ret) {
    sf_ast_node_func_decl *func = semantic->current_func;

    if (func == NULL) {
        sf_log(
            "Return Outside Function", "'return' cannot be used outside of a function",
            "Move this return inside a function body, or remove it.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_RETURN_OUTSIDE_FUNCTION, ret->base.span,
            SF_LOG_SEVERITY_ERROR
        );

        return;
    }

    if (ret->value == NULL) {
        if (func->return_type != sf_type_primitive(SF_TYPE_KIND_VOID)) {
            sf_log(
                "Missing Return Value",
                "Function '%s' expects a return value of type '%s', but this "
                "'return' provides none.",
                "Return a value of the expected type, or change the "
                "function's return type to 'void'.",
                semantic->filename, SF_LOG_CODE_SEMANTIC_NO_VALUE_RETURN, ret->base.span,
                SF_LOG_SEVERITY_ERROR, func->name, sf_type_name(func->return_type)
            );
        }

        return;
    }

    if (func->return_type == sf_type_primitive(SF_TYPE_KIND_VOID)) {
        if (!sf_semantic_run_expr(semantic, ret->value, SF_TYPE_KIND_UNRESOLVED)) {
            return;
        }

        sf_log(
            "Unexpected Return Value",
            "Function '%s' is 'void' and should not return a value, but this "
            "'return' provides a value of type '%s'.",
            "Remove the returned value, or give the function a non-void "
            "return type.",
            semantic->filename, SF_LOG_CODE_SEMANTIC_VOID_RETURN_VALUE, ret->base.span,
            SF_LOG_SEVERITY_ERROR, func->name,
            sf_type_kind_name(ret->value->resolved->kind)
        );

        return;
    }

    if (!sf_semantic_run_expr(semantic, ret->value, func->return_type->kind)) {
        return;
    }

    check_return_type(
        ret->value->resolved->kind, func->return_type->kind, func->name, ret->base.span,
        semantic->filename
    );
}

static bool analyze_var_sym_qual(sf_semantic *semantic, sf_symbol_var sym) {
    if (sym.qualifiers.is_public) {
        sf_log(
            "Invalid Qualifier", "Variables cannot be public.",
            "Remove the invalid qualifier.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_QUALIFIER, sym.span, SF_LOG_SEVERITY_ERROR
        );

        return false;
    }

    if (sym.qualifiers.is_extern) {
        sf_log(
            "Invalid Qualifier", "Variables cannot be extern.",
            "Remove the invalid qualifier.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_QUALIFIER, sym.span, SF_LOG_SEVERITY_ERROR
        );

        return false;
    }

    return true;
}

static bool analyze_func_sym_qual(sf_semantic *semantic, sf_symbol_func sym) {
    if (sym.qualifiers.is_mutable == true) {
        sf_log(
            "Invalid Qualifier", "Function '%s' cannot be mutable.",
            "Remove the invalid qualifier.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_QUALIFIER, sym.span, SF_LOG_SEVERITY_ERROR,
            sym.name
        );

        return false;
    }

    if (sym.qualifiers.is_extern && sym.qualifiers.is_public) {
        sf_log(
            "Invalid Qualifier", "Function '%s' cannot be both public and extern.",
            "Remove one of the qualifiers.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_QUALIFIER, sym.span, SF_LOG_SEVERITY_ERROR,
            sym.name
        );

        return false;
    }

    if (sym.qualifiers.is_extern && semantic->ns.depth > 0) {
        sf_log(
            "Invalid Qualifier",
            "Extern function '%s' cannot be declared inside a namespace.",
            "Declare the extern function at the top level.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_QUALIFIER, sym.span, SF_LOG_SEVERITY_ERROR,
            sym.name
        );

        return false;
    }

    return true;
}

static bool analyze_var_sym_attr(sf_semantic *semantic, sf_symbol_var sym) {
    if (sym.attributes.no_mangle) {
        sf_log(
            "Invalid Attribute", "Variable '%s' cannot have 'no_mangle' attribute.",
            "Remove the invalid attribute.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_ATTRIBUTE, sym.span, SF_LOG_SEVERITY_ERROR,
            sym.name
        );

        return false;
    }

    return true;
}

static bool analyze_func_sym_attr(sf_semantic *semantic, sf_symbol_func sym) {
    (void)semantic;
    (void)sym;

    return true;
}

static bool check_assignment_type(
    sf_type_kind resolved, sf_type_kind target, sf_span span, const char *filename
) {
    if (resolved == SF_TYPE_KIND_UNRESOLVED) {
        return true;
    }

    if (!sf_type_kind_is_same_group(resolved, target)) {
        sf_log(
            "Type Mismatch", "Cannot assign '%s' to a variable of type '%s'.",
            "Make sure the expression type matches the variable type, or cast "
            "it.",
            filename, SF_LOG_CODE_SEMANTIC_TYPE_MISMATCH, span, SF_LOG_SEVERITY_ERROR,
            sf_type_kind_name(resolved), sf_type_kind_name(target)
        );

        return false;
    }

    if (sf_type_kind_width_bits(resolved) > sf_type_kind_width_bits(target)) {
        sf_log(
            "Narrowing Conversion", "Cannot implicitly narrow '%s' to '%s'.",
            "Cast the value explicitly, or use a wider variable type.", filename,
            SF_LOG_CODE_SEMANTIC_INVALID_IMPLICIT_CAST, span, SF_LOG_SEVERITY_ERROR,
            sf_type_kind_name(resolved), sf_type_kind_name(target)
        );

        return false;
    }

    return true;
}

static bool check_return_type(
    sf_type_kind resolved, sf_type_kind target, const char *func_name, sf_span span,
    const char *filename
) {
    if (resolved == SF_TYPE_KIND_UNRESOLVED) {
        return true;
    }

    if (!sf_type_kind_is_same_group(resolved, target)) {
        sf_log(
            "Type Mismatch",
            "Function '%s' expects to return '%s', but this expression has "
            "type '%s'.",
            "Make sure the returned expression matches the function's return "
            "type, or cast it.",
            filename, SF_LOG_CODE_SEMANTIC_TYPE_MISMATCH, span, SF_LOG_SEVERITY_ERROR,
            func_name, sf_type_kind_name(target), sf_type_kind_name(resolved)
        );

        return false;
    }

    if (sf_type_kind_width_bits(resolved) > sf_type_kind_width_bits(target)) {
        sf_log(
            "Narrowing Conversion",
            "Cannot implicitly narrow '%s' to '%s' when returning from "
            "function '%s'.",
            "Cast the value explicitly, or change the function's return type.", filename,
            SF_LOG_CODE_SEMANTIC_INVALID_IMPLICIT_CAST, span, SF_LOG_SEVERITY_ERROR,
            sf_type_kind_name(resolved), sf_type_kind_name(target), func_name
        );

        return false;
    }

    return true;
}

static void check_entry(sf_semantic *semantic) {
    const sf_symbol_func *entry =
        sf_symbol_func_table_lookup(semantic->funcs, SF_SEMANTIC_ENTRY_NAME);

    if (entry == NULL) {
        sf_log(
            "Missing Entry Point", "The program does not define a '%s' function.",
            "Define a valid function as the program entry point.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_NO_ENTRY_POINT, (sf_span){0}, SF_LOG_SEVERITY_ERROR,
            SF_SEMANTIC_ENTRY_NAME
        );

        return;
    }

    check_entry_return(semantic, entry);
    check_entry_parameters(semantic, entry);
    check_entry_qualifiers(semantic, entry);
}

static void check_entry_qualifiers(sf_semantic *semantic, const sf_symbol_func *entry) {
    if (!entry->qualifiers.is_public) {
        sf_log(
            "Invalid Qualifiers", "The entry function '%s' must be public.",
            "Make the '%s' function public.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_RETURN_TYPE, entry->span, SF_LOG_SEVERITY_ERROR,
            entry->name, entry->name
        );
    }

    if (entry->qualifiers.is_extern) {
        sf_log(
            "Invalid Qualifiers", "The entry function '%s' must not be extern.",
            "Remove the extern qualifier.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_RETURN_TYPE, entry->span, SF_LOG_SEVERITY_ERROR,
            entry->name
        );
    }
}

static void check_entry_parameters(sf_semantic *semantic, const sf_symbol_func *entry) {
    if (entry->parameter_count > 0) {
        sf_log(
            "Invalid Parameters", "The '%s' function must not have any parameters.",
            "Remove all parameters from the function.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_PARAMETERS, entry->span, SF_LOG_SEVERITY_ERROR,
            entry->name
        );
    }
}

static void check_entry_return(sf_semantic *semantic, const sf_symbol_func *entry) {
    if (entry->return_type != sf_type_primitive(SF_TYPE_KIND_I32)) {
        sf_log(
            "Invalid Return Type", "The '%s' function must return '%s'.",
            "Change the funtion return type.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_RETURN_TYPE, entry->span, SF_LOG_SEVERITY_ERROR,
            entry->name, sf_type_kind_name(SF_TYPE_KIND_I32)
        );
    }
}

static bool stmt_always_returns(sf_ast_node *node) {
    if (node == NULL) {
        return false;
    }

    switch (node->type) {
        case SF_AST_NODE_TYPE_RETURN: {
            return true;
        }
        case SF_AST_NODE_TYPE_BLOCK: {
            sf_ast_node_block *block = (sf_ast_node_block *)node;

            for (size_t i = 0; i < block->statement_count; i++) {
                if (stmt_always_returns(block->statements[i])) {
                    return true;
                }
            }

            return false;
        }
        case SF_AST_NODE_TYPE_IF: {
            sf_ast_node_if *if_stmt = (sf_ast_node_if *)node;

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
        case SF_AST_NODE_TYPE_LITERAL:
        case SF_AST_NODE_TYPE_IDENTIFIER: {
            return true;
        }
        case SF_AST_NODE_TYPE_BINARY: {
            sf_ast_node_binary *ex = (sf_ast_node_binary *)node;
            return has_no_effect(ex->left) && has_no_effect(ex->right);
        }
        case SF_AST_NODE_TYPE_UNARY: {
            sf_ast_node_unary *un = (sf_ast_node_unary *)node;

            if (sf_operation_is_increment_or_decrement(un->op)) {
                return false;
            }

            return has_no_effect(un->operand);
        }
        case SF_AST_NODE_TYPE_CAST: {
            sf_ast_node_cast *cast = (sf_ast_node_cast *)node;
            return has_no_effect(cast->operand);
        }
        default: {
            return false;
        }
    }
}

static bool shadows_parameter(sf_semantic *semantic, const char *name) {
    sf_ast_node_func_decl *func = semantic->current_func;

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

static void analyze_namespace(sf_semantic *semantic, sf_ast_node_namespace *ns) {
    if (semantic->scope->depth > 1) {
        sf_log(
            "Invalid Namespace", "Namespaces can only be declared at the top level.",
            "Move the namespace.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_NAMESPACE, ns->base.span, SF_LOG_SEVERITY_ERROR
        );

        return;
    }

    if (ns->path.segment_count == 0) {
        return;
    }

    size_t saved_depth = semantic->ns.depth;

    if (!sf_namespace_push_path(&semantic->ns, &ns->path)) {
        sf_log(
            "Namespace too Deep", "Namespace nesting exceeds the maximum depth.",
            "Flatten the namespace hierarchy.", semantic->filename,
            SF_LOG_CODE_SEMANTIC_INVALID_NAMESPACE, ns->base.span, SF_LOG_SEVERITY_ERROR
        );

        return;
    }

    for (size_t i = 0; i < ns->item_count; i++) {
        sf_ast_node *item = ns->items[i];

        switch (item->type) {
            case SF_AST_NODE_TYPE_FUNC_DECL:
            case SF_AST_NODE_TYPE_NAMESPACE: {
                analyze_statement(semantic, item);
                break;
            }

            default: {
                sf_log(
                    "Invalid Namespace",
                    "Only top-level statements can be inside a namespace.",
                    "Move this statement out of the namespace.", semantic->filename,
                    SF_LOG_CODE_SEMANTIC_INVALID_NAMESPACE, item->span,
                    SF_LOG_SEVERITY_ERROR
                );
                break;
            }
        }
    }

    semantic->ns.depth = saved_depth;
}
