#include <string.h>

#ifndef AGNES_SINGLE_HEADER
#include "cpu.h"

#include "ppu.h"
#include "agnes_types.h"
#include "instructions.h"
#include "mapper.h"
#endif

static uint16_t cpu_read16_indirect_bug(cpu_t *cpu, uint16_t addr);
static uint16_t get_instruction_operand(cpu_t *cpu, addr_mode_t mode, bool *out_pages_differ);
static int handle_interrupt(cpu_t *cpu);
static bool check_pages_differ(uint16_t a, uint16_t b);

void cpu_init(cpu_t *cpu, agnes_t *agnes) {
    memset(cpu, 0, sizeof(cpu_t));
    cpu->agnes = agnes;
    cpu->pc = cpu_read16(cpu, 0xfffc); // RESET
    cpu->sp = 0xfd;
    cpu_restore_flags(cpu, 0x24);
}

int cpu_tick(cpu_t *cpu) {
    if (cpu->stall > 0) {
        cpu->stall--;
        return 1;
    }

    int cycles = 0;

    if (cpu->cpu_interrupt != INTERRUPT_NONE) {
        cycles += handle_interrupt(cpu);
    }

    uint8_t opcode = cpu_read8(cpu, cpu->pc);
    instruction_t *ins = instruction_get(opcode);
    if (ins->operation == NULL) {
        return 0;
    }
    
    uint8_t ins_size = instruction_get_size(ins->mode);
    bool page_crossed = false;
    uint16_t addr = get_instruction_operand(cpu, ins->mode, &page_crossed);

    cpu->pc += ins_size;

    cycles += ins->cycles;
    cycles += ins->operation(cpu, addr, ins->mode);

    if (page_crossed && ins->page_cross_cycle) {
        cycles += 1;
    }

    cpu->cycles += cycles;

    return cycles;
}

void cpu_update_zn_flags(cpu_t *cpu, uint8_t val) {
    cpu->flag_zero = val == 0;
    cpu->flag_negative = AGNES_GET_BIT(val, 7);
}

void cpu_stack_push8(cpu_t *cpu, uint8_t val) {
    uint16_t addr = 0x0100 + (uint16_t)(cpu->sp);
    cpu_write8(cpu, addr, val);
    cpu->sp--;
}

void cpu_stack_push16(cpu_t *cpu, uint16_t val) {
    cpu_stack_push8(cpu, val >> 8);
    cpu_stack_push8(cpu, val);
}

uint8_t cpu_stack_pop8(cpu_t *cpu) {
    cpu->sp++;
    uint16_t addr = 0x0100 + (uint16_t)(cpu->sp);
    uint8_t res = cpu_read8(cpu, addr);
    return res;
}

uint16_t cpu_stack_pop16(cpu_t *cpu) {
    uint16_t lo = cpu_stack_pop8(cpu);
    uint16_t hi = cpu_stack_pop8(cpu);
    uint16_t res = (hi << 8) | lo;
    return res;
}

uint8_t cpu_get_flags(const cpu_t *cpu) {
    uint8_t res = 0;
    res |= cpu->flag_carry         << 0;
    res |= cpu->flag_zero          << 1;
    res |= cpu->flag_dis_interrupt << 2;
    res |= cpu->flag_decimal       << 3;
    res |= cpu->flag_overflow      << 6;
    res |= cpu->flag_negative      << 7;
    return res;
}

void cpu_restore_flags(cpu_t *cpu, uint8_t flags) {
    cpu->flag_carry         = AGNES_GET_BIT(flags, 0);
    cpu->flag_zero          = AGNES_GET_BIT(flags, 1);
    cpu->flag_dis_interrupt = AGNES_GET_BIT(flags, 2);
    cpu->flag_decimal       = AGNES_GET_BIT(flags, 3);
    cpu->flag_overflow      = AGNES_GET_BIT(flags, 6);
    cpu->flag_negative      = AGNES_GET_BIT(flags, 7);
}

void cpu_trigger_nmi(cpu_t *cpu) {
    cpu->cpu_interrupt = INTERRUPT_NMI;
}

void cpu_trigger_irq(cpu_t *cpu) {
    if (!cpu->flag_dis_interrupt) {
        cpu->cpu_interrupt = INTERRUPT_IRQ;
    }
}

void cpu_set_dma_stall(cpu_t *cpu) {
    cpu->stall = (cpu->cycles & 0x1) ? 514 : 513;
}

void cpu_write8(cpu_t *cpu, uint16_t addr, uint8_t val) {
    agnes_t *agnes = cpu->agnes;

    if (addr < 0x2000) {
        agnes->ram[addr & 0x7ff] = val;
    } else if (addr < 0x4000) {
        ppu_write_register(&agnes->ppu, 0x2000 | (addr & 0x7), val);
    } else if (addr == 0x4014) {
        ppu_write_register(&agnes->ppu, 0x4014, val);
    } else if (addr == 0x4016) {
        agnes->controllers_latch = val & 0x1;
        if (agnes->controllers_latch) {
            agnes->controllers[0].shift = agnes->controllers[0].state;
            agnes->controllers[1].shift = agnes->controllers[1].state;
        }
    } else {
        mapper_write(agnes, addr, val);
    }
}

