#include "sulfur/pipeline/backend/codegen/codegen.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sulfur/pipeline/backend/codegen/stack.h"
#include "sulfur/pipeline/backend/ir/ir.h"
#include "sulfur/utils/log.h"
#include "sulfur/utils/string.h"
#include "sulfur/utils/type_utils.h"

#define FMT_SIZE 256

typedef enum {
    SF_REGISTER_RAX,
    SF_REGISTER_EAX,
    SF_REGISTER_AX,
    SF_REGISTER_AL,
} sf_register;

typedef enum {
    SF_PREFIX_BYTE,
    SF_PREFIX_WORD,
    SF_PREFIX_DWORD,
    SF_PREFIX_QWORD,
} sf_size_prefix;

static const size_t k_initial_capacity = 4096;

static void format_operand(
    char *dst_str, size_t max_len, sf_operand op, const sf_stack_map *map
);

static void emitf(
    char **buff, size_t *len, size_t *capacity, const char *format, ...
);

static sf_stack_offset_size_t stack_offset(
    const sf_stack_map *map, const char *name
);
static void load_large_immediate(
    char **buff, size_t *len, size_t *capacity, sf_operand operand,
    uint8_t size, char *fmt, size_t fmt_size
);

static void emit_binary(
    char **buff, size_t *len, size_t *capacity, const char *mnemonic,
    sf_operation op, const sf_stack_map *map
);
static void emit_mult(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);
static void emit_div(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);
static void emit_dividend_extension(
    char **buff, size_t *len, size_t *capacity, uint8_t size, bool is_signed
);
static void emit_neg(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);
static void emit_not(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);
static void emit_not_on(
    char **buff, size_t *len, size_t *capacity, sf_opcode opcode, const char *target
);

static void emit_shift(
    char **buff, size_t *len, size_t *capacity, const char *mnemonic,
    sf_operation op, const sf_stack_map *map
);
static void emit_rshift(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);
static void emit_lshift(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);

static void emit_convert(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const char *src_fmt, const char *dst_fmt
);
static void emit_assign(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);
static void emit_cast(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);

static void emit_relational(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);

static void emit_label(char **buff, size_t *len, size_t *capacity, sf_operation op);
static void emit_jmp_incond(
    char **buff, size_t *len, size_t *capacity, sf_operation op
);
static void emit_jmp_cond(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);

static void emit_return(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
);

static void emit_function(
    char **buff, size_t *len, size_t *capacity, const sf_ir_function *func
);

static void emit_operations(
    char **buff, size_t *len, size_t *capacity,
    const sf_operation *operations, size_t count, const sf_stack_map *map
);

static sf_stack_offset_size_t frame_size(const sf_stack_map *map);

static sf_register register_from_size(uint8_t size);
static sf_size_prefix prefix_from_size(uint8_t size);
static const char *counter_register(uint8_t size);

static const char *register_to_string(sf_register reg);
static const char *prefix_to_string(sf_size_prefix prefix);

char *sf_generate_assembly(const sf_ir_program *program) {
    if (program == NULL) {
        return NULL;
    }

    size_t as_capacity = k_initial_capacity;
    size_t as_len = 0;
    char *as = malloc(as_capacity);

    if (as == NULL) {
        return NULL;
    }

    as[0] = '\0';

    sf_string_push("bits 64\n\n", &as, &as_len, &as_capacity);
    sf_string_push("section .data\n", &as, &as_len, &as_capacity);

    sf_string_push("section .text\n", &as, &as_len, &as_capacity);
    sf_string_push("\tglobal _start\n\n", &as, &as_len, &as_capacity);

    sf_string_push("_start:\n", &as, &as_len, &as_capacity);
    sf_string_push("\tcall sf_main\n", &as, &as_len, &as_capacity);
    sf_string_push("\tmov edi, eax\n", &as, &as_len, &as_capacity);
    sf_string_push("\tmov rax, 60\n", &as, &as_len, &as_capacity);
    sf_string_push("\tsyscall\n", &as, &as_len, &as_capacity);

    for (size_t i = 0; i < program->function_count; i++) {
        sf_string_push("\n", &as, &as_len, &as_capacity);
        emit_function(&as, &as_len, &as_capacity, &program->functions[i]);
    }

    return as;
}

