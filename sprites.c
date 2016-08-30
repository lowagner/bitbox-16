#include "bitbox.h"
#include "sprites.h"
#include "tiles.h"

#include <stdlib.h> // rand

// break sprites up into 16x16 tiles:
uint8_t sprite_draw[16][8][16][8] CCM_MEMORY; // 16 sprites, 8 frames, 16x16 pixels...
/*
info about a sprite (first frame, frame 0):
    1 bit for Block-like.
    if Block-like:
        4 bits ignored (what could we put here?)
    else:
        4 bits for "what color is invisible in this sprite"
    3 bits for inverse weight
    4 bits for vulnerability, must have (attack & vulnerability) > 0 to destroy it.
    1 bit for impervious, x 4 sides.  some sides can be impervious to damage...
    4 bits for surface property, x 4 sides 
      see common.h for information
*/
uint32_t sprite_info[16][8] CCM_MEMORY; 

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

void make_unseen_object_viewable(uint8_t i)
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
    if (x > tile_map_x-16 && x < tile_map_x + SCREEN_W &&
        y > tile_map_y-16 && y < tile_map_y + SCREEN_H)
        return 1;
    return 0;
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
    object[i].y = y;
    object[i].x = x;
    object[i].z = z;
    if (on_screen(object[i].x, object[i].y))
        make_unseen_object_viewable(i);
    else
        object[i].draw_order_index = -1;

    object[i].sprite_index = sprite_index;
    return i;
}

void move_object(uint8_t i, int16_t x, int16_t y)
{
    if (object[i].draw_order_index < 255) // object was visible...
    {
        if (on_screen(x, y))
        {
            // object is still visible, need to sort draw_order.. but do it later!
            object[i].iy = y - tile_map_y;
            object[i].ix = x - tile_map_x;
        }
        else // object is no longer visible, but still exists
        {
            remove_object_from_view(i);
        }
    }
    else // wasn't visible
    {
        if (on_screen(x, y))
        {
            // object has become visible
            make_unseen_object_viewable(i);
        }
        else // object is still not visible
        {
            // do nothing!
        }
    }
    object[i].y = y;
    object[i].x = x;
}

void remove_object(uint8_t index)
{
    // free the object by linking previous indices to next indices and vice versa:
    uint8_t previous_index = object[index].previous_object_index;
    uint8_t next_index = object[index].next_object_index;
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

void remove_object_from_view(uint8_t i)
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
    uint8_t index = first_used_object_index;
    while (index < 255)
    {
        move_object(index, object[index].x, object[index].y);
        index = object[index].next_object_index;
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
        if (o->ix < 0) // object left of screen but still visible
        {
            uint16_t *dst = draw_buffer;
            uint8_t *src = &sprite_draw[o->sprite_index][o->sprite_frame][sprite_draw_row][-o->ix/2];
            uint8_t invisible_color = sprite_info[o->sprite_index][o->sprite_frame] & 31;
            if ((-o->ix)%2)
            {
                // if -o->ix == 15
                // this executes no matter what:
                uint8_t color = ((*src)>>4); //&15; //unnecessary!
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst;
                // if (o->ix == -13)
                // we run from 7 to <8, which is just once:
                for (int pxl=(-o->ix+1)/2; pxl<16/2; ++pxl) // 
                {
                    color = ((*(++src)))&15;
                    if (color != invisible_color)
                        *dst = palette[color]; // &65535; // unnecessary...
                    ++dst; 
                    color = ((*src)>>4); //&15; //unnecessary!
                    if (color != invisible_color)
                        *dst = palette[color]; // &65535; // unnecessary...
                    ++dst; 
                }
            }
            else // not odd
            {
                --src;
                for (int pxl=-o->ix/2; pxl<16/2; ++pxl)
                {
                    uint8_t color = (*(++src))&15;
                    if (color != invisible_color)
                        *dst = palette[color]; // &65535; // unnecessary...
                    ++dst; 
                    color = ((*src)>>4); //&15; //unnecessary!
                    if (color != invisible_color)
                        *dst = palette[color]; // &65535; // unnecessary...
                    ++dst; 
                }
            }
        }
        else if (o->ix > SCREEN_W-16)
        {
            // object right of screen but still visible
            int num_pxls = 1 + (SCREEN_W-1) - o->ix;
            uint16_t *dst = draw_buffer + o->ix;
            uint8_t *src = &sprite_draw[o->sprite_index][o->sprite_frame][sprite_draw_row][0]-1;
            uint8_t invisible_color = sprite_info[o->sprite_index][o->sprite_frame] & 31;
            uint8_t color;
            for (int pxl=0; pxl<num_pxls/2; ++pxl)
            {
                color = (*(++src))&15;
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst; 
                color = ((*src)>>4); //&15; //unnecessary!
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst; 
            }
            if (num_pxls%2)
            {
                color = (*(++src))&15;
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
            }
        }
        else
        {
            // object is somewhere in the middle of the screen
            uint16_t *dst = draw_buffer + o->ix;
            uint8_t *src = &sprite_draw[o->sprite_index][o->sprite_frame][sprite_draw_row][0]-1;
            uint8_t invisible_color = sprite_info[o->sprite_index][o->sprite_frame] & 31;
            for (int pxl=0; pxl<8; ++pxl)
            {
                uint8_t color = (*(++src))&15;
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst; 
                color = ((*src)>>4); //&15; //unnecessary!
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst; 
            }
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

void sprites_reset()
{
    // create some random sprites...
    uint8_t *sc = sprite_draw[0][0][0];
    int color_index = 0;
    for (int tile=0; tile<15; ++tile)
    {
        for (int frame=0; frame<8; ++frame)
        {
            sprite_info[tile][frame] = 16;
            int forward = frame % 2;
            for (int line=0; line<16; ++line)
            {
                for (int col=0; col<8; ++col)
                    *sc++ = (((color_index+1)>>(tile/4))&15)|((((color_index - forward)>>(tile/4))&15)<<4);
                ++color_index;
            }
            color_index -= 16;
        }
        ++color_index;
    }
    // 16th sprite is random
    for (int l=0; l<8; ++l)
    {
        sprite_info[15][l] = 0; // with black as invisible for all frames
        for (int k=0; k<256/2; ++k)
            *sc++ = rand()%256;
    }
}
