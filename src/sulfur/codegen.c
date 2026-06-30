#include "sulfur/codegen.h"
#include "sulfur/ast.h"
#include "sulfur/ir.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static void push_string(const char* src, char** dst, uint64_t* len, uint64_t* capacity);

static sf_stack_offset_size_t lookup_stack(const sf_stack_map* map, const char* name);
static sf_stack_entry* lookup_stack_entry(const sf_stack_map* map, const char* name);
static void push_stack(sf_stack_map* map, sf_stack_entry entry);
static void populate_stack(sf_stack_map* map, const sf_ir_program* program);

static void map_operand(sf_stack_map* map, sf_operand op, sf_stack_offset_size_t* next_offset);

static void emit_assign(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map);

static void emit_add(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map);
static void emit_sub(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map);
static void emit_mult(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map);
static void emit_div(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map);

static bool is_signed(sf_value_type type);

static uint8_t size_from_type(sf_value_type type);

static sf_register register_from_size(uint8_t size);
static sf_size_prefix prefix_from_size(uint8_t size);

static const char* register_to_string(sf_register reg);
static const char* prefix_to_string(sf_size_prefix prefix);

static sf_stack_offset_size_t next_aligned_offset(sf_stack_offset_size_t current, uint8_t size);

static void free_stack_map(sf_stack_map* map);

char* sf_generate_assembly(const sf_ir_program* program) {
    uint64_t as_capacity = 4096;
    uint64_t as_len = 0;
    char* as = malloc(as_capacity);
    if (as == NULL) return NULL;
    as[0] = '\0';

    sf_stack_map map = {
        .entries  = NULL,
        .capacity = 0,
        .count    = 0,
    };

    populate_stack(&map, program);

    push_string("bits 64\n\n", &as, &as_len, &as_capacity);
    push_string("section .data\n", &as, &as_len, &as_capacity);

    push_string("section .text\n", &as, &as_len, &as_capacity);
    push_string("\tglobal _start\n\n", &as, &as_len, &as_capacity);

    push_string("_start:\n", &as, &as_len, &as_capacity);

    push_string("\tpush rbp\n", &as, &as_len, &as_capacity);
    push_string("\tmov rbp, rsp\n", &as, &as_len, &as_capacity);

    sf_stack_offset_size_t stack_size = 0;
    for (sf_stack_map_size_t i = 0; i < map.count; i++) {
        sf_stack_offset_size_t abs_offset = (sf_stack_offset_size_t)(-map.entries[i].offset);
        if (abs_offset > stack_size) stack_size = abs_offset;
    }

    stack_size = (stack_size + 15) / 16 * 16;

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", stack_size);
    
    push_string("\tsub rsp, ", &as, &as_len, &as_capacity);
    push_string(buf, &as, &as_len, &as_capacity);
    push_string("\n\n", &as, &as_len, &as_capacity);

    for (uint32_t i = 0; i < program->count; i++) {
        sf_operation op = program->operations[i];

        switch (op.opcode) {
            case SF_OPCODE_ADD: emit_add(&as, &as_len, &as_capacity, op, &map); break;
            case SF_OPCODE_SUB: emit_sub(&as, &as_len, &as_capacity, op, &map); break;
            case SF_OPCODE_MULT: emit_mult(&as, &as_len, &as_capacity, op, &map); break;
            case SF_OPCODE_DIV: emit_div(&as, &as_len, &as_capacity, op, &map); break;

            case SF_OPCODE_ASSIGN: emit_assign(&as, &as_len, &as_capacity, op, &map); break;

            default: break;
        }
    }

    push_string("\n\tmov rsp, rbp\n", &as, &as_len, &as_capacity);
    push_string("\tpop rbp\n\n", &as, &as_len, &as_capacity);

    push_string("\tmov rax, 60\n", &as, &as_len, &as_capacity);
    push_string("\txor rdi, rdi\n", &as, &as_len, &as_capacity);
    push_string("\tsyscall\n", &as, &as_len, &as_capacity);

    free_stack_map(&map);

    return as;
}

