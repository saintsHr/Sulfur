#include "sulfur/ast.h"
#include <stdlib.h>
#include <sulfur/ir.h>

static void push(sfIRProgram* program, sfOperation operation) {
	if (program->capacity <= 0) {
		program->operations = realloc(
			program->operations,
			8 * sizeof(sfOperation)
		);

		program->capacity = 8;
	}

	if (program->count >= program->capacity) {
		program->operations = realloc(
			program->operations,
			program->capacity * 2 * sizeof(sfOperation)
		);

		program->capacity *= 2;
	}

	program->operations[program->count++] = operation;
}

static sfOperand new_temp(sfIRProgram* program, sfValueType type) {
    sfOperand op;
    op.type        = TEMPORARY;
    op.valueType   = type;
    op.temporaryID = program->nextTemp++;
    return op;
}

static sfOpcode optype_to_opcode(sfOperationType type) {
	sfOpcode op;

	switch (type) {
	    case SF_OP_TYPE_ADD: op = ADD; break;
	    case SF_OP_TYPE_SUB: op = SUB; break;
	    case SF_OP_TYPE_MUL: op = MULT; break;
	    case SF_OP_TYPE_DIV: op = DIV; break;
	}

	return op;
}

static sfOperand generate_expression(sfIRProgram* program, sfASTNode* node) {
	sfOperand operand;

	switch (node->type) {
        case SF_NODE_BINARY_EXPR: {
        	sfBinaryExprNode* ex = (sfBinaryExprNode*)node;

        	sfOperand left  = generate_expression(program, ex->left);
        	sfOperand right = generate_expression(program, ex->right);

        	sfOperand dst = new_temp(program, node->resolved);

        	operand = dst;

        	push(
        		program,
        		(sfOperation){
        			.opcode=optype_to_opcode(ex->op),
        			.destiny=dst,
        			.source1=left,
        			.source2=right
        		}
        	);

        	break;
        }

        case SF_NODE_LITERAL: {
        	sfLiteralNode* lt = (sfLiteralNode*)node;

        	operand.type = IMMEDIATE;
        	operand.valueType = lt->base.resolved;
        	operand.immediateValue = lt->value;
        	break;
        }

        case SF_NODE_IDENTIFIER: {
        	sfIdentifierNode* id = (sfIdentifierNode*)node;

        	operand.type = VARIABLE;
        	operand.valueType = id->base.resolved;
        	operand.variableName = id->name;
        	break;
        }

    	default: {
    		break;
    	}
    }

    return operand;
}

void sfGenerateIR(const sfProgramNode* program, char* output){
	return;
}
