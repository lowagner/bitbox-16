#include "bitbox.h"
#include "common.h"
#include "sprites.h"
#include "chiptune.h"
#include "instrument.h"
#include "verse.h"
#include "anthem.h"
#include "tiles.h"
#include "edit.h"
#include "edit2.h"
#include "name.h"
#include "save.h"
#include "fill.h"
#include "font.h"
#include "run.h"
#include "io.h"

#include "string.h" // memcpy

VisualMode visual_mode CCM_MEMORY; 
VisualMode previous_visual_mode CCM_MEMORY;
VisualMode old_visual_mode CCM_MEMORY;
uint16_t old_gamepad[2] CCM_MEMORY;
uint8_t gamepad_press_wait CCM_MEMORY;
uint8_t game_message[32] CCM_MEMORY;
uint8_t parade_offset CCM_MEMORY;

#define BSOD 140

void game_init()
{ 
    run_init();
    map_init();
    font_init();
    save_init();
    edit_init();
    edit2_init();
    tiles_init();
    sprites_init();
    palette_init();
    anthem_init();
    verse_init();
    chip_init();
    instrument_init();

    // now load everything else
    if (io_get_recent_filename())
    {
        message("resetting everything\n");
        // had troubles loading a filename
        base_filename[0] = 'T';
        base_filename[1] = 'M';
        base_filename[2] = 'P';
        base_filename[3] = 0;

        // need to reset everything
        palette_reset();
        tiles_reset();
        map_reset();
        sprites_reset();
        anthem_reset();
        verse_reset();
        instrument_reset();
    }
    else // there was a filename to look into
    {
        if (io_load_palette())
        {
            // had troubles loading a palette
            palette_reset();
        }
        if (io_load_tile(16))
        {
            // had troubles loading tiles...
            tiles_reset();
        }
        if (io_load_map())
        {
            // etc...
            map_reset();
        }
        if (io_load_sprite(16, 8))
        {
            // and so on...
            sprites_reset();
        }
        if (io_load_anthem())
        {
            anthem_reset();
        }
        if (io_load_verse(16))
        {
            verse_reset();
        }
        if (io_load_instrument(16))
        {
            instrument_reset();
        }
    }

    // init game mode
    old_visual_mode = None;
    previous_visual_mode = None;
    game_switch(GameOn);
}

void game_frame()
{
    kbd_emulate_gamepad();
    switch (visual_mode)
    {
    case GameOn:
        run_controls();
        break;
    case EditMap:
        map_controls();
        break;
    case EditTileOrSprite:
        edit_controls();
        break;
    case EditTileOrSpriteProperties:
        edit2_controls();
        break;
    case EditPalette:
        palette_controls();
        break;
    case SaveLoadScreen:
        save_controls();
        break;
    case ChooseFilename:
        name_controls();
        break;
    case EditAnthem:
        anthem_controls();
        break;
    case EditVerse:
        verse_controls();
        break;
    case EditInstrument:
        instrument_controls();
        break;
    default:
        if (GAMEPAD_PRESS(0, select))
            game_switch(SaveLoadScreen);
        break;
    }
    
    old_gamepad[0] = gamepad_buttons[0];
    old_gamepad[1] = gamepad_buttons[1];

    if (vga_frame % 2)
        fill_frame();
    
    if (gamepad_press_wait)
        --gamepad_press_wait;
}

void graph_frame() 
{
    switch (visual_mode)
    {
    case GameOn:
    case EditMap:
        sprites_frame();
        break;
    default:
        break;
    }
}

void graph_line() 
{
    if (vga_odd)
        return;
    switch (visual_mode)
    {
        case GameOn:
            run_line();
            break;
        case EditMap:
            map_line();
            break;
        case EditTileOrSprite:
            edit_line();
            break;
        case EditTileOrSpriteProperties:
            edit2_line();
            break;
        case SaveLoadScreen:
            save_line();
            break;
        case EditPalette:
            palette_line();
            break;
        case ChooseFilename:
            name_line();
            break;
        case EditAnthem:
            anthem_line();
            break;
        case EditVerse:
            verse_line();
            break;
        case EditInstrument:
            instrument_line();
            break;
        default:
        {
            int line = vga_line/10;
            int internal_line = vga_line%10;
            if (vga_line/2 == 0 || (internal_line/2 == 4))
            {
                memset(draw_buffer, BSOD, 2*SCREEN_W);
                return;
            }
            if (line >= 4 && line < 20)
            {
                line -= 4;
                uint32_t *dst = (uint32_t *)draw_buffer + 37;
                uint32_t color_choice[2] = { (BSOD*257)|((BSOD*257)<<16), 65535|(65535<<16) };
                int shift = ((internal_line/2))*4;
                for (int c=0; c<16; ++c)
                {
                    uint8_t row = (font[c+line*16] >> shift) & 15;
                    for (int j=0; j<4; ++j)
                    {
                        *(++dst) = color_choice[row&1];
                        row >>= 1;
                    }
                    *(++dst) = color_choice[0];
                }
                return;
            }
            break;
        }
    }
}

void draw_parade(int line, uint8_t bg_color)
{
    int internal_line = line % 48;
    if (internal_line/2 == 32/2)
    {
        memset(draw_buffer, bg_color, 2*SCREEN_W);
        if (line == 32)
        {
            // reseting the parade
            if (old_visual_mode != visual_mode)
            {
                for (int i=0; i<16; ++i)
                    tile_translator[i] = i;
                old_visual_mode = visual_mode;
                parade_offset = 0;
            }
            else
            {
                // update parade offset
                if (vga_frame % 64 == 0)
                {
                    ++parade_offset;
                }
                // TODO:  update tile translator here 

            }
        }
        return;
    }
    else if (internal_line > 32)
        return;
    line = line / 48;
    if (line >= 2)
        return;
    if (internal_line/16 == 0) //
    {
        internal_line %= 16;
        // draw sprites
        if (1) //(line % 2)
        {
            // on odd lines, let them fly
            uint16_t *dst = draw_buffer + 31;
            for (int s=0; s<16; ++s)
            {
                uint8_t *tile_color = &sprite_draw[s][parade_offset%8][internal_line][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }

        }
        else
        {
            /*
            // on even lines, make them walk
            uint16_t *dst = draw_buffer + 31;
            if (parade_offset % 16 == 0)
            for (uint8_t s=0; s<16; ++s)
            {
                uint8_t *tile_color = &sprite_draw[(s+15*parade_offset/16)&15][2*RIGHT+parade_offset%2][internal_line][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }
            else
            {


            }
            */

        }
        return;
    }
    else
    {
        internal_line %= 16;
        // draw tiles
        if (line % 2)
        {
            // on odd lines, use the tile_translator
            uint16_t *dst = draw_buffer + 31;
            for (int tile=0; tile<16; ++tile)
            {
                uint8_t *tile_color = &tile_draw[tile_translator[tile]][internal_line][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }
        }
        else
        {
            uint16_t *dst = draw_buffer + 31;
            for (int tile=0; tile<16; ++tile)
            {
                uint8_t *tile_color = &tile_draw[tile][internal_line][0] - 1;
                for (int l=0; l<8; ++l)
                {
                    *dst++ = palette[(*(++tile_color))&15];
                    *dst++ = palette[(*tile_color)>>4];
                }
            }
        }
        return;
    }
}

void game_switch(VisualMode new_visual_mode)
{
    chip_kill();

    switch (new_visual_mode)
    {
    case EditMap:
        map_switch();
        break;
    case GameOn:
        run_switch();
        break;
    default:
        break;
    }
    visual_mode = new_visual_mode;
}
