#include "bitbox.h"
#include "common.h"
#include "sprites.h"
#include "tiles.h"
#include "edit.h"
#include "save.h"
#include "fill.h"
#include "font.h"
#include "io.h"
#include <stdint.h>
#include <stdlib.h> // abs
#include <string.h> // memset

int16_t map_tile_y CCM_MEMORY, map_tile_x CCM_MEMORY;
uint8_t map_color[2] CCM_MEMORY, map_last_painted CCM_MEMORY;
uint8_t map_menu_not_edit CCM_MEMORY;
uint8_t map_Y_not_X CCM_MEMORY;
uint8_t map_sprite CCM_MEMORY;
uint8_t map_sprite_under_cursor CCM_MEMORY;

#define MAP_HEADER 32 // and footer.  make it a multiple of 16

void map_init()
{
    map_tile_x = 0;
    map_tile_y = 0;
    map_color[0] = 0;
    map_color[1] = 1;
    map_last_painted = 0;
    map_menu_not_edit = 0;
    map_sprite = 0;
    map_sprite_under_cursor = 255;
}

void map_switch()
{
    if (map_tile_x*16 < tile_map_x)
        tile_map_x = map_tile_x*16;
    else if (map_tile_x*16 >= tile_map_x + SCREEN_W)
        tile_map_x = map_tile_x*16 - SCREEN_W + 16;
    else if (tile_map_x % 16)
        tile_map_x =16*(tile_map_x/16);
    
    if (map_tile_y*16 < tile_map_y + MAP_HEADER)
        tile_map_y = map_tile_y*16 - MAP_HEADER;
    else if (map_tile_y*16 >= tile_map_y + SCREEN_H - MAP_HEADER)
        tile_map_y = map_tile_y*16 - SCREEN_H + MAP_HEADER + 16;
    else if (tile_map_y % 16)
        tile_map_y =16*(tile_map_y/16);
   
    update_objects(); 
}

void map_reset()
{
    tile_map_width = 26;
    tile_map_height = 20;

    for (int j=0; j<tile_map_height; ++j)
    for (int i=0; i<tile_map_width/2; ++i)
    {
        if (j <= (tile_map_height-16))
            tile_map[j*(tile_map_width/2)+i] = 0;
        else
            tile_map[j*(tile_map_width/2)+i] = ((j - tile_map_height + 16)%16)|(((j - tile_map_height + 16)%16)<<4);
    }
   
    for (int k=0; k<MAX_OBJECTS; ++k)
        create_object(k%16, 16*(1+rand()%(tile_map_width-2)), 16*(1+rand()%(tile_map_height-2)), 2+rand()%128);
}

