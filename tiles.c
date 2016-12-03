#include "common.h"
#include "bitbox.h"
#include <string.h> // memcpy

uint8_t tile_draw[16][16][8] CCM_MEMORY;
uint8_t tile_map[TILE_MAP_MEMORY] CCM_MEMORY;
uint8_t tile_translator[16] CCM_MEMORY;
struct tile_info tile_info[16] CCM_MEMORY;
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
    return tile_info[t].material == 15;
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

void tiles_reset()
{
    // tile 0
    uint8_t *tc = tile_draw[0][0];
    for (int j=0; j<16; ++j)
    for (int i=0; i<8; ++i)
    {
        *tc++ = SKYBLUE|(SKYBLUE<<4);
    }
    // next tiles are mostly solid colors
    for (int k=0; k<9; ++k)
    for (int l=0; l<128; ++l)
    {
        *tc++ = (2+k)|((3+k)<<4);
    }
    // next tile
    for (int j=0; j<16; ++j)
    for (int i=0; i<8; ++i)
    {
        if (((j/8)%2 + i/4) % 2)
            *tc++ = BLAZE|(BLAZE<<4);
        else
            *tc++ = DULLBROWN|(DULLBROWN<<4);
    }
    // next tile
    for (int j=0; j<16; ++j)
    for (int i=0; i<8; ++i)
    {
        if (((j)%2 + i) % 2)
            *tc++ = CLOUDBLUE|(SEABLUE<<4);
        else
            *tc++ = BLUEGREEN|(BLUEGREEN<<4);
    }
    // next tile
    for (int j=0; j<16; ++j)
    for (int i=0; i<8; ++i)
    {
        if (((j/2)%2 + i) % 2)
            *tc++ = BROWN|(BROWN<<4);
        else
            *tc++ = DULLBROWN|(DULLBROWN<<4);
    }
    // etc.
    for (int j=0; j<16; ++j)
    for (int i=0; i<8; ++i)
    {
        if (((j/4)%2 + i/2) % 2)
            *tc++ = RED|(RED<<4);
        else
            *tc++ = GINGER|(GINGER<<4);
    }
    // next tile
    for (int j=0; j<16; ++j)
    for (int i=0; i<8; ++i)
    {
        if (((j/8)%2 + i/4) % 2)
            *tc++ = GREEN|(GREEN<<4);
        else
            *tc++ = INDIGO|(INDIGO<<4);
    }
    // splitting a tile up
    for (int i=0; i<8; ++i)
    {
        if (((0/8)%2 + i/4) % 2)
            *tc++ = CLOUDBLUE|(CLOUDBLUE<<4);
        else
            *tc++ = SLIME|(SLIME<<4);
    }
    for (int j=1; j<15; ++j)
    {
        if (((j/8)%2 + 0/8) % 2)
            *tc++ = CLOUDBLUE|(SKYBLUE<<4);
        else
            *tc++ = SLIME|(GREEN<<4);
        for (int i=1; i<7; ++i)
        {
            if (((j/8)%2 + i/4) % 2)
                *tc++ = SKYBLUE|(SKYBLUE<<4);
            else
                *tc++ = GREEN|(GREEN<<4);
        }
        if (((j/8)%2 + 15/8) % 2)
            *tc++ = SKYBLUE|(SEABLUE<<4);
        else
            *tc++ = GREEN|(BLUEGREEN<<4);
    }
    for (int i=0; i<8; ++i)
    {
        if (((15/8)%2 + i/4) % 2)
            *tc++ = SEABLUE|(SEABLUE<<4);
        else
            *tc++ = BLUEGREEN|(BLUEGREEN<<4);
    }
    
    tile_info[0].material = 0; // water
    tile_info[0].translation = 0;
    tile_info[0].timing = 0;
    tile_info[0].vuln_warp_jump = 0;
    tile_info[0].value = 0;
    for (int i=1; i<16; ++i)
    {
        tile_info[i].material = 8; // solid
        tile_info[i].translation = i;
        tile_info[i].timing = 0;
        tile_info[i].vuln_warp_jump = 0;
        tile_info[i].side[0] = Normal;
        tile_info[i].side[1] = Normal;
        tile_info[i].side[2] = Normal;
        tile_info[i].side[3] = Normal;
    }
}
