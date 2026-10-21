#include "sulfur/pipeline/backend/ir/ir.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sulfur/pipeline/backend/ir/optimization.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/type_utils.h"

static const sf_operand k_no_operand = {0};
static char k_empty_name[] = "";

static void push(sf_arena *arena, sf_ir_context *context, sf_operation operation);
static void emit(
    sf_arena *arena, sf_ir_context *context, sf_opcode opcode,
    sf_operand a, sf_operand b, sf_operand c
);

static sf_ir_context context_from_program(sf_ir_program *program);
static sf_ir_context context_from_function(
    sf_ir_function *function, sf_ir_program *program
);

static sf_operand new_temporary(sf_ir_context *context, sf_value_type type);
static sf_operand new_label(sf_ir_context *context);
static sf_operand new_immediate(sf_value_type type, const char *value);
static char *mangle_name(sf_arena *arena, const char *name, size_t id);

static void add_function(
    sf_arena *arena, sf_ir_program *program, sf_ir_function function
);

static void generate_statement(
    sf_arena *arena, sf_ir_context *context,
    sf_ir_program *program, sf_ast_node *node
);
static void generate_var_decl(
    sf_arena *arena, sf_ir_context *context, sf_var_decl_node *dcl
);
static void generate_var_assign(
    sf_arena *arena, sf_ir_context *context, sf_var_assign_node *as
);
static void generate_assignment(
    sf_arena *arena, sf_ir_context *context, sf_operand dst, sf_ast_node *value
);
static void generate_block(
    sf_arena *arena, sf_ir_context *context,
    sf_ir_program *program, sf_block_node *block
);
static void generate_if(
    sf_arena *arena, sf_ir_context *context,
    sf_ir_program *program, sf_if_stmt_node *if_stmt
);
static void generate_while(
    sf_arena *arena, sf_ir_context *context,
    sf_ir_program *program, sf_while_stmt_node *while_stmt
);
static void generate_function(
    sf_arena *arena, sf_ir_program *program, sf_func_decl_node *func
);
static void generate_return(
    sf_arena *arena, sf_ir_context *context, sf_return_stmt_node *ret
);

static sf_operand generate_expression(
    sf_arena *arena, sf_ir_context *context, sf_ast_node *node
);
static sf_operand generate_expression_into(
    sf_arena *arena, sf_ir_context *context,
    sf_ast_node *node, sf_operand *hint
);
static sf_operand generate_binary(
    sf_arena *arena, sf_ir_context *context,
    sf_binary_expr_node *ex, sf_operand *hint
);
static sf_operand generate_unary(
    sf_arena *arena, sf_ir_context *context,
    sf_unary_expr_node *un, sf_operand *hint
);
static sf_operand generate_increment(
    sf_arena *arena, sf_ir_context *context, sf_unary_expr_node *un
);
static sf_operand generate_cast(
    sf_arena *arena, sf_ir_context *context,
    sf_cast_expr_node *cast, sf_operand *hint
);
static sf_operand generate_literal(const sf_literal_node *lt);
static sf_operand generate_identifier(sf_arena *arena, const sf_identifier_node *id);

static bool is_increment_or_decrement(sf_operation_type op);

static bool ends_with_return(const sf_ir_function *func);

static void print_operand(sf_operand op);
static void print_operation(const sf_operation *op);
static void print_operations(const sf_operation *operations, size_t count);

static const char *binary_symbol(sf_opcode opcode);
static const char *unary_symbol(sf_opcode opcode);

sf_ir_program sf_ir_generate(sf_arena *arena, const sf_program_node *program) {
    sf_ir_program ir = {
        .operation_capacity = 0,
        .operation_count = 0,
        .next_temp = 0,
        .next_label = 0,
        .operations = NULL,
    };

    sf_ir_context ctx = context_from_program(&ir);

    for (size_t i = 0; i < program->statement_count; i++) {
        generate_statement(arena, &ctx, &ir, program->statements[i]);
    }

    return ir;
}

