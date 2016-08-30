#include "bitbox.h"
#include "common.h"
#include "font.h"

char base_filename[9] CCM_MEMORY; // up to 8 characters, plus a zero

#include <string.h> // memset

#define OFFSET_X 230 // offset for alphabet square
#define TEXT_OFFSET ((16+6*9))
#define BG_COLOR 32 // a uint8_t, uint16_t color is (BG_COLOR)|(BG_COLOR<<8).  32/128 is dark red/green

uint8_t name_position CCM_MEMORY; // position in the base filename

uint8_t name_x CCM_MEMORY, name_y CCM_MEMORY; // position in alphabet table (below):
static const uint8_t allowed_chars[6][7] = {
    {'E', 'T', 'A', 'O', 'I', 'N', 0},
    {'S', 'R', 'H', 'L', 'D', 'C', 0},
    {'U', 'M', 'W', 'F', 'G', 'P', 0},
    {'Y', 'B', 'V', 'K', 'X', 'J', 0},
    {'Q', 'Z', '0', '1', '2', '3', 0},
    {'4', '5', '6', '7', '8', '9', 0}
};

#define NUMBER_LINES 14
#define BOX_COLOR RGB(205, 100, 0)
#define MATRIX_WING_COLOR RGB(255, 255, 0)
   
void name_init()
{
    name_position = 0;
}

