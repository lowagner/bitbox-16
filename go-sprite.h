#ifndef GO_H
#define GO_H

#include <stdint.h>

void go_init();
void go_reset();
void go_controls();
void go_line();

#define GO_BREAK 0
#define GO_NOT_MOVE 1
#define GO_NOT_RUN 2
#define GO_NOT_AIR 3
#define GO_NOT_FIRE 4
#define GO_EXECUTE 5
#define GO_SET_PROPERTY 6
#define GO_DIRECTION 7
#define GO_SPECIAL_INPUT 8
#define GO_SPAWN_TILE 9
#define GO_SPAWN_SPRITE 10
#define GO_ACCELERATION 11
#define GO_SPEED 12
#define GO_NOISE 13
#define GO_RANDOMIZE 14
#define GO_QUAKE 15

#endif
