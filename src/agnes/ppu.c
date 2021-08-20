#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef AGNES_SINGLE_HEADER
#include "ppu.h"

#include "agnes_types.h"
#include "cpu.h"
#include "mapper.h"
#endif

static void scanline_visible_pre(ppu_t *ppu, bool *out_new_frame);
static void inc_hori_v(ppu_t *ppu);
static void inc_vert_v(ppu_t *ppu);
static void emit_pixel(ppu_t *ppu);
static uint16_t get_bg_color_addr(ppu_t *ppu);
static uint16_t get_sprite_color_addr(ppu_t *ppu, int *out_sprite_ix, bool *out_behind_bg);
static void eval_sprites(ppu_t *ppu);
static void set_pixel_color_ix(ppu_t *ppu, int x, int y, uint8_t color_ix);
static uint8_t ppu_read8(ppu_t *ppu, uint16_t addr);
static void ppu_write8(ppu_t *ppu, uint16_t addr, uint8_t val);
static uint16_t mirror_address(ppu_t *ppu, uint16_t addr);

static unsigned g_palette_addr_map[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x11, 0x12, 0x13, 0x04, 0x15, 0x16, 0x17, 0x08, 0x19, 0x1a, 0x1b, 0x0c, 0x1d, 0x1e, 0x1f,
};

void ppu_init(ppu_t *ppu, agnes_t *agnes) {
    memset(ppu, 0, sizeof(ppu_t));
    ppu->agnes = agnes;

    ppu_write_register(ppu, 0x2000, 0);
    ppu_write_register(ppu, 0x2001, 0);
}

void ppu_tick(ppu_t *ppu, bool *out_new_frame) {
    bool rendering_enabled = ppu->masks.show_background || ppu->masks.show_sprites;

    // https://wiki.nesdev.com/w/index.php/PPU_frame_timing#Even.2FOdd_Frames
    if (rendering_enabled && ppu->is_odd_frame && ppu->dot == 339 && ppu->scanline == 261) {
        ppu->dot = 0;
        ppu->scanline = 0;
        ppu->is_odd_frame = !ppu->is_odd_frame;
    } else {
        ppu->dot++;

        if (ppu->dot > 340){
            ppu->dot = 0;
            ppu->scanline++;
        }

        if (ppu->scanline > 261) {
            ppu->scanline = 0;
            ppu->is_odd_frame = !ppu->is_odd_frame;
        }
    }

    if (ppu->dot == 0) {
        return;
    }

    bool scanline_visible = ppu->scanline >= 0 && ppu->scanline < 240;
    bool scanline_pre = ppu->scanline == 261;
    bool scanline_post = ppu->scanline == 241;

    if (rendering_enabled && (scanline_visible || scanline_pre)) {
        scanline_visible_pre(ppu, out_new_frame);
    }

    if (ppu->dot == 1) {
        if (scanline_pre) {
            ppu->status.sprite_overflow = false;
            ppu->status.sprite_zero_hit = false;
            ppu->status.in_vblank = false;
        } else if (scanline_post) {
            ppu->status.in_vblank = true;
            *out_new_frame = true;
            if (ppu->ctrl.nmi_enabled) {
                cpu_trigger_nmi(&ppu->agnes->cpu);
            }
        }
    }
}

