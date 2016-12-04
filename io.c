#include "bitbox.h"
#include "chiptune.h"
#include "common.h"
#include "go-sprite.h"
#include "tiles.h"
#include "name.h"
#include <math.h>
#include "io.h"

#include "fatfs/ff.h"
#include <string.h> // strlen

#define INSTRUMENT_DATA_LENGTH (8+16*(MAX_INSTRUMENT_LENGTH*8+8))
#define VERSE_DATA_LENGTH (8+16*(MAX_TRACK_LENGTH*16+8))
#define ANTHEM_DATA_LENGTH (8+MAX_SONG_LENGTH*(8))
#define PALETTE_DATA_LENGTH (8+16*(8))
#define TILE_DATA_LENGTH (8+16*(16*18+20+8))
#define SPRITE_DATA_LENGTH (8+16*8*(16*18+20+8))
#define GO_DATA_LENGTH (8+16*(32*8+8))
#define UNLOCK_DATA_LENGTH (8+8*(16*8+8))
#define MAP_DATA_LENGTH (8+10+2*TILE_MAP_MEMORY)
#define LOCATION_DATA_LENGTH (8+12*MAX_OBJECTS) // sprite location

#define INSTRUMENT_OFFSET (4)
#define VERSE_OFFSET (INSTRUMENT_OFFSET + INSTRUMENT_DATA_LENGTH)
#define ANTHEM_OFFSET (VERSE_OFFSET + VERSE_DATA_LENGTH)
#define PALETTE_OFFSET (ANTHEM_OFFSET + ANTHEM_DATA_LENGTH)
#define TILE_OFFSET (PALETTE_OFFSET + PALETTE_DATA_LENGTH)
#define SPRITE_OFFSET (TILE_OFFSET + TILE_DATA_LENGTH)
#define GO_OFFSET (SPRITE_OFFSET + SPRITE_DATA_LENGTH)
#define UNLOCK_OFFSET (GO_OFFSET + GO_DATA_LENGTH)
#define MAP_OFFSET (UNLOCK_OFFSET + UNLOCK_DATA_LENGTH)
#define LOCATION_OFFSET (MAP_OFFSET + MAP_DATA_LENGTH)
#define FILE_SIZE (LOCATION_OFFSET + LOCATION_DATA_LENGTH)

int io_mounted = 0;
FATFS fat_fs;
FIL fat_file;
FRESULT fat_result;
char old_base_filename[9] CCM_MEMORY;
uint8_t old_chip_volume CCM_MEMORY;
       
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
}

