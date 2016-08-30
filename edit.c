#include "bitbox.h"
#include "common.h"
#include "edit.h"
#include "tiles.h"
#include "fill.h"
#include "font.h"
#include "io.h"
#include "sprites.h"
#include <string.h> // memset

uint8_t edit_tile CCM_MEMORY;
uint8_t edit_sprite CCM_MEMORY;
uint8_t edit_sprite_not_tile CCM_MEMORY;
uint8_t edit_last_painted CCM_MEMORY;

int edit_x CCM_MEMORY;
int edit_y CCM_MEMORY;
uint8_t edit_color[2] CCM_MEMORY;

const uint8_t hex[64] = { 
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', // standard hex
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', // up to 32
    'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', // up to 48
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 138, 255  // up to 64
};
const uint8_t direction[4] = { 
    [RIGHT] = 'R',
    [UP] = 'U',
    [LEFT] = 'L',
    [DOWN] = 'D'
};

void edit_init() 
{
    edit_color[0] = BLACK;
    edit_color[1] = WHITE;
}

void edit_line() 
{
    if (vga_line < 32)
    {
        if (vga_line < 4)
        {
            if (vga_line/2 == 0)
                for (int i=0; i<SCREEN_W; ++i)
                    draw_buffer[i] = palette[edit_color[0]];
        }
        else if (vga_line < 20)
        {
            if (edit_sprite_not_tile)
            {
                int tile_j = vga_line-4;
                uint32_t *dst = (uint32_t *)draw_buffer;
                
                uint8_t *tile_color;
               
                // draw a border, some of the edited sprite, plus all other tiles
                for (int step=0; step<2; ++step)
                {
                    tile_color = &sprite_draw[edit_sprite/8][edit_sprite%8][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
        
                // alternate edit tile and other tiles:
                for (int tile=0; tile<8; ++tile)
                {
                    tile_color = &tile_draw[tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                    tile_color = &sprite_draw[edit_sprite/8][edit_sprite%8][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
                
                for (int step=0; step<2; ++step)
                {
                    tile_color = &sprite_draw[edit_sprite/8][edit_sprite%8][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
            }
            else // editing a tile
            {
                int tile_j = vga_line-4;
                uint32_t *dst = (uint32_t *)draw_buffer;
                
                uint8_t *tile_color;
               
                // draw a border, some of the edited tile, plus all other tiles
                for (int step=0; step<2; ++step)
                {
                    tile_color = &tile_draw[edit_tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
        
                // alternate edit tile and other tiles:
                for (int tile=0; tile<8; ++tile)
                {
                    tile_color = &tile_draw[tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                    tile_color = &tile_draw[edit_tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
                
                for (int step=0; step<2; ++step)
                {
                    tile_color = &tile_draw[edit_tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
            }
        }
        else if (vga_line/2 == 10 || vga_line/2 == 15)
            memset(draw_buffer, 0, 2*SCREEN_W);
        else // goes from 22 to 30
        {
            int line = vga_line - 22;
            if (edit_sprite_not_tile)
            {
                uint8_t text[] = {  
                    's', 'p', 'r', 'i', 't', 'e', ' ',
                    hex[edit_sprite/8], '.', direction[(edit_sprite%8)/2], hex[(edit_sprite%8)%2],
                    ' ', 's', 't', 'a', 'r', 't', ':', 'm', 'e', 'n', 'u',
                    0 };
                font_render_line_doubled(text, 16, line, 65535, 0);
                uint8_t invisible = sprite_info[edit_sprite/8][edit_sprite%8]&31;
                if (invisible < 16)
                    font_render_line_doubled((uint8_t *)"invisible", 222, line, palette[invisible], ~palette[invisible]);
            }
            else if (previous_visual_mode)
            {
                uint8_t text[] = { 
                    't', 'i', 'l', 'e', ' ', hex[edit_tile], 
                    ' ', ' ', ' ', 's', 't', 'a', 'r', 't', ':', 'b', 'a', 'c', 'k',
                    0 };
                font_render_line_doubled(text, 16, line, 65535, 0);
            }
            else
            {
                uint8_t text[] = { 
                    't', 'i', 'l', 'e', ' ', hex[edit_tile], 
                    ' ', ' ', ' ', 's', 't', 'a', 'r', 't', ':', 'm', 'e', 'n', 'u',
                    0 };
                font_render_line_doubled(text, 16, line, 65535, 0);
            }
        }
        return;
    }
    else if (vga_line >= SCREEN_H - 32)
    {
        if (vga_line >= SCREEN_H - 4)
        {
            if (vga_line/2 == (SCREEN_H-4)/2)
            {
                for (int i=0; i<SCREEN_W; ++i)
                    draw_buffer[i] = palette[edit_color[1]];
            }
        }
        else if (vga_line >= SCREEN_H - 20)
        {
            if (edit_sprite_not_tile)
            {
                int tile_j = vga_line-(SCREEN_H - 20);
                uint32_t *dst = (uint32_t *)draw_buffer;
                
                uint8_t *tile_color;
                // draw a border, some of the edited sprite, plus all other tiles
                for (int step=0; step<2; ++step)
                {
                    tile_color = &sprite_draw[edit_sprite/8][edit_sprite%8][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
        
                // alternate edit tile and other tiles:
                for (int tile=8; tile<16; ++tile)
                {
                    tile_color = &sprite_draw[edit_sprite/8][edit_sprite%8][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                    tile_color = &tile_draw[tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
                
                for (int step=0; step<2; ++step)
                {
                    tile_color = &sprite_draw[edit_sprite/8][edit_sprite%8][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
            }
            else
            {
                int tile_j = vga_line-(SCREEN_H - 20);
                uint32_t *dst = (uint32_t *)draw_buffer;
                
                uint8_t *tile_color;
                // draw a border, some of the edited tile, plus all other tiles
                for (int step=0; step<2; ++step)
                {
                    tile_color = &tile_draw[edit_tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
        
                // alternate edit tile and other tiles:
                for (int tile=8; tile<16; ++tile)
                {
                    tile_color = &tile_draw[edit_tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                    tile_color = &tile_draw[tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
                
                for (int step=0; step<2; ++step)
                {
                    tile_color = &tile_draw[edit_tile][tile_j][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
            }
        }
        else if (vga_line/2 == (SCREEN_H-32)/2 || vga_line/2 == (SCREEN_H-22)/2)
            memset(draw_buffer, 0, 2*SCREEN_W);
        else // goes from SCREEN_H-30 to SCREEN_H-22
        {
            int line = vga_line - (SCREEN_H-30);
            if (game_message[0])
            {
                font_render_line_doubled((uint8_t *)game_message, 20, line, 65535, 0);
            }
            else if (fill_can_start())
            {
                font_render_line_doubled((const uint8_t *)"B/Y:paint A:fill X:copy color", 
                    20, line, 65535, 0);
            }
            else
            {
                font_render_line_doubled((const uint8_t *)"B/Y:paint A:stop X:copy color", 
                    20, line, 65535, 0);
            }
        }
        return;
    }

    if (edit_last_painted) // bottom color needs L/R cycles
    switch (vga_line/2)
    {
        case (SCREEN_H/2 - 40)/2:
        case (SCREEN_H/2 - 40 + 12)/2:
        {
            // draw a small color swatch of edit paint 0
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;
            uint32_t color = palette[edit_color[0]] | (palette[edit_color[0]]<<16);

            for (int k=0; k<8; ++k)
                *(++dst) = color;
            break;
        }
        case ((SCREEN_H/2 - 40 + 4)/2):
        case ((SCREEN_H/2 - 40 + 6)/2):
        case ((SCREEN_H/2 - 40 + 8)/2):
        case ((SCREEN_H/2 - 40 + 10)/2):
        {
            uint8_t button[] = { 'Y', 0 };
            font_render_line_doubled(button, 20, vga_line - (SCREEN_H/2 - 40 + 4), 
                ~palette[edit_color[0]],
                palette[edit_color[0]]);
            break;
        }
        case (SCREEN_H/2 + 16)/2:
        case (SCREEN_H/2 + 16 + 12)/2:
        {
            // draw a small color swatch of edit paint 1
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;
            uint32_t color = palette[edit_color[1]] | (palette[edit_color[1]]<<16);

            for (int k=0; k<8; ++k)
                *(++dst) = color;
            break;
        }
        case ((SCREEN_H/2 + 16 + 4)/2):
        case ((SCREEN_H/2 + 16 + 6)/2):
        case ((SCREEN_H/2 + 16 + 8)/2):
        case ((SCREEN_H/2 + 16 + 10)/2):
        {
            uint8_t button[] = { 'B', 0 };
            font_render_line_doubled(button, 20, vga_line - (SCREEN_H/2 + 16 + 4), 
                ~palette[edit_color[1]],
                palette[edit_color[1]]);
            break;
        }
        case (SCREEN_H/2 - 24)/2:
        case (SCREEN_H/2 + 32)/2:
        case ((SCREEN_H/2 + 16 - 4)/2):
        case ((SCREEN_H/2 + 16*2 + 4 + 16)/2):
        {
            // remove the swatch
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;

            for (int k=0; k<8; ++k)
                *(++dst) = 0;
            break;
        }
        case ((SCREEN_H/2 + 16 - 4 - 16)/2):
        case ((SCREEN_H/2 + 16 - 4 - 16 + 12)/2):
        {
            // draw a small color swatch for previous edit color, and kill text
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;
            uint32_t color = palette[(edit_color[1]-1)&15] | (palette[(edit_color[1]-1)&15]<<16);

            for (int k=0; k<8; ++k)
                *(++dst) = color;
            break;
        }
        case ((SCREEN_H/2 + 16 - 4 - 16 + 4)/2):
        case ((SCREEN_H/2 + 16 - 4 - 16 + 6)/2):
        case ((SCREEN_H/2 + 16 - 4 - 16 + 8)/2):
        case ((SCREEN_H/2 + 16 - 4 - 16 + 10)/2):
        {
            uint8_t button[] = { 'L', 0 };
            font_render_line_doubled(button, 20, vga_line - (SCREEN_H/2 + 16 - 4 - 16 + 4), 
                ~palette[(edit_color[1]-1)&15],
                palette[(edit_color[1]-1)&15]);
            break;
        }
        case ((SCREEN_H/2 + 16*2 + 4)/2):
        case ((SCREEN_H/2 + 16*2 + 4 + 12)/2):
        {
            // draw a small color swatch for next edit color, and kill text
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;
            uint32_t color = palette[(edit_color[1]+1)&15] | (palette[(edit_color[1]+1)&15]<<16);

            for (int k=0; k<8; ++k)
                *(++dst) = color;
            break;
        }
        case ((SCREEN_H/2 + 16*2 + 4 + 4)/2):
        case ((SCREEN_H/2 + 16*2 + 4 + 6)/2):
        case ((SCREEN_H/2 + 16*2 + 4 + 8)/2):
        case ((SCREEN_H/2 + 16*2 + 4 + 10)/2):
        {
            uint8_t button[] = { 'R', 0 };
            font_render_line_doubled(button, 20, vga_line - (SCREEN_H/2 + 16*2 + 4 + 4), 
                ~palette[(edit_color[1]+1)&15],
                palette[(edit_color[1]+1)&15]);
            break;
        }
    }
    else
    switch (vga_line/2) // top color needs L/R cycles
    {
        case (SCREEN_H/2 - 40)/2:
        case (SCREEN_H/2 - 40 + 12)/2:
        {
            // draw a small color swatch of edit paint 0
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;
            uint32_t color = palette[edit_color[0]] | (palette[edit_color[0]]<<16);

            for (int k=0; k<8; ++k)
                *(++dst) = color;
            break;
        }
        case ((SCREEN_H/2 - 40 + 4)/2):
        case ((SCREEN_H/2 - 40 + 6)/2):
        case ((SCREEN_H/2 - 40 + 8)/2):
        case ((SCREEN_H/2 - 40 + 10)/2):
        {
            uint8_t button[] = { 'Y', 0 };
            font_render_line_doubled(button, 20, vga_line - (SCREEN_H/2 - 40 + 4), 
                ~palette[edit_color[0]],
                palette[edit_color[0]]);
            break;
        }
        case (SCREEN_H/2 + 16)/2:
        case ((SCREEN_H/2 + 16 + 12)/2):
        {
            // draw a small color swatch of edit paint 1
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;
            uint32_t color = palette[edit_color[1]] | (palette[edit_color[1]]<<16);

            for (int k=0; k<8; ++k)
                *(++dst) = color;
            break;
        }
        case ((SCREEN_H/2 + 16 + 4)/2):
        case ((SCREEN_H/2 + 16 + 6)/2):
        case ((SCREEN_H/2 + 16 + 8)/2):
        case ((SCREEN_H/2 + 16 + 10)/2):
        {
            uint8_t button[] = { 'B', 0 };
            font_render_line_doubled(button, 20, vga_line - (SCREEN_H/2 + 16 + 4), 
                ~palette[edit_color[1]],
                palette[edit_color[1]]);
            break;
        }
        case (SCREEN_H/2 - 24)/2:
        case (SCREEN_H/2 + 32)/2:
        case (SCREEN_H/2 - 40 - 4)/2:
        case (SCREEN_H/2 - 40 + 2*16 + 4)/2:
        {
            // remove the swatches
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;

            for (int k=0; k<8; ++k)
                *(++dst) = 0;
            break;
        }
        case ((SCREEN_H/2 - 40 - 16 - 4)/2):
        case ((SCREEN_H/2 - 40 - 16 - 4 + 12)/2):
        {
            // draw a small color swatch for previous edit color, and kill text
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;
            uint32_t color = palette[(edit_color[0]-1)&15] | (palette[(edit_color[0]-1)&15]<<16);

            for (int k=0; k<8; ++k)
                *(++dst) = color;
            break;
        }
        case ((SCREEN_H/2 - 40 - 16 - 4 + 4)/2):
        case ((SCREEN_H/2 - 40 - 16 - 4 + 6)/2):
        case ((SCREEN_H/2 - 40 - 16 - 4 + 8)/2):
        case ((SCREEN_H/2 - 40 - 16 - 4 + 10)/2):
        {
            uint8_t button[] = { 'L', 0 };
            font_render_line_doubled(button, 20, vga_line - (SCREEN_H/2 - 40 - 16 - 4 + 4), 
                ~palette[(edit_color[0]-1)&15],
                palette[(edit_color[0]-1)&15]);
            break;
        }
        case ((SCREEN_H/2 - 40 + 16 + 4)/2):
        case ((SCREEN_H/2 - 40 + 16 + 4 + 12)/2):
        {
            // draw a small color swatch for next edit color, and kill text
            uint32_t *dst = ((uint32_t *)draw_buffer) + 16/2 - 1;
            uint32_t color = palette[(edit_color[0]+1)&15] | (palette[(edit_color[0]+1)&15]<<16);

            for (int k=0; k<8; ++k)
                *(++dst) = color;
            break;
        }
        case ((SCREEN_H/2 - 40 + 16 + 4 + 4)/2):
        case ((SCREEN_H/2 - 40 + 16 + 4 + 6)/2):
        case ((SCREEN_H/2 - 40 + 16 + 4 + 8)/2):
        case ((SCREEN_H/2 - 40 + 16 + 4 + 10)/2):
        {
            uint8_t button[] = { 'R', 0 };
            font_render_line_doubled(button, 20, vga_line - (SCREEN_H/2 - 40 + 16 + 4 + 4), 
                ~palette[(edit_color[0]+1)&15],
                palette[(edit_color[0]+1)&15]);
            break;
        }
    }

    // separate block to limit scope of variables therein:
    {
        // draw big tile
        int tile_j = (vga_line-32)/11;
        uint32_t *dst = ((uint32_t *)draw_buffer) + 40/2;
        uint8_t *tile_color = edit_sprite_not_tile ?
            &sprite_draw[edit_sprite/8][edit_sprite%8][tile_j][0] - 1 
          : &tile_draw[edit_tile][tile_j][0] - 1;
        int draw_crosshair = (edit_y == tile_j) && (((vga_line-32)%11)/2 == 2);
        
        for (int l=0; l<8; ++l)
        {
            uint32_t color = palette[(*(++tile_color))&15];
            color |= (color << 16);
            if (draw_crosshair && edit_x == l*2)
            {
                *dst++ = color;
                *dst++ = color;
                *dst++ = ~color;
                *dst++ = color;
                *dst++ = color;
            }
            else
            {
                for (int k=0; k<5; ++k)
                    *dst++ = color;
            }
           
            // in the middle
            color = (color&65535) | (palette[(*tile_color)>>4]<<16);
            *dst++ = color;

            color = ((color>>16)&65535) | (color&(65535<<16));
            if (draw_crosshair && edit_x == l*2+1)
            {
                *dst++ = color;
                *dst++ = color;
                *dst++ = ~color;
                *dst++ = color;
                *dst++ = color;
            }
            else
            {
                for (int k=0; k<5; ++k)
                    *dst++ = color;
            }
        }
    }

    {
        // draw other tiles scrolling
        uint32_t *dst = ((uint32_t *)draw_buffer) + 40/2 + 8*11 + 16/2;
        int tile_j = vga_line-32 + vga_frame/20;
   
        if (tile_j/16 % 2)
        {
            for (int step=0; step<4; ++step)
            {
                uint8_t *tile_color = &tile_draw[(tile_j/16)&15][tile_j&15][0] - 1;
                for (int l=0; l<8; ++l) 
                {
                    uint32_t color = palette[(*(++tile_color))&15];
                    color |= palette[(*tile_color)>>4] << 16;
                    *dst++ = color;
                }
            }
        }
        else
        {
            if (edit_sprite_not_tile)
            {
                int base = (edit_sprite % 8)/2; 
                // TODO:  show all animations (frames) for a given sprite
                // show them in pairs (0-1, 2-3, 4-5, 6-7) on different lines
                for (int step=0; step<2; ++step)
                {
                    uint8_t *tile_color = &sprite_draw[edit_sprite/8][2*base+(vga_frame/60)%2][tile_j&15][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                    tile_color = &sprite_draw[edit_sprite/8][2*base+1-((vga_frame/60)%2)][tile_j&15][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
            }
            else
            {
                for (int step=0; step<4; ++step)
                {
                    uint8_t *tile_color = &tile_draw[edit_tile][tile_j&15][0] - 1;
                    for (int l=0; l<8; ++l) 
                    {
                        uint32_t color = palette[(*(++tile_color))&15];
                        color |= palette[(*tile_color)>>4] << 16;
                        *dst++ = color;
                    }
                }
            }
        }
    }
}

void edit_spot_paint(uint8_t p)
{
    edit_last_painted = p;

    uint8_t *memory = edit_sprite_not_tile ? 
        &sprite_draw[edit_sprite/8][edit_sprite%8][edit_y][edit_x/2] :
        &tile_draw[edit_tile][edit_y][edit_x/2];

    if (edit_x % 2)
        *memory = ((*memory)&15) | (edit_color[p]<<4);
    else
        *memory = (edit_color[p]) | ((*memory) & 240);
}

int edit_spot_color()
{
    const uint8_t *memory = edit_sprite_not_tile ? 
        &sprite_draw[edit_sprite/8][edit_sprite%8][edit_y][edit_x/2] :
        &tile_draw[edit_tile][edit_y][edit_x/2];

    if (edit_x % 2)
        return (*memory) >> 4;
    else
        return (*memory) & 15;
}

void edit_spot_fill(uint8_t p)
{
    edit_last_painted = p;

    if (!fill_can_start())
        fill_stop();
    uint8_t previous_canvas_color = edit_spot_color();
    if (previous_canvas_color != edit_color[p])
    {
        uint8_t *memory = edit_sprite_not_tile ? 
            sprite_draw[edit_sprite/8][edit_sprite%8][0] :
            tile_draw[edit_tile][0];
        fill_init(memory, 16, 16, previous_canvas_color, edit_x, edit_y, edit_color[p]);
    }
}

void edit_controls()
{ 
    int make_wait = 0;
    if (GAMEPAD_PRESSING(0, A))
    {
        game_message[0] = 0;
        edit_spot_fill(edit_last_painted);
        make_wait = 1;
    }
    else if (GAMEPAD_PRESSING(0, X))
    {
        game_message[0] = 0;
        edit_color[edit_last_painted] = edit_spot_color();
        make_wait = 1;
    }

    int paint_if_moved = 0; 
    if (GAMEPAD_PRESSING(0, Y))
    {
        game_message[0] = 0;
        edit_spot_paint(0);
        paint_if_moved = 1;
    }
    else if (GAMEPAD_PRESSING(0, B))
    {
        game_message[0] = 0;
        edit_spot_paint(1);
        paint_if_moved = 2;
    }
    
    int switched = 0;
    if (GAMEPAD_PRESSING(0, R))
        ++switched;
    if (GAMEPAD_PRESSING(0, L))
        --switched;
    if (switched)
    {
        game_message[0] = 0;
        edit_color[edit_last_painted] = (edit_color[edit_last_painted] + switched)&15;
        make_wait = 1;
    }
    
    int moved = 0;
    if (GAMEPAD_PRESSING(0, left))
    {
        edit_x = (edit_x - 1)&15;
        moved = 1;
    }
    else if (GAMEPAD_PRESSING(0, right))
    {
        edit_x = (edit_x + 1)&15;
        moved = 1;
    }
    if (GAMEPAD_PRESSING(0, up))
    {
        edit_y = (edit_y - 1)&15;
        moved = 1;
    }
    else if (GAMEPAD_PRESSING(0, down))
    {
        edit_y = (edit_y + 1)&15;
        moved = 1;
    }
    if (moved)
    {
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
        if (paint_if_moved)
            edit_spot_paint(paint_if_moved-1);
        return;
    }
    else if (paint_if_moved || make_wait)
        gamepad_press_wait = GAMEPAD_PRESS_WAIT;
    
    if (GAMEPAD_PRESS(0, select))
    {
        game_message[0] = 0;
        fill_stop();
        previous_visual_mode = None;
        if (edit_sprite_not_tile)
        {
            game_switch(EditAnthem);
            edit_sprite_not_tile = 0;
        }
        else
        {
            edit_sprite_not_tile = 1;
        }
        return;
    }
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        fill_stop();
        if (previous_visual_mode)
        {
            game_switch(previous_visual_mode);
            previous_visual_mode = None;
        }
        else
            game_switch(EditTileOrSpriteProperties);
    }
}
