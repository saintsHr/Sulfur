#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sulfur/pipeline/frontend/ast.h"

typedef enum {
    SF_OPERAND_TYPE_NONE,

    SF_OPERAND_TYPE_TEMPORARY,
    SF_OPERAND_TYPE_VARIABLE,
    SF_OPERAND_TYPE_IMMEDIATE,
    SF_OPERAND_TYPE_LABEL,
} sf_operand_type;

typedef enum {
    SF_OPCODE_ADD,
    SF_OPCODE_SUB,
    SF_OPCODE_DIV,
    SF_OPCODE_MULT,
    SF_OPCODE_NEGATE,

    SF_OPCODE_BITWISE_AND,
    SF_OPCODE_BITWISE_OR,
    SF_OPCODE_BITWISE_XOR,
    SF_OPCODE_BITWISE_RSHIFT,
    SF_OPCODE_BITWISE_LSHIFT,
    SF_OPCODE_BITWISE_NOT,

    SF_OPCODE_RELATIONAL_EQUAL,
    SF_OPCODE_RELATIONAL_NOT_EQUAL,
    SF_OPCODE_RELATIONAL_LESS,
    SF_OPCODE_RELATIONAL_LESS_EQUAL,
    SF_OPCODE_RELATIONAL_GREATER,
    SF_OPCODE_RELATIONAL_GREATER_EQUAL,

    SF_OPCODE_LOGICAL_AND,
    SF_OPCODE_LOGICAL_OR,
    SF_OPCODE_LOGICAL_NOT,

    SF_OPCODE_LABEL,
    SF_OPCODE_JMP_COND,
    SF_OPCODE_JMP_INCOND,

    SF_OPCODE_ASSIGN,
    SF_OPCODE_CAST,
    SF_OPCODE_RETURN,

    SF_OPCODE_INVALID,
} sf_opcode;

typedef struct {
    sf_operand_type type;
    sf_value_type value_type;

    union {
        uint32_t temporary_id;
        uint32_t label_id;
        char *variable_name;
        const char *immediate_value;
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

    uint32_t next_temp;
} sf_ir_function;

typedef struct {
    sf_operation *operations;
    uint32_t operation_count;
    uint32_t operation_capacity;

    sf_ir_function *functions;
    uint32_t function_count;
    uint32_t function_capacity;

    uint32_t next_temp;
    uint32_t next_label;
} sf_ir_program;

typedef struct {
    sf_operation **operations;
    uint32_t *operation_count;
    uint32_t *operation_capacity;
    uint32_t *next_temp;
    uint32_t *next_label;
} sf_ir_context;

sf_ir_program sf_ir_generate(sf_arena *arena, const sf_program_node *program);
void sf_ir_print(const sf_ir_program *program);
