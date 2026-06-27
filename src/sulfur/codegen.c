#include "sulfur/codegen.h"
#include "sulfur/ast.h"
#include "sulfur/ir.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static void push_string(const char* src, char** dst);

static sf_stack_offset_size_t lookup_stack(const sf_stack_map* map, const char* name);
static sf_stack_entry* lookup_stack_entry(const sf_stack_map* map, const char* name);
static void push_stack(sf_stack_map* map, sf_stack_entry entry);
static void populate_stack(sf_stack_map* map, const sf_ir_program* program);

static void map_operand(sf_stack_map* map, sf_operand op, sf_stack_offset_size_t* next_offset);

static void emit_assign(char** buff, sf_operation op, const sf_stack_map* map);
static void emit_binary(char** buff, sf_operation op, const sf_stack_map* map, const char* instr);

static bool is_signed(sf_value_type type);

static uint8_t size_from_type(sf_value_type type);

static sf_register register_from_size(uint8_t size);
static sf_size_prefix prefix_from_size(uint8_t size);

static const char* register_to_string(sf_register reg);
static const char* prefix_to_string(sf_size_prefix prefix);

static sf_stack_offset_size_t next_aligned_offset(sf_stack_offset_size_t current, uint8_t size);

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
    push_string("\tglobal _start\n\n", &as);

    push_string("_start:\n", &as);

    push_string("\tpush rbp\n", &as);
    push_string("\tmov rbp, rsp\n", &as);

    sf_stack_offset_size_t stack_size = 0;

    for (sf_stack_offset_size_t i = 0; i < map.count; i++) {
        sf_stack_offset_size_t abs_offset = (sf_stack_offset_size_t)(-map.entries[i].offset);
        if (abs_offset > stack_size) stack_size = abs_offset;
    }

    stack_size = (stack_size + 15) / 16 * 16;

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", stack_size);
    
    push_string("\tsub rsp, ", &as);
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

    push_string("\n""\tmov rsp, rbp\n", &as);
    push_string("\tpop rbp\n\n", &as);

    push_string("\tmov rax, 60\n", &as);
    push_string("\txor rdi, rdi\n", &as);
    push_string("\tsyscall\n", &as);

    free(map.entries);
    return as;
}

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

static sf_stack_offset_size_t lookup_stack(const sf_stack_map* map, const char* name) {
	if (map == NULL) return 0;

	for (sf_stack_offset_size_t i = 0; i < map->count; i++) {
		if (strcmp(map->entries[i].name, name) == 0) {
			return map->entries[i].offset;
		}
	}

	return 0;
}

static sf_stack_entry* lookup_stack_entry(const sf_stack_map* map, const char* name) {
    if (map == NULL) return NULL;

    for (int64_t i = 0; i < map->count; i++) {
        if (strcmp(map->entries[i].name, name) == 0) {
            return &map->entries[i];
        }
    }

    return NULL;
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
        map->capacity *= 2;
	}

	map->entries[map->count++] = entry;
}

static void map_operand(sf_stack_map* map, sf_operand op, sf_stack_offset_size_t* next_offset) {
    if (op.type == SF_OPERAND_TYPE_IMMEDIATE) return;

    if (op.type == SF_OPERAND_TYPE_VARIABLE) {
    	char*   name   = op.variable_name;
    	int64_t offset = lookup_stack(map, name);

    	if (offset == 0) {
            *next_offset = next_aligned_offset(
                *next_offset,
                size_from_type(op.value_type)
            );

    		sf_stack_entry entry = {
    			.name = name,
    			.offset = *next_offset,
                .type = op.value_type,
    		};

    		push_stack(map, entry);
    	}
    }

    if (op.type == SF_OPERAND_TYPE_TEMPORARY) {
    	char*   name   = malloc(16 * sizeof(char));
    	sprintf(name, "t%hi", op.temporary_id);

    	int64_t offset = lookup_stack(map, name);

    	if (offset == 0) {
            *next_offset = next_aligned_offset(
                *next_offset,
                size_from_type(op.value_type)
            );

    		sf_stack_entry entry = {
    			.name = name,
    			.offset = *next_offset,
                .type = op.value_type,
    		};

    		push_stack(map, entry);
    	} else {
            free(name);
        }
    }
}