static void push_string(const char* src, char** dst, uint64_t* len, uint64_t* capacity) {
    if (dst == NULL || *dst == NULL || src == NULL) return;

    size_t src_len = strlen(src);
    
    while (*len + src_len + 1 > *capacity) {
        *capacity *= 2;
        *dst = realloc(*dst, *capacity);
    }

    memcpy(*dst + *len, src, src_len + 1);
    *len += src_len;
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
        char* name = op.variable_name;
        int64_t offset = lookup_stack(map, name);

        if (offset == 0) {
            *next_offset = next_aligned_offset(
                *next_offset,
                size_from_type(op.value_type)
            );

            sf_stack_entry entry = {
                .name = strdup(name),
                .offset = *next_offset,
                .type = op.value_type,
            };

            push_stack(map, entry);
        }
    }

    if (op.type == SF_OPERAND_TYPE_TEMPORARY) {
        char* name = malloc(16 * sizeof(char));
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
    if (map == NULL || program == NULL) return;

    sf_stack_offset_size_t next_offset = 0;

    for (uint64_t i = 0; i < program->count; i++) {
        sf_operation op = program->operations[i];

        if (op.destiny.type == SF_OPERAND_TYPE_VARIABLE || op.destiny.type == SF_OPERAND_TYPE_TEMPORARY) {
            map_operand(map, op.destiny, &next_offset);
        }

        if (op.source1.type == SF_OPERAND_TYPE_VARIABLE || op.source1.type == SF_OPERAND_TYPE_TEMPORARY) {
            map_operand(map, op.source1, &next_offset);
        }

        if (op.opcode != SF_OPCODE_ASSIGN) {
            if (op.source2.type == SF_OPERAND_TYPE_VARIABLE || op.source2.type == SF_OPERAND_TYPE_TEMPORARY) {
                map_operand(map, op.source2, &next_offset);
            }
        }
    }
}

static void emit_assign(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map) {
    if (op.opcode != SF_OPCODE_ASSIGN) return;

    const char* dst_name  = (op.destiny.type == SF_OPERAND_TYPE_VARIABLE) ? op.destiny.variable_name : NULL;
    const char* src1_name = (op.source1.type == SF_OPERAND_TYPE_VARIABLE) ? op.source1.variable_name : NULL;

    char* allocated_dst  = NULL;
    char* allocated_src1 = NULL;

    if (op.destiny.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_dst = malloc(64);
        snprintf(allocated_dst, 64, "t%hi", op.destiny.temporary_id);
        dst_name = allocated_dst;
    }

    if (op.source1.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src1 = malloc(64);
        snprintf(allocated_src1, 64, "t%hi", op.source1.temporary_id);
        src1_name = allocated_src1;
    } else if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src1 = malloc(64);
        snprintf(allocated_src1, 64, "%s", op.source1.immediate_value);
        src1_name = allocated_src1;
    }

    char* instruction = malloc(512 * sizeof(char));

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
            dst_reg_str
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

    push_string(instruction, buff, len, capacity);

    free(instruction);
    if (allocated_dst) free(allocated_dst);
    if (allocated_src1) free(allocated_src1);
}

