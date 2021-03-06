#include "bitbox.h"
#include "common.h"
#include "io.h"
#include "name.h"
#include "font.h"
#include "unlocks.h"
#include "chiptune.h"
#include "run.h"
#include "hit.h"
#include "map.h"
#include "sprites.h"

#define SPEED_MULTIPLIER 0.5f

#define BG_COLOR 96
#define NUMBER_LINES 20

#include <math.h> // round
#include <stdlib.h> // rand

float damage_multiplier CCM_MEMORY;
float gravity CCM_MEMORY;
uint8_t unlocks[8][32] CCM_MEMORY;
int unlocks_index CCM_MEMORY;
int unlocks_command_index CCM_MEMORY;
int unlocks_pattern_offset CCM_MEMORY;
static uint8_t unlocks_next[32] CCM_MEMORY;
static uint8_t unlocks_count[32] CCM_MEMORY;
int unlocks_x CCM_MEMORY;
int unlocks_y CCM_MEMORY;

uint8_t unlocks_menu_not_edit CCM_MEMORY;
uint8_t unlocks_command_copy CCM_MEMORY;
uint8_t unlocks_copying CCM_MEMORY;

void unlocks_init()
{
}

void unlocks_switch(int p, int next_unlocks_index)
{
    if (unlocks_index == UNLOCKS_LOSE)
        return run_stop(0);
    if (unlocks_index == UNLOCKS_WIN && next_unlocks_index != UNLOCKS_LOSE)
        return;

    for (int i=0; i<32; ++i)
        unlocks_next[i] = i+1;
    unlocks_count[0] = 1;
    unlocks_index = next_unlocks_index;
    unlocks_command_index = 0;
    if (player_index[p] != 255)
    {
        unlocks_x = object[player_index[p]].x;
        unlocks_y = object[player_index[p]].y;
    }
    else
    {
        unlocks_x = 0;
        unlocks_y = 0;
    }
}

void unlocks_start_run()
{
    unlocks_index = -1;
    unlocks_switch(0, UNLOCKS_INIT); // level initialization
    damage_multiplier = 1.0f;
    gravity = 1.0f;
}

void unlocks_start()
{
    unlocks_pattern_offset=0;
    unlocks_index = UNLOCKS_INIT;
    unlocks_command_index = 0;
}

void kill_player(int index)
{
    unlocks_switch(index, UNLOCKS_LOSE);
    player_index[0] = 255;
    player_index[1] = 255;
}

void unlocks_win(int p)
{
    unlocks_win(6);
}

void unlocks_load_default()
{   
    unlocks[0][0] = 0; // tile 0
    unlocks[1][0] = 0; // tile 1
    unlocks[2][0] = 0; // tile 2
    unlocks[3][0] = 0; // tile 3
    unlocks[4][0] = 0; // init
    unlocks[5][0] = 0; // loop
    unlocks[6][0] = 0; // win
    unlocks[7][0] = 0; // die
    
    damage_multiplier = 1.0f;
    gravity = 1.0f;
}

