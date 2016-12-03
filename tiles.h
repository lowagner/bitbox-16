#ifndef TILES_H
#define TILES_H

#include "common.h"

extern uint8_t tile_draw[16][16][8];
extern uint8_t tile_map[TILE_MAP_MEMORY];
struct tile_info
{
    uint8_t material; // 0-f; air-water if < 8, solid if >= 8.  == 15 if a solid block
    uint8_t translation;
    uint8_t timing;
    uint8_t vuln_warp_jump; // vulnerability, or for fluid:  warp=1,2,3, jump=4
    union
    {
        uint32_t value;
        uint8_t side[4];
        struct
        {
            int8_t vy, vx;
            uint8_t random_strength;
            uint8_t damage_angle;
        };
        struct
        {
            uint8_t press_direction;
            uint8_t jy;
            union
            {
                uint16_t jx;
                uint16_t warp;
            };
        };
    };
};
extern struct tile_info tile_info[16]; 
extern uint8_t tile_translator[16];
extern int tile_map_x, tile_map_y;
extern int tile_map_width, tile_map_height;

void tiles_init();
void tiles_line();
void tiles_reset();

int tile_is_block(uint8_t t);
int tile_xy_is_block(int16_t x, int16_t y);

#endif
