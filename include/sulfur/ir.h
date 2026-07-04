#pragma once

#include "sulfur/ast.h"
#include <stdint.h>

typedef enum {
	SF_OPERAND_TYPE_TEMPORARY,
	SF_OPERAND_TYPE_VARIABLE,
	SF_OPERAND_TYPE_IMMEDIATE,
} sf_operand_type;

typedef enum {
	SF_OPCODE_ADD,
	SF_OPCODE_SUB,
	SF_OPCODE_DIV,
	SF_OPCODE_MULT,

	SF_OPCODE_ASSIGN,

	SF_OPCODE_NEGATE,

	SF_OPCODE_CAST,

	SF_OPCODE_BITWISE_AND,
	SF_OPCODE_BITWISE_OR,
	SF_OPCODE_BITWISE_XOR,
	SF_OPCODE_BITWISE_RSHIFT,
	SF_OPCODE_BITWISE_LSHIFT,
} sf_opcode;

typedef struct {
	sf_operand_type type;
	sf_value_type value_type;

	union {
		uint32_t temporary_id;
		char* variable_name;
		char* immediate_value;
	};
} sf_operand;

typedef struct {
	sf_opcode opcode;
	sf_operand destiny;
	sf_operand source1;
	sf_operand source2;
} sf_operation;

typedef struct {
    sf_operation* operations;
    uint32_t count;
    uint32_t capacity;
    uint32_t nextTemp;
} sf_ir_program;

sf_ir_program sf_generate_ir(const sf_program_node* program);
void sf_print_ir(const sf_ir_program* program);
void sf_free_ir(sf_ir_program* program);