void name_line()
{
    if (vga_line < 22)
    {
        if (vga_line/2 == 0)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    else if (vga_line/2 == (SCREEN_H - 20)/2)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    else if (vga_line >= 22 + NUMBER_LINES*10)
    {
        if (vga_line/2 == (22 + NUMBER_LINES*10)/2)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    int line = (vga_line-22) / 10;
    int internal_line = (vga_line-22) % 10;
    if (internal_line == 0 || internal_line == 9)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        // also check for character selector
        if (line == 0)
        {
            // spot in the filename to write to
            uint16_t *dst = draw_buffer + TEXT_OFFSET + 1 + name_position * 9;
            const uint16_t color = BOX_COLOR;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
        }
        else if (name_y+1 == line)
        {
            uint16_t *dst = draw_buffer + 1 + name_x * 9 + OFFSET_X;
            const uint16_t color = BOX_COLOR;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
        }
        else if (line == 6 && internal_line == 9)
        {
            uint16_t *dst = draw_buffer + 1 + name_x * 9 + OFFSET_X;
            const uint16_t color = MATRIX_WING_COLOR;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
        }
    }
    else
    {
        --internal_line;
        if (line == 0)
        {
            font_render_line_doubled((const uint8_t *)"file:", 16, internal_line, 65535, BG_COLOR*257);
            font_render_line_doubled((uint8_t *)base_filename, TEXT_OFFSET, internal_line, 65535, BG_COLOR*257);
        }
        else if (line <= 6)
        {
            font_render_line_doubled((const uint8_t *)allowed_chars[line-1], OFFSET_X, internal_line, 65535, BG_COLOR*257);
            if (line-1 == name_y)
            {
                if (name_x < 5)
                {
                    {
                    uint16_t *dst = draw_buffer + (name_x * 9 + OFFSET_X);
                    const uint16_t color = BOX_COLOR;
                    *dst = color;
                    dst += 9;
                    *dst = color;
                    }
                    {
                    uint32_t *dst = (uint32_t *)draw_buffer + (1 + 6 * 9 + OFFSET_X)/2;
                    const uint32_t color = MATRIX_WING_COLOR | ((BG_COLOR*257)<<16);
                    *dst++ = color;
                    *dst++ = color;
                    *dst++ = color;
                    *dst++ = color;
                    *dst++ = color;
                    }
                }
                else
                {
                    {
                    uint16_t *dst = draw_buffer + (name_x * 9 + OFFSET_X);
                    const uint16_t color = BOX_COLOR;
                    *dst = color;
                    }
                    {
                    uint32_t *dst = (uint32_t *)draw_buffer + (1 + 6 * 9 + OFFSET_X)/2;
                    const uint32_t color = BOX_COLOR | ((BG_COLOR*257)<<16);
                    *dst++ = color;
                    *dst++ = color;
                    *dst++ = color;
                    *dst++ = color;
                    *dst++ = color;
                    }
                }
            }
        }
        else
        switch (line)
        {
        case 7:
            font_render_line_doubled((const uint8_t *)"A:overwrite Y:insert", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 8:
            font_render_line_doubled((const uint8_t *)"B:backspace X:delete", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 9:
            font_render_line_doubled((const uint8_t *)"<L/R>:move cursor", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 11:
            font_render_line_doubled((const uint8_t *)"start:return", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 13:
            font_render_line_doubled(game_message, 32, internal_line, 65535, BG_COLOR*257);
            break;
        }
    }
}

void name_overwrite_character()
{
    char previous = base_filename[name_position];
    base_filename[name_position] = allowed_chars[name_y][name_x];
    if (name_position < 7) 
    {
        ++name_position;
        if (previous == 0)
            base_filename[name_position] = 0;
    }
    else
        base_filename[8] = 0;
}

void name_backspace_character()
{
    if (name_position == 7)
    {
        if (base_filename[name_position])
            base_filename[name_position] = 0;
        else
            base_filename[--name_position] = 0;
    }
    else
        // if inside, move everything back
    if (name_position) // somewhere in the middle
    {
        for (int i=name_position-1; i<7; ++i)
        {
            base_filename[i] = base_filename[i+1];
            if (base_filename[i] == 0)
                break;
        }
        // ensure that we took something out, the end is zeroed.
        // shouldn't be necessary, but doesn't cost much:
        base_filename[7] = 0;
        // also move back
        --name_position;
    }
    else
    {
        for (int i=0; i<7; ++i)
        {
            base_filename[i] = base_filename[i+1];
            if (base_filename[i] == 0)
                break;
        }
        // ensure that we took something out, the end is zeroed.
        // shouldn't be necessary, but doesn't cost much:
        base_filename[7] = 0;
    }
}

void name_delete_character()
{
    // delete (and move other elements down), don't move cursor
    if (name_position == 7)
    {
        base_filename[name_position] = 0;
    }
    else
    {   
        for (int i=name_position; i<7; ++i)
        {
            base_filename[i] = base_filename[i+1];
            if (base_filename[i] == 0)
                break;
        }
        // ensure that we took something out, the end is zeroed.
        // shouldn't be necessary, but doesn't cost much:
        base_filename[7] = 0;
    }
}

void name_insert_character()
{
    // insert (and move up everything else)
    if (name_position < 7)
    {
        base_filename[8] = 0; // override anything that would get moved up here
        for (int i=6; i>=name_position; --i)
        {
            base_filename[i+1] = base_filename[i];
        }
        base_filename[name_position++] = allowed_chars[name_y][name_x];
    }
    else
    {
        base_filename[name_position] = allowed_chars[name_y][name_x];
    }
}

void name_controls()
{
    int make_wait = 0;
    if (GAMEPAD_PRESSING(0, left))
    {
        if (name_x)
            --name_x;
        else
            name_x = 5;
        make_wait = 1;
    }
    if (GAMEPAD_PRESSING(0, right))
    {
        if (name_x < 5)
            ++name_x;
        else
            name_x = 0;
        make_wait = 1;
    }
    if (GAMEPAD_PRESSING(0, up))
    {
        if (name_y)
            --name_y;
        else
            name_y = 5;
        make_wait = 1;
    }
    if (GAMEPAD_PRESSING(0, down))
    {
        if (name_y < 5)
            ++name_y;
        else
            name_y = 0;
        make_wait = 1;
    }
    if (make_wait)
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;

    if (GAMEPAD_PRESS(0, A))
    {
        game_message[0] = 0;
        name_overwrite_character();
        return;
    }
    if (GAMEPAD_PRESS(0, B))
    {
        game_message[0] = 0;
        name_backspace_character();
        return;
    }
    if (GAMEPAD_PRESS(0, X))
    {
        game_message[0] = 0;
        name_delete_character();
        return;
    }
    if (GAMEPAD_PRESS(0, Y))
    {
        game_message[0] = 0;
        name_insert_character();
        return;
    }
    if (GAMEPAD_PRESS(0, L))
    {
        game_message[0] = 0;
        if (name_position)
            --name_position;
        return;
    } 
    if (GAMEPAD_PRESS(0, R))
    {
        game_message[0] = 0;
        if (base_filename[name_position] != 0 && name_position < 7)
            ++name_position;
        return;
    }
    if (GAMEPAD_PRESS(0, select))
    {
        // switch to next visual mode and ignore previous_visual_mode
        game_switch(SaveLoadScreen);
        previous_visual_mode = None;
        return;
    }
    if (GAMEPAD_PRESS(0, start))
    {
        if (previous_visual_mode)
        {
            game_switch(previous_visual_mode);
            previous_visual_mode = None;
        }
        else
        {
            game_switch(SaveLoadScreen);
        }
    }
}
