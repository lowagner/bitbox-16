#include "bitbox.h"
#include "unlocks.h"
#include "sprites.h"
#include "hit.h"
#include "run.h"
#include "tiles.h"

#include <stdlib.h> // rand

// break sprites up into 16x16 tiles:
uint8_t sprite_draw[128][16][8] CCM_MEMORY; // 16 sprites, 8 frames, 16x16 pixels...
uint8_t sprite_pattern[16][32] CCM_MEMORY; 
/*
info about a sprite (first frame, frame 0):
    1 bit for Block-like. // maybe for projectile?
    if Block-like:
        4 bits ignored (what could we put here?) // could require all sprites to have some invisible color, no matter what...
    else:
        4 bits for "what color is invisible in this sprite"
    3 bits for attack bits
    1+3 bits for invulnerability, must have (attack & (~invulnerability)) > 0 to destroy it.
    1 bit for side invincibility, x 4 sides.  some sides can be impervious to damage...
    4 bits for surface property, x 4 sides 
      see common.h for information
*/
uint32_t sprite_info[128] CCM_MEMORY; 

struct object object[MAX_OBJECTS] CCM_MEMORY;
uint8_t first_free_object_index CCM_MEMORY;
uint8_t first_used_object_index CCM_MEMORY;

uint8_t draw_order[MAX_OBJECTS] CCM_MEMORY;
uint8_t drawing_count CCM_MEMORY; 
uint8_t first_drawing_index CCM_MEMORY; 
uint8_t last_drawing_index CCM_MEMORY; 

void sprites_init()
{
    // setup the objects and the linked list:
    for (int i=0; i<MAX_OBJECTS; ++i)
    {
        object[i] = (struct object) {
            .next_object_index = i+1,
            .previous_object_index = i-1,
        };
    }
    object[MAX_OBJECTS-1].next_object_index = 255;
    first_free_object_index = 0;
    first_used_object_index = 255;

    drawing_count = 0;
}

void make_unseen_object_viewable(int i)
{
    // object is viewable, but hasn't gone on the drawing list:
    object[i].iy = object[i].y - tile_map_y;
    object[i].ix = object[i].x - tile_map_x;
    object[i].draw_order_index = drawing_count;
    draw_order[drawing_count] = i;

    ++drawing_count;
}

int on_screen(int16_t x, int16_t y)
{
    if (x <= tile_map_x-16 || x >= tile_map_x + SCREEN_W)
        return 0;
    if (0) // non-platformers
    {
        if (y >= tile_map_y + SCREEN_H || y <= tile_map_y-16)
            return 0;
    }
    return 1;
}

uint8_t create_object(uint8_t sprite_index, int16_t x, int16_t y, uint8_t z)
{
    if (first_free_object_index == 255)
        return -1;

    // update free list:
    uint8_t i = first_free_object_index;
    first_free_object_index = object[i].next_object_index;
    // update the old root's previous index:
    object[first_used_object_index].previous_object_index = i;
    // make object i the head of the used list:
    object[i].next_object_index = first_used_object_index;
    object[i].previous_object_index = 255; // object i is root
    first_used_object_index = i;

    // add in object properties
    object[i].sprite_index = sprite_index;
    object[i].health = 15;
    object[i].blink = 0;
    object[i].y = y;
    object[i].x = x;
    object[i].z = z;
    object[i].cmd_index = 0;
    object[i].wait = 0;
    object[i].edge_behavior = 8;
    object[i].acceleration = 5;
    object[i].speed = 4;
    object[i].jump_speed = 5;
    object[i].properties = 0;
    object[i].blocked = 0;
    object[i].firing = 0;
    if (on_screen(object[i].x, object[i].y))
        make_unseen_object_viewable(i);
    else
        object[i].draw_order_index = -1;
    return i;
}

void update_object(int i)
//int16_t x, int16_t y), object[index].x, object[index].y
{
    if (object[i].draw_order_index < 255) // object was visible...
    {
        if (on_screen(object[i].x, object[i].y) || i == player_index[0])
        {
            // object is still visible, need to sort draw_order.. but do it later!
            object[i].iy = object[i].y - tile_map_y;
            object[i].ix = object[i].x - tile_map_x;
            if (object[i].z)
                object_run_commands(i);
        }
        else // object is no longer visible, but still exists
        {
            remove_object_from_view(i);
        }
    }
    else // wasn't visible
    {
        if (on_screen(object[i].x, object[i].y))
        {
            // object has become visible
            make_unseen_object_viewable(i);
            if (object[i].z)
                object_run_commands(i);
        }
        else // object is still not visible
        {
            // do nothing!
        }
    }
}

void update_object_image(int i)
//int16_t x, int16_t y), object[index].x, object[index].y
{
    if (object[i].draw_order_index < 255) // object was visible...
    {
        if (on_screen(object[i].x, object[i].y))
        {
            // object is still visible, need to sort draw_order.. but do it later!
            object[i].iy = object[i].y - tile_map_y;
            object[i].ix = object[i].x - tile_map_x;
        }
        else // object is no longer visible, but still exists
        {
            remove_object_from_view(i);
        }
    }
    else // wasn't visible
    {
        if (on_screen(object[i].x, object[i].y))
        {
            // object has become visible
            make_unseen_object_viewable(i);
        }
        else // object is still not visible
        {
            // do nothing!
        }
    }
}

