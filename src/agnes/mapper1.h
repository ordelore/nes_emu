#ifndef mapper1_h
#define mapper1_h

#ifndef AGNES_SINGLE_HEADER
#include "common.h"
#endif

typedef struct mapper1 mapper1_t;
typedef struct agnes agnes_t;

AGNES_INTERNAL void mapper1_init(mapper1_t *mapper, agnes_t *agnes);
AGNES_INTERNAL uint8_t mapper1_read(mapper1_t *mapper, uint16_t addr);
AGNES_INTERNAL void mapper1_write(mapper1_t *mapper, uint16_t addr, uint8_t val);

#endif /* mapper1_h */
