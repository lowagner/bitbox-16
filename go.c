#include "bitbox.h"
#include "common.h"
#include "common.h"
#include "chiptune.h"
#include "sprites.h"
#include "edit.h"
#include "name.h"
#include "font.h"
#include "io.h"
#include "go.h"

#include <stdlib.h> // rand
#include <string.h> // memset

#define BG_COLOR 132
#define PLAY_COLOR (RGB(200, 100, 0)|(RGB(200, 100, 0)<<16))
#define BOX_COLOR (RGB(200, 200, 230)|(RGB(200, 200, 230)<<16))
#define MATRIX_WING_COLOR (RGB(30, 90, 90) | (RGB(30, 90, 90)<<16))
#define NUMBER_LINES 20

uint8_t go_pattern_pos CCM_MEMORY;
uint8_t go_pattern_offset CCM_MEMORY;
uint8_t go_menu_not_edit CCM_MEMORY;
uint8_t go_command_copy CCM_MEMORY;
uint8_t go_copying CCM_MEMORY;
uint8_t go_show_track CCM_MEMORY;

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
                if (y == 7)
                    go_show_track = 0;
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
            // TODO: need something fancier for all these params
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
            cmd = 'T';
            param = hex[param];
            break;
        case GO_SPEED:
            cmd = 'S'; 
            param = hex[param];
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
            param = hex[2*param];
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
        return;
    }
    --internal_line;
    uint8_t buffer[24];
    switch (line)
    {
        case 0:
        {
            // edit track
            uint8_t msg[] = { 'g', 'o', ' ', 's', 'p', 'r', 'i', 't', 'e', ' ', hex[edit_sprite/8], 
            0 };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 1:
            break;
        case 2:
        {
            go_show_track = 1; 
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
                    strcpy((char *)buffer, "wait or break (if 0)");
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
                    strcpy((char *)buffer, "execute");
                    break;
                case GO_LOOK:
                    strcpy((char *)buffer, "look for player");
                    break;
                case GO_DIRECTION:
                    strcpy((char *)buffer, "handle directions");
                    break;
                case GO_SPECIAL_INPUT:
                    strcpy((char *)buffer, "handle special input");
                    break;
                case GO_SPAWN_TILE:
                    strcpy((char *)buffer, "spawn a tile");
                    break;
                case GO_SPAWN_SPRITE:
                    strcpy((char *)buffer, "spawn a sprite");
                    break;
                case GO_ACCELERATION:
                    strcpy((char *)buffer, "set acceleration");
                    break;
                case GO_SPEED:
                    strcpy((char *)buffer, "set speed");
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
            goto maybe_show_go;
        case 4:
            // TODO:  add more explanation for weird commands
            //switch (sprite_pattern[edit_sprite/8][go_pattern_pos]&15)
            //{
            //}
            goto maybe_show_go;
        case 5:
            font_render_line_doubled((uint8_t *)"switch to:", 102 - 6*go_menu_not_edit, internal_line, 65535, BG_COLOR*257); 
            goto maybe_show_go;
        case 6:
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
            goto maybe_show_go;
        case 7:
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
            goto maybe_show_go;
        case 8:
            font_render_line_doubled((uint8_t *)"dpad:", 102 - 6*go_menu_not_edit, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_go;
        case 9:
            if (go_menu_not_edit)
                {} //font_render_line_doubled((uint8_t *)"switch player", 112, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"adjust parameters", 112, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_go;
        case 11:
            if (go_menu_not_edit)
            {
                if (go_copying < 16)
                    font_render_line_doubled((uint8_t *)"A:cancel copy", 96, internal_line, 65535, BG_COLOR*257);
                else
                    font_render_line_doubled((uint8_t *)"A:save to file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                {}//font_render_line_doubled((uint8_t *)"A:play track", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_go;
        case 12:
            if (go_menu_not_edit)
            {
                if (go_copying < 16)
                    font_render_line_doubled((uint8_t *)"B/X:\"     \"", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"B:load from file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                {} //font_render_line_doubled((uint8_t *)"B:edit instrument", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_go;
        case 13:
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
            goto maybe_show_go;
        case 14:
            if (go_menu_not_edit)
            {
                if (go_copying < 16)
                    goto maybe_show_go;
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), base_filename);
                font_render_line_doubled(buffer, 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"Y:insert cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_go;
        case 16:
            if (go_menu_not_edit)
                font_render_line_doubled((uint8_t *)"start:edit pattern", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:pattern menu", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_go;
        case 17:
            font_render_line_doubled((uint8_t *)"select:edit sprite view", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_go;
        case 18:
            break;
        case 19:
            font_render_line_doubled(game_message, 36, internal_line, 65535, BG_COLOR*257);
            break;
        default:
          maybe_show_go:
            if (go_show_track)
                go_render_command(go_pattern_offset+line-2, internal_line);
            break; 
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
                previous_visual_mode = EditGoPattern;
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
            //if (save_or_load == 1)
            //    error = io_save_go(edit_sprite/8);
            //else
            //    error = io_load_go(edit_sprite/8);
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
            if (go_pattern_pos < 31 &&
                sprite_pattern[edit_sprite/8][go_pattern_pos])
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
                go_pattern_pos = 0;
                while (go_pattern_pos < 31 && 
                    sprite_pattern[edit_sprite/8][go_pattern_pos] != GO_BREAK)
                {
                    ++go_pattern_pos;
                }
                if (go_pattern_pos > go_pattern_offset + 15)
                    go_pattern_offset = go_pattern_pos - 15;
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
            go_command_copy = sprite_pattern[edit_sprite/8][go_pattern_pos];
            for (int j=go_pattern_pos; j<31; ++j)
            {
                if ((sprite_pattern[edit_sprite/8][j] = sprite_pattern[edit_sprite/8][j+1]) == 0)
                    break;
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
        if (previous_visual_mode)
            game_switch(previous_visual_mode);
        else
            game_switch(EditTileOrSprite);
        return;
    } 
}
