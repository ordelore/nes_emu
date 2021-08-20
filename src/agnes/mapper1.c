#ifndef AGNES_SINGLE_HEADER
#include "mapper1.h"

#include "agnes_types.h"
#endif

static void mapper1_write_control(mapper1_t *mapper, uint8_t val);
static void mapper1_set_offsets(mapper1_t *mapper);

void mapper1_init(mapper1_t *mapper, agnes_t *agnes) {
    mapper->agnes = agnes;
    
    mapper->shift = 0;
    mapper->shift_count = 0;
    mapper->control = 0;
    mapper->prg_mode = 3;
    mapper->chr_mode = 0;
    mapper->chr_banks[0] = 0;
    mapper->chr_banks[1] = 0;
    mapper->prg_bank = 0;
    mapper->use_chr_ram = agnes->gamepack.chr_rom_banks_count == 0;

    mapper1_set_offsets(mapper);
}

uint8_t mapper1_read(mapper1_t *mapper, uint16_t addr) {

    uint8_t res = 0;
    if (addr < 0x2000) {
        if (mapper->use_chr_ram) {
            res = mapper->chr_ram[addr];
        } else {
            unsigned bank = (addr >> 12) & 0x1;
            unsigned bank_offset = mapper->chr_bank_offsets[bank];
            unsigned addr_offset = addr & 0xfff;
            unsigned offset = mapper->agnes->gamepack.chr_rom_offset + bank_offset + addr_offset;
            res = mapper->agnes->gamepack.data[offset];
        }
    } else if (addr >= 0x6000 && addr < 0x8000) {
        res = mapper->prg_ram[addr - 0x6000];
    } else if (addr >= 0x8000) {
        int bank = (addr >> 14) & 0x1;
        unsigned bank_offset = mapper->prg_bank_offsets[bank];
        unsigned addr_offset = addr & 0x3fff;
        unsigned offset = mapper->agnes->gamepack.prg_rom_offset + bank_offset + addr_offset;
        res = mapper->agnes->gamepack.data[offset];
    }
    return res;
}

void mapper1_write(mapper1_t *mapper, uint16_t addr, uint8_t val) {

    if (addr < 0x2000) {
        if (mapper->use_chr_ram) {
            mapper->chr_ram[addr] = val;
        }
    } else if (addr >= 0x6000 && addr < 0x8000) {
        mapper->prg_ram[addr - 0x6000] = val;
    } else if (addr >= 0x8000) {
        if (AGNES_GET_BIT(val, 7)) {
            mapper->shift = 0;
            mapper->shift_count = 0;
            mapper1_write_control(mapper,  mapper->control | 0x0c);
            mapper1_set_offsets(mapper);
        } else {
            mapper->shift >>= 1;
            mapper->shift = mapper->shift | ((val & 0x1) << 4);
            mapper->shift_count++;
            if (mapper->shift_count == 5) {
                uint8_t shift_val = mapper->shift & 0x1f;
                mapper->shift = 0;
                mapper->shift_count = 0;
                uint8_t reg = (addr >> 13) & 0x3; // bits 13 and 14 select register
                switch (reg) {
                    case 0: mapper1_write_control(mapper, shift_val); break;
                    case 1: mapper->chr_banks[0] = shift_val; break;
                    case 2: mapper->chr_banks[1] = shift_val; break;
                    case 3: mapper->prg_bank = shift_val & 0xf; break;
                }
                mapper1_set_offsets(mapper);
            }
        }
    }
}

static void mapper1_write_control(mapper1_t *mapper, uint8_t val) {
    mapper->control = val;
    switch (val & 0x3) {
        case 0: mapper->agnes->mirroring_mode = MIRRORING_MODE_SINGLE_LOWER; break;
        case 1: mapper->agnes->mirroring_mode = MIRRORING_MODE_SINGLE_UPPER; break;
        case 2: mapper->agnes->mirroring_mode = MIRRORING_MODE_VERTICAL; break;
        case 3: mapper->agnes->mirroring_mode = MIRRORING_MODE_HORIZONTAL; break;
    }
    mapper->prg_mode = (val >> 2) & 0x3;
    mapper->chr_mode = (val >> 4) & 0x1;
}

static void mapper1_set_offsets(mapper1_t *mapper) {
    switch (mapper->chr_mode) {
        case 0: {
            mapper->chr_bank_offsets[0] = (mapper->chr_banks[0] & 0xfe) * (8 * 1024);
            mapper->chr_bank_offsets[1] = (mapper->chr_banks[0] & 0xfe) * (8 * 1024) + (4 * 1024);
            break;
        }
        case 1: {
            mapper->chr_bank_offsets[0] = mapper->chr_banks[0] * (4 * 1024);
            mapper->chr_bank_offsets[1] = mapper->chr_banks[1] * (4 * 1024);
            break;
        }
    }

    switch (mapper->prg_mode) {
        case 0: case 1: {
            mapper->prg_bank_offsets[0] = (mapper->prg_bank & 0xe) * (32 * 1024);
            mapper->prg_bank_offsets[1] = (mapper->prg_bank & 0xe) * (32 * 1024) + (16 * 1024);
            break;
        }
        case 2: {
            mapper->prg_bank_offsets[0] = 0;
            mapper->prg_bank_offsets[1] = mapper->prg_bank * (16 * 1024);
            break;
        }
        case 3: {
            mapper->prg_bank_offsets[0] = mapper->prg_bank * (16 * 1024);
            mapper->prg_bank_offsets[1] = (mapper->agnes->gamepack.prg_rom_banks_count - 1) * (16 * 1024);
            break;
        }
    }
}