void map_line()
{
    if (vga_line < MAP_HEADER)
    {
        if (vga_line >= MAP_HEADER - 4)
        {
            if (vga_line/2 == (MAP_HEADER-4)/2)
                memset(draw_buffer, 0, 2*SCREEN_W);
        } 
        else if (vga_line >= MAP_HEADER - 4 - 8)
        {
            if (game_message[0])
                font_render_line_doubled(game_message,
                    16, vga_line - (MAP_HEADER - 4 - 8), 65535, 0);
            else if (!map_menu_not_edit)
                font_render_line_doubled((const uint8_t *)"start:menu A:fill X:edit tile",
                    28, vga_line - (MAP_HEADER - 4 - 8), 65535, 0);
            else if (map_sprite_under_cursor < 255)
                font_render_line_doubled((const uint8_t *)"start:edit A:play X:cut sprite",
                    28, vga_line - (MAP_HEADER - 4 - 8), 65535, 0);
            else
                font_render_line_doubled((const uint8_t *)"start:edit A:play X:save map",
                    28, vga_line - (MAP_HEADER - 4 - 8), 65535, 0);
        }
        else
        {
            if (vga_line/2 == 0 || vga_line/2 == (MAP_HEADER - 4 - 10)/2)
                memset(draw_buffer, 0, 2*SCREEN_W);
            else if (vga_line >= MAP_HEADER - 4 - 10 - 8) // 14+8 =  22
            {
                uint8_t msg[] = { 
                    'X', hex[map_tile_x/100], '0' + (map_tile_x/10)%10, '0' + (map_tile_x%10), 
                    ' ', ' ',
                    'Y', hex[map_tile_y/100], '0' + (map_tile_y/10)%10, '0' + (map_tile_y%10), 
                0 };
                font_render_line_doubled(msg, 28, vga_line - (MAP_HEADER - 4 - 10 - 8), RGB(100,100,100), 0);
            }
        }
        return;
    }
    else if (vga_line >= SCREEN_H-MAP_HEADER)
    {
        if (vga_line/2 == (SCREEN_H-MAP_HEADER)/2)
            memset(draw_buffer, 0, 2*SCREEN_W); 
        else if (vga_line >= SCREEN_H - MAP_HEADER + 20)
        {
            if (vga_line/2 == (SCREEN_H - MAP_HEADER + 20)/2)
            { 
                memset(draw_buffer, 0, 2*SCREEN_W); 
                if (map_menu_not_edit && vga_line == SCREEN_H - MAP_HEADER + 4 + 8+9)
                    memset(&draw_buffer[158+7*9+map_Y_not_X*9*5], 229, 3*9*2);
            }
            else if (vga_line/2 == (SCREEN_H-1)/2)
            {
                memset(draw_buffer, 0, 2*SCREEN_W); 
            }
            else if (map_menu_not_edit && vga_line >= SCREEN_H - MAP_HEADER + 4 + 8+10)
            {
                font_render_line_doubled((const uint8_t *)"B^", 82, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8+10), 65535, 0);
            }
        }
        else if (vga_line >= SCREEN_H-MAP_HEADER+4)
        {
            int tile_j = vga_line - (SCREEN_H - MAP_HEADER + 4);
            if (map_menu_not_edit)
            {
                uint16_t *dst = draw_buffer + 24 + 19;
                uint8_t *sprite_color = &sprite_draw[(map_sprite/8-1)&15][map_sprite%8][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++sprite_color))&15];
                    *(++dst) = palette[(*sprite_color)>>4];
                }
                dst += 40-16;
                sprite_color = &sprite_draw[map_sprite/8][map_sprite%8][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++sprite_color))&15];
                    *(++dst) = palette[(*sprite_color)>>4];
                }
                dst += 40-16;
                sprite_color = &sprite_draw[(map_sprite/8+1)&15][map_sprite%8][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++sprite_color))&15];
                    *(++dst) = palette[(*sprite_color)>>4];
                }

                if (vga_line >= SCREEN_H - MAP_HEADER + 4 + 8)
                {
                    font_render_line_doubled((const uint8_t *)"L:", 24, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8), 65535, 0);
                    font_render_line_doubled((const uint8_t *)"Y:", 64, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8), 65535, 0);
                    font_render_line_doubled((const uint8_t *)"R:", 104, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8), 65535, 0);
                    uint8_t msg[] = { 'd', 'p', 'a', 'd', ':', ' ', 
                        'W', '0'+(tile_map_width/100), '0'+((tile_map_width/10)%10), '0'+(tile_map_width%10),
                        ' ',
                        'H', '0'+(tile_map_height/100), '0'+((tile_map_height/10)%10), '0'+(tile_map_height%10),
                        0 };
                    font_render_line_doubled(msg, 158, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8), 65535, 0);
                }
            }
            else if (map_last_painted == 0) // draw the tiles you can paint with
            {
                uint16_t *dst = draw_buffer + 24 + 19;
                uint8_t *tile_color = &tile_draw[(map_color[0]-1)&15][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++tile_color))&15];
                    *(++dst) = palette[(*tile_color)>>4];
                }
                dst += 40-16;
                tile_color = &tile_draw[map_color[0]][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++tile_color))&15];
                    *(++dst) = palette[(*tile_color)>>4];
                }
                dst += 40-16;
                tile_color = &tile_draw[(map_color[0]+1)&15][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++tile_color))&15];
                    *(++dst) = palette[(*tile_color)>>4];
                }
                dst += 104-16;
                tile_color = &tile_draw[map_color[1]][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++tile_color))&15];
                    *(++dst) = palette[(*tile_color)>>4];
                }
                if (vga_line >= SCREEN_H - MAP_HEADER + 4 + 8)
                {
                    font_render_line_doubled((const uint8_t *)"L:", 24, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8), 65535, 0);
                    font_render_line_doubled((const uint8_t *)"Y:", 64, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8), 65535, 0);
                    font_render_line_doubled((const uint8_t *)"R:", 104, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8), 65535, 0);
                    font_render_line_doubled((const uint8_t *)"B:", 208, vga_line - (SCREEN_H - MAP_HEADER + 4 + 8), 65535, 0);
                }
            }
            else
            {
                uint16_t *dst = draw_buffer + 64 + 19;
                uint8_t *tile_color = &tile_draw[map_color[0]][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++tile_color))&15];
                    *(++dst) = palette[(*tile_color)>>4];
                }
                dst += 104 - 16;
                tile_color = &tile_draw[(map_color[1]-1)&15][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++tile_color))&15];
                    *(++dst) = palette[(*tile_color)>>4];
                }
                dst += 40-16;
                tile_color = &tile_draw[map_color[1]][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++tile_color))&15];
                    *(++dst) = palette[(*tile_color)>>4];
                }
                dst += 40-16;
                tile_color = &tile_draw[(map_color[1]+1)&15][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *(++dst) = palette[(*(++tile_color))&15];
                    *(++dst) = palette[(*tile_color)>>4];
                }
                if (vga_line >= SCREEN_H - MAP_HEADER + 4 + 8)
                {
                    font_render_line_doubled((const uint8_t *)"Y:", 64, vga_line - (SCREEN_H - MAP_HEADER + 12), 65535, 0);
                    font_render_line_doubled((const uint8_t *)"L:", 164, vga_line - (SCREEN_H - MAP_HEADER + 12), 65535, 0);
                    font_render_line_doubled((const uint8_t *)"B:", 208, vga_line - (SCREEN_H - MAP_HEADER + 12), 65535, 0);
                    font_render_line_doubled((const uint8_t *)"R:", 248, vga_line - (SCREEN_H - MAP_HEADER + 12), 65535, 0);
                }
            }
        }
        return;
    }

    tiles_line();
    if (map_menu_not_edit || (vga_frame/32) % 4)
        sprites_line();
   
    // draw the cursor, if on this line:
    int tile_j = tile_map_y + vga_line; 
    if (tile_j/16 == map_tile_y)
    switch (tile_j%16)
    {
        case 7:
        case 8:
        {
            uint32_t *dst = (uint32_t *)draw_buffer + (map_tile_x*16 - tile_map_x + 6)/2;
            *dst = ~(*dst);
            ++dst;
            *dst = ~(*dst);
            break;
        }
        case 6:
        case 9:
        {
            uint16_t *dst = draw_buffer + (map_tile_x)*16 - tile_map_x + 7;
            *dst = ~(*dst);
            ++dst;
            *dst = ~(*dst);
            break;
        }
    }
}

