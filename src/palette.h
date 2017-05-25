#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

#define BLACK 0
#define GRAY 1
#define WHITE 2
#define PINK 3
#define RED 4
#define BLAZE 5
#define BROWN 6
#define DULLBROWN 7
#define GINGER 8
#define SLIME 9
#define GREEN 10
#define BLUEGREEN 11
#define CLOUDBLUE 12
#define SKYBLUE 13
#define SEABLUE 14
#define INDIGO 15

extern uint16_t palette[16]; 
extern uint32_t palette2[512];

void palette_init();
void palette_load_default();
void palette_controls();
void palette_line();
void update_palette2();

#endif
