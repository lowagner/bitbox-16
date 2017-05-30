#include "bitbox.h"
#include "common.h"
#include "chiptune.h"
#include "sprites.h"
#include "tiles.h"
#include "hit.h"
#include "edit.h"
#include "save.h"
#include "font.h"
#include "unlocks.h"
#include <stdint.h>
#include <stdlib.h> // abs

uint8_t run_paused CCM_MEMORY;
int player_index[2] CCM_MEMORY; // which sprite has the camera on it, and second player
int players_swapped CCM_MEMORY;
int camera_shake CCM_MEMORY;
float camera_y CCM_MEMORY;
float camera_x CCM_MEMORY;
int run_win CCM_MEMORY;

void run_init()
{
    run_paused = 0;
    player_index[0] = 255;
    player_index[1] = 255;
    camera_shake = 0;
    camera_y = 16.0f;
    camera_x = 16.0f;
}

void set_camera_from_player_position()
{
    tile_map_x = (int)object[player_index[0]].x - 10*16;
    tile_map_y = (int)object[player_index[0]].y - 8*16;

    if (tile_map_x < 16)
        tile_map_x = 16;
    else if (tile_map_x + SCREEN_W + 16 >= tile_map_width*16)
        tile_map_x = tile_map_width*16 - SCREEN_W - 16;
    
    if (tile_map_y < 16)
        tile_map_y = 16;
    else if (tile_map_y + SCREEN_H + 16 >= tile_map_height*16)
        tile_map_y = tile_map_height*16 - SCREEN_H - 16;
    
    camera_y = tile_map_y;
    camera_x = tile_map_x;

    update_object_images();
}

void run_start()
{
    run_win = 0;
    update_palette2();

    camera_shake = 0;
    player_index[0] = 255;
    player_index[1] = 255;
    players_swapped = 0;
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
            message("INIT: sprite object %d is out of bounds\n", i);
            object[i].z = 0;
            continue;
        }
        object[i].health = 15;
        object[i].blink = 0;
        if (tile_xy_is_block(x, y))
            object[i].z = 0;
        else
        {
            object[i].z = 1;
            // find the leftmost player 0 (hero) and player 1 (sidekick) sprites, to make camera follow
            if (object[i].sprite_index/16 == 0)
            {
                int p = object[i].sprite_index/8;
                if (player_index[p] == 255 || object[i].x < object[player_index[p]].x)
                {
                    player_index[p] = i;
                    message("found player %d at %f, %f\n", p, object[i].x, object[i].y);
                }
            }
        }
    }

    if (player_index[0] != 255)
        set_camera_from_player_position();
    unlocks_start();
    run_paused = 0;
    chip_play_init(0);
}

void run_line()
{
    tiles_line();
    sprites_line();
    // collision detection between sprites
    for (int i=first_drawing_index; i<last_drawing_index-1; ++i)
    for (int j=i+1; j<last_drawing_index; ++j)
    {
        // object i and j are being drawn on the same line, but are they overlapping?
        struct object *o = &object[draw_order[i]];
        struct object *p = &object[draw_order[j]];
        sprite_collide(o, p);
    }
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
        if (run_paused && player_index[0] < 255)
        {
            if (vga_frame/32 % 2)
                font_render_no_bg_line_doubled((const uint8_t *)"paused", 16, vga_line-2, 65535);
        }
        else if (game_message[0])
            font_render_no_bg_line_doubled(game_message, 16, vga_line-2, 65535);
        else
        {
            if (player_index[0] < 255)
            {
                uint8_t msg[] = { 'H', 'P', ':', hex[object[player_index[0]].health/16], hex[object[player_index[0]].health%16], 0 };
                font_render_no_bg_line_doubled(msg, 16, vga_line-2, 65535);
            }
            if (player_index[1] < 255)
            {
                uint8_t msg[] = { 'H', 'P', ':', hex[object[player_index[1]].health/16], hex[object[player_index[1]].health%16], 0 };
                font_render_no_bg_line_doubled(msg, 156, vga_line-2, 65535);
            }
        }
    }
}

void run_controls()
{
    if (GAMEPAD_PRESS(0, start))
    {
        // pause mode
        if (player_index[0] == 255)
        {
            // TODO: allow for continuing at a checkpoint
            return;
        }
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
    {
        if (player_index[0] < 255)
            return;
        if (run_win) // won, fade to black
        {
            int i = rand()%512;
            uint32_t c = palette2[i];
            palette2[i] = (c&2078178270)>>1; // (30)|(30<<5)|(30<<10)|(30<<16)|(30<<21)|(30<<26)
            i = rand()%512;
            c = palette2[i];
            palette2[i] = (c&2078178270)>>1;
        }
        else // dead, fade to red
        {
            int i = rand()%512;
            uint32_t c = palette2[i];
            palette2[i] = ((c&2078178270)>>1) | (15<<10) | (15<<26);
        }
        return;
    }

    tiles_translate();

    if (player_index[0] < 255)
    {
        if (object[player_index[0]].ix < 9*16)
        {
            if (object[player_index[0]].vx < -1.0f)
                camera_x += object[player_index[0]].vx;
            else
                --camera_x;
            if (camera_x < 16.0f)
                camera_x = 16.0f;
        }
        else if (object[player_index[0]].ix >= (20-9)*16)
        {
            if (object[player_index[0]].vx > 1.0f)
                camera_x += object[player_index[0]].vx;
            else
                ++camera_x;
            if (camera_x > 16.0f*(tile_map_width - 1 - 20))
                camera_x = 16.0f*(tile_map_width - 1 - 20);
        }
        if (object[player_index[0]].iy < 5*16)
        {
            if (object[player_index[0]].vy < -1.0f)
                camera_y += object[player_index[0]].vy;
            else
                --camera_y;
            if (camera_y < 16.0f)
                camera_y = 16.0f;
        }
        else if (object[player_index[0]].iy >= (15-5)*16)
        {
            if (object[player_index[0]].vy > 1.0f)
                camera_y += object[player_index[0]].vy;
            else
                ++camera_y;
            if (camera_y > 16.0f*(tile_map_height - 1 - 15))
                camera_y = 16.0f*(tile_map_height - 1 - 15);
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
            camera_x -= mvx;
            if (camera_x < 0)
                camera_x = 0;
        }
        else
        {
            camera_x += mvx;
            if (camera_x > tile_map_width*16 - SCREEN_W)
                camera_x = tile_map_width*16 - SCREEN_W;
        }
        if (sy)
        {
            camera_y -= mvy;
            if (camera_y < 0)
                camera_y = 0;
        }
        else
        {
            camera_y += mvy;
            if (camera_y > tile_map_height*16 - SCREEN_H)
                camera_y = tile_map_height*16 - SCREEN_H;
        }
    }
    tile_map_y = camera_y;
    tile_map_x = camera_x;
    update_objects();
    unlocks_run();
}

void run_stop(int win)
{
    player_index[0] = 255;
    run_paused = 1;
    run_win = win;
    if (run_win)
        set_game_message_timeout("you won!", 0);
    else
        set_game_message_timeout("you died...", 0);
}