static void emit_add(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map) {
    const char* dst_name  = (op.destiny.type == SF_OPERAND_TYPE_VARIABLE) ? op.destiny.variable_name : NULL;
    const char* src1_name = (op.source1.type == SF_OPERAND_TYPE_VARIABLE) ? op.source1.variable_name : NULL;
    const char* src2_name = (op.source2.type == SF_OPERAND_TYPE_VARIABLE) ? op.source2.variable_name : NULL;

    char* allocated_dst  = NULL;
    char* allocated_src1 = NULL;
    char* allocated_src2 = NULL;

    if (op.destiny.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_dst = malloc(32);
        sprintf(allocated_dst, "t%u", op.destiny.temporary_id);
        dst_name = allocated_dst;
    }
    if (op.source1.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src1 = malloc(32);
        sprintf(allocated_src1, "t%u", op.source1.temporary_id);
        src1_name = allocated_src1;
    } else if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src1 = malloc(32);
        sprintf(allocated_src1, "%s", op.source1.immediate_value);
        src1_name = allocated_src1;
    }
    if (op.source2.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src2 = malloc(32);
        sprintf(allocated_src2, "t%u", op.source2.temporary_id);
        src2_name = allocated_src2;
    } else if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src2 = malloc(32);
        sprintf(allocated_src2, "%s", op.source2.immediate_value);
        src2_name = allocated_src2;
    }

    char* instruction = malloc(512 * sizeof(char));

    uint8_t size = size_from_type(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));
    const char* pre_str = prefix_to_string(prefix_from_size(size));

    char src1_formatted[256];
    if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        strcpy(src1_formatted, src1_name);
    } else {
        sprintf(src1_formatted, "%s [rbp%li]", pre_str, lookup_stack(map, src1_name));
    }

    char src2_formatted[256];
    if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        strcpy(src2_formatted, src2_name);
    } else {
        sprintf(src2_formatted, "%s [rbp%li]", pre_str, lookup_stack(map, src2_name));
    }

    sprintf(instruction,
        "\tmov %s, %s\n"
        "\tadd %s, %s\n"
        "\tmov %s [rbp%li], %s\n",
        reg_str, src1_formatted,
        reg_str, src2_formatted,
        pre_str, lookup_stack(map, dst_name), reg_str
    );
    
    push_string(instruction, buff, len, capacity);

    free(instruction);
    if (allocated_dst) free(allocated_dst);
    if (allocated_src1) free(allocated_src1);
    if (allocated_src2) free(allocated_src2);
}

static void emit_sub(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map) {
    const char* dst_name  = (op.destiny.type == SF_OPERAND_TYPE_VARIABLE) ? op.destiny.variable_name : NULL;
    const char* src1_name = (op.source1.type == SF_OPERAND_TYPE_VARIABLE) ? op.source1.variable_name : NULL;
    const char* src2_name = (op.source2.type == SF_OPERAND_TYPE_VARIABLE) ? op.source2.variable_name : NULL;

    char* allocated_dst  = NULL;
    char* allocated_src1 = NULL;
    char* allocated_src2 = NULL;

    if (op.destiny.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_dst = malloc(32);
        sprintf(allocated_dst, "t%u", op.destiny.temporary_id);
        dst_name = allocated_dst;
    }
    if (op.source1.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src1 = malloc(32);
        sprintf(allocated_src1, "t%u", op.source1.temporary_id);
        src1_name = allocated_src1;
    } else if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src1 = malloc(32);
        sprintf(allocated_src1, "%s", op.source1.immediate_value);
        src1_name = allocated_src1;
    }
    if (op.source2.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src2 = malloc(32);
        sprintf(allocated_src2, "t%u", op.source2.temporary_id);
        src2_name = allocated_src2;
    } else if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src2 = malloc(32);
        sprintf(allocated_src2, "%s", op.source2.immediate_value);
        src2_name = allocated_src2;
    }

    char* instruction = malloc(512 * sizeof(char));

    uint8_t size = size_from_type(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));
    const char* pre_str = prefix_to_string(prefix_from_size(size));

    char src1_formatted[256];
    if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        strcpy(src1_formatted, src1_name);
    } else {
        sprintf(src1_formatted, "%s [rbp%li]", pre_str, lookup_stack(map, src1_name));
    }

    char src2_formatted[256];
    if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        strcpy(src2_formatted, src2_name);
    } else {
        sprintf(src2_formatted, "%s [rbp%li]", pre_str, lookup_stack(map, src2_name));
    }

    sprintf(instruction,
        "\tmov %s, %s\n"
        "\tsub %s, %s\n"
        "\tmov %s [rbp%li], %s\n",
        reg_str, src1_formatted,
        reg_str, src2_formatted,
        pre_str, lookup_stack(map, dst_name), reg_str
    );
    
    push_string(instruction, buff, len, capacity);

    free(instruction);
    if (allocated_dst) free(allocated_dst);
    if (allocated_src1) free(allocated_src1);
    if (allocated_src2) free(allocated_src2);
}

