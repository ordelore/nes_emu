#ifndef cpu_h
#define cpu_h

#ifndef AGNES_SINGLE_HEADER
#include "common.h"
#endif

typedef struct agnes agnes_t;
typedef struct cpu cpu_t;

AGNES_INTERNAL void cpu_init(cpu_t *cpu, agnes_t *agnes);
AGNES_INTERNAL int cpu_tick(cpu_t *cpu);
AGNES_INTERNAL void cpu_update_zn_flags(cpu_t *cpu, uint8_t val);
AGNES_INTERNAL void cpu_stack_push8(cpu_t *cpu, uint8_t val);
AGNES_INTERNAL void cpu_stack_push16(cpu_t *cpu, uint16_t val);
AGNES_INTERNAL uint8_t cpu_stack_pop8(cpu_t *cpu);
AGNES_INTERNAL uint16_t cpu_stack_pop16(cpu_t *cpu);
AGNES_INTERNAL uint8_t cpu_get_flags(const cpu_t *cpu);
AGNES_INTERNAL void cpu_restore_flags(cpu_t *cpu, uint8_t flags);
AGNES_INTERNAL void cpu_set_dma_stall(cpu_t *cpu);
AGNES_INTERNAL void cpu_trigger_nmi(cpu_t *cpu);
AGNES_INTERNAL void cpu_trigger_irq(cpu_t *cpu);
AGNES_INTERNAL void cpu_write8(cpu_t *cpu, uint16_t addr, uint8_t val);
AGNES_INTERNAL uint8_t cpu_read8(cpu_t *cpu, uint16_t addr);
AGNES_INTERNAL uint16_t cpu_read16(cpu_t *cpu, uint16_t addr);

#endif /* cpu_h */
