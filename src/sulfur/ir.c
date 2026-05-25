#include "sulfur/ast.h"
#include <stdio.h>
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
    op.type        = SF_OPERAND_TYPE_TEMPORARY;
    op.valueType   = type;
    op.temporaryID = program->nextTemp++;
    return op;
}

static sfOpcode optype_to_opcode(sfOperationType type) {
	sfOpcode op;

	switch (type) {
	    case SF_OP_TYPE_ADD: op = SF_OPCODE_ADD; break;
	    case SF_OP_TYPE_SUB: op = SF_OPCODE_SUB; break;
	    case SF_OP_TYPE_MUL: op = SF_OPCODE_MULT; break;
	    case SF_OP_TYPE_DIV: op = SF_OPCODE_DIV; break;
	    default: break;
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

        	operand.type = SF_OPERAND_TYPE_IMMEDIATE;
        	operand.valueType = lt->base.resolved;
        	operand.immediateValue = lt->value;

        	break;
        }

        case SF_NODE_IDENTIFIER: {
        	sfIdentifierNode* id = (sfIdentifierNode*)node;

        	operand.type = SF_OPERAND_TYPE_VARIABLE;
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

static void generate_statement(sfIRProgram* program, sfASTNode* node) {
	switch (node->type) {
        case SF_NODE_VAR_DECL: {
        	sfVarDeclNode* dcl = (sfVarDeclNode*)node;

        	if (dcl->value == NULL) break;

        	sfOperand src = generate_expression(program, dcl->value);

        	sfOperand dst = {
        		.type = SF_OPERAND_TYPE_VARIABLE,
        		.valueType = dcl->var_type,
        		.variableName = dcl->name,
        	};

        	sfOperation op = {
        		.opcode = SF_OPCODE_ASSIGN,
        		.destiny = dst,
        		.source1 = src,
        	};

        	push(program, op);
            break;
        }

        case SF_NODE_ASSIGN: {
        	sfAssignNode* as = (sfAssignNode*)node;

        	sfOperand src = generate_expression(program, as->value);

        	sfOperand dst = {
        		.type = SF_OPERAND_TYPE_VARIABLE,
        		.valueType = node->resolved,
        		.variableName = as->name,
        	};

        	sfOperation op = {
        		.opcode = SF_OPCODE_ASSIGN,
        		.destiny = dst,
        		.source1 = src,
        	};

        	push(program, op);

            break;
        }
        default: {
            break;
        }
    }
}

static const char* sfValueTypeName(sfValueType type) {
    switch (type) {
        case SF_VAL_TYPE_I8:  return "i8";
        case SF_VAL_TYPE_I16: return "i16";
        case SF_VAL_TYPE_I32: return "i32";
        case SF_VAL_TYPE_I64: return "i64";
        case SF_VAL_TYPE_U8:  return "u8";
        case SF_VAL_TYPE_U16: return "u16";
        case SF_VAL_TYPE_U32: return "u32";
        case SF_VAL_TYPE_U64: return "u64";
        case SF_VAL_TYPE_F32: return "f32";
        case SF_VAL_TYPE_F64: return "f64";
        default:              return "unknown";
    }
}

static void print_operand(sfOperand op) {
    switch (op.type) {
        case SF_OPERAND_TYPE_TEMPORARY: printf("t%u:%s", op.temporaryID,    sfValueTypeName(op.valueType)); break;
        case SF_OPERAND_TYPE_VARIABLE:  printf("%s:%s",  op.variableName,   sfValueTypeName(op.valueType)); break;
        case SF_OPERAND_TYPE_IMMEDIATE: printf("%s:%s",  op.immediateValue, sfValueTypeName(op.valueType)); break;
    }
}

void sfPrintIR(const sfIRProgram* program) {
    for (size_t i = 0; i < program->count; i++) {
        sfOperation* op = &program->operations[i];
        print_operand(op->destiny);
        printf(" = ");
        print_operand(op->source1);
        switch (op->opcode) {
            case SF_OPCODE_ADD:  printf(" + "); print_operand(op->source2); break;
            case SF_OPCODE_SUB:  printf(" - "); print_operand(op->source2); break;
            case SF_OPCODE_MULT: printf(" * "); print_operand(op->source2); break;
            case SF_OPCODE_DIV:  printf(" / "); print_operand(op->source2); break;
            case SF_OPCODE_ASSIGN: break;
        }
        printf("\n");
    }
}

sfIRProgram sfGenerateIR(const sfProgramNode* program){
	sfIRProgram ir = {
		.capacity = 0,
		.count = 0,
		.nextTemp = 0,
		.operations = NULL,
	};

	for (uint64_t i = 0; i < program->statement_count; i++) {
		generate_statement(&ir, program->statements[i]);
	}

	return ir;
}