void sf_ir_print(const sf_ir_program *program) {
    print_operations(program->operations, program->operation_count);

    for (size_t f = 0; f < program->function_count; f++) {
        const sf_ir_function *func = &program->functions[f];

        printf("\nfunction %s(", func->name);

        for (size_t p = 0; p < func->parameter_count; p++) {
            printf(
                "%s: %s", func->parameters[p].name,
                sf_type_value_name(func->parameters[p].type)
            );

            if (p + 1 < func->parameter_count) {
                printf(", ");
            }
        }

        printf(") -> %s:\n", sf_type_value_name(func->return_type));

        print_operations(func->operations, func->operation_count);
    }
}

static void push(sf_arena *arena, sf_ir_context *context, sf_operation operation) {
    if (*context->operation_count >= *context->operation_capacity) {
        size_t new_capacity = *context->operation_capacity == 0 ? 8 : *context->operation_capacity * 2;

        sf_operation *grown = sf_arena_grow_array(
            arena, *context->operations, *context->operation_count,
            new_capacity, sizeof(sf_operation)
        );

        if (grown == NULL) {
            sf_log(
                "Insufficient Memory.", "Cannot allocate memory for compiling.",
                "Free some memory and try again.", NULL,
                SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
            );

            return;
        }

        *context->operations = grown;
        *context->operation_capacity = new_capacity;
    }

    (*context->operations)[(*context->operation_count)++] = operation;
}

static void emit(
    sf_arena *arena, sf_ir_context *context, sf_opcode opcode,
    sf_operand a, sf_operand b, sf_operand c
) {
    push(
        arena, context,
        (sf_operation){
            .opcode = opcode,
            .operand1 = a,
            .operand2 = b,
            .operand3 = c,
        }
    );
}

static sf_ir_context context_from_program(sf_ir_program *program) {
    return (sf_ir_context){
        .operations = &program->operations,
        .operation_count = &program->operation_count,
        .operation_capacity = &program->operation_capacity,
        .next_temp = &program->next_temp,
        .next_label = &program->next_label,
    };
}

static sf_ir_context context_from_function(
    sf_ir_function *function, sf_ir_program *program
) {
    return (sf_ir_context){
        .operations = &function->operations,
        .operation_count = &function->operation_count,
        .operation_capacity = &function->operation_capacity,
        .next_temp = &function->next_temp,
        .next_label = &program->next_label,
    };
}

static sf_operand new_temporary(sf_ir_context *context, sf_value_type type) {
    sf_operand op = {0};
    op.type = SF_OPERAND_TYPE_TEMPORARY;
    op.value_type = type;
    op.temporary_id = (*context->next_temp)++;

    return op;
}

static sf_operand new_label(sf_ir_context *context) {
    sf_operand op = {0};
    op.type = SF_OPERAND_TYPE_LABEL;
    op.value_type = SF_VAL_TYPE_UNRESOLVED;
    op.label_id = (*context->next_label)++;

    return op;
}

static sf_operand new_immediate(sf_value_type type, const char *value) {
    sf_operand op = {0};
    op.type = SF_OPERAND_TYPE_IMMEDIATE;
    op.value_type = type;
    op.immediate_value = value;

    return op;
}

static char *mangle_name(sf_arena *arena, const char *name, size_t id) {
    size_t length = strlen(name) + 32;
    char *mangled = sf_arena_alloc(arena, length);

    if (mangled == NULL) {
        sf_log(
            "Insufficient Memory.", "Cannot allocate memory for compiling.",
            "Free some memory and try again.", NULL,
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );

        return k_empty_name;
    }

    snprintf(mangled, length, "%s@%zu", name, id);

    return mangled;
}

static void add_function(
    sf_arena *arena, sf_ir_program *program, sf_ir_function function
) {
    if (program->function_count >= program->function_capacity) {
        size_t new_capacity = program->function_capacity == 0 ? 8 : program->function_capacity * 2;

        sf_ir_function *grown = sf_arena_grow_array(
            arena, program->functions, program->function_count,
            new_capacity, sizeof(sf_ir_function)
        );

        if (grown == NULL) {
            sf_log(
                "Insufficient Memory.", "Cannot allocate memory for compiling.",
                "Free some memory and try again.", NULL,
                SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
            );

            return;
        }

        program->functions = grown;
        program->function_capacity = new_capacity;
    }

    program->functions[program->function_count++] = function;
}

