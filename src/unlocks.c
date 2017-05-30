#include "bitbox.h"
#include "unlocks.h"
#include "common.h"
#include "run.h"
#include "hit.h"

#include <math.h> // round
#include <stdlib.h> // rand

float damage_multiplier CCM_MEMORY;
float gravity CCM_MEMORY;
uint8_t unlocks[8][32] CCM_MEMORY;
int unlocks_index CCM_MEMORY;
int unlocks_command_index CCM_MEMORY;
static int unlock_work[32] CCM_MEMORY;
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

    memset(unlock_work, 0, sizeof(unlock_work));
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
    uint8_t cmd = unlocks[unlocks_index][unlocks_command_index++];
    uint8_t param = cmd >> 4;
    switch (cmd&15)
    {
        case UNLOCKS_BREAK:
            if (param == 0)
            {
                if (unlocks_index == UNLOCKS_WIN)
                    return run_stop(1);

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
            break;
        case UNLOCKS_DELTA_SPAWN_Y:
            break;
        case UNLOCKS_SPAWN_TILE:
            break;
        case UNLOCKS_SPAWN_SPRITE:
            break;
        case UNLOCKS_SPRITE_VELOCITY:
            break;
        case UNLOCKS_NOISE:
            break;
        case UNLOCKS_SHAKE:
            break;
        case UNLOCKS_RANDOMIZE:
            break;
        case UNLOCKS_REPEAT:
            break;
        case UNLOCKS_GROUP:
            break;
    }
}
