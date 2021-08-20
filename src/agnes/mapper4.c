#ifndef AGNES_SINGLE_HEADER
#include "mapper4.h"

#include "agnes_types.h"
#include "cpu.h"
#endif

static void mapper4_write_register(mapper4_t *mapper, uint16_t addr, uint8_t val);
static void mapper4_set_offsets(mapper4_t *mapper);

void mapper4_init(mapper4_t *mapper, agnes_t *agnes) {
    mapper->agnes = agnes;

    mapper->prg_mode = 0;
    mapper->chr_mode = 0;
    mapper->irq_enabled = false;
    mapper->reg_ix = 0;

    mapper->regs[0] = 0;
    mapper->regs[1] = 2;
    mapper->regs[2] = 4;
    mapper->regs[3] = 5;
    mapper->regs[4] = 6;
    mapper->regs[5] = 7;
    mapper->regs[6] = 0;
    mapper->regs[7] = 1;

    mapper->counter = 0;
    mapper->counter_reload = 0;
    mapper->use_chr_ram = agnes->gamepack.chr_rom_banks_count == 0;

    mapper4_set_offsets(mapper);
}

void mapper4_pa12_rising_edge(mapper4_t *mapper) {
    if (mapper->counter == 0) {
        mapper->counter = mapper->counter_reload;
    } else {
        mapper->counter--;
        if (mapper->counter == 0 && mapper->irq_enabled) {
            cpu_trigger_irq(&mapper->agnes->cpu);
        }
    }
}

uint8_t mapper4_read(mapper4_t *mapper, uint16_t addr) {
    uint8_t res = 0;
    if (addr < 0x2000) {
        int bank = (addr >> 10) & 0x7;
        unsigned bank_offset = mapper->chr_bank_offsets[bank];
        unsigned addr_offset = addr & 0x3ff;
        unsigned offset = bank_offset + addr_offset;
        if (mapper->use_chr_ram) {
            offset = offset & ((8 * 1024) - 1);
            res = mapper->chr_ram[offset];
        } else {
            unsigned chr_rom_size = (mapper->agnes->gamepack.chr_rom_banks_count * 8 * 1024);
            offset = offset % chr_rom_size;
            res = mapper->agnes->gamepack.data[mapper->agnes->gamepack.chr_rom_offset + offset];
        }
    } else if (addr >= 0x6000 && addr < 0x8000) {
        return mapper->prg_ram[addr - 0x6000];
    } else if (addr >= 0x8000) {
        int bank = (addr >> 13) & 0x3;
        unsigned bank_offset = mapper->prg_bank_offsets[bank];
        unsigned addr_offset = addr & 0x1fff;
        unsigned offset = mapper->agnes->gamepack.prg_rom_offset + bank_offset + addr_offset;
        res = mapper->agnes->gamepack.data[offset];
    }
    return res;
}

void mapper4_write(mapper4_t *mapper, uint16_t addr, uint8_t val) {
    if (addr < 0x2000 && mapper->use_chr_ram) {
        int bank = (addr >> 10) & 0x7;
        unsigned bank_offset = mapper->chr_bank_offsets[bank];
        unsigned addr_offset = addr & 0x3ff;
        unsigned full_offset = (bank_offset + addr_offset) & ((8 * 1024) - 1);
        mapper->chr_ram[full_offset] = val;
    } else if (addr >= 0x6000 && addr < 0x8000) {
         mapper->prg_ram[addr - 0x6000] = val;
    } else if (addr >= 0x8000) {
        mapper4_write_register(mapper, addr, val);
    }
}

