#include "common.h"
#include "bitbox.h"
#include <string.h> // memcpy

uint8_t tile_draw[16][16][8] CCM_MEMORY;
uint8_t tile_map[TILE_MAP_MEMORY] CCM_MEMORY;
uint8_t tile_translator[16] CCM_MEMORY;
/*
info about a tile:
    4 bits for solid/water/air
        solid has the fourth bit set, first three currently unused
        air is zero, water with varying densities up to 7 (liquid mercury)
    4 bits for tile translation
    4 bits for tile timing
    if Block:
        4 bits:
            3 bits for tile vulnerability (can only destroy a tile when (attack & vulnerability) > 0).
        4 bits for surface property, x 4 sides 
            see common.h
            passable, normal, slippery, sticky/bouncy, insta-death, checkpoint, win location
    elif 1 bit for warp (shifted over one, next bit comes before this):
        if 1 bit for load (a different map for the next/previous level location):
            lookup map level-table based on 14 remaining bits:
                change the number at the end of the filename, 
                encode the number only, and how many chars to use for the number.
            encode number length as reverse exp-like:
                0001[10 bits] -> 3 digits, up to 999
                ... (errors if other values of 0's and 1's)
                0000001[7 bits] -> 2 digits, up to 99
                ... (errors if other values of 0's and 1's)
                0000000001[4 bits] -> 1 digit, up to 9
                ...
                00000000000000 -> no digits, take off numbers at end of filename
        else:
            warp to point on map given by 14 bits
        4 bits for directions to press to warp (player_controls & direction to warp)
    else: # water/air tile
        1 bit for damage
        # 18 bit remain
        6 bits for direction (measured like math, 0 = Right, increasing angles CCW)
        6 bits for strength
        6 bits for randomness
*/
uint32_t tile_info[16] CCM_MEMORY;
int tile_map_x CCM_MEMORY, tile_map_y CCM_MEMORY;
int tile_map_width CCM_MEMORY, tile_map_height CCM_MEMORY;
// tile_map_width * tile_map_height <= TILE_MAP_MEMORY
// tile_map_x < tile_map_width - 320
// tile_map_y < tile_map_height - 240

void tiles_init()
{
    // init tile mapping
    for (int j=0; j<16; ++j)
        tile_translator[j] = j; 
}

int tile_is_block(uint8_t t)
{
    // run through the translator
    t = tile_translator[t];
    return (tile_info[t] & 8);
}

int tile_xy_is_block(int16_t x, int16_t y)
{
    int index = y*tile_map_width + x;
    uint8_t t = tile_map[index/2];
    if (index % 2)
        t >>= 4;
    else
        t &= 15;
    // run through the translator
    t = tile_translator[t];
    return (tile_info[t]&8) && ((tile_info[t]>>16)&15) && ((tile_info[t]>>20)&15) && ((tile_info[t]>>24)&15) && (tile_info[t]>>28);
}

uint32_t pack_block_info(uint8_t translation, uint8_t timing, uint8_t vulnerability, const SideType *sides)
{
    return (8)|((translation&15)<<4)|((timing&15)<<8)|((vulnerability&15)<<12)|
        (sides[0]<<16)|(sides[1]<<20)|(sides[2]<<24)|(sides[3]<<28);
}

uint32_t pack_fluid_info(uint8_t translation, uint8_t timing, uint8_t density, uint8_t damage,
    uint8_t direction_1, uint8_t strength_1, uint8_t direction_2, uint8_t strength_2)
{
    // skip bit 12, that specifies WARP.
    return ((density&7))|((translation&15)<<4)|((timing&15)<<8)|((damage&1)<<12)|
        ((direction_1&3)<<14)|((strength_1&127)<<16)|((direction_2&3)<<23)|((strength_2&127)<<25);
}

uint32_t pack_warp_info(uint8_t translation, uint8_t timing, uint8_t density, 
    uint8_t load_one_plus_digits, uint8_t warp_directions, uint16_t value)
{
    switch (load_one_plus_digits)
    {
        case 0:
            // warp within the level, not load.  skip bit 12, that indicates LOAD
            return ((density&7))|((translation&15)<<4)|((timing&15)<<8)|((1)<<13)|
                ((value&16383)<<14)|(warp_directions<<28);
        case 1:
            // load, with no numbers
            return ((density&7))|((translation&15)<<4)|((timing&15)<<8)|((1<<12)|(1<<13))|
                (warp_directions<<28);
        case 2:
            // load, with one number
            return ((density&7))|((translation&15)<<4)|((timing&15)<<8)|((1<<12)|(1<<13)|(1<<23))|
                ((value%10)<<24)|(warp_directions<<28);
        case 3:
            // load, with two numbers
            return ((density&7))|((translation&15)<<4)|((timing&15)<<8)|((1<<12)|(1<<13)|(1<<20))|
                ((value%100)<<21)|(warp_directions<<28);
        case 4:
            // load, with three numbers
            return ((density&7))|((translation&15)<<4)|((timing&15)<<8)|((1<<12)|(1<<13)|(1<<17))|
                ((value%1000)<<18)|(warp_directions<<28);
        default:
            message("unexpected value for warp: %d\n", (int)(load_one_plus_digits));
            return 0;
    }
}