static void scanline_visible_pre(ppu_t *ppu, bool *out_new_frame) {
    bool scanline_visible = ppu->scanline >= 0 && ppu->scanline < 240;
    bool scanline_pre = ppu->scanline == 261;
    bool dot_visible = ppu->dot > 0 && ppu->dot <= 256;
    bool dot_fetch = ppu->dot <= 256 || (ppu->dot >= 321 && ppu->dot < 337);

    if (scanline_visible && dot_visible) {
        emit_pixel(ppu);
    }

    if (dot_fetch) {
        ppu->bg_lo_shift <<= 1;
        ppu->bg_hi_shift <<= 1;
        ppu->at_shift = (ppu->at_shift << 2) | (ppu->at_latch & 0x3);

        switch (ppu->dot & 0x7) {
            case 1: {
                uint16_t addr = 0x2000 | (ppu->regs.v & 0x0fff);
                ppu->nt = ppu_read8(ppu, addr);
                break;
            }
            case 3: {
                uint16_t v = ppu->regs.v;
                uint16_t addr = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);
                ppu->at = ppu_read8(ppu, addr);
                if (ppu->regs.v & 0x40) {
                    ppu->at = ppu->at >> 4;
                }
                if (ppu->regs.v & 0x02) {
                    ppu->at = ppu->at >> 2;
                }
                break;
            }
            case 5: {
                uint8_t fine_y = ((ppu->regs.v) >> 12) & 0x7;
                uint16_t addr = ppu->ctrl.bg_table_addr + (ppu->nt << 4) + fine_y;
                ppu->bg_lo = ppu_read8(ppu, addr);
                break;
            }
            case 7: {
                uint8_t fine_y = ((ppu->regs.v) >> 12) & 0x7;
                uint16_t addr = ppu->ctrl.bg_table_addr + (ppu->nt << 4) + fine_y + 8;
                ppu->bg_hi = ppu_read8(ppu, addr);
                break;
            }
            case 0: {
                ppu->bg_lo_shift = (ppu->bg_lo_shift & 0xff00) | ppu->bg_lo;
                ppu->bg_hi_shift = (ppu->bg_hi_shift & 0xff00) | ppu->bg_hi;

                ppu->at_latch = ppu->at & 0x3;

                if (ppu->dot == 256) {
                    inc_vert_v(ppu);
                } else {
                    inc_hori_v(ppu);
                }
                break;
            }
            default:
                break;
        }
    }

    if (ppu->dot == 257) {
        // v: |_...|.F..| |...E|DCBA| = t: |_...|.F..| |...E|DCBA|
        ppu->regs.v = (ppu->regs.v & 0xfbe0) | (ppu->regs.t & ~(0xfbe0));

        if (scanline_visible) {
            eval_sprites(ppu);
        } else {
            ppu->sprite_ixs_count = 0;
        }
    }

    if (scanline_pre && ppu->dot >= 280 && ppu->dot <= 304) {
        // v: |_IHG|F.ED| |CBA.|....| = t: |_IHG|F.ED| |CBA.|....|
        ppu->regs.v = (ppu->regs.v & 0x841f) | (ppu->regs.t & ~(0x841f));
    }

    if (ppu->masks.show_background && ppu->masks.show_sprites) {
        if ((ppu->ctrl.bg_table_addr == 0x0000 && ppu->dot == 270) // Should be 260 but it caused glitches in Kirby
         || (ppu->ctrl.bg_table_addr == 0x1000 && ppu->dot == 324)) { // Not tested so far.
            // https://wiki.nesdev.com/w/index.php/MMC3#IRQ_Specifics
            // PA12 is 12th bit of PPU address bus that's toggled when switching between
            // background and sprite pattern tables (should happen once per scanline).
            // This might not work correctly with games using 8x16 sprites
            // or games writing to CHR RAM.
            mapper_pa12_rising_edge(ppu->agnes);
        }
    }
}

#define GET_COARSE_X(v) ((v) & 0x1f)
#define SET_COARSE_X(v, cx) do { v = (((v) & ~0x1f) | ((cx) & 0x1f)); } while (0)
#define GET_COARSE_Y(v) (((v) >> 5) & 0x1f)
#define SET_COARSE_Y(v, cy) do { v = (((v) & ~0x3e0) | ((cy) & 0x1f) << 5); } while (0)
#define GET_FINE_Y(v) ((v) >> 12)
#define SET_FINE_Y(v, fy) do { v = (((v) & ~0x7000) | (((fy) & 0x7) << 12)); } while (0)

static void inc_hori_v(ppu_t *ppu) {
    unsigned cx = GET_COARSE_X(ppu->regs.v);
    if (cx == 31) {
        SET_COARSE_X(ppu->regs.v, 0);
        ppu->regs.v ^= 0x0400; // switch horizontal nametable
    } else {
        SET_COARSE_X(ppu->regs.v, cx + 1);
    }
}

static void inc_vert_v(ppu_t *ppu) {
    unsigned fy = GET_FINE_Y(ppu->regs.v);
    if (fy < 7) {
        SET_FINE_Y(ppu->regs.v, fy + 1);
    } else {
        SET_FINE_Y(ppu->regs.v, 0);
        unsigned cy = GET_COARSE_Y(ppu->regs.v);
        if (cy == 29) {
            SET_COARSE_Y(ppu->regs.v, 0);
            ppu->regs.v ^= 0x0800; // switch vertical nametable
        } else if (cy == 31) {
            SET_COARSE_Y(ppu->regs.v, 0);
        } else {
            SET_COARSE_Y(ppu->regs.v, cy + 1);
        }
    }
}

