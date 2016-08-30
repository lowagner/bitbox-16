#include "bitbox.h"
#include "chiptune.h"
#include "common.h"
#include "tiles.h"
#include "name.h"
#include "io.h"

#include "fatfs/ff.h"
#include <string.h> // strlen

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
    --k;
    filename[++k] = '.';
    filename[++k] = *ext++;
    filename[++k] = *ext++;
    filename[++k] = *ext;
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
    char filename[13];
    if (io_set_extension(filename, "P16"))
        return MountError;

    fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
    if (fat_result != FR_OK)
        return OpenError;

    UINT bytes_get; 
    fat_result = f_write(&fat_file, palette, sizeof(palette), &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return WriteError;

    if (bytes_get != sizeof(palette))
        return MissingDataError;
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

FileError io_save_tile(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "T16"))
        return MountError;
   
    if (i >= 16)
    {
        // write them all
        fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
        if (fat_result != FR_OK)
            return OpenError;

        for (i=0; i<16; ++i)
        {
            UINT bytes_get; 
            fat_result = f_write(&fat_file, &tile_info[i], 4, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
            }
            if (bytes_get != 4)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
            fat_result = f_write(&fat_file, tile_draw[i], sizeof(tile_draw[0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
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
    FileError ferr = io_open_or_zero_file(filename, 16*(sizeof(tile_draw[0])+4));
    if (ferr)
        return ferr;

    f_lseek(&fat_file, i*(sizeof(tile_draw[0])+4)); 
    UINT bytes_get; 
    fat_result = f_write(&fat_file, &tile_info[i], 4, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 4)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    fat_result = f_write(&fat_file, tile_draw[i], sizeof(tile_draw[0]), &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != sizeof(tile_draw[0]))
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
            fat_result = f_read(&fat_file, &tile_info[i], 4, &bytes_get);
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
    fat_result = f_read(&fat_file, &tile_info[i], 4, &bytes_get);
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
    fat_result = f_read(&fat_file, tile_draw[i], sizeof(tile_draw[0]), &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get != sizeof(tile_draw[0]))
        return MissingDataError;
    return NoError;
}

FileError io_save_sprite(unsigned int i, unsigned int f)
{
    char filename[13];
    if (io_set_extension(filename, "S16"))
        return MountError;
    
    if (i >= 16)
    {
        // write them all, ignore f
        fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
        if (fat_result != FR_OK)
            return OpenError; 

        for (i=0; i<16; ++i)
        for (f=0; f<8; ++f)
        {
            UINT bytes_get;
            fat_result = f_write(&fat_file, &sprite_info[i][f], 4, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError; 
            }
            else if (bytes_get != 4)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
            fat_result = f_write(&fat_file, sprite_draw[i][f], sizeof(sprite_draw[0][0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
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
    FileError ferr = io_open_or_zero_file(filename, 16*8*(sizeof(sprite_draw[0][0])+4));
    if (ferr)
        return ferr;
    if (f >= 8)
    {
        // write all frames
        f_lseek(&fat_file, i*8*(sizeof(sprite_draw[0][0])+4)); 
        for (f=0; f<8; ++f)
        {
            UINT bytes_get;
            fat_result = f_write(&fat_file, &sprite_info[i][f], 4, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError; 
            }
            else if (bytes_get != 4)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
            fat_result = f_write(&fat_file, sprite_draw[i][f], sizeof(sprite_draw[0][0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
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
    fat_result = f_write(&fat_file, &sprite_info[i][f], 4, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError; 
    }
    else if (bytes_get != 4)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    fat_result = f_write(&fat_file, sprite_draw[i][f], sizeof(sprite_draw[0][0]), &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != sizeof(sprite_draw[0][0]))
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
            fat_result = f_read(&fat_file, &sprite_info[i][f], 4, &bytes_get);
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
            fat_result = f_read(&fat_file, &sprite_info[i][f], 4, &bytes_get);
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
    fat_result = f_read(&fat_file, &sprite_info[i][f], 4, &bytes_get);
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
    return NoError;
}

FileError io_load_map()
{
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
        uint8_t index = create_object(sprite/8, (int16_t)szxy[1], (int16_t)szxy[2], z);
        if (index == 255)
        {
            f_close(&fat_file);
            return ConstraintError;
        }
        object[index].sprite_frame = sprite%8;
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

FileError io_save_instrument(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "I16"))
        return MountError; 

    if (i >= 16)
    {
        fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
        if (fat_result != FR_OK)
            return OpenError;

        for (i=0; i<16; ++i)
        {
            UINT bytes_get; 
            uint8_t write = (instrument[i].is_drum ? 1 : 0) | (instrument[i].octave << 4);
            fat_result = f_write(&fat_file, &write, 1, &bytes_get);
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
            fat_result = f_write(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
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

    FileError ferr = io_open_or_zero_file(filename, 16*(MAX_INSTRUMENT_LENGTH+1));
    if (ferr)
        return ferr;

    f_lseek(&fat_file, i*(MAX_INSTRUMENT_LENGTH+1)); 
    UINT bytes_get; 
    uint8_t write = (instrument[i].is_drum ? 1 : 0) | (instrument[i].octave << 4);
    fat_result = f_write(&fat_file, &write, 1, &bytes_get);
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
    fat_result = f_write(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
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
            fat_result = f_read(&fat_file, &chip_track[i], sizeof(chip_track[0]), &bytes_get);
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
    fat_result = f_read(&fat_file, &chip_track[i], sizeof(chip_track[0]), &bytes_get);
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

FileError io_save_verse(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "V16"))
        return MountError; 

    if (i >= 16)
    {
        fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
        if (fat_result != FR_OK)
            return OpenError;

        for (i=0; i<16; ++i)
        {
            UINT bytes_get; 
            fat_result = f_write(&fat_file, &chip_track[i], sizeof(chip_track[0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
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

    FileError ferr = io_open_or_zero_file(filename, 16*sizeof(chip_track[0]));
    if (ferr)
        return ferr;

    f_lseek(&fat_file, i*sizeof(chip_track[0])); 
    UINT bytes_get; 
    fat_result = f_write(&fat_file, &chip_track[i], sizeof(chip_track[0]), &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
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

FileError io_save_anthem()
{
    char filename[13];
    if (io_set_extension(filename, "A16"))
        return MountError; 

    fat_result = f_open(&fat_file, filename, FA_WRITE | FA_CREATE_ALWAYS); 
    if (fat_result)
        return OpenError;

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