static void mapper4_write_register(mapper4_t *mapper, uint16_t addr, uint8_t val) {
    bool addr_odd = addr & 0x1;
    bool addr_even = !addr_odd;
    if (addr <= 0x9ffe && addr_even) { // Bank select ($8000-$9FFE, even)
        mapper->reg_ix = val & 0x7;
        mapper->prg_mode = (val >> 6) & 0x1;
        mapper->chr_mode = (val >> 7) & 0x1;
        mapper4_set_offsets(mapper);
    } else if (addr <= 0x9fff && addr_odd) { // Bank data ($8001-$9FFF, odd)
        mapper->regs[mapper->reg_ix] = val;
        mapper4_set_offsets(mapper);
    } else if (addr <= 0xbffe && addr_even) { // Mirroring ($A000-$BFFE, even)
        if (mapper->agnes->mirroring_mode != MIRRORING_MODE_FOUR_SCREEN) {
            mapper->agnes->mirroring_mode = (val & 0x1) ? MIRRORING_MODE_HORIZONTAL : MIRRORING_MODE_VERTICAL;
        }
    } else if (addr <= 0xbfff && addr_odd) { // PRG RAM protect ($A001-$BFFF, odd)
        // probably not required (according to https://wiki.nesdev.com/w/index.php/MMC3)
    } else if (addr <= 0xdffe && addr_even) { // IRQ latch ($C000-$DFFE, even)
        mapper->counter_reload = val;
    } else if (addr <= 0xdfff && addr_odd) { // IRQ reload ($C001-$DFFF, odd)
        mapper->counter = 0;
    } else if (addr <= 0xfffe && addr_even) { // IRQ disable ($E000-$FFFE, even)
        mapper->irq_enabled = false;
    } else if (addr <= 0xffff && addr_odd) { // IRQ enable ($E001-$FFFF, odd)
        mapper->irq_enabled = true;
    }
}

static void mapper4_set_offsets(mapper4_t *mapper) {
    switch (mapper->chr_mode) {
        case 0: { // R0_1, R0_2, R1_1, R1_2, R2, R3, R4, R5
            mapper->chr_bank_offsets[0] = (mapper->regs[0] & 0xfe) * 1024;
            mapper->chr_bank_offsets[1] = (mapper->regs[0] & 0xfe) * 1024 + 1024;
            mapper->chr_bank_offsets[2] = (mapper->regs[1] & 0xfe) * 1024;
            mapper->chr_bank_offsets[3] = (mapper->regs[1] & 0xfe) * 1024 + 1024;
            mapper->chr_bank_offsets[4] = mapper->regs[2] * 1024;
            mapper->chr_bank_offsets[5] = mapper->regs[3] * 1024;
            mapper->chr_bank_offsets[6] = mapper->regs[4] * 1024;
            mapper->chr_bank_offsets[7] = mapper->regs[5] * 1024;
            break;
        }
        case 1: { // R2, R3, R4, R5, R0_1, R0_2, R1_1, R1_2
            mapper->chr_bank_offsets[0] = mapper->regs[2] * 1024;
            mapper->chr_bank_offsets[1] = mapper->regs[3] * 1024;
            mapper->chr_bank_offsets[2] = mapper->regs[4] * 1024;
            mapper->chr_bank_offsets[3] = mapper->regs[5] * 1024;
            mapper->chr_bank_offsets[4] = (mapper->regs[0] & 0xfe) * 1024;
            mapper->chr_bank_offsets[5] = (mapper->regs[0] & 0xfe) * 1024 + 1024;
            mapper->chr_bank_offsets[6] = (mapper->regs[1] & 0xfe) * 1024;
            mapper->chr_bank_offsets[7] = (mapper->regs[1] & 0xfe) * 1024 + 1024;
            break;
        }
    }

    switch (mapper->prg_mode) {
        case 0: { // R6, R7, -2, -1
            mapper->prg_bank_offsets[0] = mapper->regs[6] * (8 * 1024);
            mapper->prg_bank_offsets[1] = mapper->regs[7] * (8 * 1024);
            mapper->prg_bank_offsets[2] = (mapper->agnes->gamepack.prg_rom_banks_count - 1) * (16 * 1024);
            mapper->prg_bank_offsets[3] = (mapper->agnes->gamepack.prg_rom_banks_count - 1) * (16 * 1024) + (8 * 1024);
            break;
        }
        case 1: { // -2, R7, R6, -1
            mapper->prg_bank_offsets[0] = (mapper->agnes->gamepack.prg_rom_banks_count - 1) * (16 * 1024);
            mapper->prg_bank_offsets[1] = mapper->regs[7] * (8 * 1024);
            mapper->prg_bank_offsets[2] = mapper->regs[6] * (8 * 1024);
            mapper->prg_bank_offsets[3] = (mapper->agnes->gamepack.prg_rom_banks_count - 1) * (16 * 1024) + (8 * 1024);
            break;
        }
    }
}