static void format_operand(
    char *dst_str, size_t max_len, sf_operand op, const sf_stack_map *map
) {
    dst_str[0] = '\0';

    uint8_t size = sf_type_value_width_bytes(op.value_type);
    const char *prefix = prefix_to_string(prefix_from_size(size));

    switch (op.type) {
        case SF_OPERAND_TYPE_IMMEDIATE: {
            snprintf(dst_str, max_len, "%s", op.immediate_value);
            break;
        }
        case SF_OPERAND_TYPE_VARIABLE: {
            snprintf(
                dst_str, max_len, "%s [rbp%" PRId64 "]", prefix,
                stack_offset(map, op.variable_name)
            );
            break;
        }
        case SF_OPERAND_TYPE_TEMPORARY: {
            char temp_name[32];
            snprintf(temp_name, sizeof(temp_name), "t%zu", op.temporary_id);

            snprintf(
                dst_str, max_len, "%s [rbp%" PRId64 "]", prefix,
                stack_offset(map, temp_name)
            );
            break;
        }

        default: {
            break;
        }
    }
}

static void emitf(
    char **buff, size_t *len, size_t *capacity, const char *format, ...
) {
    char stack_buffer[512];

    va_list args;
    va_list args_copy;
    va_start(args, format);
    va_copy(args_copy, args);

    int needed = vsnprintf(stack_buffer, sizeof(stack_buffer), format, args);

    va_end(args);

    if (needed < 0) {
        va_end(args_copy);

        return;
    }

    if ((size_t)needed < sizeof(stack_buffer)) {
        va_end(args_copy);
        sf_string_push(stack_buffer, buff, len, capacity);

        return;
    }

    char *heap_buffer = malloc((size_t)needed + 1);

    if (heap_buffer == NULL) {
        va_end(args_copy);

        sf_log(
            "Insufficient Memory.", "Cannot allocate memory for compiling.",
            "Free some memory and try again.", NULL,
            SF_LOG_GENERAL_INSUFFICIENT_MEMORY, (sf_span){0}, SF_LOG_SEVERITY_FATAL
        );

        return;
    }

    vsnprintf(heap_buffer, (size_t)needed + 1, format, args_copy);
    va_end(args_copy);

    sf_string_push(heap_buffer, buff, len, capacity);
    free(heap_buffer);
}

static sf_stack_offset_size_t stack_offset(
    const sf_stack_map *map, const char *name
) {
    sf_stack_offset_size_t offset = sf_stack_lookup(map, name);

    if (offset == 0) {
        fprintf(
            stderr,
            "internal compiler error: '%s' has no stack slot "
            "(this is a bug in the codegen, not in the user's code)\n",
            name
        );

        abort();
    }

    return offset;
}

// 64-bit immediates outside the signed 32-bit range cannot be encoded in most
// instructions, so they are loaded into rcx and the operand text becomes "rcx".
static void load_large_immediate(
    char **buff, size_t *len, size_t *capacity, sf_operand operand,
    uint8_t size, char *fmt, size_t fmt_size
) {
    if (operand.type != SF_OPERAND_TYPE_IMMEDIATE || size < 8) {
        return;
    }

    long long value = strtoll(operand.immediate_value, NULL, 10);

    if (value >= INT32_MIN && value <= INT32_MAX) {
        return;
    }

    emitf(buff, len, capacity, "\tmov rcx, %s\n", fmt);
    snprintf(fmt, fmt_size, "rcx");
}

