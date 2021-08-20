#include <stdlib.h>
#include <string.h>
#include <debug.h>
#ifndef AGNES_SINGLE_HEADER
#include "agnes.h"

#include "common.h"

#include "agnes_types.h"
#include "cpu.h"
#include "ppu.h"

#include "mapper.h"
#endif

typedef struct {
    uint8_t magic[4];
    uint8_t prg_rom_banks_count;
    uint8_t chr_rom_banks_count;
    uint8_t flags_6;
    uint8_t flags_7;
    uint8_t prg_ram_banks_count;
    uint8_t flags_9;
    uint8_t flags_10;
    uint8_t zeros[5];
} ines_header_t;

typedef struct agnes_state {
    agnes_t agnes;
} agnes_state_t;

static uint8_t get_input_byte(const agnes_input_t* input);

static agnes_color_t g_colors[64] = {
    {0x7c, 0x7c, 0x7c, 0xff}, {0x00, 0x00, 0xfc, 0xff}, {0x00, 0x00, 0xbc, 0xff}, {0x44, 0x28, 0xbc, 0xff},
    {0x94, 0x00, 0x84, 0xff}, {0xa8, 0x00, 0x20, 0xff}, {0xa8, 0x10, 0x00, 0xff}, {0x88, 0x14, 0x00, 0xff},
    {0x50, 0x30, 0x00, 0xff}, {0x00, 0x78, 0x00, 0xff}, {0x00, 0x68, 0x00, 0xff}, {0x00, 0x58, 0x00, 0xff},
    {0x00, 0x40, 0x58, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0x00, 0x00, 0x00, 0xff},
    {0xbc, 0xbc, 0xbc, 0xff}, {0x00, 0x78, 0xf8, 0xff}, {0x00, 0x58, 0xf8, 0xff}, {0x68, 0x44, 0xfc, 0xff},
    {0xd8, 0x00, 0xcc, 0xff}, {0xe4, 0x00, 0x58, 0xff}, {0xf8, 0x38, 0x00, 0xff}, {0xe4, 0x5c, 0x10, 0xff},
    {0xac, 0x7c, 0x00, 0xff}, {0x00, 0xb8, 0x00, 0xff}, {0x00, 0xa8, 0x00, 0xff}, {0x00, 0xa8, 0x44, 0xff},
    {0x00, 0x88, 0x88, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0x00, 0x00, 0x00, 0xff},
    {0xf8, 0xf8, 0xf8, 0xff}, {0x3c, 0xbc, 0xfc, 0xff}, {0x68, 0x88, 0xfc, 0xff}, {0x98, 0x78, 0xf8, 0xff},
    {0xf8, 0x78, 0xf8, 0xff}, {0xf8, 0x58, 0x98, 0xff}, {0xf8, 0x78, 0x58, 0xff}, {0xfc, 0xa0, 0x44, 0xff},
    {0xf8, 0xb8, 0x00, 0xff}, {0xb8, 0xf8, 0x18, 0xff}, {0x58, 0xd8, 0x54, 0xff}, {0x58, 0xf8, 0x98, 0xff},
    {0x00, 0xe8, 0xd8, 0xff}, {0x78, 0x78, 0x78, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0x00, 0x00, 0x00, 0xff},
    {0xfc, 0xfc, 0xfc, 0xff}, {0xa4, 0xe4, 0xfc, 0xff}, {0xb8, 0xb8, 0xf8, 0xff}, {0xd8, 0xb8, 0xf8, 0xff},
    {0xf8, 0xb8, 0xf8, 0xff}, {0xf8, 0xa4, 0xc0, 0xff}, {0xf0, 0xd0, 0xb0, 0xff}, {0xfc, 0xe0, 0xa8, 0xff},
    {0xf8, 0xd8, 0x78, 0xff}, {0xd8, 0xf8, 0x78, 0xff}, {0xb8, 0xf8, 0xb8, 0xff}, {0xb8, 0xf8, 0xd8, 0xff},
    {0x00, 0xfc, 0xfc, 0xff}, {0xf8, 0xd8, 0xf8, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0x00, 0x00, 0x00, 0xff},
};

agnes_t* agnes_make(void) {
    agnes_t *agnes = (agnes_t*)malloc(sizeof(*agnes));
    dbg_sprintf(dbgout, "Malloc: %d bytes\n", sizeof(*agnes));
    if (!agnes) {
        return NULL;
    }
    memset(agnes, 0, sizeof(*agnes));
    memset(agnes->ram, 0xff, sizeof(agnes->ram));
    return agnes;
}

