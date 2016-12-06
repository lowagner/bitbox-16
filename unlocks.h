#ifndef UNLOCKS_H
#define UNLOCKS_H

void unlocks_init();
void set_unlock(int i, int j);
void unlocks_win(int i);
void kill_player();

extern float damage_multiplier;
extern int game_type;


#endif