static void emit_binary(
    char **buff, size_t *len, size_t *capacity, const char *mnemonic,
    sf_operation op, const sf_stack_map *map
) {
    char src1_fmt[FMT_SIZE], src2_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src1_fmt, sizeof(src1_fmt), op.operand2, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.operand3, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    uint8_t size = sf_type_value_width_bytes(op.operand1.value_type);

    load_large_immediate(
        buff, len, capacity, op.operand3, size, src2_fmt, sizeof(src2_fmt)
    );

    bool src2_in_memory = op.operand3.type == SF_OPERAND_TYPE_VARIABLE ||
                          op.operand3.type == SF_OPERAND_TYPE_TEMPORARY;

    if (strcmp(dst_fmt, src1_fmt) == 0 && !src2_in_memory) {
        emitf(buff, len, capacity, "\t%s %s, %s\n", mnemonic, dst_fmt, src2_fmt);
        return;
    }

    const char *reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\t%s %s, %s\n", mnemonic, reg_str, src2_fmt);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_mult(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    char src1_fmt[FMT_SIZE], src2_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src1_fmt, sizeof(src1_fmt), op.operand2, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.operand3, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    uint8_t size = sf_type_value_width_bytes(op.operand1.value_type);

    if (size == 1) {
        const char *mul_instr =
            sf_type_value_is_signed(op.operand1.value_type) ? "imul" : "mul";

        emitf(buff, len, capacity, "\tmov al, %s\n", src1_fmt);

        // The one-operand mul/imul forms do not accept immediates.
        if (op.operand3.type == SF_OPERAND_TYPE_IMMEDIATE) {
            emitf(buff, len, capacity, "\tmov cl, %s\n", src2_fmt);
            emitf(buff, len, capacity, "\t%s cl\n", mul_instr);
        } else {
            emitf(buff, len, capacity, "\t%s %s\n", mul_instr, src2_fmt);
        }

        emitf(buff, len, capacity, "\tmov %s, al\n", dst_fmt);
        return;
    }

    load_large_immediate(
        buff, len, capacity, op.operand3, size, src2_fmt, sizeof(src2_fmt)
    );

    const char *reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\timul %s, %s\n", reg_str, src2_fmt);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_div(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    char src1_fmt[FMT_SIZE], src2_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src1_fmt, sizeof(src1_fmt), op.operand2, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.operand3, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    uint8_t size = sf_type_value_width_bytes(op.operand1.value_type);
    bool is_signed = sf_type_value_is_signed(op.operand1.value_type);
    const char *div_instr = is_signed ? "idiv" : "div";

    const char *reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emit_dividend_extension(buff, len, capacity, size, is_signed);

    if (op.operand3.type == SF_OPERAND_TYPE_IMMEDIATE) {
        const char *reg_cx = counter_register(size);

        emitf(buff, len, capacity, "\tmov %s, %s\n", reg_cx, src2_fmt);
        emitf(buff, len, capacity, "\t%s %s\n", div_instr, reg_cx);
    } else {
        emitf(buff, len, capacity, "\t%s %s\n", div_instr, src2_fmt);
    }

    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_dividend_extension(
    char **buff, size_t *len, size_t *capacity, uint8_t size, bool is_signed
) {
    switch (size) {
        case 8: {
            emitf(buff, len, capacity, is_signed ? "\tcqo\n" : "\txor rdx, rdx\n");
            break;
        }
        case 4: {
            emitf(buff, len, capacity, is_signed ? "\tcdq\n" : "\txor edx, edx\n");
            break;
        }
        case 2: {
            emitf(buff, len, capacity, is_signed ? "\tcwd\n" : "\txor dx, dx\n");
            break;
        }
        case 1: {
            emitf(buff, len, capacity, is_signed ? "\tcbw\n" : "\tmov ah, 0\n");
            break;
        }

        default: {
            break;
        }
    }
}

static void emit_neg(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    char src1_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src1_fmt, sizeof(src1_fmt), op.operand2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    if (strcmp(dst_fmt, src1_fmt) == 0) {
        emitf(buff, len, capacity, "\tneg %s\n", dst_fmt);
        return;
    }

    uint8_t size = sf_type_value_width_bytes(op.operand1.value_type);
    const char *reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\tneg %s\n", reg_str);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_not(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    char src1_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src1_fmt, sizeof(src1_fmt), op.operand2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    if (strcmp(dst_fmt, src1_fmt) == 0) {
        emit_not_on(buff, len, capacity, op.opcode, dst_fmt);
        return;
    }

    uint8_t size = sf_type_value_width_bytes(op.operand1.value_type);
    const char *reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emit_not_on(buff, len, capacity, op.opcode, reg_str);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_not_on(
    char **buff, size_t *len, size_t *capacity, sf_opcode opcode, const char *target
) {
    if (opcode == SF_OPCODE_BITWISE_NOT) {
        emitf(buff, len, capacity, "\tnot %s\n", target);
    } else {
        emitf(buff, len, capacity, "\txor %s, 1\n", target);
    }
}

static void emit_shift(
    char **buff, size_t *len, size_t *capacity, const char *mnemonic,
    sf_operation op, const sf_stack_map *map
) {
    char src1_fmt[FMT_SIZE], src2_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src1_fmt, sizeof(src1_fmt), op.operand2, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.operand3, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    uint8_t size = sf_type_value_width_bytes(op.operand1.value_type);
    const char *reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);

    if (op.operand3.type == SF_OPERAND_TYPE_IMMEDIATE) {
        emitf(buff, len, capacity, "\t%s %s, %s\n", mnemonic, reg_str, src2_fmt);
    } else {
        uint8_t src2_size = sf_type_value_width_bytes(op.operand3.value_type);
        const char *reg_cx = counter_register(src2_size);

        emitf(buff, len, capacity, "\tmov %s, %s\n", reg_cx, src2_fmt);
        emitf(buff, len, capacity, "\t%s %s, cl\n", mnemonic, reg_str);
    }

    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, reg_str);
}

