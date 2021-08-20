#ifndef mapper2_h
#define mapper2_h

#ifndef AGNES_SINGLE_HEADER
#include "common.h"
#endif

typedef struct mapper2 mapper2_t;
typedef struct agnes agnes_t;

AGNES_INTERNAL void mapper2_init(mapper2_t *mapper, agnes_t *agnes);
AGNES_INTERNAL uint8_t mapper2_read(mapper2_t *mapper, uint16_t addr);
AGNES_INTERNAL void mapper2_write(mapper2_t *mapper, uint16_t addr, uint8_t val);

#endif /* mapper2_h */