static void generate_statement(
    sf_arena *arena, sf_ir_context *context,
    sf_ir_program *program, sf_ast_node *node
) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case SF_NODE_VAR_DECL: {
            generate_var_decl(arena, context, (sf_var_decl_node *)node);
            break;
        }
        case SF_NODE_VAR_ASSIGN: {
            generate_var_assign(arena, context, (sf_var_assign_node *)node);
            break;
        }
        case SF_NODE_BLOCK: {
            generate_block(arena, context, program, (sf_block_node *)node);
            break;
        }
        case SF_NODE_IF_STMT: {
            generate_if(arena, context, program, (sf_if_stmt_node *)node);
            break;
        }
        case SF_NODE_WHILE_STMT: {
            generate_while(arena, context, program, (sf_while_stmt_node *)node);
            break;
        }
        case SF_NODE_FUNC_DECL: {
            generate_function(arena, program, (sf_func_decl_node *)node);
            break;
        }
        case SF_NODE_RETURN_STMT: {
            generate_return(arena, context, (sf_return_stmt_node *)node);
            break;
        }

        default: {
            (void)generate_expression(arena, context, node);
            break;
        }
    }
}

static void generate_var_decl(
    sf_arena *arena, sf_ir_context *context, sf_var_decl_node *dcl
) {
    if (dcl->value == NULL) {
        return;
    }

    sf_operand dst = {
        .type = SF_OPERAND_TYPE_VARIABLE,
        .value_type = dcl->var_type,
        .variable_name = mangle_name(arena, dcl->name, dcl->id),
    };

    generate_assignment(arena, context, dst, dcl->value);
}

static void generate_var_assign(
    sf_arena *arena, sf_ir_context *context, sf_var_assign_node *as
) {
    sf_operand dst = {
        .type = SF_OPERAND_TYPE_VARIABLE,
        .value_type = as->base.resolved,
        .variable_name = mangle_name(arena, as->name, as->id),
    };

    generate_assignment(arena, context, dst, as->value);
}

static void generate_assignment(
    sf_arena *arena, sf_ir_context *context, sf_operand dst, sf_ast_node *value
) {
    sf_operand src = generate_expression_into(arena, context, value, &dst);

    bool wrote_directly = src.type == SF_OPERAND_TYPE_VARIABLE &&
                          strcmp(src.variable_name, dst.variable_name) == 0;

    if (!wrote_directly) {
        emit(arena, context, SF_OPCODE_ASSIGN, dst, src, k_no_operand);
    }
}

static void generate_block(
    sf_arena *arena, sf_ir_context *context,
    sf_ir_program *program, sf_block_node *block
) {
    for (size_t i = 0; i < block->statement_count; i++) {
        generate_statement(arena, context, program, block->statements[i]);
    }
}

static void generate_if(
    sf_arena *arena, sf_ir_context *context,
    sf_ir_program *program, sf_if_stmt_node *if_stmt
) {
    sf_operand cond = generate_expression(arena, context, if_stmt->condition);

    sf_operand else_id = new_label(context);
    sf_operand end_id = k_no_operand;

    bool has_else = if_stmt->branch_else != NULL;

    if (has_else) {
        end_id = new_label(context);
    }

    emit(arena, context, SF_OPCODE_JMP_COND, else_id, cond, k_no_operand);
    generate_statement(arena, context, program, if_stmt->branch_then);

    if (!has_else) {
        emit(arena, context, SF_OPCODE_LABEL, else_id, k_no_operand, k_no_operand);
        return;
    }

    emit(arena, context, SF_OPCODE_JMP_INCOND, end_id, k_no_operand, k_no_operand);
    emit(arena, context, SF_OPCODE_LABEL, else_id, k_no_operand, k_no_operand);
    generate_statement(arena, context, program, if_stmt->branch_else);
    emit(arena, context, SF_OPCODE_LABEL, end_id, k_no_operand, k_no_operand);
}