void remove_object(int index)
{
    if (player_index[0] == index)
        kill_player(0);
    if (player_index[1] == index)
        kill_player(1);
    // free the object by linking previous indices to next indices and vice versa:
    int previous_index = object[index].previous_object_index;
    int next_index = object[index].next_object_index;
    if (previous_index == 255)
        // we were at the head of the list, update root:
        first_used_object_index = next_index;
    else
        object[previous_index].next_object_index = next_index;
    if (next_index < 255)
        // update the next object's link backwards:
        object[next_index].previous_object_index = previous_index;
    // put index at the head of the free list:
    object[index].previous_object_index = 255;
    object[index].next_object_index = first_free_object_index;
    first_free_object_index = index;
    // and remove object from view
    if (object[index].draw_order_index < 255)
        remove_object_from_view(index);
}

void remove_object_from_view(int i)
{
    for (int k=object[i].draw_order_index+1; k<drawing_count; ++k)
    {
        // move the pointers around for the objects draw_order_index:
        // object at draw_order k should go to spot k-1:
        int object_k = draw_order[k];
        // move down draw_order:
        object[object_k].draw_order_index = k-1;
        draw_order[k-1] = object_k;
    }
    object[i].iy = SCREEN_H;
    object[i].ix = SCREEN_W;
    object[i].draw_order_index = -1;
    --drawing_count;
}

void update_objects()
{
    int index;
    int next_index = first_used_object_index;
    while ((index=next_index) < 255)
    {
        next_index = object[index].next_object_index;
        update_object(index);
    }
}

void sprites_line()
{
    if (!drawing_count) // none to draw...
        return;
    int16_t vga16 = (int16_t) vga_line;
    // add any new sprites to the draw line:
    while (last_drawing_index < drawing_count)
    {
        if (object[draw_order[last_drawing_index]].iy <= vga16) 
            ++last_drawing_index;
        else
            break;
    }
    // subtract any early sprites no longer on the line
    vga16-=16;
    while (first_drawing_index < drawing_count)
    {
        if (object[draw_order[first_drawing_index]].iy <= vga16) 
            ++first_drawing_index;
        else
            break;
    }
    // reset vga16
    vga16+=16;
    // draw on-screen and currently visible (on this line) sprites
    for (int k=first_drawing_index; k<last_drawing_index; ++k)
    {
        struct object *o = &object[draw_order[k]];
        if (!o->z)
            continue; // hidden object
        int sprite_draw_row = vga16 - o->iy;
        uint8_t *U8 = U8row + 15 + o->ix;
        uint8_t *src = &sprite_draw[o->sprite_index][sprite_draw_row][0]-1;
        int invisible_color = sprite_info[o->sprite_index] & 31;
        int offset = ((o->blink) && (vga_frame/8%2)) ? 16 : 0;
        for (int pxl=0; pxl<8; ++pxl)
        {
            int two_color = *(++src);
            int color = two_color&15;
            if (color != invisible_color)
                *++U8 = color | offset; 
            else
                ++U8; 
            color = two_color>>4;
            if (color != invisible_color)
                *++U8 = color | offset;
            else
                ++U8; 
        }
    }
}

static inline void swap_draw_order_k_kminus1(int k)
{
    int object_kminus1 = draw_order[k-1];
    int object_k = draw_order[k];

    object[object_kminus1].draw_order_index = k;
    object[object_k].draw_order_index = k-1;

    draw_order[k-1] = object_k;
    draw_order[k] = object_kminus1;
}

void sprites_frame()
{
    // insertion sort all the drawing objects on the map
    for (int j=0; j<drawing_count-1; ++j)
    {
        // move object k as far up as it should be.
        // lower z is drawn earlier, z=0 means the object is hidden inside a tile.
        for (int k=j+1; k > 0 && ((object[draw_order[k-1]].iy > object[draw_order[k]].iy) ||
            ((object[draw_order[k-1]].iy == object[draw_order[k]].iy) &&
             (object[draw_order[k-1]].z > object[draw_order[k]].z)) ); --k)
        {
            swap_draw_order_k_kminus1(k);
        }
    }
    
    first_drawing_index = 0;
    last_drawing_index = 0;
}

void sprites_load_default()
{
    // create some random sprites...
    uint8_t *sc = sprite_draw[0][0];
    for (int tile=0; tile<16; ++tile)
    {
        memset(sc, tile*17, 128*8);
        int alt_color = ((tile+1)&15)*17;
        for (int frame=0; frame<8; ++frame)
        {
            sprite_info[8*tile+frame] = 16;
            switch (frame/2) {
                case RIGHT:
                    for (int row=0; row<16; ++row)
                        sc[row*8 + 7] = alt_color;
                    break;
                case UP:
                    memset(sc, alt_color, 8*2);
                    break;
                case LEFT:
                    for (int row=0; row<16; ++row)
                        sc[row*8] = alt_color;
                    break;
                case DOWN:
                    memset(sc+14*8, alt_color, 8*2);
                    break;
            }
            sc += 128;
        }
    }
}

void update_object_images()
{
    int index = first_used_object_index;
    while (index < 255)
    {
        update_object_image(index);
        index = object[index].next_object_index;
    }
}
