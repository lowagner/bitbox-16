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
float camera_y CCM_MEMORY;
float camera_x CCM_MEMORY;

void run_init()
{
    run_paused = 0;
    camera_index = 255;
    camera_shake = 0;
    camera_y = 16.0f;
    camera_x = 16.0f;
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
            message("INIT: sprite object %d is out of bounds\n", i);
            object[i].z = 0;
            continue;
        }
        object[i].health_blink = 15;
        if (tile_xy_is_block(x, y))
            object[i].z = 0;
        else
        {
            object[i].z = 1;
            // find the leftmost player 0 (hero) sprite, to make camera follow
            if (object[i].sprite_index/8 == 0)
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
    
    camera_y = tile_map_y;
    camera_x = tile_map_x;

    update_object_images();
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
        if (o->ix < p->ix)
        {
            if (o->ix + 16 <= p->ix)
                continue;
            // p is lower than o (since j > i, and larger indices are lower on screen)
            if (p->iy - o->iy > 8) // large difference in y, collide vertically
            {
                if (o->vy > p->vy)
                {

                }
                float avg = (p->y+o->y)/2;
                float rel = (p->y-o->y)/2 - 8;
                o->y = avg-8;
                o->iy += rel;
                p->y = avg+8;
                p->iy -= rel;
            }
            else
            {
                if (o->vx > p->vx)
                {
                    // collision time

                }
                float avg = (p->x+o->x)/2;
                float rel = (p->x-o->x)/2-8;
                o->x = avg-8;
                o->ix += rel;
                p->x = avg+8;
                p->ix -= rel;
            }
        }
        else // p.x < o.x
        {
            if (p->ix + 16 <= o->ix)
                continue;

            if (p->iy - o->iy > 8) // large difference in y, collide vertically
            {
                if (o->vy > p->vy)
                {

                }
                float avg = (p->y+o->y)/2;
                float rel = (p->y-o->y)/2 - 8;
                o->y = avg-8;
                o->iy += rel;
                p->y = avg+8;
                p->iy -= rel;
            }
            else
            {
                if (p->vx > o->vx)
                {
                    // collision time

                }
                float avg = (p->x+o->x)/2;
                float rel = (p->x-o->x)/2+8;
                o->x = avg+8;
                o->ix += rel;
                p->x = avg-8;
                p->ix -= rel;
            }
        }

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
        if (run_paused)
        {
            if (vga_frame/32 % 2)
                font_render_no_bg_line_doubled((const uint8_t *)"paused", 16, vga_line-2, 65535);
        }
        else if (game_message[0])
            font_render_no_bg_line_doubled(game_message, 16, vga_line-2, 65535);
        else if (camera_index < 255)
        {
            uint8_t msg[] = { 'H', 'P', ':', hex[object[camera_index].health_blink&15], 0 };
            font_render_no_bg_line_doubled(msg, 16, vga_line-2, 65535);
        }
    }
}

void run_controls()
{
    if (GAMEPAD_PRESS(0, start))
    {
        // pause mode
        if (camera_index == 255)
        {
            run_paused = 0;
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
        return;

    if (camera_index < 255)
    {
        if (object[camera_index].ix < 9*16)
        {
            if (object[camera_index].vx < -1.0f)
                camera_x += object[camera_index].vx;
            else
                --camera_x;
            if (camera_x < 16.0f)
                camera_x = 16.0f;
        }
        else if (object[camera_index].ix >= (20-9)*16)
        {
            if (object[camera_index].vx > 1.0f)
                camera_x += object[camera_index].vx;
            else
                ++camera_x;
            if (camera_x > 16.0f*(tile_map_width - 1 - 20))
                camera_x = 16.0f*(tile_map_width - 1 - 20);
        }
        if (object[camera_index].iy < 5*16)
        {
            if (object[camera_index].vy < -1.0f)
                camera_y += object[camera_index].vy;
            else
                --camera_y;
            if (camera_y < 16.0f)
                camera_y = 16.0f;
        }
        else if (object[camera_index].iy >= (15-5)*16)
        {
            if (object[camera_index].vy > 1.0f)
                camera_y += object[camera_index].vy;
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
}
