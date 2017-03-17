#ifndef RUN_H
#define RUN_H

extern int camera_shake;
extern int camera_index;
extern int player_index[2];

void run_init();
void run_reset();
void run_switch();
void run_line();
void run_controls();

#endif