static void populate_stack(sf_stack_map* map, const sf_ir_program* program) {
    if (map     == NULL) return;
    if (program == NULL) return;

    sf_stack_offset_size_t next_offset = 0;

    for (uint64_t i = 0; i < program->count; i++) {
        sf_operation op = program->operations[i];

        if (op.destiny.type == SF_OPERAND_TYPE_VARIABLE) {
        	map_operand(map, op.destiny, &next_offset);
        }

        if (op.source1.type == SF_OPERAND_TYPE_VARIABLE){
        	map_operand(map, op.source1, &next_offset);
        }

        if (op.opcode != SF_OPCODE_ASSIGN && op.source2.type == SF_OPERAND_TYPE_VARIABLE) {
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

        if (op.opcode != SF_OPCODE_ASSIGN && op.source2.type == SF_OPERAND_TYPE_TEMPORARY) {
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

        uint8_t src1_size = size_from_type(op.source1.value_type);
        uint8_t dst_size = size_from_type(op.destiny.value_type);

        sf_register src1_reg = register_from_size(src1_size);
        sf_register dst_reg = register_from_size(dst_size);

        sf_size_prefix src1_pre = prefix_from_size(src1_size);
        sf_size_prefix dst_pre = prefix_from_size(dst_size);

        const char* src1_reg_str = register_to_string(src1_reg);
        const char* dst_reg_str = register_to_string(dst_reg);

        const char* src1_pre_str = prefix_to_string(src1_pre);
        const char* dst_pre_str = prefix_to_string(dst_pre);

        if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
            sprintf(
                instruction,

                "\tmov %s, %s\n"
                "\tmov %s [rbp%li], %s\n",

                dst_reg_str,
                src1_name,

                dst_pre_str,
                lookup_stack(map, dst_name),
                src1_reg_str
            );
        } else {
            if (src1_size == dst_size) {
                sprintf(
                    instruction,

                    "\tmov %s, %s [rbp%li]\n"
                    "\tmov %s [rbp%li], %s\n",

                    dst_reg_str,
                    src1_pre_str,
                    lookup_stack(map, src1_name),

                    dst_pre_str,
                    lookup_stack(map, dst_name),
                    dst_reg_str
                );
            } else if (src1_size < dst_size) {
                if (is_signed(op.source1.value_type)) {
                    sprintf(
                        instruction,

                        "\tmovsx %s, %s [rbp%li]\n"
                        "\tmov %s [rbp%li], %s\n",

                        dst_reg_str,
                        src1_pre_str,
                        lookup_stack(map, src1_name),

                        dst_pre_str,
                        lookup_stack(map, dst_name),
                        dst_reg_str
                    );
                } else {
                    sprintf(
                        instruction,

                        "\tmovzx %s, %s [rbp%li]\n"
                        "\tmov %s [rbp%li], %s\n",

                        dst_reg_str,
                        src1_pre_str,
                        lookup_stack(map, src1_name),

                        dst_pre_str,
                        lookup_stack(map, dst_name),
                        dst_reg_str
                    );
                }
            } else {
                sprintf(
                    instruction,

                    "\tmov rax, %s [rbp%li]\n"
                    "\tmov %s [rbp%li], %s\n",

                    src1_pre_str,
                    lookup_stack(map, src1_name),

                    dst_pre_str,
                    lookup_stack(map, dst_name),
                    dst_reg_str
                );
            }
        }

    	push_string(instruction, buff);

    	free(instruction);
        free(dst_name);
        free(src1_name);
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
            "\tmov rax, [rbp%li]\n"
            "\t%s rax, [rbp%li]\n"
            "\tmov [rbp%li], rax\n",
            lookup_stack(map, src1_name),
            instr,
            lookup_stack(map, src2_name),
            lookup_stack(map, dst_name)
        );
    } else if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE && op.source2.type != SF_OPERAND_TYPE_IMMEDIATE) {
        sprintf(instruction,
            "\tmov rax, %s\n"
            "\t%s rax, [rbp%li]\n"
            "\tmov [rbp%li], rax\n",
            src1_name, instr,
            lookup_stack(map, src2_name),
            lookup_stack(map, dst_name)
        );
    } else if (op.source1.type != SF_OPERAND_TYPE_IMMEDIATE && op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        sprintf(instruction,
            "\tmov rax, [rbp%li]\n"
            "\t%s rax, %s\n"
            "\tmov [rbp%li], rax\n",
            lookup_stack(map, src1_name),
            instr, src2_name,
            lookup_stack(map, dst_name)
        );
    } else {
        sprintf(instruction,
            "\tmov rax, %s\n"
            "\t%s rax, %s\n"
            "\tmov [rbp%li], rax\n",
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

static uint8_t size_from_type(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_U8:
        case SF_VAL_TYPE_I8:
            return 1;
            break;

        case SF_VAL_TYPE_U16:
        case SF_VAL_TYPE_I16:
            return 2;
            break;

        case SF_VAL_TYPE_U32:
        case SF_VAL_TYPE_I32:
            return 4;
            break;

        case SF_VAL_TYPE_U64:
        case SF_VAL_TYPE_I64:
            return 8;
            break;

        default: return 8; break;
    }
}

static bool is_signed(sf_value_type type) {
    switch (type) {
        case SF_VAL_TYPE_I8:
        case SF_VAL_TYPE_I16:
        case SF_VAL_TYPE_I32:
        case SF_VAL_TYPE_I64:
            return true; break;

        case SF_VAL_TYPE_U8:
        case SF_VAL_TYPE_U16:
        case SF_VAL_TYPE_U32:
        case SF_VAL_TYPE_U64:
            return false; break;

        case SF_VAL_TYPE_UNRESOLVED:
            return false; break;

        default: return false; break;
    }
}

static sf_register register_from_size(uint8_t size) {
    if (size <= 1) return SF_REGISTER_AL;
    if (size <= 2) return SF_REGISTER_AX;
    if (size <= 4) return SF_REGISTER_EAX;
    if (size <= 8) return SF_REGISTER_RAX;
    return SF_REGISTER_RAX;
}

static sf_size_prefix prefix_from_size(uint8_t size) {
    if (size <= 1) return SF_PREFIX_BYTE;
    if (size <= 2) return SF_PREFIX_WORD;
    if (size <= 4) return SF_PREFIX_DWORD;
    if (size <= 8) return SF_PREFIX_QWORD;
    return SF_PREFIX_QWORD;
}

static const char* register_to_string(sf_register reg) {
    switch (reg) {
        case SF_REGISTER_AL:  return "al";
        case SF_REGISTER_AX:  return "ax";
        case SF_REGISTER_EAX: return "eax";
        case SF_REGISTER_RAX: return "rax";
        default:              return "rax";
    }
}

static const char* prefix_to_string(sf_size_prefix prefix) {
    switch (prefix) {
        case SF_PREFIX_BYTE:  return "BYTE";
        case SF_PREFIX_WORD:  return "WORD";
        case SF_PREFIX_DWORD: return "DWORD";
        case SF_PREFIX_QWORD: return "QWORD";
        default:              return "QWORD";
    }
}

static sf_stack_offset_size_t next_aligned_offset(sf_stack_offset_size_t current, uint8_t size) {
    uint64_t pos = (uint64_t)(-current) + size;
    pos = (pos + size - 1) / size * size;
    return -(sf_stack_offset_size_t)pos;
}