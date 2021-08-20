#ifndef ppu_h
#define ppu_h

#ifndef AGNES_SINGLE_HEADER
#include "common.h"
#endif

typedef struct agnes agnes_t;
typedef struct ppu ppu_t;

AGNES_INTERNAL void ppu_init(ppu_t *ppu, agnes_t *agnes);
AGNES_INTERNAL void ppu_tick(ppu_t *ppu, bool *out_new_frame);
AGNES_INTERNAL uint8_t ppu_read_register(ppu_t *ppu, uint16_t reg);
AGNES_INTERNAL void ppu_write_register(ppu_t *ppu, uint16_t addr, uint8_t val);

#endif /* ppu_h */