#undef GET_COARSE_X
#undef SET_COARSE_X
#undef GET_COARSE_Y
#undef SET_COARSE_Y
#undef GET_FINE_Y
#undef SET_FINE_Y

static void eval_sprites(ppu_t *ppu) {
    ppu->sprite_ixs_count = 0;
    const sprite_t* sprites = (const sprite_t*)ppu->oam_data;
    int sprite_height = ppu->ctrl.use_8x16_sprites ? 16 : 8;
    for (int i = 0; i < 64; i++) {
        const sprite_t* sprite = &sprites[i];

        if (sprite->y_pos > 0xef || sprite->x_pos > 0xff) {
            continue;
        }

        int s_y = ppu->scanline - sprite->y_pos;
        if (s_y < 0 || s_y >= sprite_height) {
            continue;
        }

        if (ppu->sprite_ixs_count < 8) {
            ppu->sprites[ppu->sprite_ixs_count] = *sprite;
            ppu->sprite_ixs[ppu->sprite_ixs_count] = i;
            ppu->sprite_ixs_count++;
        } else {
            ppu->status.sprite_overflow = true;
            break;
        }
    }
}

static void emit_pixel(ppu_t *ppu) {
    const int x = ppu->dot - 1;
    const int y = ppu->scanline;

    if (x < 8 && !ppu->masks.show_leftmost_bg && !ppu->masks.show_leftmost_sprites) {
        set_pixel_color_ix(ppu, x, y, 63); // 63 is black in my default colour palette
        return;
    }

    uint16_t bg_color_addr = get_bg_color_addr(ppu);

    int sprite_ix = -1;
    bool behind_bg = false;
    uint16_t sp_color_addr = get_sprite_color_addr(ppu, &sprite_ix, &behind_bg);

    uint16_t color_addr = 0x3f00;
    if (bg_color_addr && sp_color_addr) {
        if (sprite_ix == 0 && x != 255) {
            ppu->status.sprite_zero_hit = true;
        }
        color_addr = behind_bg ? bg_color_addr : sp_color_addr;
    } else if (bg_color_addr && !sp_color_addr) {
        color_addr = bg_color_addr;
    } else if (!bg_color_addr && sp_color_addr) {
        color_addr = sp_color_addr;
    }

    uint8_t output_color_ix = ppu_read8(ppu, color_addr);
    set_pixel_color_ix(ppu, x, y, output_color_ix);
}

static uint16_t get_bg_color_addr(ppu_t *ppu) {
    if (!ppu->masks.show_background || (!ppu->masks.show_leftmost_bg && ppu->dot < 9)) {
        return 0;
    }

    bool hi_bit = AGNES_GET_BIT(ppu->bg_hi_shift, 15 - ppu->regs.x);
    bool lo_bit = AGNES_GET_BIT(ppu->bg_lo_shift, 15 - ppu->regs.x);

    if (!lo_bit && !hi_bit) {
        return 0;
    }

    uint8_t palette = (ppu->at_shift >> (14 - (ppu->regs.x << 1)) & 0x3);
    uint8_t palette_ix = ((uint8_t)hi_bit << 1) | (uint8_t)lo_bit;
    uint16_t color_address = 0x3f00 | (palette << 2) | palette_ix;
    return color_address;
}

