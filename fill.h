#ifndef FILL_H
#define FILL_H

#include <stdint.h>

void fill_init(uint8_t *memory, int width, int height, int old_c, int x, int y, int new_c);

int fill_can_start();
void fill_stop();

void fill_frame();

#endif
