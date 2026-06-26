#include "sulfur/codegen.h"
#include "sulfur/ir.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void push_string(const char* src, char** dst) {
	if (dst  == NULL) return;
	if (*dst == NULL) return;
	if (src  == NULL) return;

	*dst = realloc(
		*dst,
		strlen(*dst) + (strlen(src) * sizeof(char)) + 1
	);

	strcat(*dst, src);
}

static int16_t lookup_stack(const sf_stack_map* map, const char* name) {
	if (map == NULL) return 0;

	for (int16_t i = 0; i < map->count; i++) {
		if (strcmp(map->entries[i].name, name) == 0) {
			return map->entries[i].offset;
		}
	}

	return 0;
}

static void push_stack(sf_stack_map* map, sf_stack_entry entry) {
	if (map == NULL) return;

	if (map->capacity <= 0) {
		map->entries = realloc(
			map->entries,
			8 * sizeof(sf_stack_entry)
		);
		map->capacity = 8;
	}

	if (map->count >= map->capacity) {
		map->entries = realloc(
			map->entries,
			(map->capacity * 2) * sizeof(sf_stack_entry)
		);
	}

	map->entries[map->count++] = entry;
}

static void map_operand(sf_stack_map* map, sf_operand op, int16_t* next_offset) {
    if (op.type == SF_OPERAND_TYPE_IMMEDIATE) return;

    if (op.type == SF_OPERAND_TYPE_VARIABLE) {
    	char*   name   = op.variable_name;
    	int16_t offset = lookup_stack(map, name);

    	if (offset == 0) {
    		sf_stack_entry entry = {
    			.name = name,
    			.offset = *next_offset,
    		};

    		push_stack(map, entry);

    		*next_offset -= 8;
    	}
    }

    if (op.type == SF_OPERAND_TYPE_TEMPORARY) {
    	char*   name   = malloc(16 * sizeof(char));
    	sprintf(name, "t%hi", op.temporary_id);

    	int16_t offset = lookup_stack(map, name);

    	if (offset == 0) {
    		sf_stack_entry entry = {
    			.name = name,
    			.offset = *next_offset,
    		};

    		push_stack(map, entry);

    		*next_offset -= 8;
    	}
    }
}

static void populate_stack(sf_stack_map* map, const sf_ir_program* program) {
    if (map     == NULL) return;
    if (program == NULL) return;

    int16_t next_offset = -8;

    for (uint64_t i = 0; i < program->count; i++) {
        sf_operation op = program->operations[i];

        if (op.destiny.type == SF_OPERAND_TYPE_VARIABLE) {
        	map_operand(map, op.destiny, &next_offset);
        }

        if (op.source1.type == SF_OPERAND_TYPE_VARIABLE){
        	map_operand(map, op.source1, &next_offset);
        }

        if (op.source2.type == SF_OPERAND_TYPE_VARIABLE){
        	map_operand(map, op.source2, &next_offset);
        }
    }

    for (uint64_t i = 0; i < program->count; i++) {
        sf_operation op = program->operations[i];

        if (op.destiny.type == SF_OPERAND_TYPE_TEMPORARY) {
        	map_operand(map, op.destiny, &next_offset);
        }

        if (op.source1.type == SF_OPERAND_TYPE_TEMPORARY) {
        	map_operand(map, op.source1, &next_offset);
        }

        if (op.source2.type == SF_OPERAND_TYPE_TEMPORARY) {
        	map_operand(map, op.source2, &next_offset);
        }
    }
}

static void emit_assign(char** buff, sf_operation op, const sf_stack_map* map) {
	if (op.opcode == SF_OPCODE_ASSIGN) {
		char* dst_name  = NULL;
		char* src1_name = NULL;

		switch (op.destiny.type) {
            case SF_OPERAND_TYPE_TEMPORARY: {
            	dst_name = malloc(32 * sizeof(char));
    			sprintf(dst_name, "t%hi", op.destiny.temporary_id);

    			break;
            }

            case SF_OPERAND_TYPE_VARIABLE: {
            	dst_name = malloc(32 * sizeof(char));
    			sprintf(dst_name, "%s", op.destiny.variable_name);

    			break;
            }

            default: break;
        }

        switch (op.source1.type) {
        	case SF_OPERAND_TYPE_TEMPORARY: {
        		src1_name = malloc(32 * sizeof(char));
    			sprintf(src1_name, "t%hi", op.source1.temporary_id);

    			break;
        	}

        	case SF_OPERAND_TYPE_VARIABLE: {
        		src1_name = malloc(32 * sizeof(char));
    			sprintf(src1_name, "%s", op.source1.variable_name);

    			break;
        	}

        	case SF_OPERAND_TYPE_IMMEDIATE: {
        		src1_name = malloc(32 * sizeof(char));
    			sprintf(src1_name, "%s", op.source1.immediate_value);

    			break;
        	}

          	default: break;
        }

        char* instruction = malloc(256 * sizeof(char));

    	if (op.source1.type != SF_OPERAND_TYPE_IMMEDIATE) {
    		sprintf(
    			instruction,
    			"	mov rax, [rbp%hi]\n"
    			"	mov [rbp%hi], rax\n",
    			lookup_stack(map, src1_name),
    			lookup_stack(map, dst_name)
    		);
    	} else {
    		sprintf(
    			instruction,
    			"	mov rax, %s\n"
    			"	mov [rbp%hi], rax\n",
    			src1_name,
    			lookup_stack(map, dst_name)
    		);
    	}

    	push_string(instruction, buff);

    	free(instruction);
	}
}

