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
int camera_index CCM_MEMORY; // which sprite has the camera on it
int camera_shake CCM_MEMORY;

void run_init()
{
    run_paused = 0;
    camera_index = 255;
    camera_shake = 0;
}

void run_reset()
{
}

void run_switch()
{
    update_palette2();

    camera_shake = 0;
    camera_index = 255;
    // hide any sprites in objects by setting z = 0
    tile_map_x = 16*tile_map_width;
    tile_map_y = 16*tile_map_height;
    for (int i=0; i<MAX_OBJECTS; ++i)
    {
        int y = object[i].y/16;
        int x = object[i].x/16;
        if (x <= 0 || x >= tile_map_width-1 ||
            y <= 0 || y >= tile_map_height-1)
        {
            message("weird, sprite object %d is out of bounds\n", i);
            object[i].z = 0;
            continue;
        }
        if (tile_xy_is_block(x, y))
            object[i].z = 0;
        else
        {
            object[i].z = 1;
            // find the leftmost player 0 (hero) sprite, to make camera follow
            message("%d: %d\n", i, object[i].sprite_index);
            if (object[i].sprite_index == 0)
            {
                if ((int)object[i].x/16 < tile_map_x + 10)
                {
                    tile_map_x = ((int)object[i].x/16 - 10)*16;
                    tile_map_y = (int)object[i].y - 8*16;
                    camera_index = i;
                    message("found player at %f, %f\n", object[i].x, object[i].y);
                }
            }
        }
    }
    if (tile_map_x < 16)
        tile_map_x = 16;
    else if (tile_map_x + SCREEN_W + 16 >= tile_map_width*16)
        tile_map_x = tile_map_width*16 - SCREEN_W - 16;
    
    if (tile_map_y < 16)
        tile_map_y = 16;
    else if (tile_map_y + SCREEN_H + 16 >= tile_map_height*16)
        tile_map_y = tile_map_height*16 - SCREEN_H - 16;
    
    update_object_images();
    run_paused = 0;
    chip_play_init(0);
}

void run_line()
{
    tiles_line();
    sprites_line();
    uint16_t *U16 = (uint16_t *)(U8row+16) - 1;
    uint32_t *dst = (uint32_t *)draw_buffer - 1;
    const uint32_t *final_dst = dst + SCREEN_W/2;
    while (dst < final_dst)
    {
        uint16_t u16 = *++U16;
        *++dst = palette2[(u16&15)|(u16>>4)]; 
    }

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
        game_message[0] = 0;
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

    if (camera_index < 255)
    {
        if (object[camera_index].ix < 9*16)
        {
            if (object[camera_index].vx < -1)
                tile_map_x += object[camera_index].vx;
            else
                --tile_map_x;
            if (tile_map_x < 16)
                tile_map_x = 16;
        }
        else if (object[camera_index].ix >= (20-9)*16)
        {
            if (object[camera_index].vx > 1)
                tile_map_x += object[camera_index].vx;
            else
                ++tile_map_x;
            if (tile_map_x > 16*(tile_map_width - 1 - 20))
                tile_map_x = 16*(tile_map_width - 1 - 20);
        }
        if (object[camera_index].iy < 5*16)
        {
            if (object[camera_index].vy < -1)
                tile_map_y += object[camera_index].vy;
            else
                --tile_map_y;
            if (tile_map_y < 16)
                tile_map_y = 16;
        }
        else if (object[camera_index].iy >= (15-5)*16)
        {
            if (object[camera_index].vy > 1)
                tile_map_y += object[camera_index].vy;
            else
                ++tile_map_y;
            if (tile_map_y > 16*(tile_map_height - 1 - 15))
                tile_map_y = 16*(tile_map_height - 1 - 15);
        }
    }

    if (camera_shake > 0)
    {
        int mvx, mvy, sx, sy;
        if (camera_shake > 128)
        {
            mvx = rand()%1024;
            mvy = mvx >> 5;
            sx = mvx & 16;
            mvx = (mvx & 15);
            sy =  mvy & 16;
            mvy = (mvy & 15);
            camera_shake -= 64;
        }
        else if (camera_shake > 32)
        {
            mvx = rand()%256;
            mvy = mvx >> 4;
            sx = mvx & 8;
            mvx = mvx & 7;
            sy = mvy & 8;
            mvy = mvy & 7;
            camera_shake -= 16;
        }
        else
        {
            mvx = rand()%64;
            mvy = mvx >> 3;
            sx = mvx & 4;
            mvx = mvx & 3;
            sy = mvy & 4;
            mvy = mvy & 3;
            --camera_shake;
        }
        if (sx)
        {
            tile_map_x -= mvx;
            if (tile_map_x < 0)
                tile_map_x = 0;
        }
        else
        {
            tile_map_x += mvx;
            if (tile_map_x > tile_map_width*16 - SCREEN_W)
                tile_map_x = tile_map_width*16 - SCREEN_W;
        }
        if (sy)
        {
            tile_map_y -= mvy;
            if (tile_map_y < 0)
                tile_map_y = 0;
        }
        else
        {
            tile_map_y += mvy;
            if (tile_map_y > tile_map_height*16 - SCREEN_H)
                tile_map_y = tile_map_height*16 - SCREEN_H;
        }
    }
    update_objects();
}