static void generate_while(
    sf_arena *arena, sf_ir_context *context,
    sf_ir_program *program, sf_while_stmt_node *while_stmt
) {
    sf_operand start_id = new_label(context);
    sf_operand end_id = new_label(context);

    emit(arena, context, SF_OPCODE_LABEL, start_id, k_no_operand, k_no_operand);

    sf_operand cond = generate_expression(arena, context, while_stmt->condition);

    emit(arena, context, SF_OPCODE_JMP_COND, end_id, cond, k_no_operand);
    generate_statement(arena, context, program, while_stmt->branch_do);
    emit(arena, context, SF_OPCODE_JMP_INCOND, start_id, k_no_operand, k_no_operand);
    emit(arena, context, SF_OPCODE_LABEL, end_id, k_no_operand, k_no_operand);
}

static void generate_function(
    sf_arena *arena, sf_ir_program *program, sf_func_decl_node *func
) {
    sf_ir_function func_ir = {
        .name = func->name,
        .parameters = func->parameters,
        .parameter_count = func->parameter_count,
        .return_type = func->return_type,
        .operations = NULL,
        .next_temp = 0,
        .operation_capacity = 0,
        .operation_count = 0,
    };

    sf_ir_context ctx = context_from_function(&func_ir, program);

    generate_statement(arena, &ctx, program, func->body);

    if (
        func->return_type == SF_VAL_TYPE_VOID &&
        !ends_with_return(&func_ir)
    ) {
        emit(arena, &ctx, SF_OPCODE_RETURN, k_no_operand, k_no_operand, k_no_operand);
    }

    add_function(arena, program, func_ir);
}

static void generate_return(
    sf_arena *arena, sf_ir_context *context, sf_return_stmt_node *ret
) {
    sf_operand value = k_no_operand;

    if (ret->value != NULL) {
        value = generate_expression(arena, context, ret->value);
    }

    emit(arena, context, SF_OPCODE_RETURN, value, k_no_operand, k_no_operand);
}

static sf_operand generate_expression(
    sf_arena *arena, sf_ir_context *context, sf_ast_node *node
) {
    return generate_expression_into(arena, context, node, NULL);
}

static sf_operand generate_expression_into(
    sf_arena *arena, sf_ir_context *context,
    sf_ast_node *node, sf_operand *hint
) {
    if (node == NULL) {
        return k_no_operand;
    }

    switch (node->type) {
        case SF_NODE_BINARY_EXPR: {
            return generate_binary(arena, context, (sf_binary_expr_node *)node, hint);
        }
        case SF_NODE_UNARY_EXPR: {
            return generate_unary(arena, context, (sf_unary_expr_node *)node, hint);
        }
        case SF_NODE_CAST_EXPR: {
            return generate_cast(arena, context, (sf_cast_expr_node *)node, hint);
        }
        case SF_NODE_LITERAL: {
            return generate_literal((const sf_literal_node *)node);
        }
        case SF_NODE_IDENTIFIER: {
            return generate_identifier(arena, (const sf_identifier_node *)node);
        }

        default: {
            return k_no_operand;
        }
    }
}

static sf_operand generate_binary(
    sf_arena *arena, sf_ir_context *context,
    sf_binary_expr_node *ex, sf_operand *hint
) {
    sf_operand left = generate_expression(arena, context, ex->left);
    sf_operand right = generate_expression(arena, context, ex->right);

    sf_opcode opcode = sf_type_operation_to_opcode(ex->op);
    sf_operand folded;

    if (sf_fold_constants(arena, left, right, opcode, ex->base.resolved, &folded)) {
        return folded;
    }

    sf_operand dst = hint != NULL ? *hint : new_temporary(context, ex->base.resolved);

    emit(arena, context, opcode, dst, left, right);

    return dst;
}