uint8_t cpu_read8(cpu_t *cpu, uint16_t addr) {
    agnes_t *agnes = cpu->agnes;

    uint8_t res = 0;
    if (addr >= 0x4020) { // moved to top because it's the most common case
        res = mapper_read(agnes, addr);
    } else if (addr < 0x2000) {
        res = agnes->ram[addr & 0x7ff];
    } else if (addr < 0x4000) {
        res = ppu_read_register(&agnes->ppu, 0x2000 | (addr & 0x7));
    } else if (addr < 0x4016) {
        // apu
    } else if (addr < 0x4018) {
        int controller = addr & 0x1; // 0: 0x4016, 1: 0x4017
        if (agnes->controllers_latch) {
            agnes->controllers[controller].shift = agnes->controllers[controller].state;
        }
        res = agnes->controllers[controller].shift & 0x1;
        agnes->controllers[controller].shift >>= 1;
    }
    return res;
}

uint16_t cpu_read16(cpu_t *cpu, uint16_t addr) {
    uint8_t lo = cpu_read8(cpu, addr);
    uint8_t hi = cpu_read8(cpu, addr + 1);
    return (hi << 8) | lo;
}

static uint16_t cpu_read16_indirect_bug(cpu_t *cpu, uint16_t addr) {
    uint8_t lo = cpu_read8(cpu, addr);
    uint8_t hi = cpu_read8(cpu, (addr & 0xff00) | ((addr + 1) & 0x00ff));
    return (hi << 8) | lo;
}

static uint16_t get_instruction_operand(cpu_t *cpu, addr_mode_t mode, bool *out_pages_differ) {
    *out_pages_differ = false;
    switch (mode) {
        case ADDR_MODE_ABSOLUTE: {
            return cpu_read16(cpu, cpu->pc + 1);
        }
        case ADDR_MODE_ABSOLUTE_X: {
            uint16_t addr = cpu_read16(cpu, cpu->pc + 1);
            uint16_t res = addr + cpu->x;
            *out_pages_differ = check_pages_differ(addr, res);
            return res;
        }
        case ADDR_MODE_ABSOLUTE_Y: {
            uint16_t addr = cpu_read16(cpu, cpu->pc + 1);
            uint16_t res = addr + cpu->y;
            *out_pages_differ = check_pages_differ(addr, res);
            return res;
        }
        case ADDR_MODE_IMMEDIATE: {
            return cpu->pc + 1;
        }
        case ADDR_MODE_INDIRECT: {
            uint16_t addr = cpu_read16(cpu, cpu->pc + 1);
            return cpu_read16_indirect_bug(cpu, addr);
        }
        case ADDR_MODE_INDIRECT_X: {
            uint8_t addr = cpu_read8(cpu, (cpu->pc + 1));
            return cpu_read16_indirect_bug(cpu, (addr + cpu->x) & 0xff);
        }
        case ADDR_MODE_INDIRECT_Y: {
            uint8_t arg = cpu_read8(cpu, cpu->pc + 1);
            uint16_t addr2 = cpu_read16_indirect_bug(cpu, arg);
            uint16_t res = addr2 + cpu->y;
            *out_pages_differ = check_pages_differ(addr2, res);
            return res;
        }
        case ADDR_MODE_ZERO_PAGE: {
            return cpu_read8(cpu, cpu->pc + 1);
        }
        case ADDR_MODE_ZERO_PAGE_X: {
            return (cpu_read8(cpu, cpu->pc + 1) + cpu->x) & 0xff;
        }
        case ADDR_MODE_ZERO_PAGE_Y: {
            return (cpu_read8(cpu, cpu->pc + 1) + cpu->y) & 0xff;
        }
        case ADDR_MODE_RELATIVE: {
            uint8_t addr = cpu_read8(cpu, cpu->pc + 1);
            uint16_t ret_val = cpu->pc + addr + 2;
            if (addr < 0x80) {
                return ret_val;
            } else {
                return ret_val - 0x100;
            }
        }
        default: {
            return 0;
        }
    }
}

static int handle_interrupt(cpu_t *cpu) {
    uint16_t addr = 0;
    if (cpu->cpu_interrupt == INTERRUPT_NMI) {
        addr = 0xfffa;
    } else if (cpu->cpu_interrupt == INTERRUPT_IRQ) {
        addr = 0xfffe;
    } else {
        return 0;
    }
    cpu->cpu_interrupt = INTERRUPT_NONE;
    cpu_stack_push16(cpu, cpu->pc);
    uint8_t flags = cpu_get_flags(cpu);
    cpu_stack_push8(cpu, flags | 0x20);
    cpu->pc = cpu_read16(cpu, addr);
    cpu->flag_dis_interrupt = true;
    return 7;
}

static bool check_pages_differ(uint16_t a, uint16_t b) {
    return (0xff00 & a) != (0xff00 & b);
}
