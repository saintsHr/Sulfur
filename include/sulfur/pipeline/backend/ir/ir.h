#pragma once

#include <stdint.h>

#include "sulfur/pipeline/frontend/ast.h"

typedef enum {
  SF_OPERAND_TYPE_TEMPORARY,
  SF_OPERAND_TYPE_VARIABLE,
  SF_OPERAND_TYPE_IMMEDIATE,
  SF_OPERAND_TYPE_LABEL,
} sf_operand_type;

typedef enum {
  // arithmetic
  SF_OPCODE_ADD,
  SF_OPCODE_SUB,
  SF_OPCODE_DIV,
  SF_OPCODE_MULT,
  SF_OPCODE_NEGATE,

  // bitwise
  SF_OPCODE_BITWISE_AND,
  SF_OPCODE_BITWISE_OR,
  SF_OPCODE_BITWISE_XOR,
  SF_OPCODE_BITWISE_RSHIFT,
  SF_OPCODE_BITWISE_LSHIFT,
  SF_OPCODE_BITWISE_NOT,

  // relational
  SF_OPCODE_RELATIONAL_EQUAL,
  SF_OPCODE_RELATIONAL_NOT_EQUAL,
  SF_OPCODE_RELATIONAL_LESS,
  SF_OPCODE_RELATIONAL_LESS_EQUAL,
  SF_OPCODE_RELATIONAL_GREATER,
  SF_OPCODE_RELATIONAL_GREATER_EQUAL,

  // logical
  SF_OPCODE_LOGICAL_AND,
  SF_OPCODE_LOGICAL_OR,
  SF_OPCODE_LOGICAL_NOT,

  // lebels & jumps
  SF_OPCODE_LABEL,
  SF_OPCODE_JMP_COND,
  SF_OPCODE_JMP_INCOND,

  // other
  SF_OPCODE_ASSIGN,
  SF_OPCODE_CAST,
  SF_OPCODE_RETURN,
} sf_opcode;

typedef struct {
  sf_operand_type type;
  sf_value_type value_type;

  union {
    uint32_t temporary_id;
    uint32_t label_id;
    char *variable_name;
    char *immediate_value;
  };
} sf_operand;

typedef struct {
  sf_opcode opcode;
  sf_operand operand1;
  sf_operand operand2;
  sf_operand operand3;
} sf_operation;

typedef struct {
  sf_operation *operations;
  uint32_t operation_count;
  uint32_t operation_capacity;

  sf_parameter *parameters;
  size_t parameter_count;

  sf_value_type return_type;

  char *name;

  uint32_t nextTemp;
} sf_ir_function;

typedef struct {
  sf_operation *operations;
  uint32_t operation_count;
  uint32_t operation_capacity;

  sf_ir_function *functions;
  uint32_t function_count;
  uint32_t function_capacity;

  uint32_t nextTemp;
  uint32_t nextLabel;
} sf_ir_program;

typedef struct {
  sf_operation **operations;
  uint32_t *operation_count;
  uint32_t *operation_capacity;
  uint32_t *nextTemp;
  uint32_t *nextLabel;
} sf_ir_context;

sf_ir_program sf_generate_ir(sf_arena *arena, const sf_program_node *program);
void sf_print_ir(const sf_ir_program *program);
