#ifndef MAP_H
#define MAP_H

#include <stdint.h>

void map_switch();
void map_init();
void map_reset();
void map_line();
void map_controls();

extern uint8_t map_modified;

#endif
