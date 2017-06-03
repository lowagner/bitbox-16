#ifndef IO_H
#define IO_H

#include <stdint.h>
#define MAX_FILES 1024

typedef enum {
    NoError = 0,
    MountError,
    ConstraintError,
    OpenError,
    ReadError,
    WriteError,
    NoDataError,
    MissingDataError,
    BotchedIt
} FileError;

FileError io_init();
FileError io_set_recent_filename();
FileError io_get_recent_filename();
void io_message_from_error(uint8_t *msg, FileError error, int save_not_load);
FileError io_save_palette();
FileError io_load_palette();
FileError io_save_tile(unsigned int i);
FileError io_load_tile(unsigned int i);
FileError io_save_sprite(unsigned int i);
FileError io_load_sprite(unsigned int i);
FileError io_save_map();
FileError io_load_map();
FileError io_save_instrument(unsigned int i);
FileError io_load_instrument(unsigned int i);
FileError io_save_verse(unsigned int i);
FileError io_load_verse(unsigned int i);
FileError io_save_anthem();
FileError io_load_anthem();
FileError io_save_go(unsigned int i);
FileError io_load_go(unsigned int i);
FileError io_load_unlocks(unsigned int i);
FileError io_save_unlocks(unsigned int i);
void io_list_games();
void io_next_available_filename();
void io_previous_available_filename();

#endif