static void emit_mult(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map) {
    const char* dst_name  = (op.destiny.type == SF_OPERAND_TYPE_VARIABLE) ? op.destiny.variable_name : NULL;
    const char* src1_name = (op.source1.type == SF_OPERAND_TYPE_VARIABLE) ? op.source1.variable_name : NULL;
    const char* src2_name = (op.source2.type == SF_OPERAND_TYPE_VARIABLE) ? op.source2.variable_name : NULL;

    char* allocated_dst  = NULL;
    char* allocated_src1 = NULL;
    char* allocated_src2 = NULL;

    if (op.destiny.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_dst = malloc(32);
        sprintf(allocated_dst, "t%u", op.destiny.temporary_id);
        dst_name = allocated_dst;
    }
    if (op.source1.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src1 = malloc(32);
        sprintf(allocated_src1, "t%u", op.source1.temporary_id);
        src1_name = allocated_src1;
    } else if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src1 = malloc(32);
        sprintf(allocated_src1, "%s", op.source1.immediate_value);
        src1_name = allocated_src1;
    }
    if (op.source2.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src2 = malloc(32);
        sprintf(allocated_src2, "t%u", op.source2.temporary_id);
        src2_name = allocated_src2;
    } else if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src2 = malloc(32);
        sprintf(allocated_src2, "%s", op.source2.immediate_value);
        src2_name = allocated_src2;
    }

    char* instruction = malloc(512 * sizeof(char));

    uint8_t size = size_from_type(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));
    const char* pre_str = prefix_to_string(prefix_from_size(size));

    char src1_formatted[256];
    if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        strcpy(src1_formatted, src1_name);
    } else {
        sprintf(src1_formatted, "%s [rbp%li]", pre_str, lookup_stack(map, src1_name));
    }

    char src2_formatted[256];
    if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        strcpy(src2_formatted, src2_name);
    } else {
        sprintf(src2_formatted, "%s [rbp%li]", pre_str, lookup_stack(map, src2_name));
    }

    bool signed_type = is_signed(op.destiny.value_type);
    const char* mul_instr = signed_type ? "imul" : "mul";

    if (size == 1) {
        sprintf(instruction,
            "\tmov al, %s\n"
            "\t%s %s\n"
            "\tmov %s [rbp%li], al\n",
            src1_formatted,
            mul_instr, src2_formatted,
            pre_str, lookup_stack(map, dst_name)
        );
    } else {
        sprintf(instruction,
            "\tmov %s, %s\n"
            "\timul %s, %s\n"
            "\tmov %s [rbp%li], %s\n",
            reg_str, src1_formatted,
            reg_str, src2_formatted,
            pre_str, lookup_stack(map, dst_name), reg_str
        );
    }
    
    push_string(instruction, buff, len, capacity);

    free(instruction);
    if (allocated_dst) free(allocated_dst);
    if (allocated_src1) free(allocated_src1);
    if (allocated_src2) free(allocated_src2);
}

