#include "palette.h"
#include "common.h"
#include "bitbox.h"
#include "font.h"
#include "name.h"
#include "io.h"

#include "string.h" //memcpy

uint16_t palette[16] CCM_MEMORY; 

void palette_reset()
{
    static const uint16_t colors[16] = {
        [BLACK]=RGB(0, 0, 0),
        [GRAY]=RGB(157, 157, 157),
        [WHITE]=(1<<15) - 1,
        [PINK]=RGB(224, 111, 139),
        [RED]=RGB(190, 38, 51),
        [BLAZE]=RGB(235, 137, 49),
        [BROWN]=RGB(164, 100, 34),
        [DULLBROWN]=RGB(73, 60, 43),
        [GINGER]=RGB(247, 226, 107),
        [SLIME]=RGB(163, 206, 39),
        [GREEN]=RGB(68, 137, 26),
        [BLUEGREEN]=RGB(47, 72, 78),
        [CLOUDBLUE]=RGB(178, 220, 239),
        [SKYBLUE]=RGB(49, 162, 242),
        [SEABLUE]=RGB(0, 87, 132),
        [INDIGO]=RGB(28, 20, 40),
    };
    memcpy(palette, colors, sizeof(colors));
}

#define NUMBER_LINES 12

uint8_t palette_index CCM_MEMORY;
uint8_t palette_selector CCM_MEMORY; 
uint16_t palette_copying CCM_MEMORY;

void palette_init()
{
    palette_index = 0;
    palette_selector = 0;
    palette_copying = 32768;
}

void palette_line()
{
    if (vga_line < 22)
    {
        if (vga_line/2 == 0)
            memset(draw_buffer, 0, 2*SCREEN_W);
        return;
    }
    else if (vga_line/2 == (SCREEN_H - 20)/2)
    {
        memset(draw_buffer, 0, 2*SCREEN_W);
        return;
    }
    else if (vga_line >= 22 + NUMBER_LINES*10)
    {
        draw_parade(vga_line - (22 + NUMBER_LINES*10), 0);
        return;
    }
    int line = (vga_line-22) / 10;
    int internal_line = (vga_line-22) % 10;
    if (internal_line == 0 || internal_line == 9)
    {
        memset(draw_buffer, 0, 2*SCREEN_W);
        if (line == 8)
        {
            // RGB line, add a little underline to currently selected color
            memset(draw_buffer + 20+28*9 - palette_selector*4*9, 255, 2*3*9);
        }
    }
    else
    {
        --internal_line;
        switch (line)
        {
        case 0:
            font_render_line_doubled((uint8_t *)"palette in", 16, internal_line, 65535, 0);
            font_render_line_doubled((uint8_t *)base_filename, 16+9*11, internal_line, 65535, 0);
            break;
        case 2:
            font_render_line_doubled((const uint8_t *)"L/R:cycle index", 16, internal_line, 65535, 0);
            {
            uint8_t label[3] = { hex[palette_index], ':', 0 };
            font_render_line_doubled(label, 16 + 9*22, internal_line, 65535, 0);
            }
            break;
        case 3:
            if (palette_copying < 32768)
                font_render_line_doubled((const uint8_t *)"A:cancel copy", 16+2*9, internal_line, 65535, 0);
            else
                font_render_line_doubled((const uint8_t *)"A:save colors", 16+2*9, internal_line, 65535, 0);
            break;
        case 4:
            if (palette_copying < 32768)
                font_render_line_doubled((const uint8_t *)"X:  \"     \"", 16+2*9, internal_line, 65535, 0);
            else
                font_render_line_doubled((const uint8_t *)"X:load colors", 16+2*9, internal_line, 65535, 0);
            break;
        case 5:
            if (palette_copying < 32768)
                font_render_line_doubled((const uint8_t *)"B:  \"     \"", 16+2*9, internal_line, 65535, 0);
            else
                font_render_line_doubled((const uint8_t *)"B:copy", 16+2*9, internal_line, 65535, 0);
            break;
        case 6:
            if (palette_copying < 32768)
                font_render_line_doubled((const uint8_t *)"Y:paste", 16+2*9, internal_line, 65535, 0);
            else
                font_render_line_doubled((const uint8_t *)"Y:change filename", 16+2*9, internal_line, 65535, 0);
        case 7:
            break;
        case 8:
            if (previous_visual_mode)
                font_render_line_doubled((const uint8_t *)"start:back     dpad:", 16, internal_line, 65535, 0);
            else
                font_render_line_doubled((const uint8_t *)"start:file-ops dpad:", 16, internal_line, 65535, 0);
            {
            uint8_t label[] = { 'r', ':', hex[(palette[palette_index]>>10)&31], 0 };
            font_render_line_doubled(label, 20+20*9, internal_line, RGB(255, 50, 50), 0);
            label[0] = 'g'; label[2] = hex[(palette[palette_index]>>5)&31];
            font_render_line_doubled(label, 20+24*9, internal_line, RGB(50, 255, 50), 0);
            label[0] = 'b'; label[2] = hex[(palette[palette_index])&31];
            font_render_line_doubled(label, 20+28*9, internal_line, RGB(50, 100, 255), 0);
            }
            break;
        case 9:
            font_render_line_doubled((const uint8_t *)"select:tile menu", 16, internal_line, 65535, 0);
            break;
        case 11:
            font_render_line_doubled(game_message, 16, internal_line, 65535, 0);
            break;
        }
    }
    if (vga_line < 22 + 2*16)
    {
        uint32_t *dst = (uint32_t *)draw_buffer + (SCREEN_W - 24 - 16*2*2)/2 - 1;
        uint32_t color = palette[palette_index] | (palette[palette_index]<<16);
        for (int l=0; l<16; ++l) 
            *(++dst) = color;
        ++dst;
        for (int l=0; l<4; ++l)
        {
            color = palette[(l + ((vga_line - 22))/8 * 4)&15];
            color |= (color<<16);
            *(++dst) = color;
            *(++dst) = color;
            *(++dst) = color;
            *(++dst) = color;
        }
    }
    else if (vga_line/2 == (22 + 2*16)/2 || vga_line/2 == (22 + 4*16 + 2)/2)
    {
        memset(draw_buffer+(SCREEN_W - 24 - 16*2*2), 0, 64+4+64);
    }
    else if (vga_line < 22 + 2 + 2*16 + 2*16)
    {
        if (palette_copying < 32768)
        {
            uint32_t *dst = (uint32_t *)draw_buffer + (SCREEN_W - 24 - 16*4)/2 - 1;
            uint32_t color; 
            for (int l=0; l<4; ++l)
            {
                color = palette[(l + ((vga_line - (22+32+2)))/8 * 4)&15];
                color |= (color<<16);
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
            }
            ++dst;
            color = palette_copying | (palette_copying << 16);
            for (int l=0; l<16; ++l) 
                *(++dst) = color;
        }
        else
        {
            uint32_t *dst = (uint32_t *)draw_buffer + (SCREEN_W - 24 - 16*4)/2 - 1;
            uint32_t color;
            for (int l=0; l<4; ++l) 
            {
                color = palette[(l + ((vga_line - (22+32+2)))/8 * 4)&15];
                color |= (color<<16);
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
            }
        }
    }
}