void unlocks_run()
{
    static int wait = 0;
    static int spawn_x=0;
    static int spawn_y=0;
    static int spawn_vx=0;
    static int spawn_vy=0;
    if (wait)
    {
        --wait;
        return;
    }
    if (unlocks_command_index >= 32)
    {
        if (unlocks_index == UNLOCKS_WIN)
            run_stop(1);
        else
            unlocks_switch(0, UNLOCKS_LOOP);
        return;
    }
    uint8_t cmd = unlocks[unlocks_index][unlocks_command_index];
    uint8_t next_index = unlocks_next[unlocks_command_index];
    int update_index = (--unlocks_count[unlocks_command_index] <= 0);
    int update_next_count = update_index;

    uint8_t param = cmd >> 4;
    switch (cmd&15)
    {
        case UNLOCKS_BREAK:
            if (param == 0)
            {
                unlocks_command_index = 32;
                update_index = 0;
            }
            else
                wait = param;
            break;
        case UNLOCKS_GRAVITY:
            gravity = 0.25*param;
            break;
        case UNLOCKS_MULTIPLIER:
            damage_multiplier = param;
            break;
        case UNLOCKS_HURT_HEAL:
        { 
            int index = player_index[param&8];
            param &= 7;
            if (index == 255)
                break;
            if (param < 4) // heal
            {
                if (object[index].health + (++param) >= 255)
                    object[index].health = 255;
                else
                    object[index].health += param;
            }
            else // hurt
                damage_sprite(&object[index], -8+param);
            break;
        }
        case UNLOCKS_SPAWN_NEAR_0:
            if (player_index[0] == 255)
                break;
            spawn_x = round(object[player_index[0]].x/16);
            spawn_y = round(object[player_index[0]].y/16);
            goto player_delta_spawn;
        case UNLOCKS_SPAWN_NEAR_1:
            if (player_index[1] == 255)
                break;
            spawn_x = round(object[player_index[1]].x/16);
            spawn_y = round(object[player_index[1]].y/16);
            player_delta_spawn:
            switch (param&3)
            {
                case 0:
                    break;
                case 1:
                    ++spawn_x;
                    break;
                case 2:
                    spawn_x += 1 - 2*(rand()%2);
                    break;
                case 3:
                    --spawn_x;
                    break;
            }
            switch (param>>2)
            {
                case 0:
                    break;
                case 1:
                    ++spawn_y;
                    break;
                case 2:
                    spawn_y += 1 - 2*(rand()%2);
                    break;
                case 3:
                    --spawn_y;
                    break;
            }
            break;
        case UNLOCKS_DELTA_SPAWN_X:
            if (++param > 8)
                spawn_x -= 17-param;
            else
                spawn_x += param;
            break;
        case UNLOCKS_DELTA_SPAWN_Y:
            if (++param > 8)
                spawn_y -= 17-param;
            else
                spawn_y += param;
            break;
        case UNLOCKS_SPAWN_TILE:
            map_set_tile(spawn_x, spawn_y, param);            
            break;
        case UNLOCKS_SPAWN_SPRITE:
        {
            uint8_t index = create_object(8*param, 16*spawn_x, 16*spawn_y, 
                tile_xy_is_block(spawn_x, spawn_y) ? 0 : 1);
            if (index == 255)
                break;
            if (abs(spawn_vx) >= abs(spawn_vy))
            {
                if (spawn_vx >= 0)
                    object[index].sprite_index = 8*param + 2*RIGHT;
                else
                    object[index].sprite_index = 8*param + 2*LEFT;
            }
            else
            {
                if (spawn_vy >= 0)
                    object[index].sprite_index = 8*param + 2*DOWN;
                else
                    object[index].sprite_index = 8*param + 2*UP;
            }
            object[index].vx = spawn_vx * SPEED_MULTIPLIER;
            object[index].vy = spawn_vy * SPEED_MULTIPLIER;
            break;
        }
        case UNLOCKS_SPRITE_VELOCITY:
            switch (param/4)
            {
                case 0:
                    spawn_vx = param;
                    break;
                case 1:
                    spawn_vx = -8+param;
                    break;
                case 2:
                    spawn_vy = param-8;
                    break;
                case 3:
                    spawn_vy = -16+param;
                    break;
            }
            break;
        case UNLOCKS_NOISE:
        {
            chip_note(
                rand()%4, // random player
                param, // instrument
                rand()%16, // random note
                255 // full volume 
            ); 
            break;
        }
        case UNLOCKS_QUAKE:
            if (param)
                camera_shake += param + param*param;
            else
                camera_shake = 0;
            break;
        case UNLOCKS_RANDOMIZE:
        {
            if (next_index >= 32)
                break;
            uint8_t *memory = &unlocks[unlocks_index][next_index];
            *memory = ((*memory)&15) | (randomize(param)<<4);
            break;
        }
        case UNLOCKS_REPEAT:
            if (next_index >= 32)
                break;
            unlocks_count[next_index] = param + (param <= 1 ? 16 : 0);
            update_next_count = 0;
            break;
        case UNLOCKS_GROUP:
        {
            uint8_t jump_from = unlocks_command_index + param + (param <= 1 ? 16 : 0);
            if (jump_from >= 32)
                break;
            if (update_index) // continue executing after this block/grouping
                unlocks_next[jump_from] = jump_from + 1;
            else // repeat this grouping
                unlocks_next[jump_from] = unlocks_command_index;
            update_index = 1;
            update_next_count = 1;
            break;
        }
    }
    if (update_index)
    {
        unlocks_command_index = next_index;
        if (update_next_count && next_index < 32)
             unlocks_count[next_index] = 1;
    }
}

