#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sulfur/pipeline/frontend/ast/ast.h"

typedef enum {
    SF_IR_OPERAND_TYPE_NONE,

    SF_IR_OPERAND_TYPE_TEMPORARY,
    SF_IR_OPERAND_TYPE_VARIABLE,
    SF_IR_OPERAND_TYPE_IMMEDIATE,
    SF_IR_OPERAND_TYPE_LABEL,
    SF_IR_OPERAND_TYPE_FUNCTION,
} sf_ir_operand_type;

typedef enum {
    SF_IR_OPCODE_INVALID,

    SF_IR_OPCODE_ADD,
    SF_IR_OPCODE_SUB,
    SF_IR_OPCODE_DIV,
    SF_IR_OPCODE_MULT,
    SF_IR_OPCODE_NEGATE,

    SF_IR_OPCODE_ASSIGN,

    SF_IR_OPCODE_BITWISE_AND,
    SF_IR_OPCODE_BITWISE_OR,
    SF_IR_OPCODE_BITWISE_XOR,
    SF_IR_OPCODE_BITWISE_RSHIFT,
    SF_IR_OPCODE_BITWISE_LSHIFT,
    SF_IR_OPCODE_BITWISE_NOT,

    SF_IR_OPCODE_RELATIONAL_EQUAL,
    SF_IR_OPCODE_RELATIONAL_NOT_EQUAL,
    SF_IR_OPCODE_RELATIONAL_LESS,
    SF_IR_OPCODE_RELATIONAL_LESS_EQUAL,
    SF_IR_OPCODE_RELATIONAL_GREATER,
    SF_IR_OPCODE_RELATIONAL_GREATER_EQUAL,

    SF_IR_OPCODE_LOGICAL_AND,
    SF_IR_OPCODE_LOGICAL_OR,
    SF_IR_OPCODE_LOGICAL_NOT,

    SF_IR_OPCODE_LABEL,
    SF_IR_OPCODE_JMP_COND,
    SF_IR_OPCODE_JMP_INCOND,

    SF_IR_OPCODE_CAST,

    SF_IR_OPCODE_RETURN,
    SF_IR_OPCODE_PARAM,
    SF_IR_OPCODE_CALL
} sf_ir_opcode;

typedef struct {
    sf_ir_operand_type type;
    sf_value_type value_type;

    union {
        size_t temporary_id;
        size_t label_id;
        const char *variable_name;
        const char *function_name;
        const char *immediate_value;
    };
} sf_ir_operand;

typedef struct {
    sf_ir_opcode opcode;
    sf_ir_operand operand1;
    sf_ir_operand operand2;
    sf_ir_operand operand3;
} sf_ir_operation;

typedef struct {
    char *name;
    sf_qualifiers qualifiers;
} sf_ir_global;

typedef struct {
    sf_ir_operation *operations;
    size_t operation_count;
    size_t operation_capacity;

    sf_parameter *parameters;
    size_t parameter_count;

    sf_value_type return_type;

    sf_qualifiers qualifiers;

    const char *name;

    size_t next_temp;

    bool defined;
} sf_ir_function;

typedef struct {
    sf_ir_function *functions;
    size_t function_count;
    size_t function_capacity;

    size_t next_label;

    const char **externs;
    size_t extern_count;
    size_t extern_capacity;
} sf_ir_program;

typedef struct {
    sf_ir_operation **operations;
    size_t *operation_count;
    size_t *operation_capacity;
    size_t *next_temp;
} sf_ir_context;

sf_ir_program sf_ir_generate(sf_arena *arena, const sf_program_node *program);
void sf_ir_print(const sf_ir_program *program);
