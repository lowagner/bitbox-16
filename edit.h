#ifndef EDIT_H
#define EDIT_H

#include <stdint.h>

extern uint8_t edit_color[2];
extern uint8_t edit_tile, edit_sprite;
extern uint8_t edit_sprite_not_tile;

void edit_init();
void edit_line();
void edit_controls();

#endif
