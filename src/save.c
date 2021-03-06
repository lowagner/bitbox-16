#include "bitbox.h"
#include "common.h"
#include "font.h"
#include "name.h"
#include "io.h"
#include "edit.h"

#define BG_COLOR 192 // a uint8_t, uint16_t color is (BG_COLOR)|(BG_COLOR<<8)

uint8_t save_only CCM_MEMORY; // 0 - everything, 1 - map, 2 - tiles, 3 - sprites, 4 - palette
                              // 5 - music, 6 - patterns, 7 - unlock

#define NUMBER_LINES 17
   
void save_init()
{
    save_only = 0;
}

void save_line()
{
    if (vga_line < 22)
    {
        if (vga_line/2 == 0)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    else if (vga_line >= 22 + NUMBER_LINES*10)
    {
        if (vga_line/2 == (22 + NUMBER_LINES*10)/2)
        {
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
            return;
        }
        draw_parade(vga_line - (24 + NUMBER_LINES*10), BG_COLOR);
        return;
    }
    int line = (vga_line-22) / 10;
    int internal_line = (vga_line-22) % 10;
    if (internal_line == 0 || internal_line == 9)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
    }
    else
    {
        --internal_line;
        switch (line)
        {
        case 0:
        {
            font_render_line_doubled((const uint8_t *)"16 game editor", 16, internal_line, 65535, BG_COLOR*257);
            font_render_line_doubled((uint8_t *)base_filename, 16+15*9, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 2:
            font_render_line_doubled((const uint8_t *)"L/R:selective save/load", 16, internal_line, 65535, BG_COLOR*257);
            switch (save_only)
            {
            case 0:
                font_render_line_doubled((const uint8_t *)"all", 16+24*9, internal_line, 65535, BG_COLOR*257);
                break;
            case 1:
                font_render_line_doubled((const uint8_t *)"map", 16+24*9, internal_line, 65535, BG_COLOR*257);
                break;
            case 2:
                font_render_line_doubled((const uint8_t *)"tiles", 16+24*9, internal_line, 65535, BG_COLOR*257);
                break;
            case 3:
                font_render_line_doubled((const uint8_t *)"sprites", 16+24*9, internal_line, 65535, BG_COLOR*257);
                break;
            case 4:
                font_render_line_doubled((const uint8_t *)"palette", 16+24*9, internal_line, 65535, BG_COLOR*257);
                break;
            case 5:
                font_render_line_doubled((const uint8_t *)"music", 16+24*9, internal_line, 65535, BG_COLOR*257);
                break;
            case 6:
                font_render_line_doubled((const uint8_t *)"patterns", 16+24*9, internal_line, 65535, BG_COLOR*257);
                break;
            case 7:
                font_render_line_doubled((const uint8_t *)"unlocks", 16+24*9, internal_line, 65535, BG_COLOR*257);
                break;
            }
            break;
        case 3:
            font_render_line_doubled((const uint8_t *)"  A:save to file", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 4:
            font_render_line_doubled((const uint8_t *)"  B:load from file", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 5:
            font_render_line_doubled((const uint8_t *)"  Y:choose filename", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 7:
            switch (save_only)
            {
            case 0:
                font_render_line_doubled((const uint8_t *)"start:play game", 16, internal_line, 65535, BG_COLOR*257);
                break;
            case 1:
                font_render_line_doubled((const uint8_t *)"start:edit map", 16, internal_line, 65535, BG_COLOR*257);
                break;
            case 2:
                font_render_line_doubled((const uint8_t *)"start:edit tiles", 16, internal_line, 65535, BG_COLOR*257);
                break;
            case 3:
                font_render_line_doubled((const uint8_t *)"start:edit sprites", 16, internal_line, 65535, BG_COLOR*257);
                break;
            case 4:
                font_render_line_doubled((const uint8_t *)"start:edit palette", 16, internal_line, 65535, BG_COLOR*257);
                break;
            case 5:
                font_render_line_doubled((const uint8_t *)"start:edit music", 16, internal_line, 65535, BG_COLOR*257);
                break;
            case 6:
                font_render_line_doubled((const uint8_t *)"start:edit patterns", 16, internal_line, 65535, BG_COLOR*257);
                break;
            case 7:
                font_render_line_doubled((const uint8_t *)"start:edit unlocks", 16, internal_line, 65535, BG_COLOR*257);
                break;
            }
            break;
        case 10:
            font_render_line_doubled((const uint8_t *)"up/dn:choose game", 16, internal_line, 65535, BG_COLOR*257);    
            break;
        case 11:
            font_render_line_doubled((const uint8_t *)"rmbr to load before playing!", 16+2*10, internal_line, 65535, BG_COLOR*257);    
            break;
        case (NUMBER_LINES-2):
            if (game_message[0])
                font_render_line_doubled(game_message, 32, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"github.com/lowagner/bitbox-16", 27, internal_line, 65535, BG_COLOR*257);
            break;
        }
    }
}


void save_controls()
{
    int save_or_load = 0;
    if (GAMEPAD_PRESS(0, A))
        save_or_load = 1;  // save
    if (GAMEPAD_PRESS(0, B))
        save_or_load = 2; // load
    if (save_or_load)
    {
        FileError error = BotchedIt;
        int offset = 0;
        switch (save_only)
        {
        case 0: // save all
            error = (save_or_load == 1) ? io_save_tile(16) : io_load_tile(16);
            if (error != NoError)
            {
                strcpy((char *)game_message, "tiles ");
                offset = 6;
                break;
            }
            error = (save_or_load == 1) ? io_save_sprite(128) : io_load_sprite(128);
            if (error != NoError)
            {
                strcpy((char *)game_message, "sprites ");
                offset = 8;
                break;
            }
            error = (save_or_load == 1) ? io_save_map() : io_load_map();
            if (error != NoError)
            {
                strcpy((char *)game_message, "map ");
                offset = 4;
                break;
            }
            error = (save_or_load == 1) ? io_save_palette() : io_load_palette();
            if (error != NoError)
            {
                strcpy((char *)game_message, "palette ");
                offset = 8;
                break;
            }
            error = (save_or_load == 1) ? io_save_anthem() : io_load_anthem();
            if (error != NoError)
            {
                strcpy((char *)game_message, "anthem ");
                offset = 7;
                break;
            }
            error = (save_or_load == 1) ? io_save_verse(16) : io_load_verse(16);
            if (error != NoError)
            {
                strcpy((char *)game_message, "verse ");
                offset = 6;
                break;
            }
            error = (save_or_load == 1) ? io_save_instrument(16) : io_load_instrument(16);
            if (error != NoError)
            {
                strcpy((char *)game_message, "instr. ");
                offset = 7;
                break;
            }
            error = (save_or_load == 1) ? io_save_go(16) : io_load_go(16);
            if (error != NoError)
            {
                strcpy((char *)game_message, "pattn. ");
                offset = 7;
                break;
            }
            error = (save_or_load == 1) ? io_save_unlocks(8) : io_load_unlocks(8);
            if (error != NoError)
            {
                strcpy((char *)game_message, "unlock ");
                offset = 7;
            }
            break;
        case 1:
            error = (save_or_load == 1) ? io_save_map() : io_load_map();
            if (error != NoError)
            {
                strcpy((char *)game_message, "map ");
                offset = 4;
            }
            break;
        case 2:
            error = (save_or_load == 1) ? io_save_tile(16) : io_load_tile(16);
            if (error != NoError)
            {
                strcpy((char *)game_message, "tiles ");
                offset = 6;
            }
            break;
        case 3:
            error = (save_or_load == 1) ? io_save_sprite(128) : io_load_sprite(128);
            if (error != NoError)
            {
                strcpy((char *)game_message, "sprites ");
                offset = 8;
            }
            break;
        case 4:
            error = (save_or_load == 1) ? io_save_palette() : io_load_palette();
            if (error != NoError)
            {
                strcpy((char *)game_message, "palette ");
                offset = 8;
            }
            break;
        case 5:
            error = (save_or_load == 1) ? io_save_anthem() : io_load_anthem();
            if (error != NoError)
            {
                strcpy((char *)game_message, "anthem ");
                offset = 7;
                break;
            }
            error = (save_or_load == 1) ? io_save_verse(16) : io_load_verse(16);
            if (error != NoError)
            {
                strcpy((char *)game_message, "verse ");
                offset = 6;
                break;
            }
            error = (save_or_load == 1) ? io_save_instrument(16) : io_load_instrument(16);
            if (error != NoError)
            {
                strcpy((char *)game_message, "instr. ");
                offset = 7;
                break;
            }
            break;
        case 6:
            error = (save_or_load == 1) ? io_save_go(16) : io_load_go(16);
            if (error != NoError)
            {
                strcpy((char *)game_message, "pattn. ");
                offset = 7;
            }
            break;
        case 7:
            error = (save_or_load == 1) ? io_save_unlocks(8) : io_load_unlocks(8);
            if (error != NoError)
            {
                strcpy((char *)game_message, "unlock ");
                offset = 7;
            }
            break;
        }
        io_message_from_error(game_message+offset, error, save_or_load);
        return;
    }
    if (GAMEPAD_PRESS(0, X))
    {
        game_message[0] = 0;
        // TODO:  add functionality?
        return;
    }
    if (GAMEPAD_PRESS(0, Y))
    {
        game_message[0] = 0;
        game_switch(ChooseFilename);
        return;
    }
    if (GAMEPAD_PRESS(0, L) || GAMEPAD_PRESS(0, left))
    {
        game_message[0] = 0;
        save_only = (save_only - 1)&7;
        return;
    } 
    if (GAMEPAD_PRESS(0, R) || GAMEPAD_PRESS(0, right))
    {
        game_message[0] = 0;
        save_only = (save_only + 1)&7;
        return;
    }
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        switch (save_only)
        {
        case 0:
            if (map_modified)
                return set_game_message_timeout("map modified, save first!", MESSAGE_TIMEOUT);
            game_switch(GameOn);
            break;
        case 1:
            game_switch(EditMap);
            break;
        case 2:
            game_switch(EditTileOrSprite);
            edit_sprite_not_tile = 0;
            break;
        case 3:
            game_switch(EditTileOrSprite);
            edit_sprite_not_tile = 1;
            break;
        case 4:
            game_switch(EditPalette);
            break;
        case 5:
            game_switch(EditAnthem);
            break;
        case 6:
            game_switch(EditSpritePattern);
            break;
        case 7:
            game_switch(EditUnlocks);
            break;
        }
        return;
    }
    if (GAMEPAD_PRESS(0, down))
    {
        io_next_available_filename();
        return;
    }
    if (GAMEPAD_PRESS(0, up))
    {
        io_previous_available_filename();
        return;
    }
}