static void emit_rshift(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    const char *mnemonic =
        sf_type_value_is_signed(op.operand1.value_type) ? "sar" : "shr";

    emit_shift(buff, len, capacity, mnemonic, op, map);
}

static void emit_lshift(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    emit_shift(buff, len, capacity, "shl", op, map);
}

static void emit_convert(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const char *src_fmt, const char *dst_fmt
) {
    uint8_t src_size = sf_type_value_width_bytes(op.operand2.value_type);
    uint8_t dst_size = sf_type_value_width_bytes(op.operand1.value_type);

    const char *dst_reg_str = register_to_string(register_from_size(dst_size));

    if (op.operand2.type == SF_OPERAND_TYPE_IMMEDIATE || src_size == dst_size) {
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_reg_str, src_fmt);
        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
        return;
    }

    if (src_size < dst_size) {
        bool is_signed = sf_type_value_is_signed(op.operand2.value_type);

        if (is_signed && src_size == 4) {
            // movsx has no 32-bit source form.
            emitf(buff, len, capacity, "\tmovsxd %s, %s\n", dst_reg_str, src_fmt);
        } else if (is_signed) {
            emitf(buff, len, capacity, "\tmovsx %s, %s\n", dst_reg_str, src_fmt);
        } else if (src_size == 4) {
            // Writing eax already zero-extends into rax.
            const char *eax = register_to_string(SF_REGISTER_EAX);

            emitf(buff, len, capacity, "\tmov %s, %s\n", eax, src_fmt);
        } else {
            emitf(buff, len, capacity, "\tmovzx %s, %s\n", dst_reg_str, src_fmt);
        }

        emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
        return;
    }

    const char *src_reg_str = register_to_string(register_from_size(src_size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", src_reg_str, src_fmt);
    emitf(buff, len, capacity, "\tmov %s, %s\n", dst_fmt, dst_reg_str);
}

static void emit_assign(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    char src_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src_fmt, sizeof(src_fmt), op.operand2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    if (strcmp(src_fmt, dst_fmt) == 0) {
        return;
    }

    emit_convert(buff, len, capacity, op, src_fmt, dst_fmt);
}

static void emit_cast(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    char src_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src_fmt, sizeof(src_fmt), op.operand2, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    emit_convert(buff, len, capacity, op, src_fmt, dst_fmt);
}

static void emit_relational(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    char src1_fmt[FMT_SIZE], src2_fmt[FMT_SIZE], dst_fmt[FMT_SIZE];

    format_operand(src1_fmt, sizeof(src1_fmt), op.operand2, map);
    format_operand(src2_fmt, sizeof(src2_fmt), op.operand3, map);
    format_operand(dst_fmt, sizeof(dst_fmt), op.operand1, map);

    uint8_t size = sf_type_value_width_bytes(op.operand2.value_type);
    const char *reg_str = register_to_string(register_from_size(size));

    load_large_immediate(
        buff, len, capacity, op.operand3, size, src2_fmt, sizeof(src2_fmt)
    );

    bool is_signed = sf_type_value_is_signed(op.operand2.value_type);

    const char *set_instr = NULL;

    switch (op.opcode) {
        case SF_OPCODE_RELATIONAL_EQUAL: {
            set_instr = "sete";
            break;
        }
        case SF_OPCODE_RELATIONAL_NOT_EQUAL: {
            set_instr = "setne";
            break;
        }
        case SF_OPCODE_RELATIONAL_LESS: {
            set_instr = is_signed ? "setl" : "setb";
            break;
        }
        case SF_OPCODE_RELATIONAL_LESS_EQUAL: {
            set_instr = is_signed ? "setle" : "setbe";
            break;
        }
        case SF_OPCODE_RELATIONAL_GREATER: {
            set_instr = is_signed ? "setg" : "seta";
            break;
        }
        case SF_OPCODE_RELATIONAL_GREATER_EQUAL: {
            set_instr = is_signed ? "setge" : "setae";
            break;
        }

        default: {
            fprintf(
                stderr,
                "internal compiler error: unreachable opcode %d in "
                "emit_relational "
                "(this is a bug in the codegen, not in the user's code)\n",
                op.opcode
            );

            abort();
        }
    }

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, src1_fmt);
    emitf(buff, len, capacity, "\tcmp %s, %s\n", reg_str, src2_fmt);
    emitf(buff, len, capacity, "\t%s al\n", set_instr);
    emitf(buff, len, capacity, "\tmov %s, al\n", dst_fmt);
}