void unlocks_short_command_message(uint8_t *buffer, uint8_t cmd)
{
    switch (cmd&15)
    {
        case UNLOCKS_BREAK:
            strcpy((char *)buffer, "wait/break");
            break;
        case UNLOCKS_GRAVITY:
            strcpy((char *)buffer, "set gravity");
            break;
        case UNLOCKS_MULTIPLIER:
            strcpy((char *)buffer, "damage multiplier");
            break;
        case UNLOCKS_HURT_HEAL:
            strcpy((char *)buffer, "hurt/heal player");
            break;
        case UNLOCKS_SPAWN_NEAR_0:
            strcpy((char *)buffer, "spawn by player 1");
            break;
        case UNLOCKS_SPAWN_NEAR_1:
            strcpy((char *)buffer, "spawn by player 2");
            break;
        case UNLOCKS_DELTA_SPAWN_X:
            strcpy((char *)buffer, "move spawn dx");
            break;
        case UNLOCKS_DELTA_SPAWN_Y:
            strcpy((char *)buffer, "move spawn dy");
            break;
        case UNLOCKS_SPAWN_TILE:
            strcpy((char *)buffer, "spawn tile");
            break;
        case UNLOCKS_SPAWN_SPRITE:
            strcpy((char *)buffer, "spawn sprite");
            break;
        case UNLOCKS_SPRITE_VELOCITY:
            strcpy((char *)buffer, "sprite velocity");
            break;
        case UNLOCKS_NOISE:
            strcpy((char *)buffer, "noise");
            break;
        case UNLOCKS_QUAKE:
            strcpy((char *)buffer, "quake");
            break;
        case UNLOCKS_RANDOMIZE:
            strcpy((char *)buffer, "randomize");
            break;
        case UNLOCKS_REPEAT:
            strcpy((char *)buffer, "repeat next cmd");
            break;
        case UNLOCKS_GROUP:
            strcpy((char *)buffer, "group commands");
            break;
    }
}

