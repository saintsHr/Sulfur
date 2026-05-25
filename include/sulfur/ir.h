#pragma once

#include "sulfur/ast.h"
#include <stdint.h>

typedef enum {
	TEMPORARY,
	VARIABLE,
	IMMEDIATE,
} sfOperandType;

typedef enum {
	ADD,
	SUB,
	DIV,
	MULT,
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

void sfGenerateIR(const sfProgramNode* program, char* output);