static void emit_label(char **buff, size_t *len, size_t *capacity, sf_operation op) {
    emitf(buff, len, capacity, ".L%zu:\n", op.operand1.label_id);
}

static void emit_jmp_incond(
    char **buff, size_t *len, size_t *capacity, sf_operation op
) {
    emitf(buff, len, capacity, "\tjmp .L%zu\n", op.operand1.label_id);
}

static void emit_jmp_cond(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    char cond_fmt[FMT_SIZE];

    format_operand(cond_fmt, sizeof(cond_fmt), op.operand2, map);

    uint8_t size = sf_type_value_width_bytes(op.operand2.value_type);
    const char *reg_str = register_to_string(register_from_size(size));

    emitf(buff, len, capacity, "\tmov %s, %s\n", reg_str, cond_fmt);
    emitf(buff, len, capacity, "\tcmp %s, 0\n", reg_str);
    emitf(buff, len, capacity, "\tje .L%zu\n", op.operand1.label_id);
}

static void emit_return(
    char **buff, size_t *len, size_t *capacity,
    sf_operation op, const sf_stack_map *map
) {
    if (op.operand1.type != SF_OPERAND_TYPE_NONE) {
        uint8_t size = sf_type_value_width_bytes(op.operand1.value_type);
        const char *reg = register_to_string(register_from_size(size));

        char fmt[FMT_SIZE];
        format_operand(fmt, sizeof(fmt), op.operand1, map);

        emitf(buff, len, capacity, "\tmov %s, %s\n", reg, fmt);
    }

    emitf(buff, len, capacity, "\tmov rsp, rbp\n");
    emitf(buff, len, capacity, "\tpop rbp\n");
    emitf(buff, len, capacity, "\tret\n");
}

static void emit_function(
    char **buff, size_t *len, size_t *capacity, const sf_ir_function *func
) {
    sf_stack_map map = {0};
    sf_stack_populate(&map, func->operations, func->operation_count);

    emitf(buff, len, capacity, "sf_%s:\n", func->name);
    emitf(buff, len, capacity, "\tpush rbp\n");
    emitf(buff, len, capacity, "\tmov rbp, rsp\n");
    emitf(buff, len, capacity, "\tsub rsp, %"PRId64"\n", frame_size(&map));

    emit_operations(buff, len, capacity, func->operations, func->operation_count, &map);

    sf_stack_free(&map);
}

