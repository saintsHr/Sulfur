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

static int16_t lookup_stack(const sfStackMap* map, const char* name) {
	if (map == NULL) return 0;

	for (int16_t i = 0; i < map->count; i++) {
		if (strcmp(map->entries[i].name, name) == 0) {
			return map->entries[i].offset;
		}
	}

	return 0;
}

static void push_stack(sfStackMap* map, sfStackEntry entry) {
	if (map == NULL) return;

	if (map->capacity <= 0) {
		map->entries = realloc(
			map->entries,
			8 * sizeof(sfStackEntry)
		);
		map->capacity = 8;
	}

	if (map->count >= map->capacity) {
		map->entries = realloc(
			map->entries,
			(map->capacity * 2) * sizeof(sfStackEntry)
		);
	}

	map->entries[map->count++] = entry;
}

static void map_operand(sfStackMap* map, sfOperand op, int16_t* next_offset) {
    if (op.type == SF_OPERAND_TYPE_IMMEDIATE) return;

    if (op.type == SF_OPERAND_TYPE_VARIABLE) {
    	char*   name   = op.variableName;
    	int16_t offset = lookup_stack(map, name);

    	if (offset == 0) {
    		sfStackEntry entry = {
    			.name = name,
    			.offset = *next_offset,
    		};

    		push_stack(map, entry);

    		*next_offset -= 8;
    	}
    }

    if (op.type == SF_OPERAND_TYPE_TEMPORARY) {
    	char*   name   = malloc(16 * sizeof(char));
    	sprintf(name, "t%hi", op.temporaryID);

    	int16_t offset = lookup_stack(map, name);

    	if (offset == 0) {
    		sfStackEntry entry = {
    			.name = name,
    			.offset = *next_offset,
    		};

    		push_stack(map, entry);

    		*next_offset -= 8;
    	}
    }
}

static void populate_stack(sfStackMap* map, const sfIRProgram* program) {
    if (map     == NULL) return;
    if (program == NULL) return;

    int16_t next_offset = -8;

    for (uint64_t i = 0; i < program->count; i++) {
        sfOperation op = program->operations[i];

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
        sfOperation op = program->operations[i];

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

static void emit_assign(char** buff, sfOperation op, const sfStackMap* map) {
	if (op.opcode == SF_OPCODE_ASSIGN) {
		char* dstName  = NULL;
		char* src1Name = NULL;

		switch (op.destiny.type) {
            case SF_OPERAND_TYPE_TEMPORARY: {
            	dstName = malloc(16 * sizeof(char));
    			sprintf(dstName, "t%hi", op.destiny.temporaryID);
    			break;
            }

            case SF_OPERAND_TYPE_VARIABLE: {
            	dstName = malloc(16 * sizeof(char));
    			sprintf(dstName, "%s", op.destiny.variableName);
    			break;
            }

            default: break;
        }

        switch (op.source1.type) {
        	case SF_OPERAND_TYPE_TEMPORARY: {
        		src1Name = malloc(16 * sizeof(char));
    			sprintf(src1Name, "t%hi", op.source1.temporaryID);
    			break;
        	}

        	case SF_OPERAND_TYPE_VARIABLE: {
        		src1Name = malloc(16 * sizeof(char));
    			sprintf(src1Name, "%s", op.source1.variableName);
    			break;
        	}

        	case SF_OPERAND_TYPE_IMMEDIATE: {
        		src1Name = malloc(16 * sizeof(char));
    			sprintf(src1Name, "%s", op.source1.immediateValue);
    			break;
        	}

          	default: break;
        }

        char* instruction = malloc(128 * sizeof(char));

    	if (op.source1.type != SF_OPERAND_TYPE_IMMEDIATE) {
    		sprintf(
    			instruction,
    			"	mov rax, [rbp%hi]\n"
    			"	mov [rbp%hi], rax\n",
    			lookup_stack(map, src1Name),
    			lookup_stack(map, dstName)
    		);
    	} else {
    		sprintf(
    			instruction,
    			"	mov rax, %s\n"
    			"	mov [rbp%hi], rax\n",
    			src1Name,
    			lookup_stack(map, dstName)
    		);
    	}

    	push_string(instruction, buff);

    	free(instruction);
	}
}

char* sfGenerateAssembly(const sfIRProgram* program) {
	char* as = strdup("");

	sfStackMap map = {
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
		sfOperation op = program->operations[i];

		switch (op.opcode) {
            case SF_OPCODE_ADD: {
            	break;
            }

            case SF_OPCODE_SUB: {
            	break;
            }

            case SF_OPCODE_DIV: {
            	break;
            }

            case SF_OPCODE_MULT: {
            	break;
            }

            case SF_OPCODE_ASSIGN: {
            	emit_assign(&as, op, &map);
            	break;
            }

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