static void emit_div(char** buff, size_t* len, size_t* capacity, sf_operation op, const sf_stack_map* map) {
    const char* dst_name  = (op.destiny.type == SF_OPERAND_TYPE_VARIABLE) ? op.destiny.variable_name : NULL;
    const char* src1_name = (op.source1.type == SF_OPERAND_TYPE_VARIABLE) ? op.source1.variable_name : NULL;
    const char* src2_name = (op.source2.type == SF_OPERAND_TYPE_VARIABLE) ? op.source2.variable_name : NULL;
    
    char* allocated_dst  = NULL;
    char* allocated_src1 = NULL;
    char* allocated_src2 = NULL;

    if (op.destiny.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_dst = malloc(32);
        sprintf(allocated_dst, "t%u", op.destiny.temporary_id);
        dst_name = allocated_dst;
    }

    if (op.source1.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src1 = malloc(32);
        sprintf(allocated_src1, "t%u", op.source1.temporary_id);
        src1_name = allocated_src1;
    } else if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src1 = malloc(32);
        sprintf(allocated_src1, "%s", op.source1.immediate_value);
        src1_name = allocated_src1;
    }

    if (op.source2.type == SF_OPERAND_TYPE_TEMPORARY) {
        allocated_src2 = malloc(32);
        sprintf(allocated_src2, "t%u", op.source2.temporary_id);
        src2_name = allocated_src2;
    } else if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        allocated_src2 = malloc(32);
        sprintf(allocated_src2, "%s", op.source2.immediate_value);
        src2_name = allocated_src2;
    }

    uint8_t size = size_from_type(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));
    const char* pre_str = prefix_to_string(prefix_from_size(size));

    char src1_formatted[256];
    if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        strcpy(src1_formatted, src1_name);
    } else {
        sprintf(src1_formatted, "%s [rbp%li]", pre_str, lookup_stack(map, src1_name));
    }

    char src2_formatted[256];
    if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        strcpy(src2_formatted, src2_name);
    } else {
        sprintf(src2_formatted, "%s [rbp%li]", pre_str, lookup_stack(map, src2_name));
    }

    bool signed_type = is_signed(op.destiny.value_type);
    const char* div_instr = signed_type ? "idiv" : "div";

    const char* reg_cx = "rcx";
    if (size == 4) reg_cx = "ecx";
    if (size == 2) reg_cx = "cx";
    if (size == 1) reg_cx = "cl";

    char* instruction = malloc(512);

    sprintf(instruction, "\tmov %s, %s\n", reg_str, src1_formatted);
    push_string(instruction, buff, len, capacity);

    if (signed_type) {
        if (size == 8) sprintf(instruction, "\tcqo\n");
        if (size == 4) sprintf(instruction, "\tcdq\n");
        if (size == 2) sprintf(instruction, "\tcwd\n");
        if (size == 1) sprintf(instruction, "\tcbw\n");

        push_string(instruction, buff, len, capacity);
    } else {
        if (size == 8) sprintf(instruction, "\txor rdx, rdx\n");
        if (size == 4) sprintf(instruction, "\txor edx, edx\n");
        if (size == 2) sprintf(instruction, "\txor dx, dx\n");
        if (size == 1) sprintf(instruction, "\tmov ah, 0\n");

        push_string(instruction, buff, len, capacity);
    }

    if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        sprintf(instruction, "\tmov %s, %s\n", reg_cx, src2_formatted);
        push_string(instruction, buff, len, capacity);

        sprintf(instruction, "\t%s %s\n", div_instr, reg_cx);
        push_string(instruction, buff, len, capacity);
    } else {
        sprintf(instruction, "\t%s %s\n", div_instr, src2_formatted);
        push_string(instruction, buff, len, capacity);
    }

    sprintf(instruction, "\tmov %s [rbp%li], %s\n", pre_str, lookup_stack(map, dst_name), reg_str);
    push_string(instruction, buff, len, capacity);

    free(instruction);
    if (allocated_dst) free(allocated_dst);
    if (allocated_src1) free(allocated_src1);
    if (allocated_src2) free(allocated_src2);
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

static void free_stack_map(sf_stack_map* map) {
    if (map == NULL) return;
    for (sf_stack_map_size_t i = 0; i < map->count; i++) {
        free(map->entries[i].name);
    }
    free(map->entries);
}