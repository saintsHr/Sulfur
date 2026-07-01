#include "sulfur/ast.h"
#include "sulfur/ir.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void push(sf_ir_program* program, sf_operation operation);
static sf_operand new_temporary(sf_ir_program* program, sf_value_type type);
static sf_opcode optype_to_opcode(sf_operation_type type);
static void generate_statement(sf_ir_program* program, sf_ast_node* node, uint32_t depth);
static sf_operand generate_expression(sf_ir_program* program, sf_ast_node* node, uint32_t depth);
static const char* value_type_name(sf_value_type type);
static void print_operand(sf_operand op);

sf_ir_program sf_generate_ir(const sf_program_node* program){
    sf_ir_program ir = {
        .capacity = 0,
        .count = 0,
        .nextTemp = 0,
        .operations = NULL,
    };

    for (uint64_t i = 0; i < program->statement_count; i++) {
        generate_statement(&ir, program->statements[i], 0);
    }

    return ir;
}

void sf_print_ir(const sf_ir_program* program) {
    for (size_t i = 0; i < program->count; i++) {
        sf_operation* op = &program->operations[i];

        print_operand(op->destiny);
        printf(" = ");

        switch (op->opcode) {
            case SF_OPCODE_ASSIGN: {
                print_operand(op->source1);
                break;
            }

            case SF_OPCODE_NEGATE: {
                printf("neg "); 
                print_operand(op->source1);

                break;
            }

            case SF_OPCODE_CAST: {
                printf("cast ");
                print_operand(op->source1);
                break;
            }

            case SF_OPCODE_ADD: {
                print_operand(op->source1); printf(" + "); print_operand(op->source2); 
                break;
            }

            case SF_OPCODE_SUB: {
                print_operand(op->source1); printf(" - "); print_operand(op->source2); 
                break;
            }

            case SF_OPCODE_MULT: {
                print_operand(op->source1); printf(" * "); print_operand(op->source2); 
                break;
            }

            case SF_OPCODE_DIV: {
                print_operand(op->source1); printf(" / "); print_operand(op->source2); 
                break;
            }
        }

        printf("\n");
    }
}

void sf_free_ir(sf_ir_program* program) {
    for (uint32_t i = 0; i < program->count; i++) {
        sf_operation* op = &program->operations[i];

        if (op->destiny.type == SF_OPERAND_TYPE_VARIABLE) {
            free(op->destiny.variable_name);
        }

        if (op->source1.type == SF_OPERAND_TYPE_VARIABLE) {
            free(op->source1.variable_name);
        }

        if (
            op->opcode != SF_OPCODE_ASSIGN &&
            op->opcode != SF_OPCODE_NEGATE &&
            op->opcode != SF_OPCODE_CAST &&
            op->source2.type == SF_OPERAND_TYPE_VARIABLE
        ) {
            free(op->source2.variable_name);
        }
    }

    free(program->operations);
    
    program->operations = NULL;
    program->count      = 0;
    program->capacity   = 0;
}

static void push(sf_ir_program* program, sf_operation operation) {
	if (program->capacity <= 0) {
		program->operations = realloc(
			program->operations,
			8 * sizeof(sf_operation)
		);

		program->capacity = 8;
	}

	if (program->count >= program->capacity) {
		program->operations = realloc(
			program->operations,
			program->capacity * 2 * sizeof(sf_operation)
		);

		program->capacity *= 2;
	}

	program->operations[program->count++] = operation;
}

static sf_operand new_temporary(sf_ir_program* program, sf_value_type type) {
    sf_operand op;
    op.type        = SF_OPERAND_TYPE_TEMPORARY;
    op.value_type   = type;
    op.temporary_id = program->nextTemp++;

    return op;
}

static sf_opcode optype_to_opcode(sf_operation_type type) {
	sf_opcode op;

	switch (type) {
	    case SF_OP_TYPE_ADD: op = SF_OPCODE_ADD; break;
	    case SF_OP_TYPE_SUB: op = SF_OPCODE_SUB; break;
	    case SF_OP_TYPE_MUL: op = SF_OPCODE_MULT; break;
	    case SF_OP_TYPE_DIV: op = SF_OPCODE_DIV; break;
	    default: break;
	}

	return op;
}