void map_spot_paint(uint8_t p)
{
    map_last_painted = p;

    int index = map_tile_y * tile_map_width + map_tile_x;
    uint8_t *memory = &tile_map[index/2];

    if (index % 2)
        *memory = ((*memory)&15) | (map_color[p]<<4);
    else
        *memory = (map_color[p]) | ((*memory) & 240);
}

int map_spot_color()
{
    int index = map_tile_y * tile_map_width + map_tile_x;
    const uint8_t *memory = &tile_map[index/2];

    if (index % 2)
        return (*memory) >> 4;
    else
        return (*memory) & 15;
}

void map_spot_fill(uint8_t p)
{
    map_last_painted = p;

    if (!fill_can_start())
        fill_stop();
    uint8_t previous_canvas_color = map_spot_color();
    if (previous_canvas_color != map_color[p])
    {
        fill_init(tile_map, tile_map_width, tile_map_height, 
            previous_canvas_color, map_tile_x, map_tile_y, map_color[p]);
    }
}

void map_controls()
{
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        // switch between editing and menu
        if (map_menu_not_edit)
        {
            map_menu_not_edit = 0;
            map_sprite_under_cursor = 255;
        }
        else
        {
            map_menu_not_edit = 1;

            // do not allow sprites to be placed at edges of map:
            if (map_tile_x == 0)
                map_tile_x = 1;
            else if (map_tile_x == tile_map_width-1)
                map_tile_x = tile_map_width-2;
            
            if (map_tile_y == 0)
                map_tile_y = 1;
            else if (map_tile_y == tile_map_height-1)
                map_tile_y = tile_map_height-2;
        
            // check for a sprite under the cursor:
            map_sprite_under_cursor = 255;
            for (int draw_index=0; draw_index<drawing_count; ++draw_index)
            { 
                uint8_t index = draw_order[draw_index];
                if (object[index].y == map_tile_y*16 && 
                    object[index].x == map_tile_x*16)
                {
                    map_sprite_under_cursor = index;
                    break;
                }
            }
        }
        return;
    }

    if (map_menu_not_edit)
    {
        int make_wait = 0;
        if (GAMEPAD_PRESSING(0, R))
        {
            game_message[0] = 0;
            map_sprite = (map_sprite+8)&127;
            make_wait = 1;
        }
        if (GAMEPAD_PRESSING(0, L))
        {
            game_message[0] = 0;
            map_sprite = (map_sprite-8)&127;
            make_wait = 1;
        }
        if (GAMEPAD_PRESSING(0, B))
        {
            game_message[0] = 0;
            if ((++map_sprite) % 8 == 0)
                map_sprite -= 8;
            make_wait = 1;
        }
       
        int modified = 0;
        if (GAMEPAD_PRESS(0, X))
        {
            // cut sprite under cursor, if there is one, otherwise save map
            if (map_sprite_under_cursor == 255)
            {
                // save
                io_message_from_error(game_message, io_save_map(), 1);
                gamepad_press_wait = GAMEPAD_PRESS_WAIT;
                return;
            }
            game_message[0] = 0;
            // copy the sprite property into map_sprite:
            map_sprite = 8*object[map_sprite_under_cursor].sprite_index + object[map_sprite_under_cursor].sprite_frame;
            remove_object(map_sprite_under_cursor);
            map_sprite_under_cursor = 255;
            modified = 1;
        }
        if (GAMEPAD_PRESS(0, Y))
        {
            game_message[0] = 0;
            // paste sprite in here
            // first check if we should delete something first.
            if (map_sprite_under_cursor < 255)
            {
                // just reuse the sprite that was there
                object[map_sprite_under_cursor].sprite_index = map_sprite/8;
                object[map_sprite_under_cursor].sprite_frame = map_sprite%8;
                gamepad_press_wait = GAMEPAD_PRESS_WAIT;
                return;
            }
            // no object found under cursor, create one:
            map_sprite_under_cursor = create_object(map_sprite/8, 16*map_tile_x, 16*map_tile_y, 0);
            if (map_sprite_under_cursor == 255) // could not create one
            {
                strcpy((char *)game_message, "too many sprites");
                gamepad_press_wait = GAMEPAD_PRESS_WAIT;
                return;
            }
            object[map_sprite_under_cursor].sprite_frame = map_sprite%8;
            modified = 1;
        }
        if (modified)
        {
            update_objects(); 
            return;
        }
        
        if (GAMEPAD_PRESSING(0, up))
        {
            game_message[0] = 0;
            if (map_Y_not_X)
            {
                if (((tile_map_height+1)*(tile_map_width)+1)/2 <= TILE_MAP_MEMORY)
                    ++tile_map_height;
            }
            else
            {
                if (((tile_map_height)*(tile_map_width+1)+1)/2 <= TILE_MAP_MEMORY)
                    ++tile_map_width;
            }
            make_wait = 1;
        }
        if (GAMEPAD_PRESSING(0, down))
        {
            game_message[0] = 0;
            if (map_Y_not_X)
            {
                if (tile_map_height > 17) // 240/16 = 15, but we want above and below a tile
                    --tile_map_height;
            }
            else
            {
                if (tile_map_width > 22) // 320/16 = 20, but we want left and right a tile
                    --tile_map_width;
            }
            make_wait = 1;
        }
        
        if (make_wait)
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        
        if (GAMEPAD_PRESS(0, left) || GAMEPAD_PRESS(0, right))
        {
            map_Y_not_X = 1 - map_Y_not_X;
            game_message[0] = 0;
            return;
        }
        
        if (GAMEPAD_PRESSING(0, A))
        {
            previous_visual_mode = None;
            game_switch(GameOn);
            return;
        }
    }
    else
    {
        int make_wait = 0;
        if (GAMEPAD_PRESSING(0, R))
        {
            game_message[0] = 0;
            map_color[map_last_painted] = (map_color[map_last_painted] + 1)&15;
            make_wait = 1;
        }
        if (GAMEPAD_PRESSING(0, L))
        {
            game_message[0] = 0;
            map_color[map_last_painted] = (map_color[map_last_painted] - 1)&15;
            make_wait = 1;
        }
        
        if (GAMEPAD_PRESSING(0, A))
        {
            game_message[0] = 0;
            map_spot_fill(map_last_painted);
            make_wait = 1;
        }
        if (GAMEPAD_PRESSING(0, X))
        {
            game_message[0] = 0;
            map_color[map_last_painted] = map_spot_color();
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            previous_visual_mode = EditMap;
            game_switch(EditTileOrSprite);
            // TODO:  maybe go to sprite if a sprite was selected.
            edit_sprite_not_tile = 0;
            edit_tile = map_color[map_last_painted];
            return;
        }

        int paint_if_moved = 0; 
        if (GAMEPAD_PRESSING(0, Y))
        {
            game_message[0] = 0;
            map_spot_paint(0);
            paint_if_moved = 1;
        }
        else if (GAMEPAD_PRESSING(0, B))
        {
            game_message[0] = 0;
            map_spot_paint(1);
            paint_if_moved = 2;
        }
        
        int moved = 0;
        if (GAMEPAD_PRESSING(0, left))
        {
            if (map_tile_x > 0)
            {
                --map_tile_x;
                make_wait = 1;
                if (map_tile_x < tile_map_x/16)
                {
                    tile_map_x -= 16;
                    moved = 1;
                }
            }
            else
            {
                map_tile_x = tile_map_width-1;
                tile_map_x = map_tile_x*16 - SCREEN_W + 16;
                moved = 1;
            }
        }
        else if (GAMEPAD_PRESSING(0, right))
        {
            if (map_tile_x < tile_map_width - 1)
            {
                ++map_tile_x;
                make_wait = 1;
                if (map_tile_x >= tile_map_x/16 + SCREEN_W/16)
                {
                    tile_map_x += 16;
                    moved = 1;
                }
            }
            else
            {
                map_tile_x = 0;
                tile_map_x = 0;
                moved = 1;
            }
        }
        if (GAMEPAD_PRESSING(0, up))
        {
            if (map_tile_y > 0)
            {
                --map_tile_y;
                make_wait = 1;
                if (map_tile_y < tile_map_y/16 + MAP_HEADER/16) // 0 < -32/16 + MAP_HEADER/16
                {
                    tile_map_y -= 16;
                    moved = 1;
                }
            }
            else
            {
                map_tile_y = tile_map_height-1;
                tile_map_y = map_tile_y*16 - SCREEN_H + MAP_HEADER + 16;
                moved = 1;
            }
        }
        else if (GAMEPAD_PRESSING(0, down))
        {
            if (map_tile_y < tile_map_height - 1)
            {
                ++map_tile_y;
                make_wait = 1;
                if (map_tile_y >= tile_map_y/16 + (SCREEN_H-MAP_HEADER)/16)
                {
                    tile_map_y += 16;
                    moved = 1;
                }
            }
            else
            {
                map_tile_y = 0;
                tile_map_y = -MAP_HEADER;
                moved = 1;
            }
        }
        if (moved)
        {
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            if (paint_if_moved)
                map_spot_paint(paint_if_moved-1);
            update_objects(); 
            return;
        }
        else if (paint_if_moved || make_wait)
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
    }

    if (GAMEPAD_PRESS(0, select))
    {
        previous_visual_mode = None;
        game_switch(EditTileOrSprite);
        edit_sprite_not_tile = 0;
        return;
    }
}
