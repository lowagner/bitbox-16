#include "bitbox.h"
#include "common.h"
#include "common.h"
#include "chiptune.h"
#include "anthem.h"
#include "instrument.h"
#include "name.h"
#include "font.h"
#include "io.h"

#include <stdlib.h> // rand
#include <string.h> // memset

#define BG_COLOR 132
#define PLAY_COLOR (RGB(200, 100, 0)|(RGB(200, 100, 0)<<16))
#define BOX_COLOR (RGB(200, 200, 230)|(RGB(200, 200, 230)<<16))
#define MATRIX_WING_COLOR (RGB(30, 90, 90) | (RGB(30, 90, 90)<<16))
#define NUMBER_LINES 20

uint8_t verse_track CCM_MEMORY;
uint8_t verse_track_pos CCM_MEMORY;
uint8_t verse_track_offset CCM_MEMORY;
uint8_t verse_menu_not_edit CCM_MEMORY;
uint8_t verse_copying CCM_MEMORY;
uint8_t verse_show_track CCM_MEMORY;
uint8_t verse_player CCM_MEMORY;
uint8_t verse_command_copy CCM_MEMORY;
uint8_t verse_bad CCM_MEMORY;

void verse_init()
{
    verse_track = 0;
    verse_track_pos = 0;
    verse_track_offset = 0;
    verse_command_copy = rand()%16;
    verse_bad = 0;
}

void verse_reset()
{
}

void verse_short_command_message(uint8_t *buffer, uint8_t cmd)
{
    switch (cmd&15)
    {
        case TRACK_BREAK:
            strcpy((char *)buffer, "break");
            break;
        case TRACK_OCTAVE:
            strcpy((char *)buffer, "octave");
            break;
        case TRACK_INSTRUMENT:
            strcpy((char *)buffer, "instrument");
            break;
        case TRACK_VOLUME:
            strcpy((char *)buffer, "volume");
            break;
        case TRACK_NOTE:
            strcpy((char *)buffer, "note");
            break;
        case TRACK_WAIT:
            strcpy((char *)buffer, "wait");
            break;
        case TRACK_NOTE_WAIT:
            strcpy((char *)buffer, "note+wait");
            break;
        case TRACK_FADE_IN:
            strcpy((char *)buffer, "fade in");
            break;
        case TRACK_FADE_OUT:
            strcpy((char *)buffer, "fade out");
            break;
        case TRACK_INERTIA:
            strcpy((char *)buffer, "inertia");
            break;
        case TRACK_VIBRATO:
            strcpy((char *)buffer, "vibrato");
            break;
        case TRACK_TRANSPOSE:
            strcpy((char *)buffer, "transpose");
            break;
        case TRACK_SPEED:
            strcpy((char *)buffer, "speed");
            break;
        case TRACK_LENGTH:
            strcpy((char *)buffer, "track length");
            break;
        case TRACK_RANDOMIZE:
            strcpy((char *)buffer, "randomize");
            break;
        case TRACK_JUMP:
            strcpy((char *)buffer, "jump");
            break;
    }
}

