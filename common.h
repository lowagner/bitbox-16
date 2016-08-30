#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#define TILE_MAP_MEMORY 8192
#define MAX_OBJECTS 64 // no more than 255
#define SCREEN_W 320
#define SCREEN_H 240

typedef enum {
    Passable=0,
    Normal=1,
    Slippery=2,
    SuperSlippery=3,
    Sticky=4,
    SuperSticky=5,
    Bouncy=6,
    SuperBouncy=7,
    Damaging=8,
    SuperDamaging=9,
    Unlock0=10,
    Unlock1=11,
    Unlock2=12,
    Unlock3=13,
    Checkpoint=14,
    Win=15,
} SideType; // no more than 8 total types

#include "tiles.h"
#include "sprites.h"
#include "palette.h"
#include "map.h"

#define GAMEPAD_PRESS_WAIT 8

typedef enum {
    None=0,
    GameOn,
    EditMap,
    EditTileOrSprite,
    EditTileOrSpriteProperties,
    EditPalette,
    EditAnthem,
    EditVerse,
    EditInstrument,
    SaveLoadScreen,
    ChooseFilename
} VisualMode;

extern VisualMode visual_mode;
extern VisualMode previous_visual_mode;

#define RIGHT 0
#define UP 1
#define LEFT 2
#define DOWN 3

#define GAMEPAD_PRESS(id, key) ((gamepad_buttons[id]) & (~old_gamepad[id]) & (gamepad_##key))
#define GAMEPAD_PRESSING(id, key) ((gamepad_buttons[id]) & (gamepad_##key) & (~old_gamepad[id] | ((gamepad_press_wait == 0)*gamepad_##key)))
extern uint16_t old_gamepad[2];
extern uint8_t gamepad_press_wait;

extern uint8_t game_message[32];
extern const uint8_t hex[64]; // not exactly hex but ok!
extern const uint8_t direction[4];

void draw_parade(int line, uint8_t bg_color);
void game_switch(VisualMode new_visual_mode);

uint8_t randomize(uint8_t arg);
#endif
