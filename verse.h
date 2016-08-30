#ifndef VERSE_H
#define VERSE_H

#include <stdint.h>

extern uint8_t verse_menu_not_edit;
extern uint8_t verse_track;
extern uint8_t verse_player;
void render_command(uint8_t value, int x, int y);

void verse_init();
void verse_reset();
void verse_controls();
void verse_line();

#endif
