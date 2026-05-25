#pragma once

#include "sulfur/ast.h"
#include <stdint.h>

typedef enum {
	SF_OPERAND_TYPE_TEMPORARY,
	SF_OPERAND_TYPE_VARIABLE,
	SF_OPERAND_TYPE_IMMEDIATE,
} sfOperandType;

typedef enum {
	SF_OPCODE_ADD,
	SF_OPCODE_SUB,
	SF_OPCODE_DIV,
	SF_OPCODE_MULT,
	SF_OPCODE_ASSIGN,
} sfOpcode;

typedef struct {
	sfOperandType type;
	sfValueType valueType;

	union {
		uint8_t temporaryID;
		char* variableName;
		char* immediateValue;
	};
} sfOperand;

typedef struct {
	sfOpcode  opcode;
	sfOperand destiny;
	sfOperand source1;
	sfOperand source2;
} sfOperation;

typedef struct {
    sfOperation* operations;
    uint32_t     count;
    uint32_t     capacity;
    uint32_t     nextTemp;
} sfIRProgram;

sfIRProgram sfGenerateIR(const sfProgramNode* program);
void sfPrintIR(const sfIRProgram* program);