void verse_render_command(int j, int y)
{
    int x = 32;
    #ifdef EMULATOR
    if (y < 0 || y >= 8)
    {
        message("got too big a line count for verse %d, line %d\n", (int)verse_track, j);
        return;
    }
    #endif
    
    uint8_t cmd = chip_track[verse_track][verse_player][j];
    uint8_t param = cmd>>4;
    cmd &= 15;

    uint32_t *dst = (uint32_t *)draw_buffer + x/2;
    uint32_t color_choice[2];
    if (j%2)
        color_choice[0] = 16843009u*BG_COLOR;
    else
        color_choice[0] = 16843009u*149;
    
    if (j != verse_track_pos)
        color_choice[1] = 65535u*65537u;
    else
    {
        color_choice[1] = RGB(190, 245, 255)|(RGB(190, 245, 255)<<16);
        if (!verse_menu_not_edit)
        {
            if ((y+1)/2 == 1)
            {
                dst -= 4;
                *dst = color_choice[1];
                ++dst;
                *dst = color_choice[1];
                dst += 4 - 1;
            }
            else if ((y+1)/2 == 3)
            {
                dst -= 4;
                *dst = 16843009u*BG_COLOR;
                ++dst;
                *dst = 16843009u*BG_COLOR;
                dst += 4 - 1;
            }
        }
    }
    
    if (cmd == TRACK_BREAK)
    {
        if (param == 0)
        {
            if (y == 7)
                verse_show_track = 0;
            cmd = '0';
            param = '0';
        }
        else
        {
            cmd = '0' + (4*param)/10;
            param = '0' + (4*param)%10; 
        }
    }
    else 
    switch (cmd)
    {
        case TRACK_OCTAVE:
            if (param < 7)
            {
                cmd = 'O';
                param += '0';
            }
            else if (param == 7)
            {
                cmd = '=';
                param = '=';
            }
            else if (param < 12)
            {
                cmd = (param%2) ? '+' : '/';
                param = '0' + (param - 6)/2;
            }
            else
            {
                cmd = (param%2) ? '\\' : '-';
                param = '0' + (17-param)/2;
            }
            break;
        case TRACK_INSTRUMENT:
            cmd = 'I';
            param = hex[param];
            break;
        case TRACK_VOLUME:
            cmd = 'V';
            param = hex[param];
            break;
        case TRACK_NOTE:
            if (param >= 12)
                color_choice[1] = RGB(150,150,255)|(65535<<16);
            param %= 12;
            cmd = note_name[param][0];
            param = note_name[param][1];
            break;
        case TRACK_WAIT:
            cmd = 'W';
            if (param)
                param = hex[param];
            else
                param = 'g';
            break;
        case TRACK_NOTE_WAIT:
            cmd = 'N';
            param = hex[param];
            break;
        case TRACK_FADE_IN:
            cmd = '<';
            param = hex[param];
            break;
        case TRACK_FADE_OUT:
            cmd = '>';
            if (param)
                param = hex[param];
            else
                param = 'g';
            break;
        case TRACK_INERTIA:
            cmd = 'i';
            param = hex[param];
            break;
        case TRACK_VIBRATO:
            cmd = '~';
            param = hex[param];
            break;
        case TRACK_TRANSPOSE:
            cmd = 'T';
            param = hex[param];
            break;
        case TRACK_SPEED:
            cmd = 'S'; 
            param = hex[param];
            break;
        case TRACK_LENGTH:
            cmd = 'L';
            if (param)
                param = hex[param];
            else
                param = 'g';
            break;
        case TRACK_RANDOMIZE:
            cmd = 'R';
            param = 224 + param;
            break;
        case TRACK_JUMP:
            cmd = 'J';
            param = hex[2*param];
            break;
    }
    
    uint8_t shift = ((y/2))*4;
    uint8_t row = (font[hex[j]] >> shift) & 15;
    *(++dst) = color_choice[0];
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    row = (font[':'] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    *(++dst) = color_choice[0];
    row = (font[cmd] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    
    row = (font[param] >> shift) & 15;
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
  
    if (!chip_play_track)
        return;
    int cmd_index = chip_player[verse_player].track_cmd_index;
    if (cmd_index)
    switch (chip_track[verse_track][verse_player][cmd_index-1]&15)
    {
        case TRACK_BREAK:
        case TRACK_WAIT:
        case TRACK_NOTE_WAIT:
            --cmd_index;
    }
    if (j == cmd_index)
    {
        if ((y+1)/2 == 1)
        {
            dst += 4;
            *dst = PLAY_COLOR;
            ++dst;
            *dst = PLAY_COLOR;
        }
        else if ((y+1)/2 == 3)
        {
            dst += 4;
            *dst = 16843009u*BG_COLOR;
            ++dst;
            *dst = 16843009u*BG_COLOR;
        }
    }
}

int _check_verse();

void check_verse()
{
    // check if that parameter broke something
    if (_check_verse())
    {
        verse_bad = 1; 
        strcpy((char *)game_message, "bad jump, need wait in loop.");
    }
    else
    {
        verse_bad = 0; 
        game_message[0] = 0;
    }
    chip_kill();
}

void verse_adjust_parameter(int direction)
{
    if (!direction)
        return;
    uint8_t cmd = chip_track[verse_track][verse_player][verse_track_pos];
    uint8_t param = cmd>>4;
    cmd &= 15;
    param = (param + direction)&15;
    chip_track[verse_track][verse_player][verse_track_pos] = cmd | (param<<4);

    check_verse();
}

int _check_verse()
{
    // check for a JUMP which loops back on itself without waiting at least a little bit.
    // return 1 if so, 0 if not.
    int j=0; // current command index
    int j_last_jump = -1;
    int found_wait = 0;
    for (int k=0; k<64; ++k)
    {
        if (j >= 32) // got to the end
            return 0;
        if (j_last_jump >= 0)
        {
            if (j == j_last_jump) // we found our loop-back point
                return !(found_wait); // did we find a wait?
            int j_next_jump = -1;
            switch (chip_track[verse_track][verse_player][j]&15)
            {
                case TRACK_JUMP:
                    j_next_jump = 2*(chip_track[verse_track][verse_player][j]>>4);
                    if (j_next_jump == j_last_jump) // jumping forward to the original jump
                    {
                        message("jumped to the old jump\n");
                        return !(found_wait);
                    }
                    else if (j_next_jump > j_last_jump) // jumping past original jump
                    {
                        message("This probably shouldn't happen.\n");
                        j_last_jump = -1; // can't look for loops...
                    }
                    else
                    {
                        message("jumped backwards again?\n");
                        j_last_jump = j;
                        found_wait = 0;
                    }
                    j = j_next_jump;
                    break;
                case TRACK_WAIT:
                case TRACK_NOTE_WAIT:
                    message("saw wait at j=%d\n", j);
                    found_wait = 1;
                    ++j;
                    break;
                default:
                    ++j;
            }
        }
        else
        {
            if ((chip_track[verse_track][verse_player][j]&15) != TRACK_JUMP)
                ++j;
            else
            {
                j_last_jump = j;
                j = 2*(chip_track[verse_track][verse_player][j]>>4);
                if (j > j_last_jump)
                {
                    message("This probably shouldn't happen??\n");
                    j_last_jump = -1; // don't care, we moved ahead
                }
                else if (j == j_last_jump)
                {
                    message("jumped to itself\n");
                    return 1; // this is bad
                }
                else
                    found_wait = 0;
            }
        }
    }
    message("couldn't finish after 32 iterations. congratulations.\nprobably looping back on self, but with waits.");
    return 0;
}

void verse_line()
{
    if (vga_line < 16)
    {
        if (vga_line/2 == 0)
        {
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
            return;
        }
        return;
    }
    else if (vga_line >= 16 + NUMBER_LINES*10)
    {
        if (vga_line/2 == (16 +NUMBER_LINES*10)/2)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    int line = (vga_line-16) / 10;
    int internal_line = (vga_line-16) % 10;
    if (internal_line == 0 || internal_line == 9)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    --internal_line;
    uint8_t buffer[24];
    switch (line)
    {
        case 0:
        {
            // edit track
            uint8_t msg[] = { (chip_play_track && (track_pos/4 % 2==0)) ? '*':' ', 't', 'r', 'a', 'c', 'k', ' ', hex[verse_track], 
                ' ', 'P', hex[verse_player], 
                ' ', 'I', hex[chip_player[verse_player].instrument],
                ' ', 't', 'k', 'l', 'e', 'n', 
                ' ', '0' + track_length/10, '0' + track_length%10,
            0 };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 1:
            break;
        case 2:
        {
            verse_show_track = 1; 
            verse_render_command(verse_track_offset+line-2, internal_line);
            // command
            uint8_t msg[] = { 'c', 'o', 'm', 'm', 'a', 'n', 'd', ' ', hex[verse_track_pos], ':', 0 };
            font_render_line_doubled(msg, 96, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 3:
            switch (chip_track[verse_track][verse_player][verse_track_pos]&15)
            {
                case TRACK_BREAK:
                    strcpy((char *)buffer, "break until time");
                    break;
                case TRACK_OCTAVE:
                    strcpy((char *)buffer, "octave");
                    break;
                case TRACK_INSTRUMENT:
                    strcpy((char *)buffer, "instrument");
                    break;
                case TRACK_VOLUME:
                    strcpy((char *)buffer, "volume");
                    break;
                case TRACK_NOTE:
                    strcpy((char *)buffer, "relative note from C");
                    break;
                case TRACK_WAIT:
                    strcpy((char *)buffer, "wait");
                    break;
                case TRACK_NOTE_WAIT:
                    strcpy((char *)buffer, "wait-1 + note 0,+,++,-");
                    break;
                case TRACK_FADE_IN:
                    strcpy((char *)buffer, "fade in");
                    break;
                case TRACK_FADE_OUT:
                    strcpy((char *)buffer, "fade out");
                    break;
                case TRACK_INERTIA:
                    strcpy((char *)buffer, "note inertia");
                    break;
                case TRACK_VIBRATO:
                    strcpy((char *)buffer, "vibrato depth + rate");
                    break;
                case TRACK_TRANSPOSE:
                    strcpy((char *)buffer, "global song transpose");
                    break;
                case TRACK_SPEED:
                    strcpy((char *)buffer, "track speed");
                    break;
                case TRACK_LENGTH:
                    strcpy((char *)buffer, "track length / 4");
                    break;
                case TRACK_RANDOMIZE:
                    strcpy((char *)buffer, "randomize next cmd");
                    break;
                case TRACK_JUMP:
                    strcpy((char *)buffer, "jump to cmd index");
                    break;
            }
            font_render_line_doubled(buffer, 102, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 4:
            switch (chip_track[verse_track][verse_player][verse_track_pos]&15)
            {
                case TRACK_VIBRATO:
                case TRACK_NOTE_WAIT:
                    font_render_line_doubled((uint8_t *)"0-3 + 0,4,8,c", 112, internal_line, 65535, BG_COLOR*257);
                    break;
            }
            goto maybe_show_track;
        case 5:
            font_render_line_doubled((uint8_t *)"switch to:", 102 - 6*verse_menu_not_edit, internal_line, 65535, BG_COLOR*257); 
            goto maybe_show_track;
        case 6:
            if (verse_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"L:prev track", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'L'; buffer[1] = ':';
                verse_short_command_message(buffer+2, chip_track[verse_track][verse_player][verse_track_pos]-1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_track;
        case 7:
            if (verse_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"R:next track", 112, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                buffer[0] = 'R'; buffer[1] = ':';
                verse_short_command_message(buffer+2, chip_track[verse_track][verse_player][verse_track_pos]+1);
                font_render_line_doubled(buffer, 112, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_track;
        case 8:
            font_render_line_doubled((uint8_t *)"dpad:", 102 - 6*verse_menu_not_edit, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 9:
            if (verse_menu_not_edit)
                font_render_line_doubled((uint8_t *)"switch player", 112, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"adjust parameters", 112, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 11:
            if (verse_menu_not_edit)
            {
                if (verse_copying < 64)
                    font_render_line_doubled((uint8_t *)"A:cancel copy", 96, internal_line, 65535, BG_COLOR*257);
                else
                    font_render_line_doubled((uint8_t *)"A:save to file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else if (chip_play_track)
                font_render_line_doubled((uint8_t *)"A:stop track", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"A:play track", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 12:
            if (verse_menu_not_edit)
            {
                if (verse_copying < 64)
                    font_render_line_doubled((uint8_t *)"B/X:\"     \"", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"B:load from file", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"B:edit instrument", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 13:
            if (verse_menu_not_edit)
            {
                if (verse_copying < 64)
                    font_render_line_doubled((uint8_t *)"Y:paste", 96, internal_line, 65535, BG_COLOR*257);

                else
                    font_render_line_doubled((uint8_t *)"X:copy", 96, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((uint8_t *)"X:cut cmd", 96, internal_line, 65535, BG_COLOR*257);
            }
            goto maybe_show_track;
        case 14:
            if (verse_menu_not_edit)
            {
                if (verse_copying < 64)
                    goto maybe_show_track;
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), base_filename);
                font_render_line_doubled(buffer, 96, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"Y:insert cmd", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 16:
            if (verse_menu_not_edit)
                font_render_line_doubled((uint8_t *)"start:edit track", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:track menu", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 17:
            if (verse_menu_not_edit)
                font_render_line_doubled((uint8_t *)"select:instrument menu", 96, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"select:edit anthem", 96, internal_line, 65535, BG_COLOR*257);
            goto maybe_show_track;
        case 18:
            break;
        case 19:
            font_render_line_doubled(game_message, 36, internal_line, 65535, BG_COLOR*257);
            break;
        default:
          maybe_show_track:
            if (verse_show_track)
                verse_render_command(verse_track_offset+line-2, internal_line);
            break; 
    }
}

void verse_controls()
{
    if (verse_menu_not_edit)
    {
        int switched = 0;
        if (GAMEPAD_PRESSING(0, down))
            ++switched;
        if (GAMEPAD_PRESSING(0, up))
            --switched;
        if (GAMEPAD_PRESSING(0, left))
            --switched;
        if (GAMEPAD_PRESSING(0, right))
            ++switched;
        if (switched)
        {
            game_message[0] = 0;
            verse_track_pos = 0;
            verse_track_offset = 0;
            verse_player = (verse_player+switched)&3;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }
        
        if (GAMEPAD_PRESSING(0, L))
            --switched;
        if (GAMEPAD_PRESSING(0, R))
            ++switched;
        if (switched)
        {
            game_message[0] = 0;
            verse_track = (verse_track+switched)&15;
            verse_track_pos = 0;
            verse_track_offset = 0;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }

        if (GAMEPAD_PRESS(0, X)) // copy
        {
            if (verse_copying < 64)
            {
                verse_copying = 64;
                game_message[0] = 0;
            }
            else
            {
                verse_copying = 4*verse_track + verse_player;
                strcpy((char *)game_message, "copied.");
            }
        }

        if (GAMEPAD_PRESS(0, Y)) // paste
        {
            if (verse_copying < 64)
            {
                // paste
                uint8_t *src, *dst;
                src = &chip_track[verse_copying/4][verse_copying%4][0];
                dst = &chip_track[verse_track][verse_player][0];
                if (src == dst)
                {
                    verse_copying = 64;
                    strcpy((char *)game_message, "pasting to same thing"); 
                    return;
                }
                memcpy(dst, src, sizeof(chip_track[0][0]));
                strcpy((char *)game_message, "pasted."); 
                verse_copying = 64;
            }
            else
            {
                // switch to choose name and hope to come back
                game_message[0] = 0;
                game_switch(ChooseFilename);
                previous_visual_mode = EditVerse;
            }
            return;
        }
        
        int save_or_load = 0;
        if (GAMEPAD_PRESS(0, A))
            save_or_load = 1; // save
        if (GAMEPAD_PRESS(0, B))
            save_or_load = 2; // load
        if (save_or_load)
        {
            if (verse_copying < 64)
            {
                // cancel a copy 
                verse_copying = 64;
                game_message[0] = 0;
                return;
            }

            FileError error = BotchedIt;
            if (save_or_load == 1)
                error = io_save_verse(verse_track);
            else
                error = io_load_verse(verse_track);
            io_message_from_error(game_message, error, save_or_load);
            return;
        }

    }
    else // editing, not menu
    {
        int movement = 0;
        if (GAMEPAD_PRESSING(0, L))
            --movement;
        if (GAMEPAD_PRESSING(0, R))
            ++movement;
        if (movement)
        {
            uint8_t *memory = &chip_track[verse_track][verse_player][verse_track_pos];
            *memory = ((*memory+movement)&15)|((*memory)&240);
            check_verse();
        }
        if (GAMEPAD_PRESSING(0, down))
        {
            if (verse_track_pos < MAX_TRACK_LENGTH-1 &&
                chip_track[verse_track][verse_player][verse_track_pos])
            {
                ++verse_track_pos;
                if (verse_track_pos > verse_track_offset + 15)
                    verse_track_offset = verse_track_pos - 15;
            }
            else
            {
                verse_track_pos = 0;
                verse_track_offset = 0;
            }
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, up))
        {
            if (verse_track_pos)
            {
                --verse_track_pos;
                if (verse_track_pos < verse_track_offset)
                    verse_track_offset = verse_track_pos;
            }
            else
            {
                verse_track_pos = 0;
                while (verse_track_pos < MAX_TRACK_LENGTH-1 && 
                    chip_track[verse_track][verse_player][verse_track_pos] != TRACK_BREAK)
                {
                    ++verse_track_pos;
                }
                if (verse_track_pos > verse_track_offset + 15)
                    verse_track_offset = verse_track_pos - 15;
            }
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, left))
        {
            verse_adjust_parameter(-1);
            movement = 1;
        }
        if (GAMEPAD_PRESSING(0, right))
        {
            verse_adjust_parameter(+1);
            movement = 1;
        }
        if (movement)
        {
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }

        if (GAMEPAD_PRESS(0, X))
        { 
            // delete / cut
            verse_command_copy = chip_track[verse_track][verse_player][verse_track_pos];
            for (int j=verse_track_pos; j<MAX_TRACK_LENGTH-1; ++j)
            {
                if ((chip_track[verse_track][verse_player][j] = chip_track[verse_track][verse_player][j+1]) == 0)
                    break;
            }
            chip_track[verse_track][verse_player][MAX_TRACK_LENGTH-1] = TRACK_BREAK;
            check_verse();
            return;
        }

        if (GAMEPAD_PRESS(0, Y))
        {
            // insert
            if ((chip_track[verse_track][verse_player][MAX_TRACK_LENGTH-1]&15) != TRACK_BREAK)
            {
                strcpy((char *)game_message, "list full, can't insert.");
                return;
            }
            for (int j=MAX_TRACK_LENGTH-1; j>verse_track_pos; --j)
            {
                chip_track[verse_track][verse_player][j] = chip_track[verse_track][verse_player][j-1];
            }
            chip_track[verse_track][verse_player][verse_track_pos] = verse_command_copy;
            check_verse();
            return;
        }
        
        if (verse_bad) // can't do anything else until you fix this
            return;

        if (GAMEPAD_PRESS(0, A))
        {
            // play all instruments (or stop them all)
            track_pos = 0;
            if (chip_play_track)
            {
                message("stop play\n");
                chip_kill();
            }
            else
            {
                // play this instrument track.
                // after the repeat, all tracks will sound.
                chip_play_track_init(verse_track);
                // avoid playing other instruments for now:
                for (int i=0; i<verse_player; ++i)
                    chip_player[i].track_cmd_index = MAX_TRACK_LENGTH;
                for (int i=verse_player+1; i<4; ++i)
                    chip_player[i].track_cmd_index = MAX_TRACK_LENGTH;
            }
            return;
        }
        
        if (GAMEPAD_PRESSING(0, B))
        {
            instrument_i = chip_player[verse_player].instrument;
            game_switch(EditInstrument);
            return;
        }
    }
    
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        verse_menu_not_edit = 1 - verse_menu_not_edit; 
        verse_copying = 64;
        chip_kill();
        return;
    }

    if (GAMEPAD_PRESS(0, select))
    {
        verse_copying = 64;
        game_message[0] = 0;
        previous_visual_mode = None;
        if (verse_menu_not_edit)
        {
            instrument_menu_not_edit = verse_menu_not_edit;
            game_switch(EditInstrument);
        }
        else
        {
            anthem_menu_not_edit = 0;
            game_switch(EditAnthem);
        }
        return;
    } 
}
