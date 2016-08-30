#include "bitbox.h"
#include "common.h"
#include "common.h"
#include "chiptune.h"
#include "instrument.h"
#include "verse.h"
#include "name.h"
#include "font.h"
#include "io.h"

#include <stdlib.h> // rand
#include <string.h> // memset

#define BG_COLOR 128
#define BOX_COLOR (RGB(180, 250, 180)|(RGB(180, 250, 180)<<16))
#define MATRIX_WING_COLOR (RGB(30, 90, 30) | (RGB(30, 90, 30)<<16))
#define NUMBER_LINES 20

uint8_t anthem_menu_not_edit CCM_MEMORY;
uint8_t anthem_song_pos CCM_MEMORY;
uint8_t anthem_song_offset CCM_MEMORY;
uint8_t anthem_color[2] CCM_MEMORY;
uint8_t anthem_last_painted CCM_MEMORY;

void anthem_init()
{
    song_speed = 4;
    track_length = 32;
    anthem_color[1] = 0;
    anthem_color[0] = 1;
}

void anthem_reset()
{
    song_length = 16;
    song_speed = 4;
    track_length = 32;
}

void anthem_render(uint8_t value, int x, int y)
{
    value &= 15;
    uint32_t *dst = (uint32_t *)draw_buffer + x/2;
    uint8_t row = (font[hex[value]] >> (((y/2)*4))) & 15;
    const uint32_t color_choice[2] = { 
        palette[value] | (palette[value]<<16),
        ~(palette[value] | (palette[value]<<16))
    };
    *(++dst) = color_choice[0];
    *(++dst) = color_choice[0];
    for (int k=0; k<4; ++k)
    {
        *(++dst) = color_choice[row&1];
        row >>= 1;
    }
    *(++dst) = color_choice[0];
    *(++dst) = color_choice[0];
}