static sf_operand generate_expression(sf_ir_program* program, sf_ast_node* node, uint32_t depth) {
	sf_operand operand;

	switch (node->type) {
        case SF_NODE_BINARY_EXPR: {
        	sf_binary_expr_node* ex = (sf_binary_expr_node*)node;

        	sf_operand left  = generate_expression(program, ex->left, depth);
        	sf_operand right = generate_expression(program, ex->right, depth);

        	sf_operand dst = new_temporary(program, node->resolved);

        	operand = dst;

        	push(
        		program,
        		(sf_operation){
        			.opcode=optype_to_opcode(ex->op),
        			.destiny=dst,
        			.source1=left,
        			.source2=right
        		}
        	);

        	break;
        }

        case SF_NODE_UNARY_EXPR: {
            sf_unary_expr_node* un = (sf_unary_expr_node*)node;

            sf_operand src = generate_expression(program, un->operand, depth);
            sf_operand dst = new_temporary(program, node->resolved);

            operand = dst;

            push(
                program,
                (sf_operation){
                    .opcode = SF_OPCODE_NEGATE,
                    .destiny = dst,
                    .source1 = src,
                    .source2 = {0}
                }
            );

            break;
        }

        case SF_NODE_LITERAL: {
        	sf_literal_node* lt = (sf_literal_node*)node;

        	operand.type = SF_OPERAND_TYPE_IMMEDIATE;
        	operand.value_type = lt->base.resolved;
        	operand.immediate_value = lt->value;

        	break;
        }

        case SF_NODE_IDENTIFIER: {
        	sf_identifier_node* id = (sf_identifier_node*)node;

        	operand.type = SF_OPERAND_TYPE_VARIABLE;
        	operand.value_type = id->base.resolved;

            char* mangled = malloc(strlen(id->name) + 32);
            sprintf(mangled, "%s@%u", id->name, id->id);

        	operand.variable_name = mangled;

        	break;
        }

        case SF_NODE_CAST_EXPR: {
            sf_cast_expr_node* cast = (sf_cast_expr_node*)node;

            sf_operand src = generate_expression(program, cast->operand, depth);
            sf_operand dst = new_temporary(program, node->resolved);

            operand = dst;

            push(
                program,
                (sf_operation){
                    .opcode  = SF_OPCODE_CAST,
                    .destiny = dst,
                    .source1 = src,
                    .source2 = {0}
                }
            );

            break;
        }

    	default: {
    		break;
    	}
    }

    return operand;
}

static void generate_statement(sf_ir_program* program, sf_ast_node* node, uint32_t depth) {
    switch (node->type) {
        case SF_NODE_VAR_DECL: {
        	sf_var_decl_node* dcl = (sf_var_decl_node*)node;

        	if (dcl->value == NULL) break;

        	sf_operand src = generate_expression(program, dcl->value, depth);

            char* mangled = malloc(strlen(dcl->name) + 32);
            sprintf(mangled, "%s@%u", dcl->name, dcl->id);

        	sf_operand dst = {
        		.type = SF_OPERAND_TYPE_VARIABLE,
        		.value_type = dcl->var_type,
        		.variable_name = mangled,
        	};

        	sf_operation op = {
        		.opcode = SF_OPCODE_ASSIGN,
        		.destiny = dst,
        		.source1 = src,
                .source2 = {0},
        	};

        	push(program, op);
            break;
        }

        case SF_NODE_VAR_ASSIGN: {
        	sf_var_assign_node* as = (sf_var_assign_node*)node;

        	sf_operand src = generate_expression(program, as->value, depth);

            char* mangled = malloc(strlen(as->name) + 32);
            sprintf(mangled, "%s@%u", as->name, as->id);

        	sf_operand dst = {
        		.type = SF_OPERAND_TYPE_VARIABLE,
        		.value_type = node->resolved,
        		.variable_name = mangled,
        	};

        	sf_operation op = {
        		.opcode = SF_OPCODE_ASSIGN,
        		.destiny = dst,
        		.source1 = src,
                .source2 = {0},
        	};

        	push(program, op);

            break;
        }

        case SF_NODE_BLOCK: {
            sf_block_node* block = (sf_block_node*)node;

            for (size_t i = 0; i < block->statement_count; i++) {
                generate_statement(program, block->statements[i], depth + 1);
            }

            break;
        }

        default: {
            break;
        }
    }
}

static const char* value_type_name(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:  return "i8";
        case SF_VAL_TYPE_I16: return "i16";
        case SF_VAL_TYPE_I32: return "i32";
        case SF_VAL_TYPE_I64: return "i64";

        case SF_VAL_TYPE_U8:  return "u8";
        case SF_VAL_TYPE_U16: return "u16";
        case SF_VAL_TYPE_U32: return "u32";
        case SF_VAL_TYPE_U64: return "u64";

        default:              return "unknown";
    }
}

static void print_operand(sf_operand op) {
    switch (op.type) {
        case SF_OPERAND_TYPE_TEMPORARY: printf("t%u:%s", op.temporary_id, value_type_name(op.value_type)); break;
        case SF_OPERAND_TYPE_VARIABLE: printf("%s:%s", op.variable_name, value_type_name(op.value_type)); break;
        case SF_OPERAND_TYPE_IMMEDIATE: printf("%s:%s", op.immediate_value, value_type_name(op.value_type)); break;
    }
}
