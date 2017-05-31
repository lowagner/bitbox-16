#include "bitbox.h"
#include "unlocks.h"
#include "common.h"
#include "run.h"
#include "hit.h"
#include "map.h"
#include "sprites.h"

#define SPEED_MULTIPLIER 0.5f

#include <math.h> // round
#include <stdlib.h> // rand

float damage_multiplier CCM_MEMORY;
float gravity CCM_MEMORY;
uint8_t unlocks[8][32] CCM_MEMORY;
int unlocks_index CCM_MEMORY;
int unlocks_command_index CCM_MEMORY;
static uint8_t unlocks_next[32] CCM_MEMORY;
static uint8_t unlocks_count[32] CCM_MEMORY;
int unlocks_x CCM_MEMORY;
int unlocks_y CCM_MEMORY;

void unlocks_init()
{
}

void unlocks_switch(int p, int next_unlocks_index)
{
    if (unlocks_index == UNLOCKS_DEAD)
        return run_stop(0);
    if (unlocks_index == UNLOCKS_WIN && next_unlocks_index != UNLOCKS_DEAD)
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

void unlocks_start()
{
    unlocks_index = -1;
    unlocks_switch(0, UNLOCKS_INIT); // level initialization
    damage_multiplier = 1.0f;
    gravity = 1.0f;
}

void kill_player(int index)
{
    unlocks_switch(index, UNLOCKS_DEAD);
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
                if (unlocks_index == UNLOCKS_WIN)
                {
                    run_stop(1);
                    break;
                }

                unlocks_switch(0, UNLOCKS_LOOP);
            }
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
                if (param == 0)
                    param = 4;
                if (object[index].health + param >= 255)
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
            uint8_t index = create_object(8*param, spawn_x, spawn_y, 
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