FileError io_init()
{
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

static inline FileError file_write(void *v, unsigned int length)
{
    UINT bytes_get; 
    fat_result = f_write(&fat_file, v, length, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    /*
    if (bytes_get != length)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    */
    return NoError;
}

FileError io_set_extension(char *filename, const char *ext)
{
    if (io_mounted == 0)
    {
        if (io_init())
            return MountError;
    }
    io_set_recent_filename();

    int k = 0;
    for (; k<8 && base_filename[k]; ++k)
    { 
        filename[k] = base_filename[k];
    }
    filename[k] = '.';
    filename[++k] = *ext;
    filename[++k] = *++ext;
    filename[++k] = *++ext;
    filename[++k] = 0;
    return NoError;
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
        uint8_t zero[256] = {0};
        while (size) 
        {
            int write_size;
            if (size <= sizeof(zero))
            {
                write_size = size;
                size = 0;
            }
            else
            {
                write_size = sizeof(zero);
                size -= sizeof(zero);
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

FileError io_load_palette()
{
    char filename[13];
    if (io_set_extension(filename, "P16"))
        return MountError;
    
    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

    UINT bytes_get; 
    fat_result = f_read(&fat_file, palette, sizeof(palette), &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return ReadError;

    if (bytes_get != sizeof(palette))
        return MissingDataError;
    return NoError;
}

FileError io_load_tile(unsigned int i) 
{
    char filename[13];
    if (io_set_extension(filename, "T16"))
        return MountError;
    
    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;
    
    if (i >= 16)
    {
        // read them all
        for (i=0; i<16; ++i)
        {
            UINT bytes_get;
            uint32_t info;
            fat_result = f_read(&fat_file, &info, 4, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError; 
            }
            else if (bytes_get != 4)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
            // if someone is wanting to convert their tile data, write a routine here
            fat_result = f_read(&fat_file, tile_draw[i], sizeof(tile_draw[0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != sizeof(tile_draw[0]))
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }
    // i < 16
    f_lseek(&fat_file, i*(sizeof(tile_draw[0])+4)); 
    UINT bytes_get;
    uint32_t info;
    fat_result = f_read(&fat_file, &info, 4, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError; 
    }
    else if (bytes_get != 4)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    // if someone is wanting to convert their tile data, write a routine here
    fat_result = f_read(&fat_file, tile_draw[i], sizeof(tile_draw[0]), &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != sizeof(tile_draw[0]))
        return MissingDataError;
    return NoError;
}

FileError io_load_sprite(unsigned int i, unsigned int f) 
{
    char filename[13];
    if (io_set_extension(filename, "S16"))
        return MountError;
        
    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;
    
    if (i >= 16)
    {
        // read them all
        for (i=0; i<16; ++i)
        for (int f=0; f<8; ++f)
        {
            UINT bytes_get;
            uint32_t info;
            fat_result = f_read(&fat_file, &info, 4, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError; 
            }
            else if (bytes_get != 4)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
            fat_result = f_read(&fat_file, sprite_draw[i][f], sizeof(sprite_draw[0][0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != sizeof(sprite_draw[0][0]))
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }
    // i < 16
    if (f >= 8)
    {
        f_lseek(&fat_file, i*8*(sizeof(sprite_draw[0][0])+4));
        for (f=0; f<8; ++f) // frame
        {
            UINT bytes_get;
            uint32_t info;
            fat_result = f_read(&fat_file, &info, 4, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError; 
            }
            else if (bytes_get != 4)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
            fat_result = f_read(&fat_file, sprite_draw[i][f], sizeof(sprite_draw[0][0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != sizeof(sprite_draw[0][0]))
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }
    // f < 8
    f_lseek(&fat_file, (i*8 + f)*(sizeof(sprite_draw[0][0])+4)); 
    UINT bytes_get;
    uint32_t info;
    fat_result = f_read(&fat_file, &info, 4, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError; 
    }
    else if (bytes_get != 4)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    fat_result = f_read(&fat_file, sprite_draw[i][f], sizeof(sprite_draw[0][0]), &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != sizeof(sprite_draw[0][0]))
        return MissingDataError;
    return NoError;
}

FileError io_save_map()
{
    if (tile_map_height <= 0 || tile_map_width <= 0 || 
        ((tile_map_width*tile_map_height+1)/2 > TILE_MAP_MEMORY))
        return ConstraintError;
    
    char filename[13];
    if (io_set_extension(filename, "M16"))
        return MountError;
    
    fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
    if (fat_result != FR_OK)
        return OpenError;

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
            o->sprite_index*8 + o->sprite_frame + (o->z << 8),
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

    char filename[13];
    if (io_set_extension(filename, "M16"))
        return MountError;
    
    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

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
        uint8_t sprite = szxy[0]&255;
        uint8_t z = szxy[0]>>8;
        uint8_t index = create_object(sprite/8, sprite%8, (int16_t)szxy[1], (int16_t)szxy[2], z);
        if (index == 255)
        {
            f_close(&fat_file);
            return ConstraintError;
        }
    } 
    f_close(&fat_file);
    return NoError;
}

FileError io_load_instrument(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "I16"))
        return MountError;
    
    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

    UINT bytes_get; 
    if (i >= 16)
    {
        for (i=0; i<16; ++i)
        {
            uint8_t read;
            fat_result = f_read(&fat_file, &read, 1, &bytes_get);
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
            instrument[i].is_drum = read&15;
            instrument[i].octave = read >> 4;
            fat_result = f_read(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != MAX_INSTRUMENT_LENGTH)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, i*(MAX_INSTRUMENT_LENGTH+1)); 
    uint8_t read;
    fat_result = f_read(&fat_file, &read, 1, &bytes_get);
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
    instrument[i].is_drum = read&15;
    instrument[i].octave = read >> 4;
    fat_result = f_read(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != MAX_INSTRUMENT_LENGTH)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    
    f_close(&fat_file);
    return NoError;
}


FileError io_load_verse(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "V16"))
        return MountError; 

    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

    if (i >= 16)
    {
        for (i=0; i<16; ++i)
        {
            UINT bytes_get; 
            fat_result = f_read(&fat_file, chip_track[i], sizeof(chip_track[0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != sizeof(chip_track[0]))
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, i*sizeof(chip_track[0])); 
    UINT bytes_get; 
    fat_result = f_read(&fat_file, chip_track[i], sizeof(chip_track[0]), &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != sizeof(chip_track[0]))
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    f_close(&fat_file);
    return NoError;
}

FileError io_load_anthem()
{
    char filename[13];
    if (io_set_extension(filename, "A16"))
        return MountError; 

    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING); 
    if (fat_result)
        return OpenError;

    // set some defaults
    track_length = 16;
    song_speed = 4;

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

FileError io_load_go(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "E16"))
        return MountError; 

    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

    if (i >= 16)
    {
        for (i=0; i<16; ++i)
        {
            UINT bytes_get; 
            fat_result = f_read(&fat_file, sprite_pattern[i], sizeof(sprite_pattern[0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != sizeof(sprite_pattern[0]))
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, i*sizeof(sprite_pattern[0])); 
    UINT bytes_get; 
    fat_result = f_read(&fat_file, sprite_pattern[i], sizeof(sprite_pattern[0]), &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != sizeof(sprite_pattern[0]))
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    f_close(&fat_file);
    return NoError;
}

void write_instrument_command_argument(char *s, uint8_t cmd)
{
    int param = cmd>>4;
    switch (cmd&15)
    {
        case BREAK:
            s[0] = '0';
            s[1] = hex[param];
            break;
        case SIDE:
            s[0] = 'S';
            s[1] = hex[param];
            break;
        case WAVEFORM:
            s[0] = '#';
            switch (param)
            {
                case WF_SINE:
                    s[1] = 's';
                    break;
                case WF_TRIANGLE:
                    s[1] = '^';
                    break;
                case WF_SAW:
                    s[1] = '/';
                    break;
                case WF_PULSE:
                    s[1] = '_';
                    break;
                case WF_NOISE:
                    s[1] = '=';
                    break;
                case WF_RED:
                    s[1] = '&';
                    break;
                case WF_VIOLET:
                    s[1] = '!';
                    break;
                default:
                    s[1] = '?';
                    break;
            }
            break;
        case VOLUME:
            s[0] = 'V';
            s[1] = hex[param];
            break;
        case NOTE:
            s[0] = 'C';
            s[1] = hex[param];
            break;
        case WAIT:
            s[0] = 'W';
            if (param)
                s[1] = hex[param];
            else
                s[1] = 'g';
            break;
        case FADE_IN:
            s[0] = '<';
            s[1] = hex[param];
            break;
        case FADE_OUT:
            s[0] = '>';
            if (param)
                s[1] = hex[param];
            else
                s[1] = 'g';
            break;
        case INERTIA:
            s[0] = 'i';
            s[1] = hex[param];
            break;
        case VIBRATO:
            s[0] = '~';
            s[1] = hex[param];
            break;
        case BEND:
            if (param < 8)
            {
                s[0] = '/';
                s[1] = hex[param];
            }
            else
            {
                s[0] = '\\';
                s[1] = hex[16-param];
            }
            break;
        case BITCRUSH:
            s[0] = 'B';
            s[1] = hex[param];
            break;
        case DUTY:
            s[0] = 'D';
            s[1] = hex[param];
            break;
        case DUTY_DELTA:
            s[0] = 'd';
            s[1] = hex[param];
            break;
        case RANDOMIZE:
            s[0] = 'R';
            switch (param)
            {
                case 0:
                    s[1] = 'a';
                    break;
                case 1:
                    s[1] = 'o';
                    break;
                case 2:
                    s[1] = 'e';
                    break;
                case 3:
                    s[1] = 's';
                    break;
                case 4:
                    s[1] = 'l';
                    break;
                case 5:
                    s[1] = 'm';
                    break;
                case 6:
                    s[1] = 'h';
                    break;
                case 7:
                    s[1] = 'b';
                    break;
                case 8:
                    s[1] = 'L';
                    break;
                case 9:
                    s[1] = 'K';
                    break;
                case 10:
                    s[1] = 'J';
                    break;
                case 11:
                    s[1] = 'H';
                    break;
                case 12:
                    s[1] = '0';
                    break;
                case 13:
                    s[1] = '1';
                    break;
                case 14:
                    s[1] = '2';
                    break;
                case 15:
                    s[1] = '3';
                    break;
            }
            break;
        case JUMP:
            s[0] = 'J';
            s[1] = hex[param];
            break;
    }
}

FileError _io_save_instrument(unsigned int i)
{
    char third_bracket[4] = { ' ', hex[i], '[', '\n' };
    FileError ferr;
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;
    
    char two_data[8] = { ' ', ' ', ' ', '0', ':', '0', '0', '\n' };
    int j=0;
    two_data[0] = instrument[i].is_drum ? 'D' : 'O';
    two_data[1] = hex[instrument[i].octave];

    write_instrument_command_argument(two_data+5, instrument[i].cmd[j]);
    if ((ferr = file_write(two_data, 8)))
        return ferr;
    two_data[0] = ' ';
    two_data[1] = ' ';

    for (j=1; j<16; ++j)
    {
        two_data[3] = hex[j];
        write_instrument_command_argument(two_data+5, instrument[i].cmd[j]);
        if ((ferr = file_write(two_data, 8)))
            return ferr;
    }

    third_bracket[2] = ']';
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;
    return NoError;
}

FileError io_save_instrument(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "G16"))
        return MountError; 
    
    FileError ferr = io_open_or_zero_file(filename, FILE_SIZE);
    if (ferr)
        return ferr;

    if (i >= 16)
    {
        f_lseek(&fat_file, 0);
        if ((ferr = file_write("G16\n{IN\n", 8)))
            return ferr;
        for (i=0; i<16; ++i)
        {
            if ((ferr = _io_save_instrument(i)))
                return ferr;
        }
        if ((ferr = file_write("}IN\n", 4)))
            return ferr;
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, INSTRUMENT_OFFSET + 4 + i*(MAX_INSTRUMENT_LENGTH*8+8));
    if ((ferr = _io_save_instrument(i)))
        return ferr;
    f_close(&fat_file);
    return NoError;
}

void write_verse_command_argument(char *s, unsigned int i, unsigned int j)
{
    --s;
    for (int p=0; p<CHIP_PLAYERS; ++p)
    {
        int cmd = chip_track[i][p][j];
        int param = cmd>>4;
        switch (cmd&15)
        {
            case TRACK_BREAK:
                *++s = '0';
                *++s = hex[param];
                break;
            case TRACK_OCTAVE:
                if (param < 7)
                {
                    *++s = 'O';
                    *++s = '0' + param;
                }
                else if (param == 7)
                {
                    *++s = 'O';
                    *++s = '=';
                }
                else if (param < 12)
                {
                    *++s = (param%2) ? '+' : '/';
                    *++s= '0' + (param - 6)/2;
                }
                else
                {
                    *++s = (param%2) ? '+' : '/';
                    *++s = '0' + (17-param)/2;
                }
                break;
            case TRACK_INSTRUMENT:
                *++s = 'I';
                *++s = hex[param];
                break;
            case TRACK_VOLUME:
                *++s = 'V';
                *++s = hex[param];
                break;
            case TRACK_NOTE:
                *++s = 'C';
                *++s = hex[param];
                break;
            case TRACK_WAIT:
                *++s = 'W';
                if (param)
                    *++s = hex[param];
                else
                    *++s = 'g';
                break;
            case TRACK_NOTE_WAIT:
                *++s = 'N';
                *++s = hex[param];
                break;
            case TRACK_FADE_IN:
                *++s = '<';
                *++s = hex[param];
                break;
            case TRACK_FADE_OUT:
                *++s = '>';
                if (param)
                    *++s = hex[param];
                else
                    *++s = 'g';
                break;
            case TRACK_INERTIA:
                *++s = 'i';
                *++s = hex[param];
                break;
            case TRACK_VIBRATO:
                *++s = '~';
                *++s = hex[param];
                break;
            case TRACK_TRANSPOSE:
                *++s = 'T';
                *++s = hex[param];
                break;
            case TRACK_SPEED:
                *++s = 'S'; 
                *++s = hex[param];
                break;
            case TRACK_LENGTH:
                *++s = 'L';
                if (param)
                    *++s = hex[param];
                else
                    *++s = 'g';
                break;
            case TRACK_RANDOMIZE:
                *++s = 'R';
                switch (param)
                {
                    case 0:
                        *++s = 'a';
                        break;
                    case 1:
                        *++s = 'o';
                        break;
                    case 2:
                        *++s = 'e';
                        break;
                    case 3:
                        *++s = 's';
                        break;
                    case 4:
                        *++s = 'l';
                        break;
                    case 5:
                        *++s = 'm';
                        break;
                    case 6:
                        *++s = 'h';
                        break;
                    case 7:
                        *++s = 'b';
                        break;
                    case 8:
                        *++s = 'L';
                        break;
                    case 9:
                        *++s = 'K';
                        break;
                    case 10:
                        *++s = 'J';
                        break;
                    case 11:
                        *++s = 'H';
                        break;
                    case 12:
                        *++s = '0';
                        break;
                    case 13:
                        *++s = '1';
                        break;
                    case 14:
                        *++s = '2';
                        break;
                    case 15:
                        *++s = '3';
                        break;
                }
                break;
            case TRACK_JUMP:
                *++s = 'J';
                *++s = hex[2*param];
                break;
        }
        ++s;
    }
}

FileError _io_save_verse(unsigned int i)
{
    char third_bracket[4] = { ' ', hex[i], '[', '\n' };
    FileError ferr;
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;
    
    char four_data[16] = { ' ', ' ', '0', ':', '0', '0', ' ', '0', '0', ' ', '0', '0', ' ', '0', '0', '\n' };
    for (int j=0; j<MAX_TRACK_LENGTH; ++j)
    {
        four_data[2] = hex[j];
        write_verse_command_argument(four_data+4, i, j);
        if ((ferr = file_write(four_data, 16)))
            return ferr;
    }

    third_bracket[2] = ']';
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;
    return NoError;
}

FileError io_save_verse(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "G16"))
        return MountError; 
    
    FileError ferr = io_open_or_zero_file(filename, FILE_SIZE);
    if (ferr)
        return ferr;

    if (i >= 16)
    {
        f_lseek(&fat_file, VERSE_OFFSET);
        if ((ferr = file_write("{VS\n", 4)))
            return ferr;
        for (i=0; i<16; ++i)
        {
            if ((ferr = _io_save_verse(i)))
                return ferr;
        }
        if ((ferr = file_write("}VS\n", 4)))
            return ferr;
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, VERSE_OFFSET + 4 + i*(MAX_TRACK_LENGTH*16+8));
    if ((ferr = _io_save_verse(i)))
        return ferr;
    f_close(&fat_file);
    return NoError;
}

FileError io_save_anthem()
{
    char filename[13];
    if (io_set_extension(filename, "G16"))
        return MountError; 

    fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS); 
    if (fat_result)
        return OpenError;

    f_lseek(&fat_file, ANTHEM_OFFSET);
   
    char msg[4] = { '{', 'A', hex[song_length], '\n' };
    FileError ferr;
    if ((ferr = file_write(msg, 4)))
        return ferr;

    char four_data[8] = { ' ', '0', ':', '0', '0', '0', '0', '\n' };
    int i;
    for (i=0; i<song_length; ++i)
    {
        uint16_t s = chip_song[i];
        four_data[1] = hex[i];
        four_data[3] = hex[s&15];
        four_data[4] = hex[(s>>4)&15];
        four_data[5] = hex[(s>>8)&15];
        four_data[6] = hex[(s>>12)];
        if ((ferr = file_write(four_data, 8)))
            return ferr;
    }
    four_data[3] = '0';
    four_data[4] = '0';
    four_data[5] = '0';
    four_data[6] = '0';
    for (;i < MAX_SONG_LENGTH; ++i)
    {
        four_data[1] = hex[i];
        if ((ferr = file_write(four_data, 8)))
            return ferr;
    }
   
    msg[0] = '}';
    if ((ferr = file_write(msg, 4)))
        return ferr;

    f_close(&fat_file);
    return NoError;
}

FileError io_save_palette()
{
    char filename[13];
    if (io_set_extension(filename, "G16"))
        return MountError;

    fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
    if (fat_result != FR_OK)
        return OpenError;
    
    f_lseek(&fat_file, PALETTE_OFFSET);

    char msg[4] = { '{', 'P', 'C', '\n' };
    FileError ferr;
    if ((ferr = file_write(msg, 4)))
        return ferr;

    for (int i=0; i<16; ++i)
    {
        uint16_t c = palette[i];
        char rgb_data[8] = { ' ', ' ', hex[i], ':', hex[(c>>10)&31], hex[(c>>5)&31], hex[c&31], '\n' };
        if ((ferr = file_write(rgb_data, 8)))
            return ferr;
    }
   
    msg[0] = '}';
    if ((ferr = file_write(msg, 4)))
        return ferr;
    return NoError;
}

FileError _io_save_tile(unsigned int i)
{
    char third_bracket[4] = { ' ', hex[i], '[', '\n' };
    FileError ferr;
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;

    char meta_data[20] = { 'M', hex[tile_info[i].material&15], ' ', 'T', hex[tile_info[i].translation&15], ' ', 't', hex[tile_info[i].timing&15], ' '};
    char *m = &meta_data[8];
    if (tile_info[i].material & 8)
    {
        // solid
        *++m = 'V'; // vulnerability
        *++m = hex[(tile_info[i].vuln_warp_jump)&15]; // vulnerability
        *++m = ' '; 
        *++m = hex[(tile_info[i].side[0])&15]; // right side
        *++m = ' '; 
        *++m = hex[(tile_info[i].side[1])&15]; // top side
        *++m = ' '; 
        *++m = hex[(tile_info[i].side[2])&15]; // left side
        *++m = ' '; 
        *++m = hex[(tile_info[i].side[3])&15]; // bottom side
    }
    else switch (tile_info[i].vuln_warp_jump) // warp bit
    {
        case 0: // fluid flow
            *++m = 'A'; // flow angle
            *++m = hex[tile_info[i].damage_angle>>4];
            *++m = ' '; 
            *++m = 'S'; // flow strength
            *++m = hex[tile_info[i].random_strength>>4];
            *++m = ' '; 
            *++m = 'R';
            *++m = hex[tile_info[i].random_strength&15];
            *++m = ' '; 
            *++m = (tile_info[i].damage_angle&15) ? 'D' : 'K'; 
            break;
        case 1: // warp to a new game
            *++m = 'W';
            *++m = 'A';
            *++m = 'R';
            *++m = 'P';
            *++m = '1';
            *++m = hex[tile_info[i].press_direction&15];
            *++m = ' ';
            *++m = ' ';
            *++m = ' ';
            *++m = '0' + tile_info[i].warp%10;
            break;
        case 2: // warp to a new game
            *++m = 'W';
            *++m = 'A';
            *++m = 'R';
            *++m = 'P';
            *++m = '2';
            *++m = hex[tile_info[i].press_direction&15];
            *++m = ' ';
            *++m = ' ';
            *++m = '0' + (tile_info[i].warp/10)%10;
            *++m = '0' + tile_info[i].warp%10;
            break;
        case 3: // warp to a new game
            *++m = 'W';
            *++m = 'A';
            *++m = 'R';
            *++m = 'P';
            *++m = '3';
            *++m = hex[tile_info[i].press_direction&15];
            *++m = ' ';
            *++m = '0' + (tile_info[i].warp/100)%10;
            *++m = '0' + (tile_info[i].warp/10)%10;
            *++m = '0' + tile_info[i].warp%10;
            break;
        case 4: // jump within the level
            *++m = 'J';
            *++m = hex[tile_info[i].press_direction&15];
            *++m = '(';
            *++m = hex[(tile_info[i].jx>>8)&15];
            *++m = hex[(tile_info[i].jx>>4)&15];
            *++m = hex[(tile_info[i].jx)&15];
            *++m = ',';
            *++m = hex[(tile_info[i].jy>>4)&15];
            *++m = hex[(tile_info[i].jy)&15];
            *++m = ')';
            break;
        default:
            *++m = '?';
            *++m = '?';
            *++m = '?';
            *++m = '?';
            *++m = '?';
            *++m = '?';
            *++m = '?';
            *++m = '?';
            *++m = '?';
            *++m = '?';
    }
    meta_data[19] = '\n';
    if ((ferr = file_write(meta_data, 20)))
        return ferr;
    
    for (int j=0; j<16; ++j)
    {
        char data16[18] = { ' ', 
            hex[tile_draw[i][j][0]&15], hex[tile_draw[i][j][0]>>4],
            hex[tile_draw[i][j][1]&15], hex[tile_draw[i][j][1]>>4],
            hex[tile_draw[i][j][2]&15], hex[tile_draw[i][j][2]>>4],
            hex[tile_draw[i][j][3]&15], hex[tile_draw[i][j][3]>>4],
            hex[tile_draw[i][j][4]&15], hex[tile_draw[i][j][4]>>4],
            hex[tile_draw[i][j][5]&15], hex[tile_draw[i][j][5]>>4],
            hex[tile_draw[i][j][6]&15], hex[tile_draw[i][j][6]>>4],
            hex[tile_draw[i][j][7]&15], hex[tile_draw[i][j][7]>>4],
            '\n' };
        if ((ferr = file_write(data16, 18)))
            return ferr;
    }

    third_bracket[2] = ']';
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;
    return NoError;
}

FileError io_save_tile(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "G16"))
        return MountError; 
    
    FileError ferr = io_open_or_zero_file(filename, FILE_SIZE);
    if (ferr)
        return ferr;

    if (i >= 16)
    {
        f_lseek(&fat_file, TILE_OFFSET);
        if ((ferr = file_write("{TL\n", 4)))
            return ferr;
        for (i=0; i<16; ++i)
        {
            if ((ferr = _io_save_tile(i)))
                return ferr;
        }
        if ((ferr = file_write("}TL\n", 4)))
            return ferr;
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, TILE_OFFSET + 4 + i*(16*18 + 20 + 8));
    if ((ferr = _io_save_tile(i)))
        return ferr;
    f_close(&fat_file);
    return NoError;
}

FileError _io_save_sprite(unsigned int i, unsigned int f)
{
    static const char dir[8] = { 'R', 'r', 'U', 'u', 'L', 'l', 'D', 'd' };
    char third_bracket[4] = { hex[i], dir[f], '[', '\n' };
    FileError ferr;
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;

    char meta_data[20] = { 
        'I', hex[sprite_info[i][f].invisible_color&31], ' ', 
        'W', hex[sprite_info[i][f].inverse_weight&15], ' ', 
        'V', hex[sprite_info[i][f].vulnerability&15], ' ',
        'P', hex[sprite_info[i][f].impervious&15], ' ',
        hex[sprite_info[i][f].side[0]&15], ' ',
        hex[sprite_info[i][f].side[1]&15], ' ',
        hex[sprite_info[i][f].side[2]&15], ' ',
        hex[sprite_info[i][f].side[3]&15], '\n',
    };
    if ((ferr = file_write(meta_data, 20)))
        return ferr;
    
    for (int j=0; j<16; ++j)
    {
        char data16[18] = { ' ', 
            hex[sprite_draw[i][f][j][0]&15], hex[sprite_draw[i][f][j][0]>>4],
            hex[sprite_draw[i][f][j][1]&15], hex[sprite_draw[i][f][j][1]>>4],
            hex[sprite_draw[i][f][j][2]&15], hex[sprite_draw[i][f][j][2]>>4],
            hex[sprite_draw[i][f][j][3]&15], hex[sprite_draw[i][f][j][3]>>4],
            hex[sprite_draw[i][f][j][4]&15], hex[sprite_draw[i][f][j][4]>>4],
            hex[sprite_draw[i][f][j][5]&15], hex[sprite_draw[i][f][j][5]>>4],
            hex[sprite_draw[i][f][j][6]&15], hex[sprite_draw[i][f][j][6]>>4],
            hex[sprite_draw[i][f][j][7]&15], hex[sprite_draw[i][f][j][7]>>4],
            '\n' };
        if ((ferr = file_write(data16, 18)))
            return ferr;
    }
    
    third_bracket[2] = ']';
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;
    return NoError;
}

FileError io_save_sprite(unsigned int i, unsigned int f)
{
    char filename[13];
    if (io_set_extension(filename, "G16"))
        return MountError; 
    
    FileError ferr = io_open_or_zero_file(filename, FILE_SIZE);
    if (ferr)
        return ferr;

    if (i >= 16)
    {
        f_lseek(&fat_file, SPRITE_OFFSET);
        if ((ferr = file_write("{SP\n", 4)))
            return ferr;
        for (i=0; i<16; ++i)
        for (f=0; f<8; ++f)
        {
            if ((ferr = _io_save_sprite(i, f)))
                return ferr;
        }
        if ((ferr = file_write("}SP\n", 4)))
            return ferr;
        f_close(&fat_file);
        return NoError;
    }
    if (f >= 8)
    {
        f_lseek(&fat_file, SPRITE_OFFSET + 4 + (8*i)*(16*18+20+8));
        for (f=0; f<8; ++f)
        {
            if ((ferr = _io_save_sprite(i, f)))
                return ferr;
        }
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, SPRITE_OFFSET + 4 + (8*i+f)*(16*18+20+8));
    if ((ferr = _io_save_sprite(i, f)))
        return ferr;
    f_close(&fat_file);
    return NoError;
}

void write_go_command_argument(char *s, int i, int j)
{
    int cmd = sprite_pattern[i][j];
    int param = cmd>>4;
    switch (cmd&15)
    {
        case GO_BREAK:
            *s = '0';
            *++s = hex[param];
            break;
        case GO_NOT_MOVE:
            *s = 'm';
            if (!param)
                param = 16;
            *++s = hex[j+1+param];
            break;
        case GO_NOT_RUN:
            *s = 'r';
            if (!param)
                param = 16;
            *++s = hex[j+1+param];
            break;
        case GO_NOT_AIR:
            *s = 'a';
            if (!param)
                param = 16;
            *++s = hex[j+1+param];
            break;
        case GO_NOT_FIRE:
            *s = 'f';
            if (!param)
                param = 16;
            *++s = hex[j+1+param];
            break;
        case GO_EXECUTE:
            *s = 'E';
            *++s = hex[param];
            break;
        case GO_LOOK:
            *s = 'L';
            *++s = hex[param];
            break;
        case GO_DIRECTION:
            *s = 'D';
            *++s = hex[param];
            break;
        case GO_SPECIAL_INPUT:
            *s = 'I';
            *++s = hex[param];
            break;
        case GO_SPAWN_TILE:
            *s = 'T';
            *++s = hex[param];
            break;
        case GO_SPAWN_SPRITE:
            *s = 'S';
            *++s = hex[param];
            break;
        case GO_ACCELERATION:
            *s = '/';
            *++s = hex[param];
            break;
        case GO_SPEED:
            if (param < 8)
            {
                *s = '+';
                *++s = '0' + param;
            }
            else
            {
                *s = '^';
                *++s = '0' + param - 8;
            }
            break;
        case GO_NOISE:
            *s = 'N';
            *++s = hex[param];
            break;
        case GO_RANDOMIZE:
            *s = 'R';
            switch (param)
            {
                case 0:
                    *++s = 'a';
                    break;
                case 1:
                    *++s = 'o';
                    break;
                case 2:
                    *++s = 'e';
                    break;
                case 3:
                    *++s = 's';
                    break;
                case 4:
                    *++s = 'l';
                    break;
                case 5:
                    *++s = 'm';
                    break;
                case 6:
                    *++s = 'h';
                    break;
                case 7:
                    *++s = 'b';
                    break;
                case 8:
                    *++s = 'L';
                    break;
                case 9:
                    *++s = 'K';
                    break;
                case 10:
                    *++s = 'J';
                    break;
                case 11:
                    *++s = 'H';
                    break;
                case 12:
                    *++s = '0';
                    break;
                case 13:
                    *++s = '1';
                    break;
                case 14:
                    *++s = '2';
                    break;
                case 15:
                    *++s = '3';
                    break;
            }
            break;
        case GO_QUAKE:
            *s = 'Q';
            *++s = hex[param];
            break;
    }
}

FileError _io_save_go(unsigned int i)
{
    char third_bracket[4] = { ' ', hex[i], '[', '\n' };
    FileError ferr;
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;
    
    char two_data[8] = { ' ', ' ', ' ', '0', ':', '0', '0', '\n' };
    for (int j=0; j<32; ++j)
    {
        two_data[3] = hex[j];
        write_go_command_argument(two_data+5, i, j);
        if ((ferr = file_write(two_data, 8)))
            return ferr;
    }

    third_bracket[2] = ']';
    if ((ferr = file_write(third_bracket, 4)))
        return ferr;
    return NoError;
}

FileError io_save_go(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "G16"))
        return MountError; 
    
    FileError ferr = io_open_or_zero_file(filename, FILE_SIZE);
    if (ferr)
        return ferr;

    if (i >= 16)
    {
        f_lseek(&fat_file, GO_OFFSET);
        if ((ferr = file_write("{GO\n", 4)))
            return ferr;
        for (i=0; i<16; ++i)
        {
            if ((ferr = _io_save_go(i)))
                return ferr;
        }
        if ((ferr = file_write("}GO\n", 4)))
            return ferr;
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, GO_OFFSET + 4 + i*(32*8+8));
    if ((ferr = _io_save_go(i)))
        return ferr;
    f_close(&fat_file);
    return NoError;
}

