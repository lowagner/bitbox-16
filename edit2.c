#include "bitbox.h"
#include "common.h"
#include "font.h"
#include "edit.h"
#include "name.h"
#include "io.h"

#include <string.h> // memset

#define TILE_COLOR 136 // a uint8_t, uint16_t color is (_COLOR)|(_COLOR<<8)
#define SPRITE_COLOR 164
#define BG_COLOR (edit_sprite_not_tile ? SPRITE_COLOR : TILE_COLOR)

#define NUMBER_LINES 17

uint8_t edit2_copying CCM_MEMORY; // 0 for not copying, 1 for sprite, 2 for tile
uint8_t edit2_copy_location CCM_MEMORY;
uint8_t edit2_cursor CCM_MEMORY;

void edit2_init()
{
    edit2_copying = 0;
}

void edit2_line()
{
    static uint8_t edit2_side = 0;
    if (vga_line < 22)
    {
        if (vga_line/2 == 0)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    else if (vga_line >= 22 + NUMBER_LINES*10)
    {
        if (vga_line == 22+NUMBER_LINES*10)
        {
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
            return;
        }
        draw_parade(vga_line - (23 + NUMBER_LINES*10), BG_COLOR);
        return;
    }
    int line = (vga_line-22) / 10;
    int internal_line = (vga_line-22) % 10;
    if (internal_line == 0 || internal_line == 9)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        if (edit2_cursor < 4)
        {
            if (line == 12)
            {
                memset(&draw_buffer[16+6*9+8*9*edit2_cursor], 229, 9*2);
            }
        }
        else if (line == 13)
        {
            if (edit_sprite_not_tile || tile_info[edit_tile]&8)
                memset(&draw_buffer[16+7*9], 229, 9*2*5);
            else if (tile_info[edit_tile]&(1<<13)) // a warp or a load level
                memset(&draw_buffer[16+6*9+9*10*(edit2_cursor-4)], 229, 9*2*3);
            else // current
            {
                memset(&draw_buffer[16+11*9+9*9*(edit2_cursor-4)], 229, 9*2*2);
            }
        }
    }
    else
    {
        --internal_line;
        switch (line)
        {
        case 0:
            if (edit_sprite_not_tile)
            {

                uint8_t label[] = { 's', 'p', 'r', 'i', 't', 'e', ' ', hex[edit_sprite/8], '.', 
                    direction[(edit_sprite%8)/2], hex[(edit_sprite%8)%2], ' ', 'i', 'n', 0 };
                font_render_line_doubled(label, 16, internal_line, 65535, SPRITE_COLOR*257);
                font_render_line_doubled((uint8_t *)base_filename, 16+9*15, internal_line, 65535, SPRITE_COLOR*257);
            }
            else
            {
                uint8_t label[] = { 't', 'i', 'l', 'e', ' ', hex[edit_tile], ' ', 'i', 'n', 0 };
                font_render_line_doubled(label, 16, internal_line, 65535, TILE_COLOR*257);
                font_render_line_doubled((uint8_t *)base_filename, 16+9*10, internal_line, 65535, TILE_COLOR*257);
            }
            break;
        case 1: 
            break;
        case 2:
            if (edit_sprite_not_tile)
                font_render_line_doubled((const uint8_t *)"L/R:cycle sprite", 16, internal_line, 65535, SPRITE_COLOR*257);
            else
                font_render_line_doubled((const uint8_t *)"L/R:cycle tile", 16, internal_line, 65535, TILE_COLOR*257);
            break;
        case 3:
            if (edit2_copying)
                font_render_line_doubled((const uint8_t *)"A:cancel copy", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            else
                font_render_line_doubled((const uint8_t *)"A:save to file", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            break;
        case 4:
            if (edit2_copying)
                font_render_line_doubled((const uint8_t *)"B:  \"     \"", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            else
                font_render_line_doubled((const uint8_t *)"B:load from file", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            break;
        case 5:
            if (edit2_copying)
                font_render_line_doubled((const uint8_t *)"X:  \"     \"", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            else
                font_render_line_doubled((const uint8_t *)"X:copy", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            break;
        case 6:
            if (edit2_copying)
                font_render_line_doubled((const uint8_t *)"Y:paste", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            else
                font_render_line_doubled((const uint8_t *)"Y:change filename", 16+2*9, internal_line, 65535, 257*BG_COLOR);
        case 7:
            break;
        case 8:
            if (previous_visual_mode)
                font_render_line_doubled((const uint8_t *)"start:back", 16, internal_line, 65535, 257*BG_COLOR);
            else if (edit_sprite_not_tile)
                font_render_line_doubled((const uint8_t *)"start:edit sprite", 16, internal_line, 65535, 257*BG_COLOR);
            else
                font_render_line_doubled((const uint8_t *)"start:edit tile", 16, internal_line, 65535, 257*BG_COLOR);
            break;
        case 9:
            if (edit_sprite_not_tile)
                font_render_line_doubled((const uint8_t *)"select:palette menu", 16, internal_line, 65535, SPRITE_COLOR*257);
            else
                font_render_line_doubled((const uint8_t *)"select:sprite menu", 16, internal_line, 65535, TILE_COLOR*257);
            break;
        case 11:
            font_render_line_doubled((const uint8_t *)"dpad:", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 12:
        if (edit_sprite_not_tile)
        {
            uint8_t msg[32];
            uint32_t info = sprite_info[edit_sprite/8][edit_sprite%8];
            if (info&16)
                strcpy((char *)msg, "block ");
            else
                strcpy((char *)msg, "invis ");
            msg[6] = hex[info&15];
            strcpy((char *)msg+7,   "   1/m=");
            msg[14] = hex[(info>>5)&7];
            strcpy((char *)msg+15,  " vulnr ");
            msg[22] = hex[(info>>8)&15];
            strcpy((char *)msg+23, " imprv ");
            msg[30] = hex[(info>>12)&15];
            msg[31] = 0;
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            if (!(info&16))
            {
                uint32_t color = palette[info&15] | (palette[info&15]<<16);
                uint32_t *dst = (uint32_t *)&draw_buffer[16+7*9+4];
                for (int c=0; c<7; ++c)
                    *(++dst) = color;
            }
        }
        else
        {
            uint8_t msg[32];
            uint8_t param = tile_info[edit_tile]&15;
            if (tile_info[edit_tile]&8)
                strcpy((char *)msg, "solid ");
            else if (param)
                strcpy((char *)msg, "water ");
            else
                strcpy((char *)msg, "  air ");
            msg[6] = hex[param];
            strcpy((char *)msg+7, " trans ");
            msg[14] = hex[(tile_info[edit_tile]>>4)&15];
            strcpy((char *)msg+15, " time. ");
            param = (tile_info[edit_tile]>>8)&15;
            if (param)
                msg[22] = hex[param];
            else
                msg[22] = hex[16];
            if (tile_info[edit_tile]&8) // solid
            {
                strcpy((char *)msg+23, " vulnr ");
                msg[30] = hex[(tile_info[edit_tile]>>12)&15];
                msg[31] = 0;
            }
            else if (tile_info[edit_tile]&(1<<13)) // warp
            {
                if (tile_info[edit_tile]&(1<<12)) // warp to another level
                    strcpy((char *)msg+23, " gotolvl");
                else // warp within level
                    strcpy((char *)msg+23, " gotopos"); 
            }
            else 
            {
                // does the water/air tile damage you
                if (tile_info[edit_tile]&(1<<12)) 
                    strcpy((char *)msg+23, " damge:Y");
                else
                    strcpy((char *)msg+23, " damge:N"); 
            }
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
        }
        break;
        case 13:
        if (edit_sprite_not_tile || tile_info[edit_tile]&8) // solid block or sprite
        {
            uint32_t info = edit_sprite_not_tile ? sprite_info[edit_sprite/8][edit_sprite%8] :
                tile_info[edit_tile];
            uint8_t msg[32];
            if (edit2_cursor >= 4)
                edit2_side = edit2_cursor - 4;
            switch (edit2_side)
            {
                case RIGHT:
                    strcpy((char *)msg, " right ");
                    break;
                case UP:
                    strcpy((char *)msg, "   top ");
                    break;
                case LEFT:
                    strcpy((char *)msg, "  left ");
                    break;
                case DOWN:
                    strcpy((char *)msg, "bottom ");
                    break;
            }
            switch ((info>>(16+4*edit2_side))&15)
            {
                case Passable:
                    strcpy((char *)msg+7, "passable");
                    break;
                case Normal:
                    strcpy((char *)msg+7, "normal");
                    break;
                case Slippery:
                    strcpy((char *)msg+7, "slippery");
                    break;
                case SuperSlippery:
                    strcpy((char *)msg+7, "super slippery");
                    break;
                case Sticky:
                    strcpy((char *)msg+7, "sticky");
                    break;
                case SuperSticky:
                    strcpy((char *)msg+7, "super sticky");
                    break;
                case Bouncy:
                    strcpy((char *)msg+7, "bouncy");
                    break;
                case SuperBouncy:
                    strcpy((char *)msg+7, "super bouncy");
                    break;
                case Damaging:
                    strcpy((char *)msg+7, "damage");
                    break;
                case SuperDamaging:
                    strcpy((char *)msg+7, "super damage");
                    break;
                case Unlock0:
                    strcpy((char *)msg+7, "unlock 0");
                    break;
                case Unlock1:
                    strcpy((char *)msg+7, "unlock 1");
                    break;
                case Unlock2:
                    strcpy((char *)msg+7, "unlock 2");
                    break;
                case Unlock3:
                    strcpy((char *)msg+7, "unlock 3");
                    break;
                case Checkpoint:
                    strcpy((char *)msg+7, "checkpoint");
                    break;
                case Win:
                    strcpy((char *)msg+7, "win!!");
                    break;
            }
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
        }
        else if (tile_info[edit_tile]&(1<<13)) // warp
        {
            if (tile_info[edit_tile]&(1<<12)) // external warp
            {
                uint8_t msg[32];
                strcpy((char *)msg, "digits ");
                int digits = 0, number = 0;
                if (tile_info[edit_tile]&(1<<17)) // 3 digits
                {
                    digits = 3;
                    number = (tile_info[edit_tile]>>18)&1023;
                    if (number > 999)
                        number = 999;
                }
                else if (tile_info[edit_tile]&(1<<20)) // 2 digits
                {
                    digits = 2;
                    number = (tile_info[edit_tile]>>21)&127;
                    if (number > 99)
                        number = 99;
                }
                else if (tile_info[edit_tile]&(1<<23))
                {
                    digits = 1;
                    number = (tile_info[edit_tile]>>24)&15;
                    if (number > 9)
                        number = 9;
                }
                msg[7] = '0'+digits;
                if (!digits)
                {
                    msg[8] = 0;
                }
                else
                {
                    strcpy((char *)msg+8, " number ");
                    int index = 16+digits;
                    msg[index] = 0;
                    while (digits-- > 0)
                    {
                        msg[--index] = '0'+number%10;
                        number /= 10;
                    }
                }
                font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
                strcpy((char *)msg, "direxn ");
                msg[7] = hex[tile_info[edit_tile]>>28];
                msg[8] = 0;
                font_render_line_doubled(msg, 16+20*9, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                // internal warp
                int y = (tile_info[edit_tile]>>14)&16383;
                uint8_t msg[32];
                int x = y % tile_map_width;
                y /= tile_map_width;
                strcpy((char *)msg, "pos X ");
                msg[8] = '0' + x%10;
                msg[7] = '0' + (x/10)%10;
                msg[6] = '0' + ((x/10)/10)%10;
                strcpy((char *)msg+9, " pos Y ");
                msg[18] = '0' + y%10;
                msg[17] = '0' + (y/10)%10;
                msg[16] = '0' + ((y/10)/10)%10;
                msg[19] = 0;
                font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
                strcpy((char *)msg, "direxn ");
                msg[7] = hex[tile_info[edit_tile]>>28];
                msg[8] = 0;
                font_render_line_doubled(msg, 16+20*9, internal_line, 65535, BG_COLOR*257);
            }
        } 
        else // current
        {
            uint8_t msg[32];
            uint32_t current = (tile_info[edit_tile]>>14);
            if (tile_info[edit_tile]&7)
                strcpy((char *)msg, "flow angle ");
            else
                strcpy((char *)msg, "wind angle ");
            msg[11] = direction[(current&63)/16];
            msg[12] = hex[(current&63)%16];
            strcpy((char *)msg+13, " power ");
            current >>= 6;
            msg[20] = '0' + (current&63)/8;
            msg[21] = '0' + (current&63)%8;
            strcpy((char *)msg+22, " randm ");
            current >>= 6;
            msg[29] = '0' + (current&63)/8;
            msg[30] = '0' + (current&63)%8;
            msg[31] = 0;
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
        }
        break;
        case (NUMBER_LINES-2):
            font_render_line_doubled(game_message, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
    }
    if (vga_line < 22 + 2*16)
    {
        uint32_t *dst = (uint32_t *)draw_buffer + (SCREEN_W - 24 - 16*2*2)/2 - 1;
        internal_line = (vga_line - 22)/2;
        uint8_t *tile_color = edit_sprite_not_tile ?
            &sprite_draw[edit_sprite/8][edit_sprite%8][internal_line][0] - 1:
            &tile_draw[edit_tile][internal_line][0] - 1;
        for (int l=0; l<8; ++l) 
        {
            uint32_t color = palette[(*(++tile_color))&15];
            color |= color << 16;
            *(++dst) = color;
            
            color = palette[(*tile_color)>>4];
            color |= color << 16;
            *(++dst) = color;
        }
    }
    else if (vga_line/2 == (22 + 2*16)/2)
    {
        memset(draw_buffer+(SCREEN_W - 24 - 16*2*2), BG_COLOR, 64);
    }
    else if (vga_line < 22 + 2 + 2*16 + 2*16 && edit2_copying)
    {
        uint32_t *dst = (uint32_t *)draw_buffer + (SCREEN_W - 24 - 16*2)/2;
        internal_line = (vga_line - (22 + 2 + 2*16))/2;
        uint8_t *tile_color = edit2_copying == 1 ?
            &sprite_draw[edit2_copy_location/8][edit2_copy_location%8][internal_line][0] - 1:
            &tile_draw[edit2_copy_location][internal_line][0] - 1;
        for (int l=0; l<8; ++l) 
        {
            uint32_t color = palette[(*(++tile_color))&15];
            color |= color << 16;
            *(++dst) = color;
            
            color = palette[(*tile_color)>>4];
            color |= color << 16;
            *(++dst) = color;
        }
    }
}

void edit2_controls()
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
        edit2_cursor = 0;
        game_message[0] = 0;
        if (edit_sprite_not_tile)
        {
            edit_sprite = (edit_sprite + moved)&127;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        }
        else
        {
            edit_tile = (edit_tile + moved)&15;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT+GAMEPAD_PRESS_WAIT/2;
        }
        return;
    }
    if (GAMEPAD_PRESS(0, left))
    {
        if (edit2_cursor)
            --edit2_cursor;
        else if (edit_sprite_not_tile || tile_info[edit_tile]&8)
            edit2_cursor = 7;
        else
            edit2_cursor = 6;
        return;
    }
    if (GAMEPAD_PRESS(0, right))
    {
        ++edit2_cursor;
        if (edit_sprite_not_tile || tile_info[edit_tile]&8)
        {
            if (edit2_cursor > 7)
                edit2_cursor = 0;
        }
        else if (edit2_cursor > 6)
            edit2_cursor = 0;
        return;
    }
    if (GAMEPAD_PRESSING(0, up))
    {
        if (edit_sprite_not_tile)
        {
            if (edit2_cursor == 0)
            {
                uint32_t param = sprite_info[edit_sprite/8][edit_sprite%8]&(31);
                sprite_info[edit_sprite/8][edit_sprite%8] &= ~(31);
                param = (param+1)&(31);
                sprite_info[edit_sprite/8][edit_sprite%8] |= param;
            }
            else if (edit2_cursor == 1)
            {
                uint32_t param = sprite_info[edit_sprite/8][edit_sprite%8]&(7<<5);
                sprite_info[edit_sprite/8][edit_sprite%8] &= ~(7<<5);
                param = (param+(1<<5))&(7<<5);
                sprite_info[edit_sprite/8][edit_sprite%8] |= param;
            }
            else
            {
                uint32_t param = sprite_info[edit_sprite/8][edit_sprite%8]&(15<<(edit2_cursor*4));
                sprite_info[edit_sprite/8][edit_sprite%8] &= ~(15<<(edit2_cursor*4));
                param = (param+(1<<(edit2_cursor*4)))&(15<<(edit2_cursor*4));
                sprite_info[edit_sprite/8][edit_sprite%8] |= param;
            }
        }
        else if (tile_info[edit_tile]&8 || (edit2_cursor < 3))
        {
            uint32_t param = tile_info[edit_tile]&(15<<(edit2_cursor*4));
            tile_info[edit_tile] &= ~(15<<(edit2_cursor*4));
            param = (param+(1<<(edit2_cursor*4)))&(15<<(edit2_cursor*4));
            tile_info[edit_tile] |= param;
        }
        else if (edit2_cursor == 3)
        {
            uint32_t param = tile_info[edit_tile]&(3<<12);
            tile_info[edit_tile] &= ~(3<<12);
            param = (param+(1<<12))&(3<<12);
            tile_info[edit_tile] |= param;
        }
        else if (tile_info[edit_tile]&(1<<13)) // warp
        {
            if (edit2_cursor == 6)
            {
                // adjust directional warp
                uint32_t param = tile_info[edit_tile]&(15<<28);
                tile_info[edit_tile] &= ~(15<<28);
                param = (param+(1<<28))&(15<<28);
                tile_info[edit_tile] |= param; 
            }
            else if (tile_info[edit_tile]&(1<<12)) // level warp
            {
                int digits = 0;
                int number = 0, max_number = 0;
                if (tile_info[edit_tile]&(1<<17)) // 3 digits
                {
                    digits = 3;
                    number = (tile_info[edit_tile]>>18)&1023;
                    if (number > 999)
                        number = 999;
                    max_number = 999;
                }
                else if (tile_info[edit_tile]&(1<<20)) // 2 digits
                {
                    digits = 2;
                    number = (tile_info[edit_tile]>>21)&127;
                    if (number > 99)
                        number = 99;
                    max_number = 99;
                }
                else if (tile_info[edit_tile]&(1<<23))
                {
                    digits = 1;
                    number = (tile_info[edit_tile]>>24)&15;
                    if (number > 9)
                        number = 9;
                    max_number = 9;
                }
                if (edit2_cursor == 4) // edit digits
                {
                    if (++digits == 4)
                        digits = 0;
                }
                else
                {
                    if (++number > max_number)
                        number = 0;
                }
                tile_info[edit_tile] &= ~(16383<<14);
                if (digits)
                switch (digits)
                {
                    case 1:
                        tile_info[edit_tile] |= (1<<23)|(number<<24);
                        break;
                    case 2:
                        tile_info[edit_tile] |= (1<<20)|(number<<21);
                        break;
                    case 3:
                        tile_info[edit_tile] |= (1<<17)|(number<<18);
                        break;
                }
            }
            else
            {
                int16_t pos = (tile_info[edit_tile]>>14) & (16383);
                if (edit2_cursor == 4) // internal warp, x coordinate
                {
                    if (++pos % tile_map_width == 0)
                        pos -= tile_map_width;
                }
                else // internal warp, y coordinate
                {
                    pos += tile_map_width;
                    if (pos >= tile_map_width*tile_map_height)
                        pos %= tile_map_width;
                }
                tile_info[edit_tile] &= ~(16383<<14);
                tile_info[edit_tile] |= (pos<<14);
            }
        }
        else // current
        {
            uint32_t param = tile_info[edit_tile]&(63<<(edit2_cursor*6-10));
            tile_info[edit_tile] &= ~(63<<(edit2_cursor*6-10));
            param = (param+(1<<(edit2_cursor*6-10)))&(63<<(edit2_cursor*6-10));
            tile_info[edit_tile] |= param;
        }
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        return;
    }
    if (GAMEPAD_PRESSING(0, down))
    {
        if (edit_sprite_not_tile)
        {
            if (edit2_cursor == 0)
            {
                uint32_t param = sprite_info[edit_sprite/8][edit_sprite%8]&(31);
                sprite_info[edit_sprite/8][edit_sprite%8] &= ~(31);
                param = (param-1)&(31);
                sprite_info[edit_sprite/8][edit_sprite%8] |= param;
            }
            else if (edit2_cursor == 1)
            {
                uint32_t param = sprite_info[edit_sprite/8][edit_sprite%8]&(7<<5);
                sprite_info[edit_sprite/8][edit_sprite%8] &= ~(7<<5);
                param = (param-(1<<5))&(7<<5);
                sprite_info[edit_sprite/8][edit_sprite%8] |= param;
            }
            else
            {
                uint32_t param = sprite_info[edit_sprite/8][edit_sprite%8]&(15<<(edit2_cursor*4));
                sprite_info[edit_sprite/8][edit_sprite%8] &= ~(15<<(edit2_cursor*4));
                param = (param-(1<<(edit2_cursor*4)))&(15<<(edit2_cursor*4));
                sprite_info[edit_sprite/8][edit_sprite%8] |= param;
            }
        }
        else if (tile_info[edit_tile]&8 || (edit2_cursor < 3))
        {
            uint32_t param = tile_info[edit_tile]&(15<<(edit2_cursor*4));
            tile_info[edit_tile] &= ~(15<<(edit2_cursor*4));
            param = (param-(1<<(edit2_cursor*4)))&(15<<(edit2_cursor*4));
            tile_info[edit_tile] |= param;
        }
        else if (edit2_cursor == 3)
        {
            uint32_t param = tile_info[edit_tile]&(3<<12);
            tile_info[edit_tile] &= ~(3<<12);
            param = (param-(1<<12))&(3<<12);
            tile_info[edit_tile] |= param;
        }
        else if (tile_info[edit_tile]&(1<<13)) // warp
        {
            if (edit2_cursor == 6)
            {
                // adjust directional warp
                uint32_t param = tile_info[edit_tile]&(15<<28);
                tile_info[edit_tile] &= ~(15<<28);
                param = (param-(1<<28))&(15<<28);
                tile_info[edit_tile] |= param; 
            }
            else if (tile_info[edit_tile]&(1<<12)) // level warp
            {
                int digits = 0;
                int number = 0, max_number = 0;
                if (tile_info[edit_tile]&(1<<17)) // 3 digits
                {
                    digits = 3;
                    number = (tile_info[edit_tile]>>18)&1023;
                    if (number > 999)
                        number = 999;
                    max_number = 999;
                }
                else if (tile_info[edit_tile]&(1<<20)) // 2 digits
                {
                    digits = 2;
                    number = (tile_info[edit_tile]>>21)&127;
                    if (number > 99)
                        number = 99;
                    max_number = 99;
                }
                else if (tile_info[edit_tile]&(1<<23))
                {
                    digits = 1;
                    number = (tile_info[edit_tile]>>24)&15;
                    if (number > 9)
                        number = 9;
                    max_number = 9;
                }
                if (edit2_cursor == 4) // edit digits
                {
                    if (--digits < 0)
                        digits = 3;
                }
                else
                {
                    if (--number < 0)
                        number = max_number;
                }
                tile_info[edit_tile] &= ~(16383<<14);
                if (digits)
                switch (digits)
                {
                    case 1:
                        tile_info[edit_tile] |= (1<<23)|(number<<24);
                        break;
                    case 2:
                        tile_info[edit_tile] |= (1<<20)|(number<<21);
                        break;
                    case 3:
                        tile_info[edit_tile] |= (1<<17)|(number<<18);
                        break;
                }
            }
            else
            {
                int16_t pos = (tile_info[edit_tile]>>14) & (16383);
                if (edit2_cursor == 4) // internal warp, x coordinate
                {
                    if (pos % tile_map_width == 0)
                        pos += tile_map_width-1;
                    else
                        --pos;
                }
                else // internal warp, y coordinate
                {
                    pos -= tile_map_width;
                    if (pos < 0)
                        pos += tile_map_width*tile_map_height;
                }
                tile_info[edit_tile] &= ~(16383<<14);
                tile_info[edit_tile] |= (pos<<14);
            }
        }
        else // current
        {
            uint32_t param = tile_info[edit_tile]&(63<<(edit2_cursor*6-10));
            tile_info[edit_tile] &= ~(63<<(edit2_cursor*6-10));
            param = (param-(1<<(edit2_cursor*6-10)))&(63<<(edit2_cursor*6-10));
            tile_info[edit_tile] |= param;
        }
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        return;
    }

    if (GAMEPAD_PRESS(0, X))
    {
        // copy or uncopy
        if (edit2_copying)
        {
            edit2_copying = 0;
        }
        else if (edit_sprite_not_tile)
        {
            edit2_copying = 1;
            edit2_copy_location = edit_sprite;
        }
        else
        {
            edit2_copying = 2;
            edit2_copy_location = edit_tile;
        }
        return;
    }
    int save_or_load = 0;
    if (GAMEPAD_PRESS(0, A)) // save
        save_or_load = 1;
    else if (GAMEPAD_PRESS(0, B)) // load
        save_or_load = 2;
    if (save_or_load)
    {
        if (edit2_copying)
        {
            // or cancel a copy
            edit2_copying = 0;
            return;
        }
            
        FileError error;
        if (save_or_load == 1) // save
        {
            if (edit_sprite_not_tile)
                error = io_save_sprite(edit_sprite/8, edit_sprite%8);
            else
                error = io_save_tile(edit_tile);
        }
        else // load
        {
            if (edit_sprite_not_tile)
                error = io_load_sprite(edit_sprite/8, edit_sprite%8);
            else
                error = io_load_tile(edit_tile);
        }
        io_message_from_error(game_message, error, save_or_load);
        return;
    }
    if (GAMEPAD_PRESS(0, Y))
    {
        if (edit2_copying)
        {
            // paste
            uint8_t *src, *dst;
            if (edit2_copying == 1) // sprite
                src = sprite_draw[edit2_copy_location/8][edit2_copy_location%8][0];
            else
                src = tile_draw[edit2_copy_location][0];
            if (edit_sprite_not_tile)
                dst = sprite_draw[edit_sprite/8][edit_sprite%8][0];
            else
                dst = tile_draw[edit_tile][0];
            if (src == dst)
                strcpy((char *)game_message, "pasting to same thing");
            else
            {
                memcpy(dst, src, 16*8);
                strcpy((char *)game_message, "pasted.");
            }
            edit2_copying = 0;
        }
        else
        {
            edit2_cursor = 0;
            previous_visual_mode = EditTileOrSpriteProperties;
            game_switch(ChooseFilename);
        }
        return;
    }
    if (GAMEPAD_PRESS(0, select))
    {
        edit2_cursor = 0;
        game_message[0] = 0;
        if (edit_sprite_not_tile)
        {
            edit_sprite_not_tile = 0; 
            game_switch(EditPalette);
            previous_visual_mode = None;
        }
        else
        {
            edit_sprite_not_tile = 1;
        }
        return;
    }
    if (GAMEPAD_PRESS(0, start))
    {
        edit2_cursor = 0;
        game_message[0] = 0;
        if (previous_visual_mode)
        {
            game_switch(previous_visual_mode);
            previous_visual_mode = None;
        }
        else
        {
            game_switch(EditTileOrSprite);
        }
        return;
    }
}