static void emit_operations(
    char **buff, size_t *len, size_t *capacity,
    const sf_operation *operations, size_t count, const sf_stack_map *map
) {
    for (size_t i = 0; i < count; i++) {
        sf_operation op = operations[i];

        switch (op.opcode) {
            case SF_OPCODE_ADD: {
                emit_binary(buff, len, capacity, "add", op, map);
                break;
            }
            case SF_OPCODE_SUB: {
                emit_binary(buff, len, capacity, "sub", op, map);
                break;
            }
            case SF_OPCODE_MULT: {
                emit_mult(buff, len, capacity, op, map);
                break;
            }
            case SF_OPCODE_DIV: {
                emit_div(buff, len, capacity, op, map);
                break;
            }
            case SF_OPCODE_NEGATE: {
                emit_neg(buff, len, capacity, op, map);
                break;
            }

            case SF_OPCODE_BITWISE_AND:
            case SF_OPCODE_LOGICAL_AND: {
                emit_binary(buff, len, capacity, "and", op, map);
                break;
            }
            case SF_OPCODE_BITWISE_OR:
            case SF_OPCODE_LOGICAL_OR: {
                emit_binary(buff, len, capacity, "or", op, map);
                break;
            }
            case SF_OPCODE_BITWISE_XOR: {
                emit_binary(buff, len, capacity, "xor", op, map);
                break;
            }
            case SF_OPCODE_BITWISE_RSHIFT: {
                emit_rshift(buff, len, capacity, op, map);
                break;
            }
            case SF_OPCODE_BITWISE_LSHIFT: {
                emit_lshift(buff, len, capacity, op, map);
                break;
            }
            case SF_OPCODE_BITWISE_NOT:
            case SF_OPCODE_LOGICAL_NOT: {
                emit_not(buff, len, capacity, op, map);
                break;
            }

            case SF_OPCODE_RELATIONAL_EQUAL:
            case SF_OPCODE_RELATIONAL_NOT_EQUAL:
            case SF_OPCODE_RELATIONAL_LESS:
            case SF_OPCODE_RELATIONAL_LESS_EQUAL:
            case SF_OPCODE_RELATIONAL_GREATER:
            case SF_OPCODE_RELATIONAL_GREATER_EQUAL: {
                emit_relational(buff, len, capacity, op, map);
                break;
            }

            case SF_OPCODE_LABEL: {
                emit_label(buff, len, capacity, op);
                break;
            }
            case SF_OPCODE_JMP_INCOND: {
                emit_jmp_incond(buff, len, capacity, op);
                break;
            }
            case SF_OPCODE_JMP_COND: {
                emit_jmp_cond(buff, len, capacity, op, map);
                break;
            }

            case SF_OPCODE_ASSIGN: {
                emit_assign(buff, len, capacity, op, map);
                break;
            }
            case SF_OPCODE_CAST: {
                emit_cast(buff, len, capacity, op, map);
                break;
            }

            case SF_OPCODE_RETURN: {
                emit_return(buff, len, capacity, op, map);
                break;
            }

            default: {
                break;
            }
        }

        sf_string_push("\n", buff, len, capacity);
    }
}

static sf_stack_offset_size_t frame_size(const sf_stack_map *map) {
    sf_stack_offset_size_t stack_size = 0;

    for (sf_stack_map_size_t i = 0; i < map->count; i++) {
        sf_stack_offset_size_t abs_offset = -map->entries[i].offset;

        if (abs_offset > stack_size) {
            stack_size = abs_offset;
        }
    }

    stack_size = (stack_size + 15) / 16 * 16;

    return stack_size;
}

static sf_register register_from_size(uint8_t size) {
    if (size <= 1) {
        return SF_REGISTER_AL;
    }

    if (size <= 2) {
        return SF_REGISTER_AX;
    }

    if (size <= 4) {
        return SF_REGISTER_EAX;
    }

    return SF_REGISTER_RAX;
}

static sf_size_prefix prefix_from_size(uint8_t size) {
    if (size <= 1) {
        return SF_PREFIX_BYTE;
    }

    if (size <= 2) {
        return SF_PREFIX_WORD;
    }

    if (size <= 4) {
        return SF_PREFIX_DWORD;
    }

    return SF_PREFIX_QWORD;
}

static const char *counter_register(uint8_t size) {
    if (size <= 1) {
        return "cl";
    }

    if (size <= 2) {
        return "cx";
    }

    if (size <= 4) {
        return "ecx";
    }

    return "rcx";
}

static const char *register_to_string(sf_register reg) {
    switch (reg) {
        case SF_REGISTER_AL: {
            return "al";
        }
        case SF_REGISTER_AX: {
            return "ax";
        }
        case SF_REGISTER_EAX: {
            return "eax";
        }
        case SF_REGISTER_RAX: {
            return "rax";
        }

        default: {
            return "rax";
        }
    }
}

static const char *prefix_to_string(sf_size_prefix prefix) {
    switch (prefix) {
        case SF_PREFIX_BYTE: {
            return "BYTE";
        }
        case SF_PREFIX_WORD: {
            return "WORD";
        }
        case SF_PREFIX_DWORD: {
            return "DWORD";
        }
        case SF_PREFIX_QWORD: {
            return "QWORD";
        }

        default: {
            return "QWORD";
        }
    }
}
