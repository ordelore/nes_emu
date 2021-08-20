#ifndef mapper0_h
#define mapper0_h

#ifndef AGNES_SINGLE_HEADER
#include "common.h"
#endif

typedef struct mapper0 mapper0_t;
typedef struct agnes agnes_t;

AGNES_INTERNAL void mapper0_init(mapper0_t *mapper, agnes_t *agnes);
AGNES_INTERNAL uint8_t mapper0_read(mapper0_t *mapper, uint16_t addr);
AGNES_INTERNAL void mapper0_write(mapper0_t *mapper, uint16_t addr, uint8_t val);

#endif /* mapper0_h */
