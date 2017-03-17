#ifndef RUN_H
#define RUN_H

extern int camera_shake;
extern int camera_index;
extern int player_index[2];
extern int players_swapped;

void set_camera_from_player_position();

void run_init();
void run_reset();
void run_switch();
void run_line();
void run_controls();

#endif