void unlocks_render_command(int j, int y)
{
    int x = 32;
    #ifdef EMULATOR
    if (y < 0 || y >= 8)
    {
        message("got too big a line count for unlocks %d, line %d\n", (int)unlocks_index, j);
        return;
    }
    #endif
    
    uint8_t cmd = unlocks[unlocks_index][j];
    uint8_t param = cmd>>4;
    cmd &= 15;

    uint32_t *dst = (uint32_t *)draw_buffer + x/2;
    uint32_t color_choice[2];
    if (j%2)
        color_choice[0] = 16843009u*BG_COLOR;
    else
        color_choice[0] = 16843009u*149;
    
    if (j != unlocks_command_index)
        color_choice[1] = 65535u*65537u;
    else
    {
        color_choice[1] = RGB(190, 245, 255)|(RGB(190, 245, 255)<<16);
        if (!unlocks_menu_not_edit)
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
        case UNLOCKS_BREAK:
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
        case UNLOCKS_GRAVITY:
            cmd = 'G';
            param = hex[param];
            break;
        case UNLOCKS_MULTIPLIER:
            cmd = 'M';
            param = hex[param];
            break;
        case UNLOCKS_HURT_HEAL:
            cmd = 'H';
            param = hex[param];
            break;
        case UNLOCKS_SPAWN_NEAR_0:
            cmd = '1';
            param += 16;
            break;
        case UNLOCKS_SPAWN_NEAR_1:
            cmd = '2';
            param += 16;
            break;
        case UNLOCKS_DELTA_SPAWN_X:
            if (++param > 8)
            {
                cmd = '-';
                param = hex[17-param];
            }
            else
            {
                cmd = '+';
                param = hex[param];
            }
            break;
        case UNLOCKS_DELTA_SPAWN_Y:
            if (++param > 8)
            {
                cmd = '^';
                param = hex[17-param];
            }
            else
            {
                cmd = 127; // upside down arrow
                param = hex[param];
            }
            break;
        case UNLOCKS_SPAWN_TILE:
            cmd = 'T';
            param = hex[param];
            break;
        case UNLOCKS_SPAWN_SPRITE:
            cmd = 'S';
            param = hex[param];
            break;
        case UNLOCKS_SPRITE_VELOCITY:
            if (param < 8)
            {
                cmd = 'U';
                param = '0' + param;
            }
            else
            {
                cmd = 'V';
                param = '0' + param - 8;
            }
            break;
        case UNLOCKS_NOISE:
            cmd = 'N';
            param = hex[param];
            break;
        case UNLOCKS_QUAKE:
            cmd = 'Q';
            param = hex[param];
            break;
        case UNLOCKS_RANDOMIZE:
            cmd = 'R';
            param = 224 + param;
            break;
        case UNLOCKS_REPEAT:
            cmd = 'D';
            param = hex[param + (param <= 1 ? 16 : 0)];
            break;
        case UNLOCKS_GROUP:
            cmd = 'Q';
            param = hex[param + j + (param <= 1 ? 16 : 0)];
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

void unlocks_adjust_parameter(int direction)
{
    if (!direction)
        return;
    uint8_t cmd = unlocks[unlocks_index][unlocks_command_index];
    uint8_t param = cmd>>4;
    cmd &= 15;
    param = (param + direction)&15;
    unlocks[unlocks_index][unlocks_command_index] = cmd | (param<<4);
}

void unlocks_line()
{
    if (vga_line < 16)
    {
        if (vga_line/2 == 0)
        {
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
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
            uint8_t msg[16] = { 'u', 'n', 'l', 'o', 'c', 'k', 's', ' ' };
            if (unlocks_index < 4)
            {
                msg[8] = '0'+unlocks_index;
                msg[9] = 0;
            }
            else switch (unlocks_index)
            {
                case UNLOCKS_INIT:
                    strcpy((char *)msg+8, "init");
                    break;
                case UNLOCKS_LOOP:
                    strcpy((char *)msg+8, "loop");
                    break;
                case UNLOCKS_WIN:
                    strcpy((char *)msg+8, "win!");
                    break;
                case UNLOCKS_LOSE:
                    strcpy((char *)msg+8, "lose");
                    break;
            }
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 1:
            break;
        case 2:
        {
            unlocks_render_command(unlocks_pattern_offset+line-2, internal_line);
            // command
            uint8_t msg[] = { 'c', 'o', 'm', 'm', 'a', 'n', 'd', ' ', hex[unlocks_command_index], ':', 0 };
            font_render_line_doubled(msg, 96, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 3:
            switch (unlocks[unlocks_index][unlocks_command_index]&15)
            {
                case UNLOCKS_BREAK:
                    strcpy((char *)buffer, "wait or break (if 00)");
                    break;
                case UNLOCKS_GRAVITY:
                    strcpy((char *)buffer, "set gravity strength");
                    break;
                case UNLOCKS_MULTIPLIER:
                    strcpy((char *)buffer, "set damage multiplier");
                    break;
                case UNLOCKS_HURT_HEAL:
                    strcpy((char *)buffer, "hurt or heal player");
                    break;
                case UNLOCKS_SPAWN_NEAR_0:
                    strcpy((char *)buffer, "spawn near player 1");
                    break;
                case UNLOCKS_SPAWN_NEAR_1:
                    strcpy((char *)buffer, "spawn near player 2");
                    break;
                case UNLOCKS_DELTA_SPAWN_X:
                    strcpy((char *)buffer, "change spawn x pos");
                    break;
                case UNLOCKS_DELTA_SPAWN_Y:
                    strcpy((char *)buffer, "change spawn y pos");
                    break;
                case UNLOCKS_SPAWN_TILE:
                    strcpy((char *)buffer, "spawn a tile");
                    break;
                case UNLOCKS_SPAWN_SPRITE:
                    strcpy((char *)buffer, "spawn a sprite");
                    break;
                case UNLOCKS_SPRITE_VELOCITY:
                    strcpy((char *)buffer, "set sprite velocity");
                    break;
                case UNLOCKS_NOISE:
                    strcpy((char *)buffer, "make noise with inst.");
                    break;
                case UNLOCKS_QUAKE:
                    strcpy((char *)buffer, "quake screen");
                    break;
                case UNLOCKS_RANDOMIZE:
                    strcpy((char *)buffer, "randomize next cmd");
                    break;
                case UNLOCKS_REPEAT:
                    strcpy((char *)buffer, "repeat next command");
                    break;
                case UNLOCKS_GROUP:
                    strcpy((char *)buffer, "group next cmds as one");
                    break;
            }
            font_render_line_doubled(buffer, 102, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_unlocks;
        case 4:
            switch (unlocks[unlocks_index][unlocks_command_index]&15)
            {
                case UNLOCKS_HURT_HEAL:
                {
                    char *b = (char *)buffer;
                    uint8_t param = (unlocks[unlocks_index][unlocks_command_index]>>4);
                    if (param & 8)
                        strcpy(b, "P2: ");
                    else
                        strcpy(b, "P1: ");
                    b += 4;
                    param &= 7;
                    if (param < 4)
                    {
                        ++param;
                        strcpy(b, "heal ");
                    }
                    else
                    {
                        param = 8-param;
                        strcpy(b, "hurt ");
                    }
                    b[5] = '0' + param;
                    b[6] = 0;
                    break;
                }
                default:
                    buffer[0] = 0;
            }
            font_render_line_doubled(buffer, 106, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_unlocks;
        case 6:
            font_render_line_doubled((uint8_t *)"switch to:", 102 - 6*unlocks_menu_not_edit, internal_line, 65535, BG_COLOR*257); 
            goto definitely_show_unlocks;
        case 7:
            if (unlocks_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"L:prev unlock", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'L'; buffer[1] = ':';
                unlocks_short_command_message(buffer+2, unlocks[unlocks_index][unlocks_command_index]-1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto definitely_show_unlocks;
        case 8:
            if (unlocks_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"R:next unlock", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'R'; buffer[1] = ':';
                unlocks_short_command_message(buffer+2, unlocks[unlocks_index][unlocks_command_index]+1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto definitely_show_unlocks;
        case 9:
            if (!unlocks_menu_not_edit)
                font_render_line_doubled((uint8_t *)"dpad:", 102 - 6*unlocks_menu_not_edit, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_unlocks;
        case 10:
            if (unlocks_menu_not_edit)
            {
                if (unlocks_copying < 8)
                    font_render_line_doubled((uint8_t *)"A:cancel copy", 96, internal_line, 65535, BG_COLOR*257);
                else
                    font_render_line_doubled((uint8_t *)"A:save to file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"adjust parameters", 112, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_unlocks;
        case 11:
            if (unlocks_menu_not_edit)
            {
                if (unlocks_copying < 8)
                    font_render_line_doubled((uint8_t *)"B/X:\"     \"", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"B:load from file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                {} //font_render_line_doubled((uint8_t *)"B:edit instrument", 96, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_unlocks;
        case 12:
            if (unlocks_menu_not_edit)
            {
                if (unlocks_copying < 8)
                    font_render_line_doubled((uint8_t *)"Y:paste", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"X:copy", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((uint8_t *)"X:cut cmd", 96, internal_line, 65535, BG_COLOR*257);
            }
            goto definitely_show_unlocks;
        case 13:
            if (unlocks_menu_not_edit)
            {
                if (unlocks_copying < 8)
                    goto definitely_show_unlocks;
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), base_filename);
                font_render_line_doubled(buffer, 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"Y:insert cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_unlocks;
        case 15:
            if (unlocks_menu_not_edit)
                font_render_line_doubled((uint8_t *)"start:edit unlock", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:unlocks menu", 96, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_unlocks;
        case 16:
            font_render_line_doubled((uint8_t *)"select:return", 96, internal_line, 65535, BG_COLOR*257);
            goto definitely_show_unlocks;
        case 18:
            break;
        case 19:
            font_render_line_doubled(game_message, 36, internal_line, 65535, BG_COLOR*257);
            break;
        default:
          definitely_show_unlocks:
            unlocks_render_command(unlocks_pattern_offset+line-2, internal_line);
            break; 
    }
}

void unlocks_controls()
{
    if (unlocks_menu_not_edit)
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
            unlocks_index = (unlocks_index+switched)&7;
            unlocks_command_index = 0;
            unlocks_pattern_offset = 0;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }

        if (GAMEPAD_PRESS(0, X)) // copy
        {
            if (unlocks_copying < 8)
            {
                unlocks_copying = 8;
                game_message[0] = 0;
            }
            else
            {
                unlocks_copying = unlocks_index;
                set_game_message_timeout("copied.", MESSAGE_TIMEOUT);
            }
        }

        if (GAMEPAD_PRESS(0, Y)) // paste
        {
            if (unlocks_copying < 8)
            {
                // paste
                uint8_t *src, *dst;
                src = &unlocks[unlocks_copying][0];
                dst = &unlocks[unlocks_index][0];
                if (src == dst)
                {
                    unlocks_copying = 8;
                    set_game_message_timeout("pasting to same thing.", MESSAGE_TIMEOUT);
                    return;
                }
                memcpy(dst, src, sizeof(unlocks[0]));
                set_game_message_timeout("pasted.", MESSAGE_TIMEOUT);
                unlocks_copying = 8;
            }
            else
            {
                // switch to choose name and hope to come back
                game_message[0] = 0;
                game_switch(ChooseFilename);
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
            if (unlocks_copying < 8)
            {
                // cancel a copy 
                unlocks_copying = 8;
                game_message[0] = 0;
                return;
            }

            FileError error = BotchedIt;
            if (save_or_load == 1)
                error = io_save_unlocks(unlocks_index);
            else
                error = io_load_unlocks(unlocks_index);
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
            uint8_t *memory = &unlocks[unlocks_index][unlocks_command_index];
            *memory = ((*memory+movement)&15)|((*memory)&240);
        }
        if (GAMEPAD_PRESSING(0, down))
        {
            if (unlocks_command_index < 31)
            {
                ++unlocks_command_index;
                if (unlocks_command_index > unlocks_pattern_offset + 15)
                    unlocks_pattern_offset = unlocks_command_index - 15;
            }
            else
            {
                unlocks_command_index = 0;
                unlocks_pattern_offset = 0;
            }
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, up))
        {
            if (unlocks_command_index)
            {
                --unlocks_command_index;
                if (unlocks_command_index < unlocks_pattern_offset)
                    unlocks_pattern_offset = unlocks_command_index;
            }
            else
            {
                unlocks_command_index = 31;
                unlocks_pattern_offset = 31 - 15;
            }
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, left))
        {
            unlocks_adjust_parameter(-1);
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, right))
        {
            unlocks_adjust_parameter(+1);
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
            unlocks_command_copy = unlocks[unlocks_index][unlocks_command_index];
            for (int j=unlocks_command_index; j<31; ++j)
            {
                unlocks[unlocks_index][j] = unlocks[unlocks_index][j+1];
            }
            unlocks[unlocks_index][31] = UNLOCKS_BREAK;
            return;
        }

        if (GAMEPAD_PRESS(0, Y))
        {
            // insert
            if ((unlocks[unlocks_index][31]&15) != UNLOCKS_BREAK)
            {
                set_game_message_timeout("list full, can't insert.", MESSAGE_TIMEOUT);
                return;
            }
            for (int j=31; j>unlocks_command_index; --j)
            {
                unlocks[unlocks_index][j] = unlocks[unlocks_index][j-1];
            }
            unlocks[unlocks_index][unlocks_command_index] = unlocks_command_copy;
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
        unlocks_copying = 8;
        unlocks_menu_not_edit = 1 - unlocks_menu_not_edit;
        return;
    }

    if (GAMEPAD_PRESS(0, select))
    {
        unlocks_copying = 8;
        game_message[0] = 0;
        game_switch(SaveLoadScreen);
        return;
    } 
}
