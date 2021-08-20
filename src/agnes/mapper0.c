#ifndef AGNES_SINGLE_HEADER
#include "mapper0.h"

#include "agnes_types.h"
#endif

void mapper0_init(mapper0_t *mapper, agnes_t *agnes) {
    mapper->agnes = agnes;

    mapper->prg_bank_offsets[0] = 0;
    mapper->prg_bank_offsets[1] = agnes->gamepack.prg_rom_banks_count > 1 ? (16 * 1024) : 0;
    mapper->use_chr_ram = agnes->gamepack.chr_rom_banks_count == 0;
}

uint8_t mapper0_read(mapper0_t *mapper, uint16_t addr) {
    uint8_t res = 0;
    if (addr < 0x2000) {
        if (mapper->use_chr_ram) {
            res = mapper->chr_ram[addr];
        } else {
            res = mapper->agnes->gamepack.data[mapper->agnes->gamepack.chr_rom_offset + addr];
        }
    } else if (addr >= 0x8000) {
        int bank = addr >> 14 & 0x1;
        unsigned bank_offset = mapper->prg_bank_offsets[bank];
        unsigned addr_offset = addr & 0x3fff;
        unsigned offset = mapper->agnes->gamepack.prg_rom_offset + bank_offset + addr_offset;
        res = mapper->agnes->gamepack.data[offset];
    }
    return res;
}

void mapper0_write(mapper0_t *mapper, uint16_t addr, uint8_t val) {
    if (mapper->use_chr_ram && addr < 0x2000) {
        mapper->chr_ram[addr] = val;
    }
}
