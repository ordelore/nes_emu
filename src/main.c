#include <assert.h>
#include <string.h>
#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <debug.h>
#include <fileioc.h>

#define AGNES_IMPLEMENTATION
#include "agnes/agnes.h"

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240

static void get_input(agnes_input_t *out_input);

int main(void) {

    // initialize agnes
    agnes_t *agnes = agnes_make();
    uint8_t x_offset = (WINDOW_WIDTH / 2) - (AGNES_SCREEN_WIDTH / 2);
    if (agnes == NULL) {
        return 1;
    }

    // load rom
    ti_Close(3);
    const ti_var_t fp = ti_Open("ROMIMG", "r");
    if (!fp) {
        ti_Close(fp);
        return 1;
    }
    const size_t rom_size = ti_GetSize(fp);
    unsigned char *file_contents = ti_GetDataPtr(fp);
    bool ok = agnes_load_ines_data(agnes, file_contents, rom_size);
    if (!ok) {
        ti_Close(fp);
        return 1;
    }
    
    // gfx init

    // all gfx should be drawn to the buffer and only updated each frame
    gfx_Begin();
    gfx_SetDrawBuffer();

    // Fill the screen with black
    gfx_FillScreen(0);

    // Build the palette
    agnes_color_t *g_colors = get_gcolors();
    uint16_t num_colors = 64;
    for (uint16_t i = 0; i < num_colors; i++) {
        gfx_palette[i] = gfx_RGBTo1555(g_colors[i].r, g_colors[i].g, g_colors[i].b);
    }

    agnes_input_t input;
    kb_Scan();
    while (!(kb_Data[6] & kb_Clear)) {
        dbg_sprintf(dbgout, "Processing input\n");
        kb_Scan();
        get_input(&input);
        agnes_set_input(agnes, &input, NULL);

        dbg_sprintf(dbgout, "Processing frame\n");
        ok = agnes_next_frame(agnes);
        kb_Scan();
        if (!ok) {
            ti_Close(fp);
            return 1;
        }

        dbg_sprintf(dbgout, "Writing to screen\n");
        for (int x = 0; x < AGNES_SCREEN_WIDTH; x++) {
            for(int y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
                int ix = agnes_get_screen_index(agnes, x, y);

                gfx_SetColor(ix);
                gfx_SetPixel(x + x_offset, y);
            }
        }
        gfx_BlitBuffer();
    }
    agnes_destroy(agnes);
    ti_Close(fp);
    gfx_End();
    return 0;
}
static void get_input(agnes_input_t *out_input) {
    out_input->a = kb_Data[1] & kb_2nd;
    out_input->b = kb_Data[2] & kb_Alpha;
    out_input->start = kb_Data[1] & kb_Mode;
    out_input->select = kb_Data[1] & kb_Del;
    out_input->up = kb_Data[7] & kb_Up;
    out_input->down = kb_Data[7] & kb_Down;
    out_input->left = kb_Data[7] & kb_Left;
    out_input->right = kb_Data[7] & kb_Right;
    return;
}