static sf_operand generate_unary(
    sf_arena *arena, sf_ir_context *context,
    sf_unary_expr_node *un, sf_operand *hint
) {
    if (is_increment_or_decrement(un->op)) {
        return generate_increment(arena, context, un);
    }

    sf_operand src = generate_expression(arena, context, un->operand);

    sf_opcode opcode = sf_type_operation_to_opcode(un->op);
    sf_operand folded;

    if (sf_fold_constants(arena, src, k_no_operand, opcode, un->base.resolved, &folded)) {
        return folded;
    }

    sf_operand dst = hint != NULL ? *hint : new_temporary(context, un->base.resolved);

    emit(arena, context, opcode, dst, src, k_no_operand);

    return dst;
}

static sf_operand generate_increment(
    sf_arena *arena, sf_ir_context *context, sf_unary_expr_node *un
) {
    sf_operand variable = generate_expression(arena, context, un->operand);
    sf_operand one = new_immediate(un->base.resolved, "1");
    sf_operand updated = new_temporary(context, un->base.resolved);

    bool is_increment = un->op == SF_OP_TYPE_PREFIX_INCREMENT ||
                        un->op == SF_OP_TYPE_POSTFIX_INCREMENT;

    bool is_postfix = un->op == SF_OP_TYPE_POSTFIX_INCREMENT ||
                      un->op == SF_OP_TYPE_POSTFIX_DECREMENT;

    sf_opcode opcode = is_increment ? SF_OPCODE_ADD : SF_OPCODE_SUB;
    sf_operand result = updated;

    if (is_postfix) {
        result = new_temporary(context, un->base.resolved);
        emit(arena, context, SF_OPCODE_ASSIGN, result, variable, k_no_operand);
    }

    emit(arena, context, opcode, updated, variable, one);
    emit(arena, context, SF_OPCODE_ASSIGN, variable, updated, k_no_operand);

    return result;
}

static sf_operand generate_cast(
    sf_arena *arena, sf_ir_context *context,
    sf_cast_expr_node *cast, sf_operand *hint
) {
    sf_operand src = generate_expression(arena, context, cast->operand);
    sf_operand dst = hint != NULL ? *hint : new_temporary(context, cast->base.resolved);

    emit(arena, context, SF_OPCODE_CAST, dst, src, k_no_operand);

    return dst;
}

static sf_operand generate_literal(const sf_literal_node *lt) {
    const char *value = lt->value;

    if (lt->token_type == SF_TOKEN_TYPE_KW_TRUE) {
        value = "1";
    } else if (lt->token_type == SF_TOKEN_TYPE_KW_FALSE) {
        value = "0";
    }

    return new_immediate(lt->base.resolved, value);
}

static sf_operand generate_identifier(sf_arena *arena, const sf_identifier_node *id) {
    sf_operand op = {0};
    op.type = SF_OPERAND_TYPE_VARIABLE;
    op.value_type = id->base.resolved;
    op.variable_name = mangle_name(arena, id->name, id->id);

    return op;
}

static bool is_increment_or_decrement(sf_operation_type op) {
    return
        op == SF_OP_TYPE_PREFIX_INCREMENT ||
        op == SF_OP_TYPE_PREFIX_DECREMENT ||
        op == SF_OP_TYPE_POSTFIX_INCREMENT ||
        op == SF_OP_TYPE_POSTFIX_DECREMENT;
}

static void print_operand(sf_operand op) {
    switch (op.type) {
        case SF_OPERAND_TYPE_NONE: {
            break;
        }
        case SF_OPERAND_TYPE_TEMPORARY: {
            printf("t%zu:%s", op.temporary_id, sf_type_value_name(op.value_type));
            break;
        }
        case SF_OPERAND_TYPE_VARIABLE: {
            printf("%s:%s", op.variable_name, sf_type_value_name(op.value_type));
            break;
        }
        case SF_OPERAND_TYPE_IMMEDIATE: {
            printf("%s:%s", op.immediate_value, sf_type_value_name(op.value_type));
            break;
        }
        case SF_OPERAND_TYPE_LABEL: {
            printf("L%zu", op.label_id);
            break;
        }
    }
}

