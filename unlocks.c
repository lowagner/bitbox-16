#include "bitbox.h"
#include "common.h"
#include "run.h"
#include <string.h>

float damage_multiplier CCM_MEMORY;
float gravity CCM_MEMORY;

void unlocks_init()
{
    damage_multiplier = 1.0f;
    gravity = 1.0f;
}

void kill_player()
{
    player_index[0] = 255;
    strcpy((char *)game_message, "you died...");
}

void unlocks_win(int i)
{
    if (i != player_index[0])
        return;
    player_index[0] = 255;
    strcpy((char *)game_message, "you won!");
}

void set_unlock(int i, int j)
{
    if (i != player_index[0])
        return;
    message("unlock work to do %d\n", j);
}
