#ifndef mapper4_h
#define mapper4_h

#ifndef AGNES_SINGLE_HEADER
#include "common.h"
#endif

typedef struct mapper4 mapper4_t;
typedef struct agnes agnes_t;

AGNES_INTERNAL void mapper4_init(mapper4_t *mapper, agnes_t *agnes);
AGNES_INTERNAL uint8_t mapper4_read(mapper4_t *mapper, uint16_t addr);
AGNES_INTERNAL void mapper4_write(mapper4_t *mapper, uint16_t addr, uint8_t val);
AGNES_INTERNAL void mapper4_pa12_rising_edge(mapper4_t *mapper);

#endif /* mapper4_h */