static void print_operations(const sf_operation *operations, size_t count) {
    for (size_t i = 0; i < count; i++) {
        print_operation(&operations[i]);
    }
}

static void print_operation(const sf_operation *op) {
    bool is_control_flow =
        op->opcode == SF_OPCODE_LABEL || op->opcode == SF_OPCODE_JMP_COND ||
        op->opcode == SF_OPCODE_JMP_INCOND || op->opcode == SF_OPCODE_RETURN;

    if (!is_control_flow) {
        print_operand(op->operand1);
        printf(" = ");
    }

    const char *binary = binary_symbol(op->opcode);
    const char *unary = unary_symbol(op->opcode);

    if (binary != NULL) {
        print_operand(op->operand2);
        printf(" %s ", binary);
        print_operand(op->operand3);
    } else if (unary != NULL) {
        printf("%s", unary);
        print_operand(op->operand2);
    } else {
        switch (op->opcode) {
            case SF_OPCODE_LABEL: {
                printf("label .L%zu:", op->operand1.label_id);
                break;
            }
            case SF_OPCODE_JMP_COND: {
                printf("if not ");
                print_operand(op->operand2);
                printf(" jmp L%zu", op->operand1.label_id);
                break;
            }
            case SF_OPCODE_JMP_INCOND: {
                printf("jmp L%zu", op->operand1.label_id);
                break;
            }
            case SF_OPCODE_ASSIGN: {
                print_operand(op->operand2);
                break;
            }
            case SF_OPCODE_CAST: {
                printf("cast ");
                print_operand(op->operand2);
                break;
            }
            case SF_OPCODE_RETURN: {
                printf("return");

                if (op->operand1.type != SF_OPERAND_TYPE_NONE) {
                    printf(" ");
                    print_operand(op->operand1);
                }

                break;
            }

            default: {
                print_operand(op->operand2);
                printf(" ? ");
                print_operand(op->operand3);
                break;
            }
        }
    }

    printf("\n");
}

static const char *binary_symbol(sf_opcode opcode) {
    switch (opcode) {
        case SF_OPCODE_ADD: {
            return "+";
        }
        case SF_OPCODE_SUB: {
            return "-";
        }
        case SF_OPCODE_MULT: {
            return "*";
        }
        case SF_OPCODE_DIV: {
            return "/";
        }
        case SF_OPCODE_BITWISE_AND: {
            return "&";
        }
        case SF_OPCODE_BITWISE_OR: {
            return "|";
        }
        case SF_OPCODE_BITWISE_XOR: {
            return "^";
        }
        case SF_OPCODE_BITWISE_RSHIFT: {
            return ">>";
        }
        case SF_OPCODE_BITWISE_LSHIFT: {
            return "<<";
        }
        case SF_OPCODE_LOGICAL_AND: {
            return "&&";
        }
        case SF_OPCODE_LOGICAL_OR: {
            return "||";
        }
        case SF_OPCODE_RELATIONAL_EQUAL: {
            return "==";
        }
        case SF_OPCODE_RELATIONAL_NOT_EQUAL: {
            return "!=";
        }
        case SF_OPCODE_RELATIONAL_LESS: {
            return "<";
        }
        case SF_OPCODE_RELATIONAL_LESS_EQUAL: {
            return "<=";
        }
        case SF_OPCODE_RELATIONAL_GREATER: {
            return ">";
        }
        case SF_OPCODE_RELATIONAL_GREATER_EQUAL: {
            return ">=";
        }

        default: {
            return NULL;
        }
    }
}

static const char *unary_symbol(sf_opcode opcode) {
    switch (opcode) {
        case SF_OPCODE_NEGATE: {
            return "-";
        }
        case SF_OPCODE_BITWISE_NOT: {
            return "~";
        }
        case SF_OPCODE_LOGICAL_NOT: {
            return "!";
        }

        default: {
            return NULL;
        }
    }
}

static bool ends_with_return(const sf_ir_function *func) {
    if (
        func->operation_count > 0 &&
        func->operations[func->operation_count - 1].opcode == SF_OPCODE_RETURN
    ) {
        return true;
    }

    return false;
}
