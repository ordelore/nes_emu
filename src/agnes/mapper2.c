#ifndef AGNES_SINGLE_HEADER
#include "mapper2.h"
#include "agnes_types.h"
#endif

void mapper2_init(mapper2_t *mapper, agnes_t *agnes) {
    mapper->agnes = agnes;
    mapper->prg_bank_offsets[0] = 0;
    mapper->prg_bank_offsets[1] = (agnes->gamepack.prg_rom_banks_count - 1) * (16 * 1024);
}

uint8_t mapper2_read(mapper2_t *mapper, uint16_t addr) {
    uint8_t res = 0;
    if (addr < 0x2000) {
        res = mapper->chr_ram[addr];
    } else if (addr >= 0x8000) {
        int bank = (addr >> 14) & 0x1;
        unsigned bank_offset = mapper->prg_bank_offsets[bank];
        unsigned addr_offset = addr & 0x3fff;
        unsigned offset = mapper->agnes->gamepack.prg_rom_offset + bank_offset + addr_offset;
        res = mapper->agnes->gamepack.data[offset];
    }
    return res;
}

void mapper2_write(mapper2_t *mapper, uint16_t addr, uint8_t val) {
    if (addr < 0x2000) {
        mapper->chr_ram[addr] = val;
    } else if (addr >= 0x8000) {
        int bank = val % (mapper->agnes->gamepack.prg_rom_banks_count);
        mapper->prg_bank_offsets[0] = bank * (16 * 1024);
    }
}