bool agnes_load_ines_data(agnes_t *agnes, void *data, size_t data_size) {
    if (data_size < sizeof(ines_header_t)) {
        return false;
    }

    ines_header_t *header = (ines_header_t*)data;
    if (strncmp((char*)header->magic, "NES\x1a", 4) != 0) {
        return false;
    }

    unsigned prg_rom_offset = sizeof(ines_header_t);
    bool has_trainer = AGNES_GET_BIT(header->flags_6, 2);
    if (has_trainer) {
        prg_rom_offset += 512;
    }
    agnes->gamepack.chr_rom_banks_count = header->chr_rom_banks_count;
    agnes->gamepack.prg_rom_banks_count = header->prg_rom_banks_count;
    if (AGNES_GET_BIT(header->flags_6, 3)) {
        agnes->mirroring_mode = MIRRORING_MODE_FOUR_SCREEN;
    } else {
        agnes->mirroring_mode = AGNES_GET_BIT(header->flags_6, 0) ? MIRRORING_MODE_VERTICAL : MIRRORING_MODE_HORIZONTAL;
    }
    agnes->gamepack.mapper = ((header->flags_6 & 0xf0) >> 4) | (header->flags_7 & 0xf0);
    unsigned prg_rom_size = header->prg_rom_banks_count * (16 * 1024);
    unsigned chr_rom_size = header->chr_rom_banks_count * (8 * 1024);
    unsigned chr_rom_offset = prg_rom_offset + prg_rom_size;

    if ((chr_rom_offset + chr_rom_size) > data_size) {
        return false;
    }

    agnes->gamepack.data = (const uint8_t *)data;
    agnes->gamepack.prg_rom_offset = prg_rom_offset;
    agnes->gamepack.chr_rom_offset = chr_rom_offset;

    bool ok = mapper_init(agnes);
    if (!ok) {
        return false;
    }

    cpu_init(&agnes->cpu, agnes);
    ppu_init(&agnes->ppu, agnes);
    
    return true;
}

void agnes_set_input(agnes_t *agn, const agnes_input_t *input_1, const agnes_input_t *input_2) {
    if (input_1 != NULL) {
        agn->controllers[0].state = get_input_byte(input_1);
    }
    if (input_2 != NULL) {
        agn->controllers[1].state = get_input_byte(input_2);
    }
}

size_t agnes_state_size() {
    return sizeof(agnes_state_t);
}

void agnes_dump_state(const agnes_t *agnes, agnes_state_t *out_res) {
    memmove(out_res, agnes, sizeof(agnes_t));
    out_res->agnes.gamepack.data = NULL;
    out_res->agnes.cpu.agnes = NULL;
    out_res->agnes.ppu.agnes = NULL;
    switch (out_res->agnes.gamepack.mapper) {
        case 0: out_res->agnes.mapper.m0.agnes = NULL; break;
        case 1: out_res->agnes.mapper.m1.agnes = NULL; break;
        case 2: out_res->agnes.mapper.m2.agnes = NULL; break;
        case 4: out_res->agnes.mapper.m4.agnes = NULL; break;
    }
}

bool agnes_restore_state(agnes_t *agnes, const agnes_state_t *state) {
    const uint8_t *gamepack_data = agnes->gamepack.data;
    memmove(agnes, state, sizeof(agnes_t));
    agnes->gamepack.data = gamepack_data;
    agnes->cpu.agnes = agnes;
    agnes->ppu.agnes = agnes;
    switch (agnes->gamepack.mapper) {
        case 0: agnes->mapper.m0.agnes = agnes; break;
        case 1: agnes->mapper.m1.agnes = agnes; break;
        case 2: agnes->mapper.m2.agnes = agnes; break;
        case 4: agnes->mapper.m4.agnes = agnes; break;
    }
    return true;
}

bool agnes_tick(agnes_t *agnes, bool *out_new_frame) {
    int cpu_cycles = cpu_tick(&agnes->cpu);
    if (cpu_cycles == 0) {
        return false;
    }

    int ppu_cycles = cpu_cycles * 3;
    for (int i = 0; i < ppu_cycles; i++) {
        ppu_tick(&agnes->ppu, out_new_frame);
    }
    
    return true;
}

bool agnes_next_frame(agnes_t *agnes) {
    while (true) {
        bool new_frame = false;
        bool ok = agnes_tick(agnes, &new_frame);
        if (!ok) {
            return false;
        }
        if (new_frame) {
            break;
        }
    }
    return true;
}

agnes_color_t agnes_get_screen_pixel(const agnes_t *agnes, int x, int y) {
    int ix = (y * AGNES_SCREEN_WIDTH) + x;
    uint8_t color_ix = agnes->ppu.screen_buffer[ix];

    return g_colors[color_ix & 0x3f];
}
uint8_t agnes_get_screen_index(const agnes_t *agnes, int x, int y) {
    int ix = (y * AGNES_SCREEN_WIDTH) + x;
    return agnes->ppu.screen_buffer[ix] & 0x3f;
}
agnes_color_t *get_gcolors(void) {
    return g_colors;
}
void agnes_destroy(agnes_t *agnes) {
    free(agnes);
}

static uint8_t get_input_byte(const agnes_input_t* input) {
    uint8_t res = 0;
    res |= input->a      << 0;
    res |= input->b      << 1;
    res |= input->select << 2;
    res |= input->start  << 3;
    res |= input->up     << 4;
    res |= input->down   << 5;
    res |= input->left   << 6;
    res |= input->right  << 7;
    return res;
}

