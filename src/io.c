#include "bitbox.h"
#include "chiptune.h"
#include "common.h"
#include "tiles.h"
#include "name.h"
#include "io.h"
#include "unlocks.h"

#include "fatfs/ff.h"
#include <stdlib.h> // qsort
#include <strings.h> // strcasecmp

#define PALETTE_LENGTH (sizeof(palette))
#define INSTRUMENT_OFFSET (PALETTE_LENGTH)
#define INSTRUMENT_LENGTH (16+sizeof(instrument))
#define VERSE_OFFSET (INSTRUMENT_OFFSET + INSTRUMENT_LENGTH)
#define VERSE_LENGTH (sizeof(chip_track))
#define ANTHEM_OFFSET (VERSE_OFFSET + VERSE_LENGTH)
#define ANTHEM_LENGTH (1 + sizeof(chip_song))
#define GO_OFFSET (ANTHEM_OFFSET + ANTHEM_LENGTH)
#define GO_LENGTH (sizeof(sprite_pattern))
#define UNLOCKS_OFFSET (GO_OFFSET + GO_LENGTH)
#define UNLOCKS_LENGTH (sizeof(unlocks))
#define TILE_OFFSET (UNLOCKS_OFFSET + UNLOCKS_LENGTH)
#define TILE_LENGTH (sizeof(tile_info)+sizeof(tile_draw))
#define SPRITE_OFFSET (TILE_OFFSET + TILE_LENGTH)
#define SPRITE_LENGTH (sizeof(sprite_info)+sizeof(sprite_draw))
#define MAP_OFFSET (SPRITE_OFFSET + SPRITE_LENGTH)
#define MAP_LENGTH (4+TILE_MAP_MEMORY+1+3*2*MAX_OBJECTS)
#define TOTAL_FILE_SIZE (MAP_OFFSET+MAP_LENGTH)

