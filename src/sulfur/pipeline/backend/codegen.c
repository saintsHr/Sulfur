#include "sulfur/pipeline/backend/codegen.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/pipeline/backend/ir.h"
#include "sulfur/utils/type_utils.h"

static void push_string(
    const char* src, char** dst, uint64_t* len, uint64_t* capacity
);
static void format_operand(
    char* dst_str, size_t max_len, sf_operand op, const sf_stack_map* map
);

static sf_stack_offset_size_t lookup_stack(
    const sf_stack_map* map, const char* name
);
static sf_stack_entry* lookup_stack_entry(
    const sf_stack_map* map, const char* name
);
static void push_stack(sf_stack_map* map, sf_stack_entry entry);
static void populate_stack(sf_stack_map* map, const sf_ir_program* program);
static void free_stack_map(sf_stack_map* map);

static void map_operand(
    sf_stack_map* map, sf_operand op, sf_stack_offset_size_t* next_offset
);

static void emitf(
    char** buff, size_t* len, size_t* capacity, const char* format, ...
);

static void emit_add(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_sub(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_mult(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_div(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_neg(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);

static void emit_cast(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_assign(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);

static void emit_bitwise_and(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_bitwise_or(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_bitwise_xor(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_bitwise_rshift(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_bitwise_lshift(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);
static void emit_bitwise_not(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
);

static sf_register register_from_size(uint8_t size);
static sf_size_prefix prefix_from_size(uint8_t size);

static const char* register_to_string(sf_register reg);
static const char* prefix_to_string(sf_size_prefix prefix);

static sf_stack_offset_size_t next_aligned_offset(
    sf_stack_offset_size_t current, uint8_t size
);

char* sf_generate_assembly(const sf_ir_program* program) {
    uint64_t as_capacity = 4096;
    uint64_t as_len = 0;
    char* as = malloc(as_capacity);
    if (as == NULL) return NULL;
    as[0] = '\0';

    sf_stack_map map = {
        .entries = NULL,
        .capacity = 0,
        .count = 0,
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
        sf_stack_offset_size_t abs_offset =
            (sf_stack_offset_size_t)(-map.entries[i].offset);
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
            case SF_OPCODE_ADD:
                emit_add(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_SUB:
                emit_sub(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_MULT:
                emit_mult(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_DIV:
                emit_div(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_NEGATE:
                emit_neg(&as, &as_len, &as_capacity, op, &map);
                break;

            case SF_OPCODE_ASSIGN:
                emit_assign(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_CAST:
                emit_cast(&as, &as_len, &as_capacity, op, &map);
                break;

            case SF_OPCODE_BITWISE_AND:
                emit_bitwise_and(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_BITWISE_OR:
                emit_bitwise_or(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_BITWISE_XOR:
                emit_bitwise_xor(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_BITWISE_RSHIFT:
                emit_bitwise_rshift(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_BITWISE_LSHIFT:
                emit_bitwise_lshift(&as, &as_len, &as_capacity, op, &map);
                break;
            case SF_OPCODE_BITWISE_NOT:
                emit_bitwise_not(&as, &as_len, &as_capacity, op, &map);
                break;

            default:
                break;
        }

        push_string("\n", &as, &as_len, &as_capacity);
    }

    push_string("\tmov rsp, rbp\n", &as, &as_len, &as_capacity);
    push_string("\tpop rbp\n\n", &as, &as_len, &as_capacity);

    push_string("\tmov rax, 60\n", &as, &as_len, &as_capacity);
    push_string("\txor rdi, rdi\n", &as, &as_len, &as_capacity);
    push_string("\tsyscall\n", &as, &as_len, &as_capacity);

    free_stack_map(&map);

    return as;
}

static void push_string(
    const char* src, char** dst, uint64_t* len, uint64_t* capacity
) {
    if (dst == NULL || *dst == NULL || src == NULL) return;

    size_t src_len = strlen(src);

    while (*len + src_len + 1 > *capacity) {
        *capacity *= 2;
        *dst = realloc(*dst, *capacity);
    }

    memcpy(*dst + *len, src, src_len + 1);
    *len += src_len;
}

static void format_operand(
    char* dst_str, size_t max_len, sf_operand op, const sf_stack_map* map
) {
    if (op.type == SF_OPERAND_TYPE_IMMEDIATE) {
        snprintf(dst_str, max_len, "%s", op.immediate_value);
    } else {
        uint8_t size = type_value_width_bytes(op.value_type);
        const char* pre_str = prefix_to_string(prefix_from_size(size));

        if (op.type == SF_OPERAND_TYPE_VARIABLE) {
            snprintf(
                dst_str,
                max_len,
                "%s [rbp%li]",
                pre_str,
                lookup_stack(map, op.variable_name)
            );
        } else if (op.type == SF_OPERAND_TYPE_TEMPORARY) {
            char temp_name[32];
            snprintf(temp_name, sizeof(temp_name), "t%u", op.temporary_id);
            snprintf(
                dst_str,
                max_len,
                "%s [rbp%li]",
                pre_str,
                lookup_stack(map, temp_name)
            );
        }
    }
}

static sf_stack_offset_size_t lookup_stack(
    const sf_stack_map* map, const char* name
) {
    if (map == NULL) return 0;
    if (name == NULL) return 0;

    for (sf_stack_offset_size_t i = 0; i < map->count; i++) {
        if (strcmp(map->entries[i].name, name) == 0) {
            return map->entries[i].offset;
        }
    }

    return 0;
}

static sf_stack_entry* lookup_stack_entry(
    const sf_stack_map* map, const char* name
) {
    if (map == NULL) return NULL;
    if (name == NULL) return NULL;

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
        map->entries = realloc(map->entries, 8 * sizeof(sf_stack_entry));
        map->capacity = 8;
    }

    if (map->count >= map->capacity) {
        map->entries =
            realloc(map->entries, (map->capacity * 2) * sizeof(sf_stack_entry));
        map->capacity *= 2;
    }

    map->entries[map->count++] = entry;
}

static void map_operand(
    sf_stack_map* map, sf_operand op, sf_stack_offset_size_t* next_offset
) {
    if (op.type == SF_OPERAND_TYPE_IMMEDIATE) return;

    if (op.type == SF_OPERAND_TYPE_VARIABLE) {
        char* name = op.variable_name;
        int64_t offset = lookup_stack(map, name);

        if (offset == 0) {
            *next_offset = next_aligned_offset(
                *next_offset, type_value_width_bytes(op.value_type)
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
                *next_offset, type_value_width_bytes(op.value_type)
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

        if (op.destiny.type == SF_OPERAND_TYPE_VARIABLE ||
            op.destiny.type == SF_OPERAND_TYPE_TEMPORARY) {
            map_operand(map, op.destiny, &next_offset);
        }

        if (op.source1.type == SF_OPERAND_TYPE_VARIABLE ||
            op.source1.type == SF_OPERAND_TYPE_TEMPORARY) {
            map_operand(map, op.source1, &next_offset);
        }

        if (op.opcode != SF_OPCODE_ASSIGN) {
            if (op.source2.type == SF_OPERAND_TYPE_VARIABLE ||
                op.source2.type == SF_OPERAND_TYPE_TEMPORARY) {
                map_operand(map, op.source2, &next_offset);
            }
        }
    }
}

static void emitf(
    char** buff, size_t* len, size_t* capacity, const char* format, ...
) {
    char temp[512];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    push_string(temp, buff, len, capacity);
}

static void emit_assign(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_ASSIGN) return;

    char src1_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    if (strcmp(src1_fmt, dst_fmt) == 0) return;

    uint8_t src1_size = type_value_width_bytes(op.source1.value_type);
    uint8_t dst_size = type_value_width_bytes(op.destiny.value_type);

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* dst_reg_str = register_to_string(register_from_size(size));

    if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_reg_str, src1_fmt);
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
    } else {
        if (src1_size == dst_size) {
            emitf(buff, len, capacity, "\tmov %s, %s\n", dst_reg_str, src1_fmt);
            emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
        } else if (src1_size < dst_size) {
            if (type_value_is_signed(op.source1.value_type)) {
                emitf(
                    buff,
                    len,
                    capacity,
                    "\tmovsx %s, %s\n",
                    dst_reg_str,
                    src1_fmt
                );
            } else {
                emitf(
                    buff,
                    len,
                    capacity,
                    "\tmovzx %s, %s\n",
                    dst_reg_str,
                    src1_fmt
                );
            }
            emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
        } else {
            const char* src_reg_str =
                register_to_string(register_from_size(src1_size));
            emitf(buff, len, capacity, "\tmov %s, %s\n", src_reg_str, src1_fmt);
            emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
        }
    }
}

static void emit_add(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_ADD) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    if (strcmp(dst_fmt, src1_fmt) == 0 &&
        op.source2.type != SF_OPERAND_TYPE_VARIABLE &&
        op.source2.type != SF_OPERAND_TYPE_TEMPORARY) {
        emitf(buff, len, capacity, "\tadd %s, %s\n", dst_fmt, src2_fmt);
        return;
    }

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\tadd %s, %s\n", reg_str, src2_fmt);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_sub(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_SUB) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    if (strcmp(dst_fmt, src1_fmt) == 0 &&
        op.source2.type != SF_OPERAND_TYPE_VARIABLE &&
        op.source2.type != SF_OPERAND_TYPE_TEMPORARY) {
        emitf(buff, len, capacity, "\tsub %s, %s\n", dst_fmt, src2_fmt);
        return;
    }

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\tsub %s, %s\n", reg_str, src2_fmt);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_mult(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_MULT) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    bool signed_type = type_value_is_signed(op.destiny.value_type);
    const char* mul_instr = signed_type ? "imul" : "mul";

    if (size == 1) {
        emitf(buff, len, capacity, "\tmov al, %s\n", src1_fmt);
        emitf(buff, len, capacity, "\t%s %s\n", mul_instr, src2_fmt);
        emitf(buff, len, capacity, "\tmov %s, al\n", dst_fmt);
    } else {
        const char* reg_str = register_to_string(register_from_size(size));
        emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
        emitf(buff, len, capacity, "\timul %s, %s\n", reg_str, src2_fmt);
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
    }
}

static void emit_div(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_DIV) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    bool signed_type = type_value_is_signed(op.destiny.value_type);
    const char* div_instr = signed_type ? "idiv" : "div";

    const char* reg_str = register_to_string(register_from_size(size));

    const char* reg_cx = "rcx";
    if (size == 4) reg_cx = "ecx";
    if (size == 2) reg_cx = "cx";
    if (size == 1) reg_cx = "cl";

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);

    if (signed_type) {
        if (size == 8) emitf(buff, len, capacity, "\tcqo\n");
        if (size == 4) emitf(buff, len, capacity, "\tcdq\n");
        if (size == 2) emitf(buff, len, capacity, "\tcwd\n");
        if (size == 1) emitf(buff, len, capacity, "\tcbw\n");
    } else {
        if (size == 8) emitf(buff, len, capacity, "\txor rdx, rdx\n");
        if (size == 4) emitf(buff, len, capacity, "\txor edx, edx\n");
        if (size == 2) emitf(buff, len, capacity, "\txor dx, dx\n");
        if (size == 1) emitf(buff, len, capacity, "\tmov ah, 0\n");
    }

    if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        emitf(buff, len, capacity, "\tmov %s, %s\n", reg_cx, src2_fmt);
        emitf(buff, len, capacity, "\t%s %s\n", div_instr, reg_cx);
    } else {
        emitf(buff, len, capacity, "\t%s %s\n", div_instr, src2_fmt);
    }

    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_neg(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_NEGATE) return;

    char src1_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    if (strcmp(dst_fmt, src1_fmt) == 0) {
        emitf(buff, len, capacity, "\tneg %s\n", dst_fmt);
        return;
    }

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\tneg %s\n", reg_str);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_cast(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_CAST) return;

    char src1_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    uint8_t src1_size = type_value_width_bytes(op.source1.value_type);
    uint8_t dst_size = type_value_width_bytes(op.destiny.value_type);

    uint8_t size = dst_size;
    const char* dst_reg_str = register_to_string(register_from_size(size));

    if (op.source1.type == SF_OPERAND_TYPE_IMMEDIATE) {
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_reg_str, src1_fmt);
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
    } else if (src1_size == dst_size) {
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_reg_str, src1_fmt);
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
    } else if (src1_size < dst_size) {
        if (type_value_is_signed(op.source1.value_type)) {
            emitf(
                buff, len, capacity, "\tmovsx %s, %s\n", dst_reg_str, src1_fmt
            );
        }
        if (!type_value_is_signed(op.source1.value_type)) {
            emitf(
                buff, len, capacity, "\tmovzx %s, %s\n", dst_reg_str, src1_fmt
            );
        }
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
    } else if (src1_size > dst_size) {
        const char* src_reg_str =
            register_to_string(register_from_size(src1_size));
        emitf(buff, len, capacity, "\tmov %s, %s\n", src_reg_str, src1_fmt);
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
    }
}

static void emit_bitwise_and(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_BITWISE_AND) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    if (strcmp(dst_fmt, src1_fmt) == 0 &&
        op.source2.type != SF_OPERAND_TYPE_VARIABLE &&
        op.source2.type != SF_OPERAND_TYPE_TEMPORARY) {
        emitf(buff, len, capacity, "\tand %s, %s\n", dst_fmt, src2_fmt);
        return;
    }

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\tand %s, %s\n", reg_str, src2_fmt);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_bitwise_or(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_BITWISE_OR) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    if (strcmp(dst_fmt, src1_fmt) == 0 &&
        op.source2.type != SF_OPERAND_TYPE_VARIABLE &&
        op.source2.type != SF_OPERAND_TYPE_TEMPORARY) {
        emitf(buff, len, capacity, "\tor %s, %s\n", dst_fmt, src2_fmt);
        return;
    }

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\tor %s, %s\n", reg_str, src2_fmt);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_bitwise_xor(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_BITWISE_XOR) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    if (strcmp(dst_fmt, src1_fmt) == 0 &&
        op.source2.type != SF_OPERAND_TYPE_VARIABLE &&
        op.source2.type != SF_OPERAND_TYPE_TEMPORARY) {
        emitf(buff, len, capacity, "\txor %s, %s\n", dst_fmt, src2_fmt);
        return;
    }

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\txor %s, %s\n", reg_str, src2_fmt);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_bitwise_rshift(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_BITWISE_RSHIFT) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    const char* shift_instr =
        type_value_is_signed(op.destiny.value_type) ? "sar" : "shr";

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);

    if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        emitf(
            buff, len, capacity, "\t%s %s, %s\n", shift_instr, reg_str, src2_fmt
        );
    } else {
        uint8_t src2_size = type_value_width_bytes(op.source2.value_type);
        const char* rcx_variant = (src2_size == 8) ? "rcx"
            : (src2_size == 4)                     ? "ecx"
            : (src2_size == 2)                     ? "cx"
                                                   : "cl";

        emitf(buff, len, capacity, "\tmov %s, %s\n", rcx_variant, src2_fmt);
        emitf(buff, len, capacity, "\t%s %s, cl\n", shift_instr, reg_str);
    }

    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_bitwise_lshift(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_BITWISE_LSHIFT) return;

    char src1_fmt[256], src2_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.source2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);

    if (op.source2.type == SF_OPERAND_TYPE_IMMEDIATE) {
        emitf(buff, len, capacity, "\tshl %s, %s\n", reg_str, src2_fmt);
    } else {
        uint8_t src2_size = type_value_width_bytes(op.source2.value_type);
        const char* rcx_variant = (src2_size == 8) ? "rcx"
            : (src2_size == 4)                     ? "ecx"
            : (src2_size == 2)                     ? "cx"
                                                   : "cl";

        emitf(buff, len, capacity, "\tmov %s, %s\n", rcx_variant, src2_fmt);
        emitf(buff, len, capacity, "\tshl %s, cl\n", reg_str);
    }

    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_bitwise_not(
    char** buff,
    size_t* len,
    size_t* capacity,
    sf_operation op,
    const sf_stack_map* map
) {
    if (op.opcode != SF_OPCODE_BITWISE_NOT) return;

    char src1_fmt[256], dst_fmt[256];

    format_operand(src1_fmt, sizeof(src1_fmt), op.source1, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.destiny, map);

    if (strcmp(dst_fmt, src1_fmt) == 0) {
        emitf(buff, len, capacity, "\tneg %s\n", dst_fmt);
        return;
    }

    uint8_t size = type_value_width_bytes(op.destiny.value_type);
    const char* reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\tnot %s\n", reg_str);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
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
        case SF_REGISTER_AL:
            return "al";
        case SF_REGISTER_AX:
            return "ax";
        case SF_REGISTER_EAX:
            return "eax";
        case SF_REGISTER_RAX:
            return "rax";
        default:
            return "rax";
    }
}

static const char* prefix_to_string(sf_size_prefix prefix) {
    switch (prefix) {
        case SF_PREFIX_BYTE:
            return "BYTE";
        case SF_PREFIX_WORD:
            return "WORD";
        case SF_PREFIX_DWORD:
            return "DWORD";
        case SF_PREFIX_QWORD:
            return "QWORD";
        default:
            return "QWORD";
    }
}

static sf_stack_offset_size_t next_aligned_offset(
    sf_stack_offset_size_t current, uint8_t size
) {
    if (size == 0) return 0;

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