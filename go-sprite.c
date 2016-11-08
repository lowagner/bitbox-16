#include "bitbox.h"
#include "common.h"
#include "common.h"
#include "chiptune.h"
#include "sprites.h"
#include "edit.h"
#include "name.h"
#include "font.h"
#include "io.h"
#include "go-sprite.h"

#include <stdlib.h> // rand
#include <string.h> // memset

#define BG_COLOR 168
#define NUMBER_LINES 20

#define FIRE_COUNT 32 // number of times GO_NOT_FIRE needs to be called before you can fire again
#define SPEED_MULTIPLIER 0.5f
#define JUMP_MULTIPLIER 1.75f
#define ACCELERATION_MULTIPLIER 0.25f
#define DECELERATION_MULTIPLIER 0.25f
#define MAX_VY 10.0f


uint8_t go_pattern_pos CCM_MEMORY;
uint8_t go_pattern_offset CCM_MEMORY;
uint8_t go_menu_not_edit CCM_MEMORY;
uint8_t go_command_copy CCM_MEMORY;
uint8_t go_copying CCM_MEMORY;

void go_init()
{
    go_pattern_pos = 0;
    go_pattern_offset = 0;
    go_copying = 16;
    go_menu_not_edit = 0;
    go_command_copy = rand()%16;
}

void go_reset()
{
}

void go_short_command_message(uint8_t *buffer, uint8_t cmd)
{
    switch (cmd&15)
    {
        case GO_BREAK:
            strcpy((char *)buffer, "wait/break");
            break;
        case GO_NOT_MOVE:
            strcpy((char *)buffer, "not move goto");
            break;
        case GO_NOT_RUN:
            strcpy((char *)buffer, "not run goto");
            break;
        case GO_NOT_AIR:
            strcpy((char *)buffer, "not air goto");
            break;
        case GO_NOT_FIRE:
            strcpy((char *)buffer, "not fire goto");
            break;
        case GO_EXECUTE:
            strcpy((char *)buffer, "execute");
            break;
        case GO_LOOK:
            strcpy((char *)buffer, "look");
            break;
        case GO_DIRECTION:
            strcpy((char *)buffer, "handle input");
            break;
        case GO_SPECIAL_INPUT:
            strcpy((char *)buffer, "handle special");
            break;
        case GO_SPAWN_TILE:
            strcpy((char *)buffer, "spawn tile");
            break;
        case GO_SPAWN_SPRITE:
            strcpy((char *)buffer, "spawn sprite");
            break;
        case GO_ACCELERATION:
            strcpy((char *)buffer, "acceleration");
            break;
        case GO_SPEED:
            strcpy((char *)buffer, "speed");
            break;
        case GO_NOISE:
            strcpy((char *)buffer, "noise");
            break;
        case GO_RANDOMIZE:
            strcpy((char *)buffer, "randomize");
            break;
        case GO_QUAKE:
            strcpy((char *)buffer, "quake");
            break;
    }
}