static void emit_binary(char** buff, sf_operation op, const sf_stack_map* map, const char* instr) {
    char* dst_name  = NULL;
    char* src1_name = NULL;
    char* src2_name = NULL;

    switch (op.destiny.type) {
        case SF_OPERAND_TYPE_TEMPORARY: {
            dst_name = malloc(32 * sizeof(char));
            sprintf(dst_name, "t%hi", op.destiny.temporary_id);

            break;
        }

        case SF_OPERAND_TYPE_VARIABLE: {
            dst_name = malloc(32 * sizeof(char));
            sprintf(dst_name, "%s", op.destiny.variable_name);

            break;
        }

        default: break;
    }

    switch (op.source1.type) {
        case SF_OPERAND_TYPE_TEMPORARY: {
            src1_name = malloc(32 * sizeof(char));
            sprintf(src1_name, "t%hi", op.source1.temporary_id);

            break;
        }

        case SF_OPERAND_TYPE_VARIABLE: {
            src1_name = malloc(32 * sizeof(char));
            sprintf(src1_name, "%s", op.source1.variable_name);

            break;
        }

        case SF_OPERAND_TYPE_IMMEDIATE: {
            src1_name = malloc(32 * sizeof(char));
            sprintf(src1_name, "%s", op.source1.immediate_value);

            break;
        }

        default: break;
    }

    switch (op.source2.type) {
        case SF_OPERAND_TYPE_TEMPORARY: {
            src2_name = malloc(32 * sizeof(char));
            sprintf(src2_name, "t%hi", op.source2.temporary_id);

            break;
        }

        case SF_OPERAND_TYPE_VARIABLE: {
            src2_name = malloc(32 * sizeof(char));
            sprintf(src2_name, "%s", op.source2.variable_name);

            break;
        }

        case SF_OPERAND_TYPE_IMMEDIATE: {
            src2_name = malloc(32 * sizeof(char));
            sprintf(src2_name, "%s", op.source2.immediate_value);

            break;
        }

        default: break;
    }

    char* instruction = malloc(256 * sizeof(char));

    if (op.source1.type != SF_OPERAND_TYPE_IMMEDIATE && op.source2.type != SF_OPERAND_TYPE_IMMEDIATE) {
        sprintf(instruction,
            "	mov rax, [rbp%hi]\n"
            "	%s rax, [rbp%hi]\n"
            "	mov [rbp%hi], rax\n",
            lookup_stack(map, src1_name),
            instr,
            lookup_stack(map, src2_name),
            lookup_stack(map, dst_name)
        );
    } else if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE && op.source2.type != SF_OPERAND_TYPE_IMMEDIATE) {
        sprintf(instruction,
            "	mov rax, %s\n"
            "	%s rax, [rbp%hi]\n"
            "	mov [rbp%hi], rax\n",
            src1_name, instr,
            lookup_stack(map, src2_name),
            lookup_stack(map, dst_name)
        );
    } else if (op.source1.type != SF_OPERAND_TYPE_IMMEDIATE && op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        sprintf(instruction,
            "	mov rax, [rbp%hi]\n"
            "	%s rax, %s\n"
            "	mov [rbp%hi], rax\n",
            lookup_stack(map, src1_name),
            instr, src2_name,
            lookup_stack(map, dst_name)
        );
    } else {
        sprintf(instruction,
            "	mov rax, %s\n"
            "	%s rax, %s\n"
            "	mov [rbp%hi], rax\n",
            src1_name, instr, src2_name,
            lookup_stack(map, dst_name)
        );
    }

    push_string(instruction, buff);

    free(instruction);
    free(dst_name);
    free(src1_name);
    free(src2_name);
}

char* sf_generate_assembly(const sf_ir_program* program) {
	char* as = strdup("");

	sf_stack_map map = {
		.entries  = NULL,
		.capacity = 0,
		.count    = 0,
	};

	populate_stack(&map, program);

	push_string("bits 64\n\n", &as);

	push_string("section .data\n", &as);

	// data goes here

	push_string("section .text\n", &as);
	push_string("	global _start\n\n", &as);

	push_string("_start:\n", &as);

	push_string("	push rbp\n", &as);
	push_string("	mov rbp, rsp\n", &as);

	char buf[32];
	snprintf(buf, sizeof(buf), "%d", map.count * 8);
	push_string("	sub rsp, ", &as);
	push_string(buf, &as);
	push_string("\n\n", &as);

	for (uint32_t i = 0; i < program->count; i++) {
		sf_operation op = program->operations[i];

		switch (op.opcode) {
            case SF_OPCODE_ADD:  emit_binary(&as, op, &map, "add");  break;
			case SF_OPCODE_SUB:  emit_binary(&as, op, &map, "sub");  break;
			case SF_OPCODE_MULT: emit_binary(&as, op, &map, "imul"); break;
			case SF_OPCODE_DIV:  emit_binary(&as, op, &map, "idiv"); break;

            case SF_OPCODE_ASSIGN: emit_assign(&as, op, &map); break;

            default: break;
        }
    }

	push_string("\n""	mov rsp, rbp\n", &as);
	push_string("	pop rbp\n\n", &as);

	push_string("	mov rax, 60\n", &as);
	push_string("	xor rdi, rdi\n", &as);
	push_string("	syscall\n", &as);

	return as;
}