static uint16_t get_sprite_color_addr(ppu_t *ppu, int *out_sprite_ix, bool *out_behind_bg) {
    *out_sprite_ix = -1;
    *out_behind_bg = false;

    const int x = ppu->dot - 1;
    const int y = ppu->scanline;

    if (!ppu->masks.show_sprites || (!ppu->masks.show_leftmost_sprites && x < 8)) {
        return 0;
    }

    int sprite_height = ppu->ctrl.use_8x16_sprites ? 16 : 8;
    uint16_t table = ppu->ctrl.sprite_table_addr;

    for (int i = 0; i < ppu->sprite_ixs_count; i++) {
        const sprite_t *sprite = &ppu->sprites[i];
        int s_x = x - sprite->x_pos;
        if (s_x < 0 || s_x >= 8) {
            continue;
        }

        int s_y = y - sprite->y_pos - 1;

        s_x = AGNES_GET_BIT(sprite->attrs, 6) ? 7 - s_x : s_x; // flip hor
        s_y = AGNES_GET_BIT(sprite->attrs, 7) ? (sprite_height - 1 - s_y) : s_y; // flip vert

        uint8_t tile_num = sprite->tile_num;
        if (ppu->ctrl.use_8x16_sprites) {
            table = tile_num & 0x1 ? 0x1000 : 0x0000;
            tile_num &= 0xfe;
            if (s_y >= 8) {
                tile_num += 1;
                s_y -= 8;
            }
        }

        uint16_t offset = table + (tile_num << 4) + s_y;

        uint8_t lo_byte = ppu_read8(ppu, offset);
        uint8_t hi_byte = ppu_read8(ppu, offset + 8);

        if (!lo_byte && !hi_byte) {
            continue;
        }

        bool lo_bit = AGNES_GET_BIT(lo_byte, 7 - s_x);
        bool hi_bit = AGNES_GET_BIT(hi_byte, 7 - s_x);

        if (lo_bit || hi_bit) {
            *out_sprite_ix = ppu->sprite_ixs[i];
            if (AGNES_GET_BIT(sprite->attrs, 5)) {
                *out_behind_bg = true;
            }
            uint8_t palette_ix = ((uint8_t)hi_bit << 1) | (uint8_t)lo_bit;
            uint16_t color_address = 0x3f10 | ((sprite->attrs & 0x3) << 2) | palette_ix;
            return color_address;
        }
    }
    return 0;
}

uint8_t ppu_read_register(ppu_t *ppu, uint16_t addr) {
    switch (addr) {
        case 0x2002: { // PPUSTATUS
            uint8_t res = 0;
            res |= ppu->last_reg_write & 0x1f;
            res |= ppu->status.sprite_overflow << 5;
            res |= ppu->status.sprite_zero_hit << 6;
            res |= ppu->status.in_vblank << 7;
            ppu->status.in_vblank = false;
            //    res |= ppu->status_in_vblank
            //    w:                  = 0
            ppu->regs.w = 0;
            return res;
        }
        case 0x2004: { // OAMDATA
            return ppu->oam_data[ppu->oam_address];
        }
        case 0x2007: { // PPUDATA
            uint8_t res = 0;
            if (ppu->regs.v < 0x3f00) {
                res = ppu->ppudata_buffer;
                ppu->ppudata_buffer = ppu_read8(ppu, ppu->regs.v);
            } else {
                res = ppu_read8(ppu, ppu->regs.v);
                ppu->ppudata_buffer = ppu_read8(ppu, ppu->regs.v - 0x1000);
            }
            ppu->regs.v += ppu->ctrl.addr_increment;
            return res;
        }
    }
    return 0;
}