void anthem_line()
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
        if (anthem_menu_not_edit)
        {
            if (line == 0) 
            {
                uint32_t *dst = (uint32_t *)draw_buffer + 57;
                const uint32_t color = BOX_COLOR;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
                *(++dst) = color;
            }
        }
        else if (line > 2 && line < 7)
        {
            if (line - 3 == verse_player)
            {
                uint32_t *dst = (uint32_t *)draw_buffer + 21 + 8*anthem_song_pos - 8*anthem_song_offset;
                (*dst) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
                (*(++dst)) = BOX_COLOR;
            }
            else if (line == 6 && internal_line == 9)
            {
                uint32_t *dst = (uint32_t *)draw_buffer + 21 + 8*anthem_song_pos - 8*anthem_song_offset;
                (*dst) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
                (*(++dst)) = MATRIX_WING_COLOR;
            }
        }
        return;
    }
    --internal_line;
    uint8_t buffer[24];
    switch (line)
    {
        case 0:
        {
            // edit track
            uint8_t msg[] = { 'a', 'n', 't', 'h', 'e', 'm',
                ' ', 'X', '0' + anthem_song_pos/10, '0' + anthem_song_pos%10, '/',
                '0' + song_length/10, '0' + song_length%10,
                ' ', 's', 'p', 'e', 'e', 'd', ' ', '0'+(16-song_speed)/10, '0'+(16-song_speed)%10,
                ' ', 't', 'k', 'l', 'e', 'n',' ', '0'+track_length/10, '0'+track_length%10,
            0 };
            font_render_line_doubled(msg, 16, internal_line, 65535, BG_COLOR*257);
            break;
        }
        case 2:
            if (chip_play)
            {
                font_render_line_doubled((const uint8_t *)"P:", 20, internal_line, 65535, BG_COLOR*257);
                uint8_t song_current = song_pos ? song_pos-1 : song_length-1;
                if ((track_pos/4%2 == 0) && song_current >= anthem_song_offset && song_current < anthem_song_offset+16)
                    font_render_line_doubled((const uint8_t *)"*", 28+16+16*song_current - 16*anthem_song_offset, internal_line, BOX_COLOR&65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((const uint8_t *)"P:tracks", 20, internal_line, 65535, BG_COLOR*257);
            
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        {
            int i = line - 3;
            if (i == verse_player)
            {
                uint32_t *dst = (uint32_t *)draw_buffer + 6;
                if ((internal_line+1)/2 == 1)
                {
                    *dst = ~(*dst);
                    ++dst;
                    *dst = ~(*dst);
                }
                else if ((internal_line+1)/2 == 3)
                {
                    *dst = 16843009u*BG_COLOR;
                    ++dst;
                    *dst = 16843009u*BG_COLOR;
                }
            }
            {
                uint8_t msg[] = { hex[i], ':', 0 };
                font_render_line_doubled(msg, 20, internal_line, 65535, BG_COLOR*257);
                uint32_t *dst = (uint32_t *)draw_buffer + 20;
                uint8_t y = ((internal_line/2))*4; // how much to shift for font row
                uint16_t *value = &chip_song[anthem_song_offset]-1;
                for (int j=0; j<8; ++j)
                {
                    uint8_t command = ((*(++value))>>(4*i))&15;
                    uint8_t row = (font[hex[command]] >> y) & 15;
                    uint32_t color_choice[2];
                    color_choice[0] = palette[command] | (palette[command]<<16);
                    color_choice[1] = ~color_choice[0];

                    *(++dst) = color_choice[0];
                    *(++dst) = color_choice[0];
                    for (int k=0; k<4; ++k)
                    {
                        *(++dst) = color_choice[row&1];
                        row >>= 1;
                    }
                    *(++dst) = color_choice[0];
                    *(++dst) = color_choice[0];

                    command = ((*(++value))>>(4*i))&15;
                    row = (font[hex[command]] >> y) & 15;
                    color_choice[0] = palette[command] | (palette[command]<<16);
                    color_choice[1] = ~color_choice[0];
                    *(++dst) = color_choice[0];
                    *(++dst) = color_choice[0];
                    for (int k=0; k<4; ++k)
                    {
                        *(++dst) = color_choice[row&1];
                        row >>= 1;
                    }
                    *(++dst) = color_choice[0];
                    *(++dst) = color_choice[0];
                }
            }
            if (anthem_menu_not_edit == 0 && i == verse_player)
            {
                const uint16_t color = (anthem_song_offset + 15 == anthem_song_pos) ? 
                    BOX_COLOR :
                    MATRIX_WING_COLOR;
                uint16_t *dst = draw_buffer + 20*2 + 16*8*2;
                *(++dst) = color;
                ++dst;
                *(++dst) = color;
                ++dst;
                *(++dst) = color;
                ++dst;
                *(++dst) = color;
                ++dst;
                *(++dst) = color;
            }

            break;
        }
        case 8:
            if (anthem_menu_not_edit)
                font_render_line_doubled((uint8_t *)"dpad:adjust song length", 16, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"dpad:move cursor", 16, internal_line, 65535, BG_COLOR*257);
            break;
        case 9:
            if (anthem_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"A:save songkit", 16+3*9, internal_line, 65535, BG_COLOR*257);
            }
            else if (chip_play)
                font_render_line_doubled((uint8_t *)"A:stop song", 16+3*9, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"A:play song", 16+3*9, internal_line, 65535, BG_COLOR*257);
            break;
        case 10:
            if (anthem_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"B:load songkit", 16+3*9, internal_line, 65535, BG_COLOR*257);
            }
            else
                font_render_line_doubled((uint8_t *)"X:edit track under cursor", 16+3*9, internal_line, 65535, BG_COLOR*257);
            break;
        case 11:
            if (anthem_menu_not_edit)
            {
                font_render_line_doubled((uint8_t *)"X:load anthem only", 16+3*9, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((uint8_t *)"B:put", 16+3*9, internal_line, 65535, BG_COLOR*257);
                anthem_render(anthem_color[1], 16+3*9+6*9, internal_line);
                if (anthem_last_painted)
                {
                    font_render_line_doubled((uint8_t *)"L:", 150, internal_line, 65535, BG_COLOR*257);
                    anthem_render(anthem_color[1]-1, 150+2*9, internal_line);
                    font_render_line_doubled((uint8_t *)"R:", 200, internal_line, 65535, BG_COLOR*257);
                    anthem_render(anthem_color[1]+1, 200+2*9, internal_line);
                }
            }
            break;
        case 12:
            if (anthem_menu_not_edit)
            {
                strcpy((char *)buffer, "Y:file ");
                strcpy((char *)(buffer+7), base_filename);
                font_render_line_doubled(buffer, 16+3*9, internal_line, 65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((uint8_t *)"Y:put", 16+3*9, internal_line, 65535, BG_COLOR*257);
                anthem_render(anthem_color[0], 16+3*9+6*9, internal_line);
                if (!anthem_last_painted)
                {
                    font_render_line_doubled((uint8_t *)"L:", 150, internal_line, 65535, BG_COLOR*257);
                    anthem_render(anthem_color[0]-1, 150+2*9, internal_line);
                    font_render_line_doubled((uint8_t *)"R:", 200, internal_line, 65535, BG_COLOR*257);
                    anthem_render(anthem_color[0]+1, 200+2*9, internal_line);
                }
            }
            break;
        case 13:
            if (anthem_menu_not_edit)
                font_render_line_doubled((uint8_t *)"L/R:adjust global volume", 16+1*9, internal_line, 65535, BG_COLOR*257);
            break;
        case 15:
            if (anthem_menu_not_edit)
                font_render_line_doubled((uint8_t *)"start:edit anthem", 16, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((uint8_t *)"start:anthem menu", 16, internal_line, 65535, BG_COLOR*257);
            break;        
        case 16:
            if (anthem_menu_not_edit)
                font_render_line_doubled((uint8_t *)"select:verse menu", 16, internal_line, 65535, BG_COLOR*257);
            break;        
        case 18:
            font_render_line_doubled(game_message, 16, internal_line, 65535, BG_COLOR*257);
            break;        
    }
}

uint8_t anthem_song_color()
{
    return (chip_song[anthem_song_pos] >> (verse_player*4))&15;
}

void anthem_song_paint(uint8_t p)
{
    anthem_last_painted = p;

    uint16_t *memory = &chip_song[anthem_song_pos];
    *memory &= ~(15 << (verse_player*4)); // clear out current value there
    *memory |= (anthem_color[p] << (verse_player*4)); // add this value
}

void anthem_controls()
{
    if (anthem_menu_not_edit)
    {
        int movement = 0;
        if (GAMEPAD_PRESSING(0, up))
            ++movement;
        if (GAMEPAD_PRESSING(0, down))
            --movement;
        if (GAMEPAD_PRESSING(0, left))
            --movement;
        if (GAMEPAD_PRESSING(0, right))
            ++movement;
        if (movement)
        {
            game_message[0] = 0;
            song_length += movement;
            if (song_length < 16)
                song_length = 16;
            else if (song_length > MAX_SONG_LENGTH)
                song_length = MAX_SONG_LENGTH;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }
        
        int save_or_load = 0;
        if (GAMEPAD_PRESS(0, A))
            save_or_load = 1; // save
        if (GAMEPAD_PRESS(0, B))
            save_or_load = 2; // load
        if (save_or_load)
        {
            FileError error = BotchedIt;
            if (save_or_load == 1)
            {
                error = io_save_anthem();
                if (error)
                {
                    strcpy((char *)game_message, "anthem ");
                    io_message_from_error(game_message+7, error, 1);
                    return;
                }
                error = io_save_verse(16);

                if (error)
                {
                    strcpy((char *)game_message, "track ");
                    io_message_from_error(game_message+6, error, 1);
                    return;
                }

                error = io_save_instrument(16);
                if (error)
                {
                    strcpy((char *)game_message, "instr. ");
                    io_message_from_error(game_message+7, error, 1);
                }
                else
                {
                    io_message_from_error(game_message, NoError, 1);
                }
            }
            else
            {
                error = io_load_anthem();
                if (error)
                {
                    strcpy((char *)game_message, "anthem ");
                    io_message_from_error(game_message+7, error, 2);
                    return;
                }
                error = io_load_verse(16);

                if (error)
                {
                    strcpy((char *)game_message, "track ");
                    io_message_from_error(game_message+6, error, 2);
                    return;
                }

                error = io_load_instrument(16);
                if (error)
                {
                    strcpy((char *)game_message, "instr. ");
                    io_message_from_error(game_message+7, error, 2);
                }
                else
                {
                    io_message_from_error(game_message, NoError, 2);
                }
            }
            return;
        }

        if (GAMEPAD_PRESSING(0, L))
        {
            if (chip_volume > 4)
                chip_volume -= 4;
            else if (chip_volume)
                --chip_volume;
            --movement;
        }
        if (GAMEPAD_PRESSING(0, R))
        {
            if (chip_volume < 252)
                chip_volume += 4;
            else if (chip_volume < 255)
                ++chip_volume;
            ++movement;
        } 
        if (movement) 
        {
            strcpy((char *)game_message, "global volume to ");
            game_message[17] = hex[chip_volume/16];
            game_message[18] = hex[chip_volume%16];
            game_message[19] = 0;
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            return;
        }

        if (GAMEPAD_PRESS(0, X))
        {
            // load just anthem
            io_message_from_error(game_message, io_load_anthem(), 2);
            return;
        }

        if (GAMEPAD_PRESS(0, Y))
        {
            // switch to choose name and hope to come back
            game_message[0] = 0;
            game_switch(ChooseFilename);
            previous_visual_mode = EditAnthem;
            return;
        }
    }
    else // editing, not menu
    {
        int paint_if_moved = 0; 
        if (GAMEPAD_PRESSING(0, Y))
        {
            anthem_song_paint(0);
            paint_if_moved = 1;
        }
        if (GAMEPAD_PRESSING(0, B))
        {
            anthem_song_paint(1);
            paint_if_moved = 2;
        }

        int switched = 0;
        if (GAMEPAD_PRESSING(0, L))
            --switched;
        if (GAMEPAD_PRESSING(0, R))
            ++switched;
        if (switched)
            anthem_color[anthem_last_painted] = (anthem_color[anthem_last_painted]+switched)&15;
        
        int moved = 0;
        if (GAMEPAD_PRESSING(0, down))
        {
            verse_player = (verse_player+1)&3;
            moved = 1;
        }
        if (GAMEPAD_PRESSING(0, up))
        {
            verse_player = (verse_player-1)&3;
            moved = 1;
        }
        if (GAMEPAD_PRESSING(0, left))
        {
            if (--anthem_song_pos >= song_length)
            {
                anthem_song_pos = song_length - 1;
                anthem_song_offset = song_length - 16;
            }
            else if (anthem_song_pos < anthem_song_offset)
                anthem_song_offset = anthem_song_pos;
            moved = 1;
        }
        if (GAMEPAD_PRESSING(0, right))
        {
            if (++anthem_song_pos >= song_length)
            {
                anthem_song_pos = 0;
                anthem_song_offset = 0;
            }
            else if (anthem_song_pos > anthem_song_offset+15)
                anthem_song_offset = anthem_song_pos - 15;
            moved = 1;
        }
        if (moved)
        {
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;
            if (paint_if_moved)
                anthem_song_paint(paint_if_moved-1);
        }
        else if (switched || paint_if_moved)
            gamepad_press_wait = GAMEPAD_PRESS_WAIT;

        if (GAMEPAD_PRESS(0, A))
        {
            track_pos = 0;
            if (chip_play)
                chip_kill();
            else
                chip_play_init(anthem_song_pos);
        } 
        
        if (GAMEPAD_PRESS(0, X))
        {
            verse_track = anthem_song_color();
            game_switch(EditVerse);
            return;
        }
    }
    
    if (GAMEPAD_PRESS(0, start))
    {
        game_message[0] = 0;
        anthem_menu_not_edit = 1 - anthem_menu_not_edit; 
        chip_kill();
        chip_play = 0;
        return;
    }

    if (GAMEPAD_PRESS(0, select))
    {
        game_message[0] = 0;
        previous_visual_mode = None;
        if (anthem_menu_not_edit)
        {
            verse_menu_not_edit = 1;
            game_switch(EditVerse);
        }
        else
        {
            game_switch(SaveLoadScreen);
        }
        return;
    } 
}

