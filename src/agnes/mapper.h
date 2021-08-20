#ifndef mapper_h
#define mapper_h

#ifndef AGNES_SINGLE_HEADER
#include "common.h"
#endif

typedef struct agnes agnes_t;

AGNES_INTERNAL bool mapper_init(agnes_t *agnes);
AGNES_INTERNAL uint8_t mapper_read(agnes_t *agnes, uint16_t addr);
AGNES_INTERNAL void mapper_write(agnes_t *agnes, uint16_t addr, uint8_t val);
AGNES_INTERNAL void mapper_pa12_rising_edge(agnes_t *agnes);

#endif /* mapper_h */
