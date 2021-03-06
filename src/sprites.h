#ifndef SPRITES_H
#define SPRITES_H

#include "common.h"

// break objects up into 16x16 sprites with 8 possible frames (down/right/left/up * 2 for animation):
extern uint8_t sprite_draw[128][16][8]; 
extern uint32_t sprite_info[128];  
extern uint8_t sprite_pattern[16][32]; 

struct object {
    // first 32 bits:
    int16_t iy, ix;
    // second 32 bits:
    uint8_t next_object_index; 
    uint8_t previous_object_index;
    uint8_t draw_order_index;
    uint8_t z; // z = 0 is a hidden object.
    // third and fourth 32 bits:
    float y, x;
    // fifth and sixth 
    float vy, vx;
    // seventh 
    uint8_t acceleration;
    uint8_t jump_speed;
    uint8_t sprite_index;
    uint8_t speed;
    // eighth
    uint8_t cmd_index;
    uint8_t wait;
    uint8_t properties; // use bit-masking for RUNNING, IN_AIR, GHOSTING, PROJECTILE
    uint8_t blocked; // on various sides
    // ninth
    uint8_t blink; // invincibility
    uint8_t health;
    uint8_t firing;
    uint8_t edge_behavior;
};

extern uint8_t draw_order[MAX_OBJECTS];

extern struct object object[MAX_OBJECTS];
extern uint8_t first_free_object_index;
extern uint8_t first_used_object_index;
// keep the object list sorted in increasing y, 
// and break ties by bringing to the front lower z:
extern uint8_t drawing_count; // keep track of how many objects you can see
// since every sprite is the same size, 
// we just need to keep a first and last active index
extern uint8_t first_drawing_index; 
extern uint8_t last_drawing_index; 

uint8_t create_object(uint8_t sprite_index, int16_t x, int16_t y, uint8_t z);
void move_object(int i, int16_t x, int16_t y);
void make_unseen_object_viewable(int i);
void remove_object(int index);
void remove_object_from_view(int i);
int on_screen(int16_t x, int16_t y);
void update_objects();

void sprites_init();
void sprites_line();
void sprites_frame();
void sprites_load_default();

void update_object_images();

static inline int get_sprite_vulnerability(struct object *o, int side)
{
    int info = sprite_info[o->sprite_index];
    if ((info>>(12+side))&1) 
        return 0; // invulnerability on this side
    return ~((info>>8)&7);
}

static inline int get_sprite_property(struct object *o, int side)
{
    int info = sprite_info[o->sprite_index];
    return (info>>(16+4*side))&15;
}

static inline int get_sprite_attack(struct object *o)
{
    int info = sprite_info[o->sprite_index];
    return (info>>5)&7;
}

#endif
