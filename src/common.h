#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#include <string.h>
#define TILE_MAP_MEMORY 8192
#define MAX_OBJECTS 64 // no more than 255
#define SCREEN_W 320
#define SCREEN_H 240

extern uint8_t U8row[SCREEN_W+32];

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
    Warp1=16, // don't try to pack this in yourself, used in hit testing
    Warp2=17, // same here, and for the rest of the enum..
    Warp3=18, 
    Jump=19
} SideType; // no more than 16 total types

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
    EditSpritePattern,
    EditPalette,
    EditAnthem,
    EditVerse,
    EditInstrument,
    SaveLoadScreen,
    ChooseFilename,
    EditUnlocks
} VisualMode;

extern VisualMode visual_mode;
extern VisualMode previous_visual_mode;

// some sprite definitions
#define RIGHT 0
#define UP 1
#define LEFT 2
#define DOWN 3

#define BLOCKED_RIGHT (1<<RIGHT)
#define BLOCKED_UP (1<<UP)
#define BLOCKED_LEFT (1<<LEFT)
#define BLOCKED_DOWN (1<<DOWN)

// the next are bit-masks for sprite/object properties
#define RUNNING 1
#define GHOSTING 2
#define PROJECTILE 4
#define CAN_JUMP 8
#define SLIPPING 16
#define SUPER_SLIPPING 32
#define STICKING 64
#define SUPER_STICKING 128

#define GAMEPAD_PRESS(id, key) ((gamepad_buttons[id]) & (~old_gamepad[id]) & (gamepad_##key))
#define GAMEPAD_PRESSING(id, key) ((gamepad_buttons[id]) & (gamepad_##key) & (~old_gamepad[id] | ((gamepad_press_wait == 0)*gamepad_##key)))
extern uint16_t new_gamepad[2];
extern uint16_t old_gamepad[2];
extern uint8_t gamepad_press_wait;

extern uint8_t game_message[32];
extern int game_message_timeout;
extern const uint8_t hex[64]; // not exactly hex but ok!
extern const uint8_t direction[4];

void draw_parade(int line, uint8_t bg_color);
void game_switch(VisualMode new_visual_mode);
void game_switch_previous_or(VisualMode new_visual_mode);

extern float gravity;

uint8_t randomize(uint8_t arg);

inline void set_game_message_timeout(const char *msg, int timeout)
{
    strcpy((char *)game_message, msg);
    game_message_timeout = timeout;
}

#define MESSAGE_TIMEOUT (10*64)
#endif