void ppu_write_register(ppu_t *ppu, uint16_t addr, uint8_t val) {
    ppu->last_reg_write = val;
    switch (addr) {
        case 0x2000: { // PPUCTRL
            ppu->ctrl.addr_increment = AGNES_GET_BIT(val, 2) ? 32 : 1;
            ppu->ctrl.sprite_table_addr = AGNES_GET_BIT(val, 3) ? 0x1000 : 0x0000;
            ppu->ctrl.bg_table_addr = AGNES_GET_BIT(val, 4) ? 0x1000 : 0x0000;
            ppu->ctrl.use_8x16_sprites = AGNES_GET_BIT(val, 5);
            ppu->ctrl.nmi_enabled = AGNES_GET_BIT(val, 7);

            //    t: |_...|BA..| |....|....| = d: |....|..BA|
            ppu->regs.t = (ppu->regs.t & 0xf3ff) | ((val & 0x03) << 10);
            break;
        }
        case 0x2001: { // PPUMASK
            ppu->masks.show_leftmost_bg = AGNES_GET_BIT(val, 1);
            ppu->masks.show_leftmost_sprites = AGNES_GET_BIT(val, 2);
            ppu->masks.show_background = AGNES_GET_BIT(val, 3);
            ppu->masks.show_sprites = AGNES_GET_BIT(val, 4);
            break;
        }
        case 0x2003: { // OAMADDR
            ppu->oam_address = val;
            break;
        }
        case 0x2004: { // OAMDATA
            ppu->oam_data[ppu->oam_address] = val;
            ppu->oam_address++;
            break;
        }
        case 0x2005: { // SCROLL
            if (ppu->regs.w) {
                //    t: |_CBA|..HG| |FED.|....| = d: |HGFE|DCBA|
                //    w:                  = 0
                ppu->regs.t = (ppu->regs.t & 0x8fff) | ((val & 0x7) << 12);
                ppu->regs.t = (ppu->regs.t & 0xfc1f) | ((val >> 3) << 5);
                ppu->regs.w = 0;
            } else {
                //    t: |_...|....| |...H|GFED| = d: HGFED...
                //    x:              CBA = d: |...|..CBA|
                //    w:                  = 1
                ppu->regs.t = (ppu->regs.t & 0xffe0) | (val >> 3);
                ppu->regs.x = (val & 0x7);
                ppu->regs.w = 1;
            }
            break;
        }
        case 0x2006: { // PPUADDR
            if (ppu->regs.w) {
                //    t: |_...|....| |HGFE|DCBA| = d: |HGFE|DCBA|
                //    v                   = t
                //    w:                  = 0
                ppu->regs.t = (ppu->regs.t & 0xff00) | val;
                ppu->regs.v = ppu->regs.t;
                ppu->regs.w = 0;
            } else {
                //    t: |_.FE|DCBA| |....|....| = d: |..FE|DCBA|
                //    t: |_X..|....| |....|....| = 0
                //    w:                  = 1
                ppu->regs.t = (ppu->regs.t & 0xc0ff) | ((val & 0x3f) << 8);
                ppu->regs.t = ppu->regs.t & 0xbfff;
                ppu->regs.w = 1;
            }
            break;
        }
        case 0x2007: { // PPUDATA
            ppu_write8(ppu, ppu->regs.v, val);
            ppu->regs.v += ppu->ctrl.addr_increment;
            break;
        }
        case 0x4014: { // OAMDMA
            uint16_t dma_addr = ((uint16_t)val) << 8;
            for (int i = 0; i < 256; i++) {
                ppu->oam_data[ppu->oam_address] = cpu_read8(&ppu->agnes->cpu, dma_addr);
                ppu->oam_address++;
                dma_addr++;
            }
            cpu_set_dma_stall(&ppu->agnes->cpu);
            break;
        }
    }
}

static void set_pixel_color_ix(ppu_t *ppu, int x, int y, uint8_t color_ix) {
    int ix = (y * AGNES_SCREEN_WIDTH) + x;
    ppu->screen_buffer[ix] = color_ix;
}

static uint8_t ppu_read8(ppu_t *ppu, uint16_t addr) {
    addr = addr & 0x3fff;
    uint8_t res = 0;
    if (addr >= 0x3f00) { // $3F00 - $3FFF, palette reads are most common
        unsigned palette_ix = g_palette_addr_map[addr & 0x1f];
        res = ppu->palette[palette_ix];
    } else if (addr < 0x2000) { // $0000 - $1FFF
        res = mapper_read(ppu->agnes, addr);
    } else { // $2000 - $3EFF
        uint16_t mirrored_addr = mirror_address(ppu, addr);
        res = ppu->nametables[mirrored_addr];
    }
    return res;
}

static void ppu_write8(ppu_t *ppu, uint16_t addr, uint8_t val) {
    addr = addr & 0x3fff;
    if (addr >= 0x3f00) { // $3F00 - $3FFF
        int palette_ix = g_palette_addr_map[addr & 0x1f];
        ppu->palette[palette_ix] = val;
    } else if (addr < 0x2000) { // $0000 - $1FFF
        mapper_write(ppu->agnes, addr, val);
    } else { // $2000 - $3EFF
        uint16_t mirrored_addr = mirror_address(ppu, addr);
        ppu->nametables[mirrored_addr] = val;
    }
}

static uint16_t mirror_address(ppu_t *ppu, uint16_t addr) {
    switch (ppu->agnes->mirroring_mode)
    {
        case MIRRORING_MODE_HORIZONTAL:   return ((addr >> 1) & 0x400) | (addr & 0x3ff);
        case MIRRORING_MODE_VERTICAL:     return addr & 0x07ff;
        case MIRRORING_MODE_SINGLE_LOWER: return addr & 0x3ff;
        case MIRRORING_MODE_SINGLE_UPPER: return 0x400 | (addr & 0x3ff);
        case MIRRORING_MODE_FOUR_SCREEN:  return addr - 0x2000;
        default: return 0;
    }
}
