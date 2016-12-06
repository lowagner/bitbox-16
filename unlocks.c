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
    camera_index = 255;
    strcpy((char *)game_message, "you died...");
}

void unlocks_win(int i)
{
    if (i != camera_index)
        return;
    camera_index = 255;
    strcpy((char *)game_message, "you won!");
}

void set_unlock(int i, int j)
{
    if (i != camera_index)
        return;
    message("unlock work to do %d\n", j);
}