#define SAVE_INDEXED(name, stride, total) \
    FileError ferr = io_set_recent_filename(); \
    if (ferr) \
        return ferr; \
    ferr = io_open_or_zero_file(full_filename, TOTAL_FILE_SIZE); \
    if (ferr) \
        return ferr; \
    if (i >= (total)) \
    { \
        f_lseek(&fat_file, name##_OFFSET); \
        for (i=0; i<(total); ++i) \
          if ((ferr = _io_save_##name(i))) \
            break; \
    } \
    else \
    { \
        f_lseek(&fat_file, name##_OFFSET + i*(stride));  \
        ferr = _io_save_##name(i); \
    } \
    f_close(&fat_file); \
    return ferr

#define LOAD_INDEXED(name, stride, total) \
    FileError ferr = io_set_recent_filename(); \
    if (ferr) \
        return ferr; \
    fat_result = f_open(&fat_file, full_filename, FA_READ | FA_OPEN_EXISTING); \
    if (fat_result != FR_OK) \
        return OpenError; \
    if (i >= (total)) \
    { \
        f_lseek(&fat_file, name##_OFFSET); \
        for (i=0; i<(total); ++i) \
          if ((ferr = _io_load_##name(i))) \
            break; \
    } \
    else \
    { \
        f_lseek(&fat_file, name##_OFFSET + i*(stride));  \
        ferr = _io_load_##name(i); \
    } \
    f_close(&fat_file); \
    return ferr

int io_mounted = 0;
FATFS fat_fs;
FIL fat_file;
FRESULT fat_result;
char old_base_filename[9] CCM_MEMORY;
uint8_t old_chip_volume CCM_MEMORY;
char full_filename[13] CCM_MEMORY;

char available_filenames[MAX_FILES+1][8];
int available_count;
int available_index;

void io_message_from_error(uint8_t *msg, FileError error, int save_not_load)
{
    switch (error)
    {
    case NoError:
        if (save_not_load == 1)
            strcpy((char *)msg, "saved!");
        else
            strcpy((char *)msg, "loaded!");
        break;
    case MountError:
        strcpy((char *)msg, "fs unmounted!");
        break;
    case ConstraintError:
        strcpy((char *)msg, "unconstrained!");
        break;
    case OpenError:
        strcpy((char *)msg, "no open!");
        break;
    case ReadError:
        strcpy((char *)msg, "no read!");
        break;
    case WriteError:
        strcpy((char *)msg, "no write!");
        break;
    case NoDataError:
        strcpy((char *)msg, "no data!");
        break;
    case MissingDataError:
        strcpy((char *)msg, "miss data!");
        break;
    case BotchedIt:
        strcpy((char *)msg, "fully bungled.");
        break;
    }
    game_message_timeout = MESSAGE_TIMEOUT;
}

FileError io_init()
{
    message("total file size is %d\n", TOTAL_FILE_SIZE);
    available_count = 0;

    old_base_filename[0] = 0;
    fat_result = f_mount(&fat_fs, "", 1); // mount now...
    if (fat_result != FR_OK)
    {
        io_mounted = 0;
        return MountError;
    }
    else
    {
        io_mounted = 1;
        return NoError;
    }
}

FileError io_open_or_zero_file(const char *fname, unsigned int size)
{
    if (size == 0)
        return NoDataError;

    fat_result = f_open(&fat_file, fname, FA_WRITE | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
    {
        fat_result = f_open(&fat_file, fname, FA_WRITE | FA_OPEN_ALWAYS);
        if (fat_result != FR_OK)
            return OpenError;
        uint8_t zero[128] = {0};
        while (size) 
        {
            int write_size;
            if (size <= 128)
            {
                write_size = size;
                size = 0;
            }
            else
            {
                write_size = 128;
                size -= 128;
            }
            UINT bytes_get;
            f_write(&fat_file, zero, write_size, &bytes_get);
            if (bytes_get != write_size)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
    }
    return NoError;
}

FileError io_set_recent_filename()
{
    int filename_len = strlen(base_filename);
    if (filename_len == 0)
        return ConstraintError;

    if (io_mounted == 0)
    {
        if (io_init())
            return MountError;
    }
    
    if (strcmp(old_base_filename, base_filename) == 0 && chip_volume == old_chip_volume)
        return NoError; // don't rewrite  

    for (int i=0; i<8; ++i)
    {
        if (base_filename[i] != 0)
        {
            full_filename[i] = base_filename[i];
            continue;
        }
        full_filename[i] = '.';
        full_filename[++i] = 'G';
        full_filename[++i] = '1';
        full_filename[++i] = '6';
        full_filename[++i] = 0;
        break;
    }

    fat_result = f_open(&fat_file, "RECENT16.TXT", FA_WRITE | FA_CREATE_ALWAYS); 
    if (fat_result != FR_OK) 
        return OpenError;
    UINT bytes_get;
    fat_result = f_write(&fat_file, base_filename, filename_len, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != filename_len)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    strcpy(old_base_filename, base_filename);
    uint8_t msg[2] = { '\n', chip_volume };
    fat_result = f_write(&fat_file, msg, 2, &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != 2)
        return MissingDataError;
    old_chip_volume = chip_volume;

    return NoError;
}

FileError io_get_recent_filename()
{
    if (io_mounted == 0)
    {
        if (io_init())
            return MountError;
    }

    fat_result = f_open(&fat_file, "RECENT16.TXT", FA_READ | FA_OPEN_EXISTING); 
    if (fat_result != FR_OK) 
        return OpenError;

    UINT bytes_get;
    uint8_t buffer[10];
    fat_result = f_read(&fat_file, &buffer[0], 10, &bytes_get); 
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get == 0)
        return NoDataError;
    int i=0;
    while (i < bytes_get)
    {
        if (buffer[i] == '\n')
        {
            base_filename[i] = 0;
            if (++i < bytes_get)
                chip_volume = buffer[i];
            return NoError;
        }
        if (i < 8)
            base_filename[i] = buffer[i];
        else
        {
            base_filename[8] = 0;
            return ConstraintError;
        }
        ++i;
    }
    base_filename[i] = 0;
    return NoError;
}

FileError io_save_palette()
{
    FileError ferr = io_set_recent_filename();
    if (ferr)
        return ferr;

    ferr = io_open_or_zero_file(full_filename, TOTAL_FILE_SIZE);
    if (ferr)
        return ferr;

    UINT bytes_get; 
    fat_result = f_write(&fat_file, palette, PALETTE_LENGTH, &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return WriteError;

    if (bytes_get != sizeof(palette))
        return MissingDataError;
    return NoError;
}

FileError io_load_palette()
{
    FileError ferr = io_set_recent_filename();
    if (ferr)
        return ferr;
    
    fat_result = f_open(&fat_file, full_filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

    UINT bytes_get; 
    fat_result = f_read(&fat_file, palette, PALETTE_LENGTH, &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return ReadError;

    if (bytes_get != sizeof(palette))
        return MissingDataError;
    return NoError;
}

FileError _io_save_TILE(unsigned int i)
{
    // i must be between 0 and 15
    // file MUST be open, and seeked to the right spot.

    UINT bytes_get; 
    fat_result = f_write(&fat_file, &tile_info[i], 4, &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != 4)
        return MissingDataError;
    fat_result = f_write(&fat_file, tile_draw[i], sizeof(tile_draw[0]), &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != sizeof(tile_draw[0]))
        return MissingDataError;
    
    return NoError;
}

FileError _io_load_TILE(unsigned int i)
{
    // i must be between 0 and 15
    // file MUST be open, and seeked to the right spot.

    UINT bytes_get;
    fat_result = f_read(&fat_file, &tile_info[i], 4, &bytes_get);
    if (fat_result != FR_OK)
        return ReadError; 
    else if (bytes_get != 4)
        return MissingDataError;
    fat_result = f_read(&fat_file, tile_draw[i], sizeof(tile_draw[0]), &bytes_get);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != sizeof(tile_draw[0]))
        return MissingDataError;
    
    return NoError;
}

FileError io_save_tile(unsigned int i)
{
    SAVE_INDEXED(TILE, sizeof(tile_draw[0])+4, 16);
}

FileError io_load_tile(unsigned int i) 
{
    LOAD_INDEXED(TILE, sizeof(tile_draw[0])+4, 16);
}

FileError _io_save_SPRITE(unsigned int i)
{
    // i must be between 0 and 127
    // file MUST be open, and seeked to the right spot.

    UINT bytes_get; 
    fat_result = f_write(&fat_file, &sprite_info[i], 4, &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != 4)
        return MissingDataError;
    fat_result = f_write(&fat_file, sprite_draw[i], sizeof(sprite_draw[0]), &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != sizeof(sprite_draw[0]))
        return MissingDataError;
    
    return NoError;
}

FileError _io_load_SPRITE(unsigned int i)
{
    // i must be between 0 and 127
    // file MUST be open, and seeked to the right spot.

    UINT bytes_get;
    fat_result = f_read(&fat_file, &sprite_info[i], 4, &bytes_get);
    if (fat_result != FR_OK)
        return ReadError; 
    else if (bytes_get != 4)
        return MissingDataError;
    fat_result = f_read(&fat_file, sprite_draw[i], sizeof(sprite_draw[0]), &bytes_get);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != sizeof(sprite_draw[0]))
        return MissingDataError;
    
    return NoError;
}

FileError io_save_sprite(unsigned int i)
{
    SAVE_INDEXED(SPRITE, sizeof(sprite_draw[0])+4, 128);
}

FileError io_load_sprite(unsigned int i) 
{
    LOAD_INDEXED(SPRITE, sizeof(sprite_draw[0])+4, 128);
}

FileError io_save_map()
{
    if (tile_map_height <= 0 || tile_map_width <= 0 || 
        ((tile_map_width*tile_map_height+1)/2 > TILE_MAP_MEMORY))
        return ConstraintError;
    
    FileError ferr = io_set_recent_filename();
    if (ferr)
        return ferr;
    
    ferr = io_open_or_zero_file(full_filename, TOTAL_FILE_SIZE);
    if (ferr)
        return ferr;

    f_lseek(&fat_file, MAP_OFFSET);

    UINT bytes_get; 
    // write width and height
    fat_result = f_write(&fat_file, &tile_map_width, 2, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 2)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    fat_result = f_write(&fat_file, &tile_map_height, 2, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 2)
    {
        f_close(&fat_file);
        return MissingDataError;
    }

    // write the tile map
    int size = (tile_map_width*tile_map_height+1)/2;
    uint8_t *src = tile_map;
    while (size) 
    {
        int write_size;
        if (size <= 128)
        {
            write_size = size;
            size = 0;
        }
        else
        {
            write_size = 128;
            size -= 128;
        }
        UINT bytes_get;
        f_write(&fat_file, src, write_size, &bytes_get);
        if (bytes_get != write_size)
        {
            f_close(&fat_file);
            return MissingDataError;
        }
        src += write_size;
    } 
    
    // write the sprites
    uint8_t object_count=0;
    uint8_t index = first_used_object_index;
    while (index < 255)
    {
        // first pass to determine object count
        ++object_count;
        index = object[index].next_object_index;
    }
    fat_result = f_write(&fat_file, &object_count, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    index = first_used_object_index;
    while (index < 255)
    {
        // now write sprite information
        struct object *o = &object[index];
        uint16_t szxy[3] = 
        { 
            o->sprite_index + (o->z << 8),
            (uint16_t)o->x, (uint16_t)o->y 
        };
        fat_result = f_write(&fat_file, &szxy[0], 6, &bytes_get);
        if (fat_result != FR_OK)
        {
            f_close(&fat_file);
            return WriteError;
        }
        if (bytes_get != 6)
        {
            f_close(&fat_file);
            return MissingDataError;
        }
        index = object[index].next_object_index;
    } 
    
    f_close(&fat_file);
    
    map_modified = 0;

    return NoError;
}

FileError io_load_map()
{
    map_modified = 0;
    sprites_init(); // destroy all sprites

    FileError ferr = io_set_recent_filename();
    if (ferr)
        return ferr;
    
    fat_result = f_open(&fat_file, full_filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

    f_lseek(&fat_file, MAP_OFFSET);

    UINT bytes_get; 
    fat_result = f_read(&fat_file, &tile_map_width, 2, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != 2)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    if (tile_map_width <= 0)
        return ConstraintError;
    fat_result = f_read(&fat_file, &tile_map_height, 2, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != 2)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    if (tile_map_height <= 0)
        return ConstraintError;
    
    int size = (tile_map_width*tile_map_height+1)/2;
    if (size > TILE_MAP_MEMORY)
        return ConstraintError;
    uint8_t *src = tile_map;
    while (size) 
    {
        int read_size;
        if (size <= 128)
        {
            read_size = size;
            size = 0;
        }
        else
        {
            read_size = 128;
            size -= 128;
        }
        UINT bytes_get;
        f_read(&fat_file, src, read_size, &bytes_get);
        if (bytes_get != read_size)
        {
            f_close(&fat_file);
            return MissingDataError;
        }
        src += read_size;
    } 
    uint8_t object_count;
    fat_result = f_read(&fat_file, &object_count, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    for (int i=0; i<object_count; ++i)
    {
        // sprite 
        uint16_t szxy[3];
        fat_result = f_read(&fat_file, &szxy[0], 6, &bytes_get);
        if (fat_result != FR_OK)
        {
            f_close(&fat_file);
            return ReadError;
        }
        if (bytes_get != 6)
        {
            f_close(&fat_file);
            return MissingDataError;
        }
        uint8_t sprite = szxy[0]&127;
        uint8_t z = szxy[0]>>8;
        uint8_t index = create_object(sprite, (int16_t)szxy[1], (int16_t)szxy[2], z);
        if (index == 255)
        {
            f_close(&fat_file);
            return ConstraintError;
        }
    } 
    f_close(&fat_file);
    return NoError;
}

FileError _io_load_INSTRUMENT(unsigned int i)
{
    UINT bytes_get;
    uint8_t read;
    fat_result = f_read(&fat_file, &read, 1, &bytes_get);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != 1)
        return MissingDataError;
    instrument[i].is_drum = read&15;
    instrument[i].octave = read >> 4;
    fat_result = f_read(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != MAX_INSTRUMENT_LENGTH)
        return MissingDataError;
    
    return NoError;
}

FileError _io_save_INSTRUMENT(unsigned int i)
{
    UINT bytes_get; 
    uint8_t write = (instrument[i].is_drum ? 1 : 0) | (instrument[i].octave << 4);
    fat_result = f_write(&fat_file, &write, 1, &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != 1)
        return MissingDataError;
    fat_result = f_write(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != MAX_INSTRUMENT_LENGTH)
        return MissingDataError;
    
    return NoError;
}

FileError io_load_instrument(unsigned int i)
{
    LOAD_INDEXED(INSTRUMENT, MAX_INSTRUMENT_LENGTH+1, 16);
}

FileError io_save_instrument(unsigned int i)
{
    SAVE_INDEXED(INSTRUMENT, MAX_INSTRUMENT_LENGTH+1, 16);
}

FileError _io_load_VERSE(unsigned int i)
{
    UINT bytes_get; 
    fat_result = f_read(&fat_file, chip_track[i], sizeof(chip_track[0]), &bytes_get);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != sizeof(chip_track[0]))
        return MissingDataError;
    return NoError;
}

FileError _io_save_VERSE(unsigned int i)
{
    UINT bytes_get; 
    fat_result = f_write(&fat_file, chip_track[i], sizeof(chip_track[0]), &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != sizeof(chip_track[0]))
        return MissingDataError;
    return NoError;
}

FileError io_load_verse(unsigned int i)
{
    LOAD_INDEXED(VERSE, sizeof(chip_track[0]), 16);
}

FileError io_save_verse(unsigned int i)
{
    SAVE_INDEXED(VERSE, sizeof(chip_track[0]), 16);
}

FileError io_load_anthem()
{
    // set some defaults
    track_length = 16;
    song_speed = 4;
    song_length = 16;

    FileError ferr = io_set_recent_filename();
    if (ferr)
        return ferr;

    fat_result = f_open(&fat_file, full_filename, FA_READ | FA_OPEN_EXISTING); 
    if (fat_result)
        return OpenError;
    
    f_lseek(&fat_file, ANTHEM_OFFSET);

    UINT bytes_get; 
    fat_result = f_read(&fat_file, &song_length, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return MissingDataError;
    }

    if (song_length < 16 || song_length > MAX_SONG_LENGTH)
    {
        message("got song length %d\n", song_length);
        song_length = 16;
        f_close(&fat_file);
        return ConstraintError;
    }
    
    fat_result = f_read(&fat_file, &chip_song[0], 2*song_length, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != 2*song_length)
    {
        f_close(&fat_file);
        return MissingDataError;
    }

    f_close(&fat_file);
    return NoError;
}

FileError io_save_anthem()
{
    FileError ferr = io_set_recent_filename();
    if (ferr)
        return ferr;

    ferr = io_open_or_zero_file(full_filename, TOTAL_FILE_SIZE);
    if (ferr)
        return ferr;
    
    f_lseek(&fat_file, ANTHEM_OFFSET);

    UINT bytes_get; 
    fat_result = f_write(&fat_file, &song_length, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    
    fat_result = f_write(&fat_file, &chip_song[0], 2*song_length, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 2*song_length)
    {
        f_close(&fat_file);
        return MissingDataError;
    }

    f_close(&fat_file);
    return NoError;
}

FileError _io_save_GO(unsigned int i)
{
    UINT bytes_get; 
    fat_result = f_write(&fat_file, sprite_pattern[i], sizeof(sprite_pattern[0]), &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != sizeof(sprite_pattern[0]))
        return MissingDataError;
    return NoError;
}

FileError _io_load_GO(unsigned int i)
{
    UINT bytes_get; 
    fat_result = f_read(&fat_file, sprite_pattern[i], sizeof(sprite_pattern[0]), &bytes_get);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != sizeof(sprite_pattern[0]))
        return MissingDataError;
    return NoError;
}

FileError io_save_go(unsigned int i)
{
    SAVE_INDEXED(GO, sizeof(sprite_pattern[0]), 16);
}

FileError io_load_go(unsigned int i)
{
    LOAD_INDEXED(GO, sizeof(sprite_pattern[0]), 16);
}

FileError _io_save_UNLOCKS(unsigned int i)
{
    UINT bytes_get; 
    fat_result = f_write(&fat_file, unlocks[i], sizeof(unlocks[0]), &bytes_get);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != sizeof(unlocks[0]))
        return MissingDataError;
    return NoError;
}

FileError _io_load_UNLOCKS(unsigned int i)
{
    UINT bytes_get; 
    fat_result = f_read(&fat_file, unlocks[i], sizeof(unlocks[0]), &bytes_get);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != sizeof(unlocks[0]))
        return MissingDataError;
    return NoError;
}

FileError io_save_unlocks(unsigned int i)
{
    SAVE_INDEXED(UNLOCKS, sizeof(unlocks[0]), 8);
}

FileError io_load_unlocks(unsigned int i)
{
    LOAD_INDEXED(UNLOCKS, sizeof(unlocks[0]), 8);
}

#ifdef EMULATOR
#define ROOT_DIR "."
#else
#define ROOT_DIR ""
#endif

static int cmp(const void *p1, const void *p2){
    return strncmp( (char * const ) p1, (char * const ) p2, 8);
}

void io_list_games()
{
    available_count = 0;
    message("listing games\n");

    DIR dir;
    if (f_opendir(&dir, ROOT_DIR) != FR_OK) 
        return set_game_message_timeout("couldn't open dir!", MESSAGE_TIMEOUT);
   
    while (1)
    {
        FILINFO fno;
        #if _USE_LFN
        char lfname[sizeof(TCHAR) * 100];
        fno.lfname = lfname;
        fno.lfsize = 100;
        #endif
        if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == 0)
            break;
        
        char *fn = fno.fname;
        /* Ignore dot entry and dirs */
        if (fn[0] == '.') continue;
        if (fno.fattrib & AM_DIR) 
            continue;

        // check extension : only keep .G16
        int i_period=0;
        while (fn[i_period] != 0 && fn[i_period] != '.')
        {
            ++i_period;
        }
        if (fn[i_period] == 0) continue;
        if (strcasecmp(fn + i_period, ".G16")) continue;

        int i;
        for (i=0; i<i_period; ++i)
            available_filenames[available_count][i] = fn[i];
        available_filenames[available_count][i] = 0; // ok if i == 8, we'll overwrite it on next file.
        if (++available_count >= MAX_FILES)
            break;
    }

    f_closedir(&dir);
    if (!available_count)
        return;

    // sort it
    qsort(available_filenames, available_count, 8, cmp);
    for (int i=0; i<available_count; ++i)
    {
        char end = available_filenames[i+1][0];
        available_filenames[i+1][0] = 0;
        message("got filename %d: %s\n", i, available_filenames[i]);
        available_filenames[i+1][0] = end;
    }
    
    // find recent file, if possible.
    char *recent = bsearch(base_filename, available_filenames, available_count, 8, cmp);
    if (recent == NULL)
        available_index = 0;
    else
        available_index = (int)(recent - available_filenames[0])/8;
    message("got recent file offset %d\n", available_index);
}

void io_next_available_filename()
{
    if (!available_count)
        return;
    if (available_index + 1 >= available_count)
        available_index = 0;
    else
        ++available_index;
    strncpy(base_filename, available_filenames[available_index], 8);
    base_filename[8] = 0;
}

void io_previous_available_filename()
{
    if (!available_count)
        return;
    if (available_index)
        --available_index;
    else
        available_index = available_count-1;
    strncpy(base_filename, available_filenames[available_index], 8);
    base_filename[8] = 0;
}