void go_render_command(int j, int y)
{
    int x = 32;
    #ifdef EMULATOR
    if (y < 0 || y >= 8)
    {
        message("got too big a line count for go %d, line %d\n", (int)edit_sprite/8, j);
        return;
    }
    #endif
    
    uint8_t cmd = sprite_pattern[edit_sprite/8][j];
    uint8_t param = cmd>>4;
    cmd &= 15;

    uint32_t *dst = (uint32_t *)draw_buffer + x/2;
    uint32_t color_choice[2];
    if (j%2)
        color_choice[0] = 16843009u*BG_COLOR;
    else
        color_choice[0] = 16843009u*149;
    
    if (j != go_pattern_pos)
        color_choice[1] = 65535u*65537u;
    else
    {
        color_choice[1] = RGB(190, 245, 255)|(RGB(190, 245, 255)<<16);
        if (!go_menu_not_edit)
        {
            if ((y+1)/2 == 1)
            {
                dst -= 4;
                *dst = color_choice[1];
                ++dst;
                *dst = color_choice[1];
                dst += 4 - 1;
            }
            else if ((y+1)/2 == 3)
            {
                dst -= 4;
                *dst = 16843009u*BG_COLOR;
                ++dst;
                *dst = 16843009u*BG_COLOR;
                dst += 4 - 1;
            }
        }
    }
    
    switch (cmd)
    {
        case GO_BREAK:
            if (param == 0)
            {
                cmd = '0';
                param = '0';
            }
            else
            {
                cmd = 'W';
                param = hex[param];
            }
            break;
        case GO_NOT_MOVE:
            cmd = 'm';
            if (!param)
                param = 16;
            param = hex[j+1+param];
            break;
        case GO_NOT_RUN:
            cmd = 'r';
            if (!param)
                param = 16;
            param = hex[j+1+param];
            break;
        case GO_NOT_AIR:
            cmd = 'a';
            if (!param)
                param = 16;
            param = hex[j+1+param];
            break;
        case GO_NOT_FIRE:
            cmd = 'f';
            if (!param)
                param = 16;
            param = hex[j+1+param];
            break;
        case GO_EXECUTE:
            cmd = 'E';
            param = hex[param];
            break;
        case GO_LOOK:
            cmd = 'L';
            param = hex[param];
            break;
        case GO_DIRECTION:
            cmd = 'D';
            param = hex[param];
            break;
        case GO_SPECIAL_INPUT:
            cmd = 'I';
            param = hex[param];
            break;
        case GO_SPAWN_TILE:
            cmd = 'T';
            param = hex[param];
            break;
        case GO_SPAWN_SPRITE:
            cmd = 'S';
            param = hex[param];
            break;
        case GO_ACCELERATION:
            cmd = '/';
            param = hex[param];
            break;
        case GO_SPEED:
            if (param < 8)
            {
                cmd = '+';
                param = '0' + param;
            }
            else
            {
                cmd = '^';
                param = '0' + param - 8;
            }
            break;
        case GO_NOISE:
            cmd = 'N';
            param = hex[param];
            break;
        case GO_RANDOMIZE:
            cmd = 'R';
            param = 224 + param;
            break;
        case GO_QUAKE:
            cmd = 'Q';
            param = hex[param];
            break;
    }
    
    uint8_t shift = ((y/2))*4;
    uint8_t row = (font[hex[j]] >> shift) & 15;
    *(++dst) = color_choice[0];
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    row = (font[':'] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    *(++dst) = color_choice[0];
    row = (font[cmd] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    
    row = (font[param] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
}

void go_adjust_parameter(int direction)
{
    if (!direction)
        return;
    uint8_t cmd = sprite_pattern[edit_sprite/8][go_pattern_pos];
    uint8_t param = cmd>>4;
    cmd &= 15;
    param = (param + direction)&15;
    sprite_pattern[edit_sprite/8][go_pattern_pos] = cmd | (param<<4);
}

void go_line()
{
    if (vga_line < 16)
    {
        if (vga_line/2 == 0)
        {
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
            return;
        }
        else if (vga_line >= 8)
        {
            uint32_t *dst = (uint32_t *)draw_buffer + (SCREEN_W - 8 - 16*2)/2 - 1;
            uint8_t *tile_color = &sprite_draw[edit_sprite/8][(vga_frame/64)%8][(vga_line-8)/2][0] - 1;
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
        return;
    }
    else if (vga_line >= 16 + NUMBER_LINES*10)
    {
        if (vga_line/2 == (16 +NUMBER_LINES*10)/2)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    int line = (vga_line-16) / 10;
    int internal_line = (vga_line-16) % 10;
    if (internal_line == 0 || internal_line == 9)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        goto maybe_draw_sprite;
    }
    --internal_line;
    uint8_t buffer[24];
    switch (line)
    {
        case 0:
        {
            // edit track
            uint8_t msg[] = { 'g', 'o', ' ', 's', 'p', 'r', 'i', 't', 'e', ' ', hex[edit_sprite/8], '!',
            0 };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 1:
            break;
        case 2:
        {
            go_render_command(go_pattern_offset+line-2, internal_line);
            // command
            uint8_t msg[] = { 'c', 'o', 'm', 'm', 'a', 'n', 'd', ' ', hex[go_pattern_pos], ':', 0 };
            font_render_line_doubled(msg, 96, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 3:
            switch (sprite_pattern[edit_sprite/8][go_pattern_pos]&15)
            {
                case GO_BREAK:
                    strcpy((char *)buffer, "wait or break (if 00)");
                    break;
                case GO_NOT_MOVE:
                    strcpy((char *)buffer, "if not moving goto");
                    break;
                case GO_NOT_RUN:
                    strcpy((char *)buffer, "if not running goto");
                    break;
                case GO_NOT_AIR:
                    strcpy((char *)buffer, "if not in air goto");
                    break;
                case GO_NOT_FIRE:
                    strcpy((char *)buffer, "if not firing goto");
                    break;
                case GO_EXECUTE:
                    strcpy((char *)buffer, "execute:");
                    break;
                case GO_LOOK:
                    strcpy((char *)buffer, "look for player");
                    break;
                case GO_DIRECTION:
                    strcpy((char *)buffer, "handle directions:");
                    break;
                case GO_SPECIAL_INPUT:
                    strcpy((char *)buffer, "handle special input:");
                    break;
                case GO_SPAWN_TILE:
                    strcpy((char *)buffer, "spawn a tile");
                    break;
                case GO_SPAWN_SPRITE:
                    strcpy((char *)buffer, "spawn a sprite");
                    break;
                case GO_ACCELERATION:
                    strcpy((char *)buffer, "set acc/deceleration");
                    break;
                case GO_SPEED:
                    strcpy((char *)buffer, "set move/jump speed");
                    break;
                case GO_NOISE:
                    strcpy((char *)buffer, "make noise w/ inst.");
                    break;
                case GO_RANDOMIZE:
                    strcpy((char *)buffer, "randomize next cmd");
                    break;
                case GO_QUAKE:
                    strcpy((char *)buffer, "quake screen");
                    break;
            }
            font_render_line_doubled(buffer, 102, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_go;
        case 4:
            switch (sprite_pattern[edit_sprite/8][go_pattern_pos]&15)
            {
                case GO_EXECUTE:
                    switch (sprite_pattern[edit_sprite/8][go_pattern_pos]>>4)
                    {
                        case 0:
                            strcpy((char *)buffer, "stop");
                            break;
                        case 1:
                            strcpy((char *)buffer, "turn CCW");
                            break;
                        case 2:
                            strcpy((char *)buffer, "turn 180");
                            break;
                        case 3:
                            strcpy((char *)buffer, "turn CW");
                            break;
                        case 4:
                            strcpy((char *)buffer, "walk");
                            break;
                        case 5:
                            strcpy((char *)buffer, "toggle run");
                            break;
                        case 6:
                            strcpy((char *)buffer, "do a jump");
                            break;
                        case 7:
                            strcpy((char *)buffer, "toggle ghost");
                            break;
                        case 8:
                            strcpy((char *)buffer, "fall off edges");
                            break;
                        case 9:
                            strcpy((char *)buffer, "turn CCW at edges");
                            break;
                        case 10:
                            strcpy((char *)buffer, "turn 180 at edges");
                            break;
                        case 11:
                            strcpy((char *)buffer, "turn CW at edges");
                            break;
                        case 12:
                            strcpy((char *)buffer, "jump at edges");
                            break;
                        case 13:
                            strcpy((char *)buffer, "turn or fall");
                            break;
                        case 14:
                            strcpy((char *)buffer, "stop or fall");
                            break;
                        case 15:
                            strcpy((char *)buffer, "stop at edges");
                            break;
                    }
                    break;
                //case GO_LOOK:
                //    break;
                case GO_DIRECTION:
                {
                    char *b = (char *)buffer;
                    uint8_t param = (sprite_pattern[edit_sprite/8][go_pattern_pos]>>4);
                    if (param & 8)
                        strcpy(b, "P2: ");
                    else
                        strcpy(b, "P1: ");
                    b += 4;
                    if (param & 7)
                    {
                        if (param & 3)
                        {
                            if (param & 1)
                            {
                                strcpy(b, "L/R ");
                                b += 4;
                            }
                            if (param & 2)
                            {
                                strcpy(b, "U/D ");
                                b += 4;
                            }
                            if (param & 4)
                                strcpy(b, "mirrored");
                        }
                        else
                            strcpy(b, "random");
                    }
                    else
                        strcpy(b, "no input"); // TODO: could make this do something
                    break;
                }
                case GO_SPECIAL_INPUT:
                {
                    char *b = (char *)buffer;
                    uint8_t param = (sprite_pattern[edit_sprite/8][go_pattern_pos]>>4);
                    if (param & 8)
                        strcpy(b, "P2: ");
                    else
                        strcpy(b, "P1: ");
                    b += 4;
                    if (!(param & 7))
                    {
                        strcpy(b, "no input"); // TODO: could make this do something
                        break;
                    }
                    if (param & 1)
                    {
                        strcpy(b, "run ");
                        b += 4;
                    }
                    if (param & 2)
                    {
                        strcpy(b, "jump ");
                        b += 5;
                    }
                    if (param & 4)
                    {
                        strcpy(b, "fire");
                    }
                    break;
                }
                case GO_ACCELERATION:
                    strcpy((char *)buffer, "0,1,2,3 / 0,4,8,c");
                    break;
                case GO_SPEED:
                    if ((sprite_pattern[edit_sprite/8][go_pattern_pos]>>4) < 8)
                        strcpy((char *)buffer, "move speed");
                    else
                        strcpy((char *)buffer, "jump speed");
                    break;
                default:
                    buffer[0] = 0;
            }
            font_render_line_doubled(buffer, 106, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_go;
        case 6:
            font_render_line_doubled((uint8_t *)"switch to:", 102 - 6*go_menu_not_edit, internal_line, 65535, BG_COLOR*257); 
            goto definitely_show_go;
        case 7:
            if (go_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"L:prev sprite", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'L'; buffer[1] = ':';
                go_short_command_message(buffer+2, sprite_pattern[edit_sprite/8][go_pattern_pos]-1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto definitely_show_go;
        case 8:
            if (go_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"R:next sprite", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'R'; buffer[1] = ':';
                go_short_command_message(buffer+2, sprite_pattern[edit_sprite/8][go_pattern_pos]+1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto definitely_show_go;
        case 9:
            if (!go_menu_not_edit)
                font_render_line_doubled((uint8_t *)"dpad:", 102 - 6*go_menu_not_edit, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_go;
        case 10:
            if (go_menu_not_edit)
            {
                if (go_copying < 16)
                    font_render_line_doubled((uint8_t *)"A:cancel copy", 96, internal_line, 65535, BG_COLOR*257);
                else
                    font_render_line_doubled((uint8_t *)"A:save to file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"adjust parameters", 112, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_go;
        case 11:
            if (go_menu_not_edit)
            {
                if (go_copying < 16)
                    font_render_line_doubled((uint8_t *)"B/X:\"     \"", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"B:load from file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                {} //font_render_line_doubled((uint8_t *)"B:edit instrument", 96, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_go;
        case 12:
            if (go_menu_not_edit)
            {
                if (go_copying < 16)
                    font_render_line_doubled((uint8_t *)"Y:paste", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"X:copy", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((uint8_t *)"X:cut cmd", 96, internal_line, 65535, BG_COLOR*257);
            }
            goto definitely_show_go;
        case 13:
            if (go_menu_not_edit)
            {
                if (go_copying < 16)
                    goto definitely_show_go;
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), base_filename);
                font_render_line_doubled(buffer, 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"Y:insert cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_go;
        case 15:
            if (go_menu_not_edit)
                font_render_line_doubled((uint8_t *)"start:edit pattern", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:pattern menu", 96, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_go;
        case 16:
            font_render_line_doubled((uint8_t *)"select:return", 96, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_go;
        case 18:
            break;
        case 19:
            font_render_line_doubled(game_message, 36, internal_line, 65535, BG_COLOR*257);
            break;
        default:
          definitely_show_go:
            go_render_command(go_pattern_offset+line-2, internal_line);
            break; 
    }
    maybe_draw_sprite:
    if (vga_line < 8 + 2*16)
    {
        uint32_t *dst = (uint32_t *)draw_buffer + (SCREEN_W - 8 - 16*2)/2 - 1;
        internal_line = (vga_line - 8)/2;
        uint8_t *tile_color = &sprite_draw[edit_sprite/8][(vga_frame/64)%8][internal_line][0] - 1;
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
    else if (vga_line/2 == (8 + 2*16)/2)
    {
        memset(draw_buffer + SCREEN_W - 8 - 16*2, BG_COLOR, 16*4);
    }
}

void go_controls()
{
    if (go_menu_not_edit)
    {
        int switched = 0;
        if (GAMEPAD_PRESSING(0, down))
            ++switched;
        if (GAMEPAD_PRESSING(0, up))
            --switched;
        if (GAMEPAD_PRESSING(0, left))
            --switched;
        if (GAMEPAD_PRESSING(0, right))
            ++switched;
        if (switched)
        {
            game_message[0] = 0;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }
        
        if (GAMEPAD_PRESSING(0, L))
            --switched;
        if (GAMEPAD_PRESSING(0, R))
            ++switched;
        if (switched)
        {
            game_message[0] = 0;
            edit_sprite = (edit_sprite+8*switched)&127;
            go_pattern_pos = 0;
            go_pattern_offset = 0;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }

        if (GAMEPAD_PRESS(0, X)) // copy
        {
            if (go_copying < 16)
            {
                go_copying = 16;
                game_message[0] = 0;
            }
            else
            {
                go_copying = edit_sprite/8;
                strcpy((char *)game_message, "copied.");
            }
        }

        if (GAMEPAD_PRESS(0, Y)) // paste
        {
            if (go_copying < 16)
            {
                // paste
                uint8_t *src, *dst;
                src = &sprite_pattern[go_copying][0];
                dst = &sprite_pattern[edit_sprite/8][0];
                if (src == dst)
                {
                    go_copying = 16;
                    strcpy((char *)game_message, "pasting to same thing"); 
                    return;
                }
                memcpy(dst, src, sizeof(sprite_pattern[0]));
                strcpy((char *)game_message, "pasted."); 
                go_copying = 16;
            }
            else
            {
                // switch to choose name and hope to come back
                game_message[0] = 0;
                game_switch(ChooseFilename);
                previous_visual_mode = EditSpritePattern;
            }
            return;
        }
        
        int save_or_load = 0;
        if (GAMEPAD_PRESS(0, A))
            save_or_load = 1; // save
        if (GAMEPAD_PRESS(0, B))
            save_or_load = 2; // load
        if (save_or_load)
        {
            if (go_copying < 16)
            {
                // cancel a copy 
                go_copying = 16;
                game_message[0] = 0;
                return;
            }

            FileError error = BotchedIt;
            if (save_or_load == 1)
                error = io_save_go(edit_sprite/8);
            else
                error = io_load_go(edit_sprite/8);
            io_message_from_error(game_message, error, save_or_load);
            return;
        }

    }
    else // editing, not menu
    {
        int movement = 0;
        if (GAMEPAD_PRESSING(0, L))
            --movement;
        if (GAMEPAD_PRESSING(0, R))
            ++movement;
        if (movement)
        {
            uint8_t *memory = &sprite_pattern[edit_sprite/8][go_pattern_pos];
            *memory = ((*memory+movement)&15)|((*memory)&240);
        }
        if (GAMEPAD_PRESSING(0, down))
        {
            if (go_pattern_pos < 31)
            {
                ++go_pattern_pos;
                if (go_pattern_pos > go_pattern_offset + 15)
                    go_pattern_offset = go_pattern_pos - 15;
            }
            else
            {
                go_pattern_pos = 0;
                go_pattern_offset = 0;
            }
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, up))
        {
            if (go_pattern_pos)
            {
                --go_pattern_pos;
                if (go_pattern_pos < go_pattern_offset)
                    go_pattern_offset = go_pattern_pos;
            }
            else
            {
                go_pattern_pos = 31;
                go_pattern_offset = 31 - 15;
            }
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, left))
        {
            go_adjust_parameter(-1);
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, right))
        {
            go_adjust_parameter(+1);
            movement = 1;
        }
        if (movement)
        {
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }

        if (GAMEPAD_PRESS(0, X))
        { 
            // delete / cut
            game_message[0] = 0;
            go_command_copy = sprite_pattern[edit_sprite/8][go_pattern_pos];
            for (int j=go_pattern_pos; j<31; ++j)
            {
                sprite_pattern[edit_sprite/8][j] = sprite_pattern[edit_sprite/8][j+1];
            }
            sprite_pattern[edit_sprite/8][31] = GO_BREAK;
            return;
        }

        if (GAMEPAD_PRESS(0, Y))
        {
            // insert
            if ((sprite_pattern[edit_sprite/8][31]&15) != GO_BREAK)
            {
                strcpy((char *)game_message, "list full, can't insert.");
                return;
            }
            for (int j=31; j>go_pattern_pos; --j)
            {
                sprite_pattern[edit_sprite/8][j] = sprite_pattern[edit_sprite/8][j-1];
            }
            sprite_pattern[edit_sprite/8][go_pattern_pos] = go_command_copy;
            return;
        }

        if (GAMEPAD_PRESS(0, A))
        {
            // ??
            return;
        }
        
        if (GAMEPAD_PRESSING(0, B))
        {
            // ??
            return;
        }
    }
    
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        go_copying = 16;
        go_menu_not_edit = 1 - go_menu_not_edit;
        return;
    }

    if (GAMEPAD_PRESS(0, select))
    {
        go_copying = 16;
        game_message[0] = 0;
        previous_visual_mode = None;
        game_switch(SaveLoadScreen);
        return;
    } 
}

static inline int test_inside_tile(int x, int y)
{
    // return 0 for no hit, 1 for solid, -1 for damage
    int index = y*tile_map_width + x;
    uint8_t tile;
    if (index % 2)
        tile = tile_map[index/2] >> 4;
    else
        tile = tile_map[index/2] & 15;
    uint32_t info = tile_info[tile_translator[tile]];
    if (!(info & 8))
        return 0; // TODO: check for warps
    SideType side_up = (info >> (16 + 4*UP))&15;
    SideType side_down = (info >> (16 + 4*DOWN)); // don't need &15 since it's at the end.
    if (side_up == 0 || side_down == 0)
        return 0;
    if (side_up/2 == Damaging/2 || side_down/2 == Damaging/2)
        return -1;
    // TODO: check for other conditions?
    return 1;
}

static inline int test_tile(int x, int y, int dir)
{
    // return 0 for no hit, 1 for solid, -1 for damage
    int index = y*tile_map_width + x;
    uint8_t tile;
    if (index % 2)
        tile = tile_map[index/2] >> 4;
    else
        tile = tile_map[index/2] & 15;
    uint32_t info = tile_info[tile_translator[tile]];
    if (!(info & 8))
        return 0; // TODO: check for warps
    return (info >> (16 + 4*dir))&15;
}

void object_run_commands(uint8_t i) 
{
    // update position here, too
    object[i].properties |= IN_AIR; // assume you're flying until you're not...
    if (object[i].properties & GHOSTING)
    {
        object[i].y += object[i].vy;
        if (object[i].y > tile_map_height*16 - 32)
        {
            object[i].y = tile_map_height*16 - 32;
            object[i].vy = 0;
        }
        
        object[i].x += object[i].vx;
        if (object[i].x < 16)
        {
            object[i].x = 16;
            object[i].vx = 0;
        }
        else if (object[i].x > tile_map_width*16-32)
        {
            object[i].x = tile_map_width*16-32;
            object[i].vx = 0;
        }

        goto object_execute_commands;
    }
    // run physics and hit tests
    //   e.g. if colliding against ground, set vy = 0

    // avoid adding in gravity for non-platformers:
    object[i].vy += gravity;
    if (object[i].vy > MAX_VY)
        object[i].vy = MAX_VY;
    object[i].y += object[i].vy;
    if (object[i].y < 16) {}
    else if (object[i].y > tile_map_height*16 - 32)
    {
        object[i].y = tile_map_height*16 - 32;
        object[i].vy = 0;
        object[i].properties &= ~IN_AIR;
        // TODO: maybe test object right below player here.
    }
    else if (object[i].vy == 0.0f || (16.0f*((int)(object[i].y/16)) == object[i].y)) {}
    else if (object[i].vy > 0)
    {
        // test collision onto some tile's UP side.
        int y_tile = object[i].y/16 + 1;
        int x_tile = object[i].x/16;
        float x_delta = object[i].x - 16.0f*x_tile;
        if (x_delta == 0.0f)
        {
            switch (test_tile(x_tile, y_tile, UP))
            {
            case Passable:
                break;
            case Damaging:
            case SuperDamaging:
                message("need to add hurt damage here!\n");
            case Normal:
                object[i].y = 16*(y_tile-1);
                object[i].vy = 0;
                object[i].properties &= ~IN_AIR;
                break;
            }
        }
        else
        {
            // gotta test left and right tiles
            int hit_left = test_tile(x_tile, y_tile, UP);
            int hit_right = test_tile(x_tile+1, y_tile, UP);
            if (hit_right)
            {
                object[i].y = 16*(y_tile-1);
                if (hit_left)
                {
                    // TODO inflict damage if necessary
                    object[i].vy = 0;
                    object[i].properties &= ~IN_AIR;
                }
                else if (x_delta < 3.0f && object[i].vx < 0)
                {
                    // about to fall off to the left:
                    // decide what to do based on edge behavior
                    switch (object[i].edge_accel&15)
                    {
                        case 9:
                        case 10:
                        case 11:
                            // turn around
                            // TODO: this doesn't seem to help a player 
                            // who is trying to avoid jumping off the ledge...
                            object[i].x = x_tile*16.0f + 3.0f;
                            object[i].vx *= -1;
                            object[i].sprite_frame = (object[i].sprite_frame + 4)%8;
                            object[i].vy = 0;
                            object[i].properties &= ~IN_AIR;
                            break;
                        case 12:
                            // jump
                            object[i].vy = -(object[i].speed_jump >> 4)*JUMP_MULTIPLIER;
                            break;
                        case 13:
                            // turn around if low enough speed
                            if (object[i].vx > -SPEED_MULTIPLIER)
                            {
                                object[i].x = x_tile*16.0f + 3.0f;
                                object[i].vx *= -1; 
                                object[i].sprite_frame = (object[i].sprite_frame + 4)%8;
                            }
                            else
                                object[i].vx /= (1 + DECELERATION_MULTIPLIER*(object[i].edge_accel>>6));
                            object[i].vy = 0;
                            object[i].properties &= ~IN_AIR;
                            break;
                        case 14:
                            // stop if small enough speed
                            if (object[i].vx > -SPEED_MULTIPLIER)
                            {
                                object[i].x = x_tile*16.0f + 3.0f;
                                object[i].vx = 0; 
                            }
                            else
                                object[i].vx /= (1 + DECELERATION_MULTIPLIER*(object[i].edge_accel>>6));
                            object[i].vy = 0;
                            object[i].properties &= ~IN_AIR;
                            break;
                        case 15:
                            // stop no matter what
                            object[i].x = x_tile*16.0f + 3.0f;
                            object[i].vx = 0;
                            object[i].vy = 0;
                            object[i].properties &= ~IN_AIR;
                            break;
                        default: // 8 or lower
                            // here is just simple fall off:
                            //message("fell off left edge %f\n", object[i].x);
                            object[i].x = x_tile*16.0f;
                            object[i].vx /= 2;
                    }
                }
                else
                {
                    object[i].vy = 0;
                    object[i].properties &= ~IN_AIR;
                }
            }
            else if (hit_left)
            {
                // TODO inflict damage if necessary
                object[i].y = 16*(y_tile-1);
                if (x_delta > 13.0f && object[i].vx > 0)
                {
                    // about to fall off to the right:
                    // decide what to do based on edge behavior
                    switch (object[i].edge_accel&15)
                    {
                        case 9:
                        case 10:
                        case 11:
                            // turn around
                            // TODO: this doesn't seem to help a player 
                            // who is trying to avoid jumping off the ledge...
                            object[i].x = (x_tile+1)*16.0f - 3.0f;
                            object[i].vx *= -1;
                            object[i].sprite_frame = (object[i].sprite_frame + 4)%8;
                            object[i].vy = 0;
                            object[i].properties &= ~IN_AIR;
                            break;
                        case 12:
                            // jump
                            object[i].vy = -(object[i].speed_jump >> 4)*JUMP_MULTIPLIER;
                            break;
                        case 13:
                            // turn around if low enough speed
                            if (object[i].vx < SPEED_MULTIPLIER)
                            {
                                object[i].x = (x_tile+1)*16.0f - 3.0f;
                                object[i].vx *= -1; 
                                object[i].sprite_frame = (object[i].sprite_frame + 4)%8;
                            }
                            else
                                object[i].vx /= (1 + DECELERATION_MULTIPLIER*(object[i].edge_accel>>6));
                            object[i].vy = 0;
                            object[i].properties &= ~IN_AIR;
                            break;
                        case 14:
                            // stop if small enough speed
                            if (object[i].vx < SPEED_MULTIPLIER)
                            {
                                object[i].x = (x_tile+1)*16.0f - 3.0f;
                                object[i].vx = 0; 
                            }
                            else
                                object[i].vx /= (1 + DECELERATION_MULTIPLIER*(object[i].edge_accel>>6));
                            object[i].vy = 0;
                            object[i].properties &= ~IN_AIR;
                            break;
                        case 15:
                            // stop no matter what
                            object[i].x = (x_tile+1)*16.0f - 3.0f;
                            object[i].vx = 0;
                            object[i].vy = 0;
                            object[i].properties &= ~IN_AIR;
                            break;
                        default: // 8 or lower
                            // here is just simple fall off:
                            //message("fell off right edge %f\n", object[i].x);
                            object[i].x = (x_tile+1)*16.0f;
                            object[i].vx /= 2;
                    }
                }
                else
                {
                    //if (hit_left < 0)
                    //    message("need to add hurt damage here\n");
                    object[i].vy = 0;
                    object[i].properties &= ~IN_AIR;
                }
            }
        }
    }
    else // vy < 0
    {
        // check bumping head
        int y_tile = object[i].y/16;
        int x_tile = object[i].x/16;
        float x_delta = object[i].x - 16.0f*x_tile;
        if (x_delta == 0.0f)
        {
            switch (test_tile(x_tile, y_tile, DOWN))
            {
            case Passable:
                break;
            case Damaging:
            case SuperDamaging:
                message("need to add hurt damage here!\n");
            case Normal:
                object[i].y = 16*(y_tile+1);
                object[i].vy = 0;
                message("bumped up to something\n");
                break;
            }
        }
        else
        {
            // gotta test left and right tiles
            int hit_left = test_tile(x_tile, y_tile, DOWN);
            int hit_right = test_tile(x_tile+1, y_tile, DOWN);
            if (hit_left)
            {
                if (hit_right)
                {
                    int hurt = 0;
                    switch (hit_right)
                    {
                    case Passable:
                        break;
                    case SuperDamaging:
                        hurt |= 2;
                    case Damaging:
                        hurt |= 1;
                    case Normal:
                        object[i].y = 16*(y_tile+1);
                        object[i].vy = 0;
                        break;
                    }
                    switch (hit_left)
                    {
                    case Passable:
                        break;
                    case SuperDamaging:
                        hurt |= 2;
                    case Damaging:
                        hurt |= 1;
                    case Normal:
                        object[i].y = 16*(y_tile+1);
                        object[i].vy = 0;
                        break;
                    }
                    // TODO check hurt!
                }
                else if (x_delta > 13.0f)
                {
                    // be nice, jump into hole
                    object[i].x = 16.0f*(x_tile+1);
                    message("jumping up into hole\n");
                }
                else // just hit left
                switch (hit_left)
                {
                case Passable:
                    break;
                case Damaging:
                case SuperDamaging:
                    message("need to add hurt damage here!\n");
                case Normal:
                    object[i].y = 16*(y_tile+1);
                    object[i].vy = 0;
                    break;
                }
            }
            else if (hit_right)
            {
                if (x_delta < 3.0f)
                {
                    // be nice, jump into hole
                    object[i].x = 16.0f*x_tile;
                    message("jumping up into hole\n");
                }
                else
                switch (hit_right)
                {
                case Passable:
                    break;
                case Damaging:
                case SuperDamaging:
                    message("need to add hurt damage here!\n");
                case Normal:
                    object[i].y = 16*(y_tile+1);
                    object[i].vy = 0;
                    break;
                }
            }
        }
    }
    object[i].x += object[i].vx;
    if (object[i].x < 16)
    {
        object[i].x = 16;
        object[i].vx = 0;
    }
    else if (object[i].x > tile_map_width*16-32)
    {
        object[i].x = tile_map_width*16-32;
        object[i].vx = 0;
    }
    else if (object[i].vx == 0.0f || ( ((int)(object[i].x/16)*16.0f) == object[i].x)) {}
    else if (object[i].vx > 0)
    {
        // test colliding into something's LEFT side
        float old_vx = object[i].vx;
        int x_tile = object[i].x/16 + 1;
        int y_tile = object[i].y/16;
        float y_delta = object[i].y - 16.0f*y_tile;
        if (y_delta == 0.0f)
        {
            switch (test_tile(x_tile, y_tile, LEFT))
            {
            case Passable:
                break;
            case Damaging:
            case SuperDamaging:
                message("need to add hurt damage here!\n");
            case Normal:
                object[i].x = 16*(x_tile-1);
                object[i].vx = 0;
                break;
            }
        }
        else
        {
            int hit_up = test_tile(x_tile, y_tile, LEFT);
            int hit_down = test_tile(x_tile, y_tile+1, LEFT);
            if (hit_up)
            {
                if (hit_down)
                {
                    int hurt = 0;
                    switch (hit_up)
                    {
                    case Passable:
                        break;
                    case SuperDamaging:
                        hurt |= 2;
                    case Damaging:
                        hurt |= 1;
                    case Normal:
                        object[i].x = 16*(x_tile-1);
                        object[i].vx = 0;
                        break;
                    }
                    switch (hit_down)
                    {
                    case Passable:
                        break;
                    case SuperDamaging:
                        hurt |= 2;
                    case Damaging:
                        hurt |= 1;
                    case Normal:
                        object[i].x = 16*(x_tile-1);
                        object[i].vx = 0;
                        break;
                    }
                    // TODO
                    // make hurt
                }
                else if (y_delta > 13.0f)
                {
                    object[i].y = 16*(y_tile+1);
                }
                else
                switch (hit_up)
                {
                case Passable:
                    break;
                case Damaging:
                case SuperDamaging:
                    message("need to add hurt damage here!\n");
                case Normal:
                    object[i].x = 16*(x_tile-1);
                    object[i].vx = 0;
                    break;
                }
            }
            else if (hit_down)
            {
                if (y_delta < 3.0f)
                {
                    object[i].y = 16*(y_tile);
                }
                else
                switch (hit_down)
                {
                case Passable:
                    break;
                case Damaging:
                case SuperDamaging:
                    message("need to add hurt damage here!\n");
                case Normal:
                    object[i].x = 16*(x_tile-1);
                    object[i].vx = 0;
                    break;
                }
            }
        }
        if (object[i].vx == 0.0f)
        switch (object[i].edge_accel&15)
        {
            case 8: // fall off edges
            case 14: // stop or fall
            case 15: // stop at edges
                break;
            case 9: // turn CCW
                object[i].vy = -old_vx;
                object[i].sprite_frame = 2*UP;
                break;
            case 11: // turn CW
                object[i].vy = old_vx;
                object[i].sprite_frame = 2*DOWN;
                break;
            default: // turn 180 (at 10), plus other things
                object[i].vx = -old_vx;
                object[i].sprite_frame = 2*LEFT;
                break;
        }
    }
    else // vx < 0
    {
        float old_vx = object[i].vx;
        int x_tile = object[i].x/16;
        int y_tile = object[i].y/16;
        float y_delta = object[i].y - 16.0f*y_tile;
        if (y_delta == 0.0f)
        {
            switch (test_tile(x_tile, y_tile, RIGHT))
            {
            case Passable:
                break;
            case Damaging:
            case SuperDamaging:
                message("need to add hurt damage here!\n");
            case Normal:
                object[i].x = 16*(x_tile+1);
                object[i].vx = 0;
                break;
            }
        }
        else
        {
            int hit_up = test_tile(x_tile, y_tile, RIGHT);
            int hit_down = test_tile(x_tile, y_tile+1, RIGHT);
            if (hit_up)
            {
                if (hit_down)
                {
                    int hurt = 0;
                    switch (hit_up)
                    {
                    case Passable:
                        break;
                    case SuperDamaging:
                        hurt |= 2;
                    case Damaging:
                        hurt |= 1;
                    case Normal:
                        object[i].x = 16*(x_tile+1);
                        object[i].vx = 0;
                        break;
                    }
                    switch (hit_down)
                    {
                    case Passable:
                        break;
                    case SuperDamaging:
                        hurt |= 2;
                    case Damaging:
                        hurt |= 1;
                    case Normal:
                        object[i].x = 16*(x_tile+1);
                        object[i].vx = 0;
                        break;
                    }
                    // TODO
                    // make hurt
                }
                else if (y_delta > 13.0f)
                {
                    object[i].y = 16*(y_tile+1);
                }
                else
                switch (hit_up)
                {
                case Passable:
                    break;
                case Damaging:
                case SuperDamaging:
                    message("need to add hurt damage here!\n");
                case Normal:
                    object[i].x = 16*(x_tile+1);
                    object[i].vx = 0;
                    break;
                }
            }
            else if (hit_down)
            {
                if (y_delta < 3.0f)
                {
                    object[i].y = 16*(y_tile);
                }
                else
                switch (hit_down)
                {
                case Passable:
                    break;
                case Damaging:
                case SuperDamaging:
                    message("need to add hurt damage here!\n");
                case Normal:
                    object[i].x = 16*(x_tile+1);
                    object[i].vx = 0;
                    break;
                }
            }
        }
        if (object[i].vx == 0.0f)
        switch (object[i].edge_accel&15)
        {
            case 8: // fall off edges
            case 14: // stop or fall
            case 15: // stop at edges
                break;
            case 9: // turn CCW
                object[i].vy = old_vx;
                object[i].sprite_frame = 2*DOWN;
                break;
            case 11: // turn CW
                object[i].vy = -old_vx;
                object[i].sprite_frame = 2*UP;
                break;
            default: // turn 180 (at 10), plus other things
                object[i].vx = -old_vx;
                object[i].sprite_frame = 2*RIGHT;
                break;
        }
    }
    // finally check the space that the sprite is in.
    int y_tile = object[i].y;
    if (y_tile%16)
    {
        y_tile = y_tile/16 + 1;
        if (tile_xy_is_block(object[i].x/16, y_tile))
        {
            object[i].y = 16.0f*(y_tile-1);
            message("internal hit %f\n", object[i].y);
            object[i].vy = 0;
            object[i].properties &= ~IN_AIR;
        }
    }

    object_execute_commands:
    if (object[i].wait)
    {
        --object[i].wait;
        return;
    }
    while (object[i].cmd_index < 32)
    {
        uint8_t cmd = sprite_pattern[object[i].sprite_index][object[i].cmd_index++];
        uint8_t param = cmd >> 4;
        switch (cmd & 15)
        {
        case GO_BREAK:
            if (param)
                object[i].wait = param + param*param/2; // tweak as necessary
            else
                object[i].cmd_index = 0;
            return;
        case GO_NOT_MOVE:
            if (object[i].vx || object[i].vy)
            {
                if (!param)
                    param = 16;
                object[i].cmd_index += param;
            }
            break;
        case GO_NOT_RUN:
            if (object[i].properties & RUNNING)
                break;
            if (!param)
                param = 16;
            object[i].cmd_index += param;
            break;
        case GO_NOT_AIR:
            if (object[i].properties & IN_AIR)
                break;
            if (!param)
                param = 16;
            object[i].cmd_index += param;
            break;
        case GO_NOT_FIRE:
            if (object[i].firing)
            {
                if (--object[i].firing == FIRE_COUNT-1)
                    // continue executing at next index if firing was FIRE_COUNT
                    break;
            }
            // otherwise, jump forward a few indices:
            if (!param)
                param = 16;
            object[i].cmd_index += param;
            break;
        case GO_EXECUTE:
            switch (param)
            {
            case 0: // stop
                object[i].vx = object[i].vy = 0;
                break;
            case 1: // turn CCW
            {
                object[i].sprite_frame = (object[i].sprite_frame + 2)%8;
                float vx = object[i].vx;
                object[i].vx = object[i].vy;
                object[i].vy = -vx;
                break;
            }
            case 2: // turn 180
                object[i].sprite_frame = (object[i].sprite_frame + 4)%8;
                object[i].vx = -object[i].vx;
                object[i].vy = -object[i].vy;
                break;
            case 3: // turn 270 = CW
            {
                object[i].sprite_frame = (object[i].sprite_frame + 6)%8;
                float vx = object[i].vx;
                object[i].vx = -object[i].vy;
                object[i].vy = vx;
                break;
            }
            case 4: // walk
                switch (object[i].sprite_frame/2)
                {
                case RIGHT:
                    object[i].vy = 0;
                    object[i].vx = (object[i].speed_jump&15)*SPEED_MULTIPLIER;
                    break;
                case UP:
                    object[i].vy = -(object[i].speed_jump&15)*SPEED_MULTIPLIER;
                    object[i].vx = 0;
                    // TODO: in platformer only:
                    object[i].properties |= IN_AIR;
                    break;
                case LEFT:
                    object[i].vy = 0;
                    object[i].vx = -(object[i].speed_jump&15)*SPEED_MULTIPLIER;
                    break;
                case DOWN:
                    object[i].vy = (object[i].speed_jump&15)*SPEED_MULTIPLIER;
                    object[i].vx = 0;
                    break;
                }
                break;
            case 5: // toggle run
                if (object[i].properties & RUNNING)
                    object[i].properties &= ~RUNNING;
                else
                    object[i].properties |= RUNNING;
                break;
            case 6: // do a jump
                object[i].vy = -(object[i].speed_jump >> 4)*JUMP_MULTIPLIER;
                object[i].properties |= IN_AIR;
                break;
            case 7: // toggle ghost
                if (object[i].properties & GHOSTING)
                    object[i].properties &= ~GHOSTING;
                else
                    object[i].properties |= GHOSTING;
                break;
            default:
                // the rest are edge behaviors
                object[i].edge_accel &= 240;
                object[i].edge_accel |= param;
            }
            break;
        case GO_LOOK:
            break;
        case GO_DIRECTION:
        {
            int p = param/8;
            if (param == 4)
            {
                // need special treatment here, random movement when pressing down
                if (!(gamepad_buttons[p] & (gamepad_up | gamepad_down | gamepad_left | gamepad_right)))
                {
                    object[i].vx /= (1 + DECELERATION_MULTIPLIER*(object[i].edge_accel>>6));
                    if ((object[i].properties & GHOSTING) || 0) // for non-platformer
                    {
                        object[i].vy /= DECELERATION_MULTIPLIER*(1 + (object[i].edge_accel>>6));
                    }
                    break;
                }
                switch (rand()%4)
                {
                    case 0:
                    {
                        object[i].vx -= (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*LEFT;
                        float vx_limit = -(object[i].speed_jump&15)*SPEED_MULTIPLIER;
                        if (object[i].vx < vx_limit)
                            object[i].vx = vx_limit;
                        break;
                    }
                    case 1:
                    {
                        object[i].vx += (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*RIGHT;
                        float vx_limit = (object[i].speed_jump&15)*SPEED_MULTIPLIER;
                        if (object[i].vx > vx_limit)
                            object[i].vx = vx_limit;
                        break;
                    }
                    case 2:
                    if ((object[i].properties & GHOSTING) || 0) // 0 = non-platformer
                    {
                        object[i].vy -= (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*UP;
                        float vy_limit = -(object[i].speed_jump>>4)*SPEED_MULTIPLIER;
                        if (object[i].vy < vy_limit)
                            object[i].vy = vy_limit;
                        break;
                    }
                    case 3:
                    {
                        object[i].vy += (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*DOWN;
                        // only check this for non-platformer,
                        // a platformer will fix vy automatically
                        if ((object[i].properties & GHOSTING) || 0)
                        {
                            float vy_limit = (object[i].speed_jump>>4)*SPEED_MULTIPLIER;
                            if (object[i].vy > vy_limit)
                                object[i].vy = vy_limit;
                        }
                        break;
                    }
                }

                break;
            }
            if (param & 1)
            {
                int motion = 0;
                if (GAMEPAD_PRESSED(p, left))
                    --motion;
                if (GAMEPAD_PRESSED(p, right))
                    ++motion;

                if (motion)
                {
                    if (param & 4)
                        motion *= -1;
                    if (motion < 0)
                    {
                        object[i].vx -= (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*LEFT + ((int)object[i].x/8)%2;
                        float vx_limit = -(object[i].speed_jump&15)*SPEED_MULTIPLIER;
                        if (object[i].vx < vx_limit)
                            object[i].vx = vx_limit;
                    }
                    else
                    {
                        object[i].vx += (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*RIGHT + ((int)object[i].x/8)%2;
                        float vx_limit = (object[i].speed_jump&15)*SPEED_MULTIPLIER;
                        if (object[i].vx > vx_limit)
                            object[i].vx = vx_limit;
                    }
                }
                else
                {
                    object[i].vx /= (1 + DECELERATION_MULTIPLIER*(object[i].edge_accel>>6));
                }
            }
            if (param & 2)
            {
                int motion = 0;
                if (GAMEPAD_PRESSED(p, up))
                    --motion;
                if (GAMEPAD_PRESSED(p, down))
                    ++motion;

                if (motion)
                {
                    if (param & 4)
                        motion *= -1;
                    if (motion < 0)
                    {
                        object[i].vy -= (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*UP + ((int)object[i].y/8)%2;
                        if ((object[i].properties & GHOSTING) || 0) // check for non-platformer
                        {
                            float vy_limit = -(object[i].speed_jump>>4)*SPEED_MULTIPLIER;
                            if (object[i].vy < vy_limit)
                                object[i].vy = vy_limit;
                        }
                    }
                    else
                    {
                        object[i].vy += (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*DOWN + ((int)object[i].y/8)%2;
                        // only check this for non-platformer,
                        // a platformer will fix vy automatically
                        if ((object[i].properties & GHOSTING) || 0)
                        {
                            float vy_limit = (object[i].speed_jump>>4)*SPEED_MULTIPLIER;
                            if (object[i].vy > vy_limit)
                                object[i].vy = vy_limit;
                        }
                    }
                }
                else if ((object[i].properties & GHOSTING) || 0) // 0 = non-platformer
                {
                    object[i].vy /= (1 + DECELERATION_MULTIPLIER*(object[i].edge_accel>>6));
                }
            }
            break;
        }
        case GO_SPECIAL_INPUT:
        {
            int p = param/8;
            if (param & 1) // run
            {
                if ((gamepad_buttons[p] & (gamepad_Y | gamepad_A)))
                    object[i].properties |= RUNNING;
                else
                    object[i].properties &= ~RUNNING;
            }
            if (param & 2) // jump
            {
                if (GAMEPAD_PRESSED(p, B))
                {
                    object[i].vy = -(object[i].speed_jump >> 4)*JUMP_MULTIPLIER;
                    object[i].properties |= IN_AIR;
                }
            }
            if (param & 4) // fire 
            {
                if (GAMEPAD_PRESSED(p, X))
                {
                    if (!object[i].firing)
                        object[i].firing = FIRE_COUNT;
                }
                else if (object[i].firing == FIRE_COUNT)
                {
                    // haven't encountered a "GO_NOT_FIRE" yet,
                    // assume we need to turn off the fire power.
                    object[i].firing = 0;
                }
            }
            break;
        }
        case GO_SPAWN_TILE:
            break;
        case GO_SPAWN_SPRITE:
            break;
        case GO_ACCELERATION:
            object[i].edge_accel &= 15; // keep edge behavior
            object[i].edge_accel |= param << 4;
            break;
        case GO_SPEED:
            if (param & 8) // jump speed
            {
                object[i].speed_jump &= 15; // keep run speed
                object[i].speed_jump |= (param-7) << 4;
            }
            else // run speed
            {
                object[i].speed_jump &= 240;
                object[i].speed_jump |= 1 + param;
            }
            break;
        case GO_NOISE:
            break;
        case GO_RANDOMIZE:
            if (object[i].cmd_index >= 32)
                goto end_run_commands;
            uint8_t *memory = &sprite_pattern[object[i].sprite_index][object[i].cmd_index];
            *memory = ((*memory)&15) | (randomize(param)<<4);
            break;
        case GO_QUAKE:
            break;
        }
    }
    // only way to get here is to get to object[i].cmd_index >= 32
    end_run_commands:
    object[i].cmd_index = 0;
}
