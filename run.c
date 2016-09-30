#include "bitbox.h"
#include "common.h"
#include "chiptune.h"
#include "sprites.h"
#include "tiles.h"
#include "edit.h"
#include "save.h"
#include "font.h"
#include <stdint.h>
#include <stdlib.h> // abs
#include <string.h> // memset

uint8_t run_paused CCM_MEMORY;

void run_init()
{
    run_paused = 0; // eventually will probably want this to be 1
}

void run_reset()
{
}

void run_switch()
{
    if (tile_map_x < 16)
        tile_map_x = 16;
    else if (tile_map_x + SCREEN_W + 16 >= tile_map_width*16)
        tile_map_x = tile_map_width*16 - SCREEN_W - 16;
    
    if (tile_map_y < 16)
        tile_map_y = 16;
    else if (tile_map_y + SCREEN_H + 16 >= tile_map_height*16)
        tile_map_y = tile_map_height*16 - SCREEN_H - 16;
    
    // hide any sprites in objects by setting z = 0
    for (int i=0; i<MAX_OBJECTS; ++i)
    {
        int y = object[i].y/16;
        int x = object[i].x/16;
        if (x < 0 || x >= tile_map_width ||
            y < 0 || y >= tile_map_height)
        {
            message("weird, sprite object %d is out of bounds\n", i);
            continue;
        }
        if (tile_yx_is_block(y, x))
            object[i].z = 0;
        else
            object[i].z = 1;
    }
    
    update_objects(); 
    chip_play_init(0);
}

void run_line()
{
    tiles_line();
    sprites_line();
    if (vga_line >= 2 && vga_line < 10)
    {
        if (run_paused)
        {
            if (vga_frame/32 % 2)
                font_render_no_bg_line_doubled((const uint8_t *)"paused", 16, vga_line-2, 65535);
        }
        else
            font_render_no_bg_line_doubled(game_message, 16, vga_line-2, 65535);
    }
}

void run_controls()
{
    if (GAMEPAD_PRESS(0, start))
    {
        // pause mode
        run_paused = 1 - run_paused;
        return;
    }
    if (GAMEPAD_PRESS(0, select))
    {
        game_message[0] = 0;
        previous_visual_mode = None;
        game_switch(SaveLoadScreen);
        return;
    }

    if (run_paused)
        return;

    if (vga_frame % 2 == 0)
    {
        if (GAMEPAD_PRESSED(0, left))
        {
            if (tile_map_x > 16)
                --tile_map_x;
        }
        else if (GAMEPAD_PRESSED(0, right))
        {
            if (tile_map_x + SCREEN_W + 17 < tile_map_width*16)
                ++tile_map_x;
        }
        if (GAMEPAD_PRESSED(0, up))
        {
            if (tile_map_y > 16)
                --tile_map_y;
        }
        else if (GAMEPAD_PRESSED(0, down))
        {
            if (tile_map_y + SCREEN_H + 17 < tile_map_height*16)
                ++tile_map_y;
        }
    }
    update_objects();
}
