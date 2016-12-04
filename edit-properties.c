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
            if (edit_sprite_not_tile || tile_info[edit_tile].material&8)
                memset(&draw_buffer[16+7*9], 229, 9*2*5);
            else if (tile_info[edit_tile].vuln_warp_jump) // a warp or a load level
                memset(&draw_buffer[16+6*9+9*10*(edit2_cursor-4)], 229, 9*2*3);
            else // current
                memset(&draw_buffer[32+11*9+9*8*(edit2_cursor-4)], 229, 9*2);
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
                font_render_line_doubled((const uint8_t *)"X:cancel copy", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            else
                font_render_line_doubled((const uint8_t *)"X:copy", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            break;
        case 4:
            if (edit2_copying)
                font_render_line_doubled((const uint8_t *)"A:paste mirrored", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            else
                font_render_line_doubled((const uint8_t *)"A:save to file", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            break;
        case 5:
            if (edit2_copying)
                font_render_line_doubled((const uint8_t *)"B:paste rotated", 16+2*9, internal_line, 65535, 257*BG_COLOR);
            else
                font_render_line_doubled((const uint8_t *)"B:load from file", 16+2*9, internal_line, 65535, 257*BG_COLOR);
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
                font_render_line_doubled((const uint8_t *)"select:tile menu", 16, internal_line, 65535, SPRITE_COLOR*257);
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
            int info = sprite_info[edit_sprite/8][edit_sprite%8].invisible_color;
            if (info&16)
                strcpy((char *)msg, "visible");
            else
            {
                strcpy((char *)msg, "invis ");
                msg[6] = hex[info&15];
            }
            strcpy((char *)msg+7,   "   1/m=");
            msg[14] = hex[sprite_info[edit_sprite/8][edit_sprite%8].inverse_weight];
            strcpy((char *)msg+15,  " vulnr ");
            msg[22] = hex[sprite_info[edit_sprite/8][edit_sprite%8].vulnerability];
            strcpy((char *)msg+23, " imprv ");
            msg[30] = hex[sprite_info[edit_sprite/8][edit_sprite%8].impervious];
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
            uint8_t param = tile_info[edit_tile].material&15;
            if (tile_info[edit_tile].material&8)
                strcpy((char *)msg, "solid ");
            else if (param)
                strcpy((char *)msg, "water ");
            else
                strcpy((char *)msg, "  air ");
            msg[6] = hex[param];
            strcpy((char *)msg+7, " trans ");
            msg[14] = hex[(tile_info[edit_tile].translation)&15];
            strcpy((char *)msg+15, " time. ");
            param = (tile_info[edit_tile].timing)&15;
            if (param)
                msg[22] = hex[param];
            else
                msg[22] = hex[16];
            if (tile_info[edit_tile].material&8) // solid
            {
                strcpy((char *)msg+23, " vulnr ");
                msg[30] = hex[(tile_info[edit_tile].vuln_warp_jump)&15];
                msg[31] = 0;
            }
            else switch (tile_info[edit_tile].vuln_warp_jump)
            {
                case 0: // 
                    // does the water/air tile damage you
                    if (tile_info[edit_tile].damage_angle&15) 
                        strcpy((char *)msg+23, " damge:Y");
                    else
                        strcpy((char *)msg+23, " damge:N"); 
                    break;
                case 1:
                case 2:
                case 3:
                    strcpy((char *)msg+23, " gotolvl");
                    break;
                default: // warp within level
                    strcpy((char *)msg+23, " gotopos"); 
            }
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
        }
        break;
        case 13:
        if (edit_sprite_not_tile || tile_info[edit_tile].material&8) // solid block or sprite
        {
            uint8_t *info = edit_sprite_not_tile ? sprite_info[edit_sprite/8][edit_sprite%8].side :
                tile_info[edit_tile].side;
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
            switch (info[edit2_side]&15)
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
        else 
        {
            uint8_t msg[32];
            switch (tile_info[edit_tile].vuln_warp_jump)
            {
                case 0:
                {
                    if (tile_info[edit_tile].material&7)
                        strcpy((char *)msg, "flow angle ");
                    else
                        strcpy((char *)msg, "wind angle ");
                    msg[11] = hex[tile_info[edit_tile].damage_angle>>4];
                    strcpy((char *)msg+12, " power ");
                    msg[19] = hex[tile_info[edit_tile].random_strength>>4];
                    strcpy((char *)msg+20, " randm ");
                    msg[27] = hex[tile_info[edit_tile].random_strength&15];
                    msg[28] = 0;
                    font_render_line_doubled(msg, 32, internal_line, 65535, BG_COLOR*257);
                    break;
                }
                case 1:
                case 2:
                case 3:
                {
                    int digits = tile_info[edit_tile].vuln_warp_jump;
                    strcpy((char *)msg, "digits ");
                    msg[7] = '0'+digits;
                    strcpy((char *)msg+8, " number ");
                    int index = 16+digits;
                    msg[index] = 0;
                    int number = tile_info[edit_tile].warp;
                    while (digits-- > 0)
                    {
                        msg[--index] = '0'+number%10;
                        number /= 10;
                    }
                    font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
                    goto render_direction;
                }
                default:
                {
                    // internal warp
                    strcpy((char *)msg, "pos X ");
                    int x = tile_info[edit_tile].jx;
                    msg[8] = hex[x%16];
                    msg[7] = hex[(x/16)%16];
                    msg[6] = hex[(x/256)%16];
                    strcpy((char *)msg+9, " pos Y ");
                    int y = tile_info[edit_tile].jy;
                    msg[17] = hex[y%16];
                    msg[16] = hex[(y/16)%16];
                    msg[18] = 0;
                    font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
                  render_direction:
                    strcpy((char *)msg, "direxn ");
                    msg[7] = hex[tile_info[edit_tile].press_direction];
                    msg[8] = 0;
                    font_render_line_doubled(msg, 16+20*9, internal_line, 65535, BG_COLOR*257);
                    break;
                }
            } 
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
    else if (edit2_copying)
    {
        if (vga_line < 22 + 2 + 2*16 + 2*16)
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
        else if (vga_line/2 == (22 + 2 + 2*16 + 2*16)/2)
            memset(draw_buffer + SCREEN_W-24-16*2 + 2, BG_COLOR, 64);
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
        else if (edit_sprite_not_tile || tile_info[edit_tile].material&8)
            edit2_cursor = 7;
        else
            edit2_cursor = 6;
        return;
    }
    if (GAMEPAD_PRESS(0, right))
    {
        ++edit2_cursor;
        if (edit_sprite_not_tile || tile_info[edit_tile].material&8)
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
        if (edit_sprite_not_tile) switch (edit2_cursor)
        {
            case 0: // invisible color
                if (++sprite_info[edit_sprite/8][edit_sprite%8].invisible_color > 16)
                    sprite_info[edit_sprite/8][edit_sprite%8].invisible_color = 0;
                break;
            case 1: // inverse weight
                if (++sprite_info[edit_sprite/8][edit_sprite%8].inverse_weight > 15)
                    sprite_info[edit_sprite/8][edit_sprite%8].inverse_weight = 0;
                break;
            case 2: // vulnerability
                if (++sprite_info[edit_sprite/8][edit_sprite%8].vulnerability > 15)
                    sprite_info[edit_sprite/8][edit_sprite%8].vulnerability = 0;
                break;
            case 3: // impervious
                if (++sprite_info[edit_sprite/8][edit_sprite%8].impervious > 15)
                    sprite_info[edit_sprite/8][edit_sprite%8].impervious = 0;
                break;
            default: // sides
                if (++sprite_info[edit_sprite/8][edit_sprite%8].side[edit2_cursor-4] > 15)
                    sprite_info[edit_sprite/8][edit_sprite%8].side[edit2_cursor-4] = 0;
                break;
        }
        else switch (edit2_cursor)
        {
            case 0: // material
                if (++tile_info[edit_tile].material > 15)
                    tile_info[edit_tile].material = 0;
                break;
            case 1: // translation
                if (++tile_info[edit_tile].translation > 15)
                    tile_info[edit_tile].translation = 0;
                break;
            case 2: // timing
                if (++tile_info[edit_tile].timing > 15)
                    tile_info[edit_tile].timing = 0;
                break;
            case 3: // damage or warp or jump
                if (tile_info[edit_tile].material&8)
                {
                    // adjusting vulnerability
                    if (++tile_info[edit_tile].vuln_warp_jump > 15)
                        tile_info[edit_tile].vuln_warp_jump = 0;
                }
                else if (tile_info[edit_tile].vuln_warp_jump)
                {
                    if (tile_info[edit_tile].vuln_warp_jump < 4)
                        tile_info[edit_tile].vuln_warp_jump = 4;
                    else // set back to no damage
                    {
                        tile_info[edit_tile].vuln_warp_jump = 0;
                        tile_info[edit_tile].damage_angle = (tile_info[edit_tile].damage_angle&240);
                    }
                }
                else 
                {
                    if (tile_info[edit_tile].damage_angle&15) // damage for a flow sprite
                        tile_info[edit_tile].vuln_warp_jump = 1;
                    else // add damage
                        tile_info[edit_tile].damage_angle = (tile_info[edit_tile].damage_angle&240) | 1;
                }
                break;
            case 4:
                if (tile_info[edit_tile].material&8)
                {
                    if (++tile_info[edit_tile].side[0] > 15)
                        tile_info[edit_tile].side[0] = 0;
                }
                else switch (tile_info[edit_tile].vuln_warp_jump)
                {
                    case 0: // fluid, angle of flow
                        tile_info[edit_tile].damage_angle += 16;
                        break;
                    case 1: // warp, one digit 
                        tile_info[edit_tile].vuln_warp_jump = 2;
                        break;
                    case 2: // warp, two digits
                        tile_info[edit_tile].vuln_warp_jump = 3;
                        break;
                    case 3: // warp, three digits
                        tile_info[edit_tile].vuln_warp_jump = 1;
                        break;
                    default: // jump, modify x position
                        if (++tile_info[edit_tile].jx >= tile_map_width)
                            tile_info[edit_tile].jx = 0;
                        break;
                }
                break;
            case 5:
                if (tile_info[edit_tile].material&8)
                {
                    if (++tile_info[edit_tile].side[1] > 15)
                        tile_info[edit_tile].side[1] = 0;
                }
                else switch (tile_info[edit_tile].vuln_warp_jump)
                {
                    case 0: // fluid, strength of flow
                        tile_info[edit_tile].random_strength += 16;
                        break;
                    case 1: // warp, one digit 
                        tile_info[edit_tile].warp = (tile_info[edit_tile].warp+1)%10;
                        break;
                    case 2: // warp, two digits
                        tile_info[edit_tile].warp = (tile_info[edit_tile].warp+1)%100;
                        break;
                    case 3: // warp, three digits
                        tile_info[edit_tile].warp = (tile_info[edit_tile].warp+1)%1000;
                        break;
                    default: // jump, modify y position
                        if (++tile_info[edit_tile].jy >= tile_map_height)
                            tile_info[edit_tile].jy = 0;
                        break;
                }
                break;
            case 6:
                if (tile_info[edit_tile].material&8)
                {
                    if (++tile_info[edit_tile].side[2] > 15)
                        tile_info[edit_tile].side[2] = 0;
                }
                else if (tile_info[edit_tile].vuln_warp_jump)
                {
                    if (++tile_info[edit_tile].press_direction > 15)
                        tile_info[edit_tile].press_direction = 0;
                }
                else // adjusting random
                {
                    tile_info[edit_tile].random_strength = (tile_info[edit_tile].random_strength&240) | ((tile_info[edit_tile].random_strength+1)&15);
                }
                break;
            case 7:
                if (tile_info[edit_tile].material&8)
                {
                    if (++tile_info[edit_tile].side[3] > 15)
                        tile_info[edit_tile].side[3] = 0;
                }
                break;
        }
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        return;
    }
    if (GAMEPAD_PRESSING(0, down))
    {
        if (edit_sprite_not_tile) switch (edit2_cursor)
        {
            case 0: // invisible color
                if (--sprite_info[edit_sprite/8][edit_sprite%8].invisible_color > 16)
                    sprite_info[edit_sprite/8][edit_sprite%8].invisible_color = 16;
                break;
            case 1: // inverse weight
                if (--sprite_info[edit_sprite/8][edit_sprite%8].inverse_weight > 15)
                    sprite_info[edit_sprite/8][edit_sprite%8].inverse_weight = 15;
                break;
            case 2: // vulnerability
                if (--sprite_info[edit_sprite/8][edit_sprite%8].vulnerability > 15)
                    sprite_info[edit_sprite/8][edit_sprite%8].vulnerability = 15;
                break;
            case 3: // impervious
                if (--sprite_info[edit_sprite/8][edit_sprite%8].impervious > 15)
                    sprite_info[edit_sprite/8][edit_sprite%8].impervious = 15;
                break;
            default: // sides
                if (--sprite_info[edit_sprite/8][edit_sprite%8].side[edit2_cursor-4] > 15)
                    sprite_info[edit_sprite/8][edit_sprite%8].side[edit2_cursor-4] = 15;
                break;
        }
        else switch (edit2_cursor)
        {
            case 0: // material
                if (--tile_info[edit_tile].material > 15)
                    tile_info[edit_tile].material = 15;
                break;
            case 1: // translation
                if (--tile_info[edit_tile].translation > 15)
                    tile_info[edit_tile].translation = 15;
                break;
            case 2: // timing
                if (--tile_info[edit_tile].timing > 15)
                    tile_info[edit_tile].timing = 15;
                break;
            case 3: // damage or warp or jump
                if (tile_info[edit_tile].material&8)
                {
                    // adjusting vulnerability
                    if (--tile_info[edit_tile].vuln_warp_jump > 15)
                        tile_info[edit_tile].vuln_warp_jump = 15;
                }
                else if (tile_info[edit_tile].vuln_warp_jump)
                {
                    if (tile_info[edit_tile].vuln_warp_jump < 4) // set to damage
                    {
                        tile_info[edit_tile].vuln_warp_jump = 0;
                        tile_info[edit_tile].damage_angle = (tile_info[edit_tile].damage_angle&240)|1;
                    }
                    else // set back to warp
                        tile_info[edit_tile].vuln_warp_jump = 1;
                }
                else 
                {
                    if (tile_info[edit_tile].damage_angle&15) // damage for a flow sprite, undamage:
                        tile_info[edit_tile].damage_angle = (tile_info[edit_tile].damage_angle&240);
                    else // no damage, set to jump
                        tile_info[edit_tile].vuln_warp_jump = 4;
                }
                break;
            case 4:
                if (tile_info[edit_tile].material&8)
                {
                    if (--tile_info[edit_tile].side[0] > 15)
                        tile_info[edit_tile].side[0] = 15;
                }
                else switch (tile_info[edit_tile].vuln_warp_jump)
                {
                    case 0: // fluid, angle of flow
                        tile_info[edit_tile].damage_angle += 240;
                        break;
                    case 1: // warp, one digit 
                        tile_info[edit_tile].vuln_warp_jump = 3;
                        break;
                    case 2: // warp, two digits
                        tile_info[edit_tile].vuln_warp_jump = 1;
                        break;
                    case 3: // warp, three digits
                        tile_info[edit_tile].vuln_warp_jump = 2;
                        break;
                    default: // jump, modify x position
                        if (--tile_info[edit_tile].jx >= tile_map_width)
                            tile_info[edit_tile].jx = tile_map_width-1;
                        break;
                }
                break;
            case 5:
                if (tile_info[edit_tile].material&8)
                {
                    if (--tile_info[edit_tile].side[1] > 15)
                        tile_info[edit_tile].side[1] = 15;
                }
                else switch (tile_info[edit_tile].vuln_warp_jump)
                {
                    case 0: // fluid, strength of flow
                        tile_info[edit_tile].random_strength += 240;
                        break;
                    case 1: // warp, one digit 
                        tile_info[edit_tile].warp = (tile_info[edit_tile].warp+9)%10;
                        break;
                    case 2: // warp, two digits
                        tile_info[edit_tile].warp = (tile_info[edit_tile].warp+99)%100;
                        break;
                    case 3: // warp, three digits
                        tile_info[edit_tile].warp = (tile_info[edit_tile].warp+999)%1000;
                        break;
                    default: // jump, modify y position
                        if (--tile_info[edit_tile].jy >= tile_map_height)
                            tile_info[edit_tile].jy = tile_map_height-1;
                        break;
                }
                break;
            case 6:
                if (tile_info[edit_tile].material&8)
                {
                    if (--tile_info[edit_tile].side[2] > 15)
                        tile_info[edit_tile].side[2] = 15;
                }
                else if (tile_info[edit_tile].vuln_warp_jump)
                {
                    if (--tile_info[edit_tile].press_direction > 15)
                        tile_info[edit_tile].press_direction = 15;
                }
                else // adjusting random
                {
                    tile_info[edit_tile].random_strength = (tile_info[edit_tile].random_strength&240) | ((tile_info[edit_tile].random_strength+15)&15);
                }
                break;
            case 7:
                if (tile_info[edit_tile].material&8)
                {
                    if (--tile_info[edit_tile].side[3] > 15)
                        tile_info[edit_tile].side[3] = 15;
                }
                break;
        }
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        return;
    }

    if (GAMEPAD_PRESS(0, X))
    {
        game_message[0] = 0;
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
            // paste with special stuff
            uint8_t *src, *dst;
            uint8_t work[16][8];
            if (edit2_copying == 1) // sprite
            {
                if (edit_sprite_not_tile)
                {
                    // copying sprite to sprite, copy also the info
                    // TODO: should rotate/mirror sides of sprite info
                    sprite_info[edit_sprite/8][edit_sprite%8] =
                        sprite_info[edit2_copy_location/8][edit2_copy_location%8];
                    dst = sprite_draw[edit_sprite/8][edit_sprite%8][0];
                }
                else
                    dst = tile_draw[edit_tile][0];

                src = sprite_draw[edit2_copy_location/8][edit2_copy_location%8][0];
            }
            else // copying from a tile
            {
                if (edit_sprite_not_tile)
                    dst = sprite_draw[edit_sprite/8][edit_sprite%8][0];
                else
                { 
                    // copying to a tile, copy also tile info
                    // TODO: rotate side info
                    tile_info[edit_tile] = tile_info[edit2_copy_location];
                    dst = tile_draw[edit_tile][0];
                }
                src = tile_draw[edit2_copy_location][0];
            }
            if (src == dst)
            {
                memcpy(work, src, 16*8);    
                src = work[0];
            }
            if (save_or_load == 1) // pressed A, mirror that tile L/R
            {
                for (int j=1; j<17; ++j)
                for (int i=1; i<=8; ++i)
                {
                    uint8_t value = src[(j*8) - i];
                    *dst++ = (value>>4) | (value<<4);
                }
                strcpy((char *)game_message, "mirror-pasted.");
            }
            else // pressed B, rotate that tile 90 degrees CCW
            {
                for (int i=15; i>=0; --i)
                {
                    if (i % 2) // odd i
                    for (int j=0; j<16; j+=2)
                        *dst++ = (src[(j*16+i)/2]>>4) | (src[((j+1)*16+i)/2]&240);
                    else
                    for (int j=0; j<16; j+=2)
                        *dst++ = (src[(j*16+i)/2]&15) | ((src[((j+1)*16+i)/2]&15)<<4);
                }
                strcpy((char *)game_message, "rota-pasted.");
            }
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
            {
                src = sprite_draw[edit2_copy_location/8][edit2_copy_location%8][0];
                if (edit_sprite_not_tile)
                {
                    // copying sprite to sprite, copy also the info
                    sprite_info[edit_sprite/8][edit_sprite%8] =
                        sprite_info[edit2_copy_location/8][edit2_copy_location%8];
                    dst = sprite_draw[edit_sprite/8][edit_sprite%8][0];
                }
                else
                    dst = tile_draw[edit_tile][0];
            }
            else
            {
                src = tile_draw[edit2_copy_location][0];
                if (edit_sprite_not_tile)
                    dst = sprite_draw[edit_sprite/8][edit_sprite%8][0];
                else
                {
                    // copy also the tile info
                    tile_info[edit_tile] = tile_info[edit2_copy_location];
                    dst = tile_draw[edit_tile][0];
                }
            }
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
        edit_sprite_not_tile = 1 - edit_sprite_not_tile;
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
