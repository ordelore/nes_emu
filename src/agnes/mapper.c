#ifndef AGNES_SINGLE_HEADER
#include "mapper0.h"

#include "agnes_types.h"

#include "mapper0.h"
#include "mapper1.h"
#include "mapper2.h"
#include "mapper4.h"
#endif

bool mapper_init(agnes_t *agnes) {
    switch (agnes->gamepack.mapper) {
        case 0: mapper0_init(&agnes->mapper.m0, agnes); return true;
        case 1: mapper1_init(&agnes->mapper.m1, agnes); return true;
        case 2: mapper2_init(&agnes->mapper.m2, agnes); return true;
        case 4: mapper4_init(&agnes->mapper.m4, agnes); return true;
        default: return false;
    }
}

uint8_t mapper_read(agnes_t *agnes, uint16_t addr) {
    switch (agnes->gamepack.mapper) {
        case 0: return mapper0_read(&agnes->mapper.m0, addr);
        case 1: return mapper1_read(&agnes->mapper.m1, addr);
        case 2: return mapper2_read(&agnes->mapper.m2, addr);
        case 4: return mapper4_read(&agnes->mapper.m4, addr);
        default: return 0;
    }
}

void mapper_write(agnes_t *agnes, uint16_t addr, uint8_t val) {
    switch (agnes->gamepack.mapper) {
        case 0: mapper0_write(&agnes->mapper.m0, addr, val); break;
        case 1: mapper1_write(&agnes->mapper.m1, addr, val); break;
        case 2: mapper2_write(&agnes->mapper.m2, addr, val); break;
        case 4: mapper4_write(&agnes->mapper.m4, addr, val); break;
    }
}

void mapper_pa12_rising_edge(agnes_t *agnes) {
    switch (agnes->gamepack.mapper) {
        case 4: mapper4_pa12_rising_edge(&agnes->mapper.m4); break;
    }
}
