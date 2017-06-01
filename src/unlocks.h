#ifndef UNLOCKS_H
#define UNLOCKS_H
#include <stdint.h>

extern uint8_t unlocks[8][32];

void unlocks_init();
void unlocks_load_default();

void unlocks_start();
void unlocks_start_run();
void unlocks_switch(int p, int next_unlock_index);
#define UNLOCKS_0 0 
#define UNLOCKS_1 1
#define UNLOCKS_2 2
#define UNLOCKS_3 3
#define UNLOCKS_INIT 4
#define UNLOCKS_LOOP 5
#define UNLOCKS_WIN 6
#define UNLOCKS_LOSE 7
void unlocks_run();
void kill_player(int p);

extern float damage_multiplier;

#define UNLOCKS_BREAK 0
#define UNLOCKS_GRAVITY 1
#define UNLOCKS_MULTIPLIER 2
#define UNLOCKS_HURT_HEAL 3
#define UNLOCKS_SPAWN_NEAR_0 4
#define UNLOCKS_SPAWN_NEAR_1 5
#define UNLOCKS_DELTA_SPAWN_X 6
#define UNLOCKS_DELTA_SPAWN_Y 7
#define UNLOCKS_SPAWN_TILE 8
#define UNLOCKS_SPAWN_SPRITE 9
#define UNLOCKS_SPRITE_VELOCITY 10
#define UNLOCKS_NOISE 11
#define UNLOCKS_QUAKE 12
#define UNLOCKS_RANDOMIZE 13
#define UNLOCKS_REPEAT 14
#define UNLOCKS_GROUP 15


void unlocks_line();
void unlocks_controls();
#endif
