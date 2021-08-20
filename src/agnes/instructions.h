#ifndef opcodes_h
#define opcodes_h

#ifndef AGNES_SINGLE_HEADER
#include "common.h"
#endif

typedef enum {
    ADDR_MODE_NONE = 0,
    ADDR_MODE_ABSOLUTE,
    ADDR_MODE_ABSOLUTE_X,
    ADDR_MODE_ABSOLUTE_Y,
    ADDR_MODE_ACCUMULATOR,
    ADDR_MODE_IMMEDIATE,
    ADDR_MODE_IMPLIED,
    ADDR_MODE_IMPLIED_BRK,
    ADDR_MODE_INDIRECT,
    ADDR_MODE_INDIRECT_X,
    ADDR_MODE_INDIRECT_Y,
    ADDR_MODE_RELATIVE,
    ADDR_MODE_ZERO_PAGE,
    ADDR_MODE_ZERO_PAGE_X,
    ADDR_MODE_ZERO_PAGE_Y
} addr_mode_t;

typedef struct cpu cpu_t;

typedef int (*instruction_op_fn)(cpu_t *cpu, uint16_t addr, addr_mode_t mode);

typedef struct {
    const char *name;
    uint8_t opcode;
    uint8_t cycles;
    bool page_cross_cycle;
    addr_mode_t mode;
    instruction_op_fn operation;
} instruction_t;

AGNES_INTERNAL instruction_t* instruction_get(uint8_t opcode);
AGNES_INTERNAL uint8_t instruction_get_size(addr_mode_t mode);

#endif /* opcodes_h */
