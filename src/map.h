#ifndef MAP_H
#define MAP_H

#include <stdint.h>

void map_start();
void map_init();
void map_load_default();
void map_line();
void map_controls();

extern uint8_t map_modified;

void map_set_tile(int x, int y, uint8_t tile);
uint8_t map_get_tile(int x, int y);

#endif