void palette_controls()
{
    int moved = 0;
    if (GAMEPAD_PRESSING(0, R))
    {
        ++moved;
    }
    if (GAMEPAD_PRESSING(0, L))
    {
        --moved;
    }
    if (moved)
    {
        game_message[0] = 0;
        palette_index = (palette_index + moved)&15;
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        return;
    }
    if (GAMEPAD_PRESS(0, left))
    {
        // blue is palette_selector = 0, so moving left goes towards red
        if (palette_selector < 2)
            ++palette_selector;
        else
            palette_selector = 0;
        return;
    }
    if (GAMEPAD_PRESS(0, right))
    {
        // red is palette_selector = 2, so moving right goes towards blue
        if (palette_selector)
            --palette_selector;
        else
            palette_selector = 2;
        return;
    }
    int make_wait = 0;
    if (GAMEPAD_PRESSING(0, up))
    {
        game_message[0] = 0;
        if (((palette[palette_index] >> (5*palette_selector)) & 31) < 31)
            palette[palette_index] += 1 << (5*palette_selector);
        make_wait = 1;
    }
    if (GAMEPAD_PRESSING(0, down))
    {
        game_message[0] = 0;
        if (((palette[palette_index] >> (5*palette_selector)) & 31) > 0)
            palette[palette_index] -= 1 << (5*palette_selector);
        make_wait = 1;
    }
    if (make_wait)
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;

    if (GAMEPAD_PRESS(0, B))
    {
        // copy or uncopy
        if (palette_copying < 32768)
            palette_copying = 32768;
        else
            palette_copying = palette[palette_index];
        return;
    }
    int save_or_load = 0;
    if (GAMEPAD_PRESS(0, A))
    {
        // save
        save_or_load = 1;
    }
    else if (GAMEPAD_PRESS(0, X))
    {
        // load
        save_or_load = 2;
    }
    if (save_or_load)
    {
        if (palette_copying < 32768)
        {
            // or cancel a copy
            palette_copying = 32768;
            return;
        }
            
        FileError error;
        if (save_or_load == 1) // save
        {
            error = io_save_palette();
        }
        else // load
        {
            error = io_load_palette();
        }
        io_message_from_error(game_message, error, save_or_load);
        return;
    }
    if (GAMEPAD_PRESS(0, Y))
    {
        if (palette_copying < 32768)
        {
            // paste:
            strcpy((char *)game_message, "pasted.");
            palette[palette_index] = palette_copying;
            palette_copying = 32768;
        }
        else
        {
            // go to filename chooser
            game_message[0] = 0;
            previous_visual_mode = EditPalette;
            game_switch(ChooseFilename);
        }
        return;
    }
    if (GAMEPAD_PRESS(0, select))
    {
        game_message[0] = 0;
        game_switch(EditTileOrSpriteProperties);
        previous_visual_mode = None;
        return;
    }
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        if (previous_visual_mode)
        {
            game_switch(previous_visual_mode);
            previous_visual_mode = None;
        }
        else
        {
            game_switch(SaveLoadScreen);
        }
        return;
    }
}
