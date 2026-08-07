#include "sulfur/pipeline/backend/ir/ir.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/pipeline/backend/ir/optimization.h"
#include "sulfur/pipeline/frontend/ast.h"
#include "sulfur/utils/type_utils.h"

static void push(sf_arena *arena, sf_ir_program *program,
                 sf_operation operation);

static void generate_statement(sf_arena *arena, sf_ir_program *program,
                               sf_ast_node *node, uint32_t depth);
static sf_operand generate_expression(sf_arena *arena, sf_ir_program *program,
                                      sf_ast_node *node, uint32_t depth);
static sf_operand generate_expression_into(sf_arena *arena,
                                           sf_ir_program *program,
                                           sf_ast_node *node, uint32_t depth,
                                           sf_operand *hint);

static void print_operand(sf_operand op);

sf_ir_program sf_generate_ir(sf_arena *arena, const sf_program_node *program) {
  sf_ir_program ir = {
      .capacity = 0,
      .count = 0,
      .nextTemp = 0,
      .operations = NULL,
  };

  for (uint64_t i = 0; i < program->statement_count; i++) {
    generate_statement(arena, &ir, program->statements[i], 0);
  }

  return ir;
}

void sf_print_ir(const sf_ir_program *program) {
  for (size_t i = 0; i < program->count; i++) {
    sf_operation *op = &program->operations[i];

    bool is_control_flow = op->opcode == SF_OPCODE_LABEL ||
                           op->opcode == SF_OPCODE_JMP_COND ||
                           op->opcode == SF_OPCODE_JMP_INCOND;

    if (!is_control_flow) {
      print_operand(op->operand1);
      printf(" = ");
    }

    switch (op->opcode) {
    // arithmetic
    case SF_OPCODE_ADD: {
      print_operand(op->operand2);
      printf(" + ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_SUB: {
      print_operand(op->operand2);
      printf(" - ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_MULT: {
      print_operand(op->operand2);
      printf(" * ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_DIV: {
      print_operand(op->operand2);
      printf(" / ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_NEGATE: {
      printf("-");
      print_operand(op->operand2);
      break;
    }

    // bitwise
    case SF_OPCODE_BITWISE_AND: {
      print_operand(op->operand2);
      printf(" & ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_BITWISE_OR: {
      print_operand(op->operand2);
      printf(" | ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_BITWISE_XOR: {
      print_operand(op->operand2);
      printf(" ^ ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_BITWISE_RSHIFT: {
      print_operand(op->operand2);
      printf(" >> ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_BITWISE_LSHIFT: {
      print_operand(op->operand2);
      printf(" << ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_BITWISE_NOT: {
      printf("~");
      print_operand(op->operand2);
      break;
    }

    // logical
    case SF_OPCODE_LOGICAL_AND: {
      print_operand(op->operand2);
      printf(" && ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_LOGICAL_OR: {
      print_operand(op->operand2);
      printf(" || ");
      print_operand(op->operand3);
      break;
    }
    case SF_OPCODE_LOGICAL_NOT: {
      printf("!");
      print_operand(op->operand2);
      break;
    }

    // relational
    case SF_OPCODE_RELATIONAL_EQUAL:
      print_operand(op->operand2);
      printf(" == ");
      print_operand(op->operand3);
      break;
    case SF_OPCODE_RELATIONAL_NOT_EQUAL:
      print_operand(op->operand2);
      printf(" != ");
      print_operand(op->operand3);
      break;
    case SF_OPCODE_RELATIONAL_LESS:
      print_operand(op->operand2);
      printf(" < ");
      print_operand(op->operand3);
      break;
    case SF_OPCODE_RELATIONAL_LESS_EQUAL:
      print_operand(op->operand2);
      printf(" <= ");
      print_operand(op->operand3);
      break;
    case SF_OPCODE_RELATIONAL_GREATER:
      print_operand(op->operand2);
      printf(" > ");
      print_operand(op->operand3);
      break;
    case SF_OPCODE_RELATIONAL_GREATER_EQUAL:
      print_operand(op->operand2);
      printf(" >= ");
      print_operand(op->operand3);
      break;

    // control flow
    case SF_OPCODE_LABEL: {
      printf("LABEL L%u:", op->operand1.label_id);
      break;
    }
    case SF_OPCODE_JMP_COND: {
      printf("if not ");
      print_operand(op->operand2);
      printf(" jmp L%u", op->operand1.label_id);
      break;
    }
    case SF_OPCODE_JMP_INCOND: {
      printf("jmp L%u", op->operand1.label_id);
      break;
    }

    // other
    case SF_OPCODE_ASSIGN: {
      print_operand(op->operand2);
      break;
    }
    case SF_OPCODE_CAST: {
      printf("cast ");
      print_operand(op->operand2);
      break;
    }

    // fallback
    default:
      print_operand(op->operand2);
      printf(" ? ");
      print_operand(op->operand3);
    }

    printf("\n");
  }
}

static void push(sf_arena *arena, sf_ir_program *program,
                 sf_operation operation) {
  if (program->capacity <= 0) {
    program->operations = sf_arena_grow_array(arena, program->operations, 0, 8,
                                              sizeof(sf_operation));
    program->capacity = 8;
  }

  if (program->count >= program->capacity) {
    size_t new_capacity = program->capacity * 2;
    program->operations =
        sf_arena_grow_array(arena, program->operations, program->capacity,
                            new_capacity, sizeof(sf_operation));
    program->capacity = new_capacity;
  }

  program->operations[program->count++] = operation;
}

static sf_operand new_temporary(sf_ir_program *program, sf_value_type type) {
  sf_operand op;
  op.type = SF_OPERAND_TYPE_TEMPORARY;
  op.value_type = type;
  op.temporary_id = program->nextTemp++;

  return op;
}

static sf_operand new_label(sf_ir_program *program) {
  sf_operand op;
  op.type = SF_OPERAND_TYPE_LABEL;
  op.value_type = SF_VAL_TYPE_UNRESOLVED;
  op.label_id = program->nextLabel++;
  return op;
}

static sf_operand generate_expression(sf_arena *arena, sf_ir_program *program,
                                      sf_ast_node *node, uint32_t depth) {
  sf_operand operand;

  switch (node->type) {
  case SF_NODE_BINARY_EXPR: {
    sf_binary_expr_node *ex = (sf_binary_expr_node *)node;

    sf_operand left = generate_expression(arena, program, ex->left, depth);
    sf_operand right = generate_expression(arena, program, ex->right, depth);

    sf_operand folded;
    if (sf_fold_constants(arena, left, right, type_operation_to_opcode(ex->op),
                          node->resolved, &folded)) {
      operand = folded;
      break;
    }

    sf_operand dst = new_temporary(program, node->resolved);

    operand = dst;

    push(arena, program,
         (sf_operation){.opcode = type_operation_to_opcode(ex->op),
                        .operand1 = dst,
                        .operand2 = left,
                        .operand3 = right});

    break;
  }

  case SF_NODE_UNARY_EXPR: {
    sf_unary_expr_node *un = (sf_unary_expr_node *)node;

    sf_operand src = generate_expression(arena, program, un->operand, depth);

    sf_operand folded;
    if (sf_fold_constants(arena, src, (sf_operand){0},
                          type_operation_to_opcode(un->op), node->resolved,
                          &folded)) {
      operand = folded;
      break;
    }

    sf_operand dst = new_temporary(program, node->resolved);

    operand = dst;

    push(arena, program,
         (sf_operation){.opcode = type_operation_to_opcode(un->op),
                        .operand1 = dst,
                        .operand2 = src,
                        .operand3 = {0}});

    break;
  }

  case SF_NODE_LITERAL: {
    sf_literal_node *lt = (sf_literal_node *)node;

    operand.type = SF_OPERAND_TYPE_IMMEDIATE;
    operand.value_type = lt->base.resolved;

    if (lt->token_type == SF_TOKEN_TYPE_KW_TRUE) {
      operand.immediate_value = "1";
    } else if (lt->token_type == SF_TOKEN_TYPE_KW_FALSE) {
      operand.immediate_value = "0";
    } else {
      operand.immediate_value = lt->value;
    }

    break;
  }

  case SF_NODE_IDENTIFIER: {
    sf_identifier_node *id = (sf_identifier_node *)node;

    operand.type = SF_OPERAND_TYPE_VARIABLE;
    operand.value_type = id->base.resolved;

    size_t mangled_len = strlen(id->name) + 32;
    char *mangled = sf_arena_alloc(arena, mangled_len);
    snprintf(mangled, mangled_len, "%s@%u", id->name, id->id);

    operand.variable_name = mangled;

    break;
  }

  case SF_NODE_CAST_EXPR: {
    sf_cast_expr_node *cast = (sf_cast_expr_node *)node;

    sf_operand src = generate_expression(arena, program, cast->operand, depth);
    sf_operand dst = new_temporary(program, node->resolved);

    operand = dst;

    push(arena, program,
         (sf_operation){.opcode = SF_OPCODE_CAST,
                        .operand1 = dst,
                        .operand2 = src,
                        .operand3 = {0}});

    break;
  }

  default: {
    break;
  }
  }

  return operand;
}

static sf_operand generate_expression_into(sf_arena *arena,
                                           sf_ir_program *program,
                                           sf_ast_node *node, uint32_t depth,
                                           sf_operand *hint) {
  switch (node->type) {
  case SF_NODE_BINARY_EXPR: {
    sf_binary_expr_node *ex = (sf_binary_expr_node *)node;

    sf_operand left = generate_expression(arena, program, ex->left, depth);
    sf_operand right = generate_expression(arena, program, ex->right, depth);

    sf_opcode opcode = type_operation_to_opcode(ex->op);

    sf_operand folded;
    if (sf_fold_constants(arena, left, right, opcode, node->resolved,
                          &folded)) {
      return folded;
    }

    sf_operand dst = hint ? *hint : new_temporary(program, node->resolved);

    push(arena, program,
         (sf_operation){.opcode = opcode,
                        .operand1 = dst,
                        .operand2 = left,
                        .operand3 = right});

    return dst;
  }

  case SF_NODE_UNARY_EXPR: {
    sf_unary_expr_node *un = (sf_unary_expr_node *)node;

    sf_operand src = generate_expression(arena, program, un->operand, depth);
    sf_operand dst = hint ? *hint : new_temporary(program, node->resolved);

    sf_operand folded;
    if (sf_fold_constants(arena, src, (sf_operand){0},
                          type_operation_to_opcode(un->op), node->resolved,
                          &folded)) {
      return folded;
    }

    push(arena, program,
         (sf_operation){.opcode = type_operation_to_opcode(un->op),
                        .operand1 = dst,
                        .operand2 = src,
                        .operand3 = {0}});

    return dst;
  }

  case SF_NODE_CAST_EXPR: {
    sf_cast_expr_node *cast = (sf_cast_expr_node *)node;

    sf_operand src = generate_expression(arena, program, cast->operand, depth);
    sf_operand dst = hint ? *hint : new_temporary(program, node->resolved);

    push(arena, program,
         (sf_operation){.opcode = SF_OPCODE_CAST,
                        .operand1 = dst,
                        .operand2 = src,
                        .operand3 = {0}});

    return dst;
  }

  default:
    return generate_expression(arena, program, node, depth);
  }
}

static void generate_statement(sf_arena *arena, sf_ir_program *program,
                               sf_ast_node *node, uint32_t depth) {
  switch (node->type) {
  case SF_NODE_VAR_DECL: {
    sf_var_decl_node *dcl = (sf_var_decl_node *)node;
    if (dcl->value == NULL)
      break;

    size_t mangled_len = strlen(dcl->name) + 32;
    char *mangled = sf_arena_alloc(arena, mangled_len);
    snprintf(mangled, mangled_len, "%s@%u", dcl->name, dcl->id);

    sf_operand dst = {
        .type = SF_OPERAND_TYPE_VARIABLE,
        .value_type = dcl->var_type,
        .variable_name = mangled,
    };

    sf_operand src =
        generate_expression_into(arena, program, dcl->value, depth, &dst);

    bool wrote_directly = src.type == SF_OPERAND_TYPE_VARIABLE &&
                          strcmp(src.variable_name, dst.variable_name) == 0;

    if (!wrote_directly) {
      sf_operation op = {
          .opcode = SF_OPCODE_ASSIGN,
          .operand1 = dst,
          .operand2 = src,
          .operand3 = {0},
      };

      push(arena, program, op);
    }

    break;
  }

  case SF_NODE_VAR_ASSIGN: {
    sf_var_assign_node *as = (sf_var_assign_node *)node;

    size_t mangled_len = strlen(as->name) + 32;
    char *mangled = sf_arena_alloc(arena, mangled_len);
    snprintf(mangled, mangled_len, "%s@%u", as->name, as->id);

    sf_operand dst = {
        .type = SF_OPERAND_TYPE_VARIABLE,
        .value_type = node->resolved,
        .variable_name = mangled,
    };

    sf_operand src =
        generate_expression_into(arena, program, as->value, depth, &dst);

    bool wrote_directly = src.type == SF_OPERAND_TYPE_VARIABLE &&
                          strcmp(src.variable_name, dst.variable_name) == 0;

    if (!wrote_directly) {
      sf_operation op = {
          .opcode = SF_OPCODE_ASSIGN,
          .operand1 = dst,
          .operand2 = src,
          .operand3 = {0},
      };

      push(arena, program, op);
    }

    break;
  }

  case SF_NODE_BLOCK: {
    sf_block_node *block = (sf_block_node *)node;

    for (size_t i = 0; i < block->statement_count; i++) {
      generate_statement(arena, program, block->statements[i], depth + 1);
    }

    break;
  }

  case SF_NODE_IF_STMT: {
    sf_if_stmt_node *if_stmt = (sf_if_stmt_node *)node;

    sf_operand cond =
        generate_expression(arena, program, if_stmt->condition, depth);

    sf_operand else_id = new_label(program);
    sf_operand end_id = new_label(program);

    sf_operation jmp_else = {
        .opcode = SF_OPCODE_JMP_COND, .operand1 = else_id, .operand2 = cond};

    sf_operation jmp_end = {.opcode = SF_OPCODE_JMP_INCOND, .operand1 = end_id};

    sf_operation else_label = {.opcode = SF_OPCODE_LABEL, .operand1 = else_id};

    sf_operation end_label = {.opcode = SF_OPCODE_LABEL, .operand1 = end_id};

    push(arena, program, jmp_else);
    generate_statement(arena, program, if_stmt->branch_then, depth);
    if (if_stmt->branch_else != NULL) {
      push(arena, program, jmp_end);

      push(arena, program, else_label);
      generate_statement(arena, program, if_stmt->branch_else, depth);
      push(arena, program, end_label);
    } else {
      push(arena, program, else_label);
    }

    break;
  }

  default: {
    break;
  }
  }
}

static void print_operand(sf_operand op) {
  switch (op.type) {
  case SF_OPERAND_TYPE_TEMPORARY:
    printf("t%u:%s", op.temporary_id, type_value_name(op.value_type));
    break;
  case SF_OPERAND_TYPE_VARIABLE:
    printf("%s:%s", op.variable_name, type_value_name(op.value_type));
    break;
  case SF_OPERAND_TYPE_IMMEDIATE:
    printf("%s:%s", op.immediate_value, type_value_name(op.value_type));
    break;
  }
}