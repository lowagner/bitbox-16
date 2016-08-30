#include "common.h"
#include "bitbox.h"

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
            for tile vulnerability (can only destroy a tile using an attack which &'s this).
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
int16_t tile_map_x CCM_MEMORY, tile_map_y CCM_MEMORY;
uint16_t tile_map_width CCM_MEMORY, tile_map_height CCM_MEMORY;
// tile_map_width * tile_map_height <= TILE_MAP_MEMORY
// tile_map_x < tile_map_width - 320
// tile_map_y < tile_map_height - 240

void tiles_init()
{
    // init tile mapping
    for (int j=0; j<16; ++j)
        tile_translator[j] = j; 
}

uint32_t pack_tile_info(uint8_t translation, uint8_t timing, uint8_t vulnerability, const SideType *sides)
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
    if (tile_map_x % 16)
    {
        uint16_t *dst = draw_buffer;
        int tile_j = tile_map_y + vga_line;
        int index = (tile_j/16)*(tile_map_width) + tile_map_x / 16;
        tile_j %= 16;
        uint8_t *tile = &tile_map[index/2];
        if (index % 2)
        {
            // draw the first tile (it's somewhat off-screen)
            uint8_t trans = tile_translator[((*tile)>>4)]; // first one is odd
                
            uint8_t *tile_color = &tile_draw[trans][tile_j][(tile_map_x%16)/2] - 1;
            if (tile_map_x % 2)
            {
                ++tile_color;
                for (int l=(tile_map_x%16)/2; l<7; ++l)
                {
                    *dst++ = palette[(*tile_color)>>4];
                    *dst++ = palette[(*(++tile_color))&15];
                }
                *dst++ = palette[(*tile_color)>>4];
            }
            else
            {
                for (int l=(tile_map_x%16)/2; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }

            // draw 19 un-broken tiles (tile 2 to tile 21)
            for (int k=0; k<19/2; ++k)
            {
                // translate the tile into what tile it should be drawn as:
                // first one is even (not odd)
                trans = tile_translator[(*(++tile))&15];
                tile_color = &tile_draw[trans][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }

                // second one is odd
                trans = tile_translator[((*tile)>>4)];
                tile_color = &tile_draw[trans][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }
            // 21st tile is even, but still unbroken
            trans = tile_translator[(*(++tile))&15];
            tile_color = &tile_draw[trans][tile_j][0] - 1;
            for (int l=0; l<8; ++l)
            {
                *dst++ = palette[(*(++tile_color))&15];
                *dst++ = palette[(*tile_color)>>4];
            }

            // draw 22'nd broken tile is odd
            trans = tile_translator[((*tile)>>4)&15];
            tile_color = &tile_draw[trans][tile_j][0]-1; 
            if (tile_map_x % 2)
            {
                *dst++ = palette[(*(++tile_color))&15];
                for (int l=0; l<(tile_map_x%16)/2; ++l)
                {
                    *dst++ = palette[(*tile_color)>>4];
                    *dst++ = palette[(*(++tile_color))&15];
                }
            }
            else
            {
                for (int l=0; l<(tile_map_x%16)/2; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }
        }
        else 
        {
            // draw the first tile (it's somewhat off-screen)
            uint8_t trans = tile_translator[(*tile)&15]; // first one is even
            uint8_t *tile_color = &tile_draw[trans][tile_j][(tile_map_x%16)/2] - 1;
            if (tile_map_x % 2)
            {
                ++tile_color;
                for (int l=(tile_map_x%16)/2; l<7; ++l)
                {
                    *dst++ = palette[(*tile_color)>>4];
                    *dst++ = palette[(*(++tile_color))&15];
                }
                *dst++ = palette[(*tile_color)>>4];
            }
            else
            {
                for (int l=(tile_map_x%16)/2; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }

            // draw 19 un-broken tiles (tile 2 to tile 21)
            for (int k=0; k<19/2; ++k)
            {
                // translate the tile into what tile it should be drawn as:
                // first one is odd
                trans = tile_translator[((*tile)>>4)];
                tile_color = &tile_draw[trans][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
                
                // second one is even
                trans = tile_translator[(*(++tile))&15];
                tile_color = &tile_draw[trans][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }
            // 21st tile is odd, but still unbroken
            trans = tile_translator[((*tile)>>4)];
            tile_color = &tile_draw[trans][tile_j][0] - 1;
            for (int l=0; l<8; ++l)
            {
                *dst++ = palette[(*(++tile_color))&15];
                *dst++ = palette[(*tile_color)>>4];
            }

            // draw 22'nd broken tile is even
            trans = tile_translator[(*(++tile))&15];
            tile_color = &tile_draw[trans][tile_j][0]-1; 
            if (tile_map_x % 2)
            {
                *dst++ = palette[(*(++tile_color))&15];
                for (int l=0; l<(tile_map_x%16)/2; ++l)
                {
                    *dst++ = palette[(*tile_color)>>4];
                    *dst++ = palette[(*(++tile_color))&15];
                }
            }
            else
            {
                for (int l=0; l<(tile_map_x%16)/2; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }
        }
    }
    else // tile_map_x puts a tile exactly on the edge of the screen
    {
        uint16_t *dst = draw_buffer;
        int tile_j = tile_map_y + vga_line;
        int index = (tile_j/16)*(tile_map_width) + tile_map_x / 16;
        tile_j %= 16;
        uint8_t *tile = &tile_map[index/2]-1;
        if (index % 2)
        {
            ++tile;
            for (int k=0; k<10; ++k)
            {
                // translate the tile into what tile it should be drawn as:
                uint8_t trans = tile_translator[((*tile)>>4)];

                uint8_t *tile_color = &tile_draw[trans][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
                    
                trans = tile_translator[(*(++tile))&15];
                tile_color = &tile_draw[trans][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }
        }
        else // not odd
        {
            for (int k=0; k<10; ++k)
            {
                // translate the tile into what tile it should be drawn as:
                uint8_t trans = tile_translator[(*(++tile))&15];

                uint8_t *tile_color = &tile_draw[trans][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
                
                // translate the tile into what tile it should be drawn as:
                trans = tile_translator[((*tile)>>4)];

                tile_color = &tile_draw[trans][tile_j][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }

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
    
    tile_info[0] = 0; // pack_fluid_info(0, 0, 0, 0, 0, 0, 0, 0);
    SideType sides[4] = { Normal, Normal, Normal, Normal };
    for (int i=1; i<16; ++i)
        tile_info[i] = pack_tile_info(i, 0, 0, sides);
}