void tiles_line()
{
    static const void *even_location[16] = {
        NULL,
        &&even_tile_1,
        &&even_tile_2,
        &&even_tile_3,
        &&even_tile_4,
        &&even_tile_5,
        &&even_tile_6,
        &&even_tile_7,
        &&even_tile_8,
        &&even_tile_9,
        &&even_tile_10,
        &&even_tile_11,
        &&even_tile_12,
        &&even_tile_13,
        &&even_tile_14,
        &&even_tile_15
    };
    static const void *odd_location[16] = {
        NULL,
        &&odd_tile_1,
        &&odd_tile_2,
        &&odd_tile_3,
        &&odd_tile_4,
        &&odd_tile_5,
        &&odd_tile_6,
        &&odd_tile_7,
        &&odd_tile_8,
        &&odd_tile_9,
        &&odd_tile_10,
        &&odd_tile_11,
        &&odd_tile_12,
        &&odd_tile_13,
        &&odd_tile_14,
        &&odd_tile_15
    };
    int count = 10;
    uint8_t *U8 = U8row + 15;
    int tile_j = tile_map_y + vga_line;
    int index = (tile_j/16)*(tile_map_width) + tile_map_x / 16;
    int remainder = tile_map_x % 16;
    tile_j %= 16;
    int two_tile;
    int trans;
    uint8_t *draw;
    int two_color;
    uint8_t *two_p = &tile_map[index/2] - 1;
    if (index % 2)
    {
        two_tile = *++two_p;
        if (remainder)
        {
            count = 11;
            trans = tile_translator[two_tile>>4];
            draw = &tile_draw[trans][tile_j][remainder/2];
            two_color = *draw;
            goto *even_location[remainder];
        }
        for (; count>0; --count)
        {
            trans = tile_translator[two_tile>>4];
            draw = &tile_draw[trans][tile_j][0];
            two_color = *draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
           
            two_tile = *++two_p;
            trans = tile_translator[two_tile&15];
            draw = &tile_draw[trans][tile_j][0];
            two_color = *draw;
            *++U8 = two_color&15;
            odd_tile_1:
            *++U8 = two_color>>4;
            two_color = *++draw;
            odd_tile_2:
            *++U8 = two_color&15;
            odd_tile_3:
            *++U8 = two_color>>4;
            two_color = *++draw;
            odd_tile_4:
            *++U8 = two_color&15;
            odd_tile_5:
            *++U8 = two_color>>4;
            two_color = *++draw;
            odd_tile_6:
            *++U8 = two_color&15;
            odd_tile_7:
            *++U8 = two_color>>4;
            two_color = *++draw;
            odd_tile_8:
            *++U8 = two_color&15;
            odd_tile_9:
            *++U8 = two_color>>4;
            two_color = *++draw;
            odd_tile_10:
            *++U8 = two_color&15;
            odd_tile_11:
            *++U8 = two_color>>4;
            two_color = *++draw;
            odd_tile_12:
            *++U8 = two_color&15;
            odd_tile_13:
            *++U8 = two_color>>4;
            two_color = *++draw;
            odd_tile_14:
            *++U8 = two_color&15;
            odd_tile_15:
            *++U8 = two_color>>4;
        }
    }
    else
    {
        if (remainder)
        {
            count = 11;
            two_tile = *++two_p;
            trans = tile_translator[two_tile&15];
            draw = &tile_draw[trans][tile_j][remainder/2];
            two_color = *draw;
            goto *odd_location[remainder];
        }
        for (; count>0; --count)
        {
            two_tile = *++two_p;
            trans = tile_translator[two_tile&15];
            draw = &tile_draw[trans][tile_j][0];
            two_color = *draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
            two_color = *++draw;
            *++U8 = two_color&15;
            *++U8 = two_color>>4;
           
            trans = tile_translator[two_tile>>4];
            draw = &tile_draw[trans][tile_j][0];
            two_color = *draw;
            *++U8 = two_color&15;
            even_tile_1:
            *++U8 = two_color>>4;
            two_color = *++draw;
            even_tile_2:
            *++U8 = two_color&15;
            even_tile_3:
            *++U8 = two_color>>4;
            two_color = *++draw;
            even_tile_4:
            *++U8 = two_color&15;
            even_tile_5:
            *++U8 = two_color>>4;
            two_color = *++draw;
            even_tile_6:
            *++U8 = two_color&15;
            even_tile_7:
            *++U8 = two_color>>4;
            two_color = *++draw;
            even_tile_8:
            *++U8 = two_color&15;
            even_tile_9:
            *++U8 = two_color>>4;
            two_color = *++draw;
            even_tile_10:
            *++U8 = two_color&15;
            even_tile_11:
            *++U8 = two_color>>4;
            two_color = *++draw;
            even_tile_12:
            *++U8 = two_color&15;
            even_tile_13:
            *++U8 = two_color>>4;
            two_color = *++draw;
            even_tile_14:
            *++U8 = two_color&15;
            even_tile_15:
            *++U8 = two_color>>4;
        }
    }
}

void tiles_load_default()
{
    for (int i=0; i<16; ++i)
        memset(tile_draw[i][0], i*17, 128);
    
    tile_info[0] = 0; // pack_fluid_info(0, 0, 0, 0, 0, 0, 0, 0);
    SideType sides[4] = { Normal, Normal, Normal, Normal };
    for (int i=1; i<16; ++i)
        tile_info[i] = pack_block_info(i, 0, 0, sides);
}

void tiles_translate()
{
    if (vga_frame%32 == 0)
    {
        int frame = vga_frame/32;
        for (int i=0; i<16; ++i)
        {
            int timing = tile_info[i]>>4;
            int translation = timing&15;
            timing = (timing>>4)&15;
            if (!timing)
                timing = 16;
            if (frame % timing)
                continue;
            if ((frame/timing)%2)
                tile_translator[i] = translation;
            else
                tile_translator[i] = i;
        }
    }
}
