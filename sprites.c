#include "bitbox.h"
#include "sprites.h"
#include "go-sprite.h"
#include "tiles.h"

#define SPEED_MULTIPLIER 0.5f
#define JUMP_MULTIPLIER 1.75f
#define ACCELERATION_MULTIPLIER 0.25f
#define MAX_VY 10.0f

#include <stdlib.h> // rand

float gravity CCM_MEMORY;

// break sprites up into 16x16 tiles:
uint8_t sprite_draw[16][8][16][8] CCM_MEMORY; // 16 sprites, 8 frames, 16x16 pixels...
uint8_t sprite_pattern[16][32] CCM_MEMORY; 
/*
info about a sprite (first frame, frame 0):
    1 bit for Block-like.
    if Block-like:
        4 bits ignored (what could we put here?)
    else:
        4 bits for "what color is invisible in this sprite"
    3 bits for inverse weight
    4 bits for vulnerability, must have (attack & vulnerability) > 0 to destroy it.
    1 bit for impervious, x 4 sides.  some sides can be impervious to damage...
    4 bits for surface property, x 4 sides 
      see common.h for information
*/
uint32_t sprite_info[16][8] CCM_MEMORY; 

struct object object[MAX_OBJECTS] CCM_MEMORY;
uint8_t first_free_object_index CCM_MEMORY;
uint8_t first_used_object_index CCM_MEMORY;

uint8_t draw_order[MAX_OBJECTS] CCM_MEMORY;
uint8_t drawing_count CCM_MEMORY; 
uint8_t first_drawing_index CCM_MEMORY; 
uint8_t last_drawing_index CCM_MEMORY; 

void sprites_init()
{
    // setup the objects and the linked list:
    for (int i=0; i<MAX_OBJECTS; ++i)
    {
        object[i] = (struct object) {
            .next_object_index = i+1,
            .previous_object_index = i-1,
        };
    }
    object[MAX_OBJECTS-1].next_object_index = 255;
    first_free_object_index = 0;
    first_used_object_index = 255;

    gravity = 1.0f;
    
    drawing_count = 0;
}

void make_unseen_object_viewable(uint8_t i)
{
    // object is viewable, but hasn't gone on the drawing list:
    object[i].iy = object[i].y - tile_map_y;
    object[i].ix = object[i].x - tile_map_x;
    object[i].draw_order_index = drawing_count;
    draw_order[drawing_count] = i;

    ++drawing_count;
}

int on_screen(int16_t x, int16_t y)
{
    if (x <= tile_map_x-16 || x >= tile_map_x + SCREEN_W)
        return 0;
    if (0) // non-platformers
    {
        if (y >= tile_map_y + SCREEN_H || y <= tile_map_y-16)
            return 0;
    }
    return 1;
}

uint8_t create_object(uint8_t sprite_index, uint8_t sprite_frame, int16_t x, int16_t y, uint8_t z)
{
    if (first_free_object_index == 255)
        return -1;

    // update free list:
    uint8_t i = first_free_object_index;
    first_free_object_index = object[i].next_object_index;
    // update the old root's previous index:
    object[first_used_object_index].previous_object_index = i;
    // make object i the head of the used list:
    object[i].next_object_index = first_used_object_index;
    object[i].previous_object_index = 255; // object i is root
    first_used_object_index = i;

    // add in object properties
    object[i].sprite_index = sprite_index;
    object[i].sprite_frame = sprite_frame;
    object[i].y = y;
    object[i].x = x;
    object[i].z = z;
    object[i].cmd_index = 0;
    object[i].wait = 0;
    object[i].edge_accel = 8 | (5<<4);
    object[i].speed_jump = 4 | (5<<4);
    object[i].properties = 0;
    object[i].firing = 0;
    if (on_screen(object[i].x, object[i].y))
        make_unseen_object_viewable(i);
    else
        object[i].draw_order_index = -1;
    return i;
}

static inline int test_inside_tile(int x, int y)
{
    // return 0 for no hit, 1 for solid, -1 for damage
    int index = y*tile_map_width + x;
    uint8_t tile;
    if (index % 2)
        tile = tile_map[index/2] >> 4;
    else
        tile = tile_map[index/2] & 15;
    uint32_t info = tile_info[tile_translator[tile]];
    if (!(info & 8))
        return 0; // TODO: check for warps
    SideType side_up = (info >> (16 + 4*UP))&15;
    SideType side_down = (info >> (16 + 4*DOWN)); // don't need &15 since it's at the end.
    if (side_up == 0 || side_down == 0)
        return 0;
    if (side_up/2 == Damaging/2 || side_down/2 == Damaging/2)
        return -1;
    // TODO: check for other conditions?
    return 1;
}

static inline int test_tile(int x, int y, int dir)
{
    // return 0 for no hit, 1 for solid, -1 for damage
    int index = y*tile_map_width + x;
    uint8_t tile;
    if (index % 2)
        tile = tile_map[index/2] >> 4;
    else
        tile = tile_map[index/2] & 15;
    uint32_t info = tile_info[tile_translator[tile]];
    if (!(info & 8))
        return 0; // TODO: check for warps
    SideType side = (info >> (16 + 4*dir))&15;
    if (side == 0)
        return 0;
    if (side/2 == Damaging/2)
        return -1;
    // TODO: check for other conditions?
    return 1;
}

void object_run_commands(uint8_t i) 
{
    // update position here, too
    object[i].properties |= IN_AIR; // assume you're flying until you're not...
    if (object[i].properties & GHOSTING)
    {
        object[i].y += object[i].vy;
        object[i].x += object[i].vx;
        goto object_execute_commands;
    }
    // run physics and hit tests
    //   e.g. if colliding against ground, set vy = 0

    // avoid adding in gravity for non-platformers:
    object[i].vy += gravity;
    if (object[i].vy > MAX_VY)
        object[i].vy = MAX_VY;
    object[i].y += object[i].vy;
    if (object[i].y < 16) {}
    else if (object[i].y > tile_map_height*16 - 32)
    {
        object[i].y = tile_map_height*16 - 32;
        object[i].vy = 0;
        // TODO: maybe test object right below player here.
    }
    else if (object[i].vy == 0.0f || (16.0f*((int)(object[i].y/16)) == object[i].y)) {}
    else if (object[i].vy > 0)
    {
        // test collision onto some tile's UP side.
        int y_tile = object[i].y/16 + 1;
        int x_tile = object[i].x/16;
        if (16.0f * x_tile == object[i].x)
        {
            switch (test_tile(x_tile, y_tile, UP))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].y = 16*(y_tile-1);
                object[i].vy = 0;
                object[i].properties &= ~IN_AIR;
                break;
            }
        }
        else
        {
            // gotta test left and right tiles
            switch (test_tile(x_tile, y_tile, UP))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].y = 16*(y_tile-1);
                object[i].vy = 0;
                object[i].properties &= ~IN_AIR;
                break;
            }
            switch (test_tile(++x_tile, y_tile, UP))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].y = 16*(y_tile-1);
                object[i].vy = 0;
                break;
            }
        }
    }
    else // vy < 0
    {
        // check bumping head
    }
    object[i].x += object[i].vx;
    if (object[i].x < 16)
    {
        object[i].x = 16;
        object[i].vx = 0;
    }
    else if (object[i].x > tile_map_width*16-32)
    {
        object[i].x = tile_map_width*16-32;
        object[i].vx = 0;
    }
    else if (object[i].vx == 0.0f || ( ((int)(object[i].x/16)*16.0f) == object[i].x)) {}
    else if (object[i].vx > 0)
    {
        // test colliding into something's LEFT side
        int x_tile = object[i].x/16 + 1;
        int y_tile = object[i].y/16;
        if (16.0f * y_tile == object[i].y)
        {
            switch (test_tile(x_tile, y_tile, LEFT))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].x = 16*(x_tile-1);
                object[i].vx = 0;
                break;
            }
        }
        else
        {
            switch (test_tile(x_tile, y_tile, LEFT))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].vx = 0;
                object[i].x = 16*(x_tile-1);
                break;
            }
            switch (test_tile(x_tile, ++y_tile, LEFT))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].x = 16*(x_tile-1);
                object[i].vx = 0;
                break;
            }
        }
    }
    else // vx < 0
    {
        int x_tile = object[i].x/16;
        int y_tile = object[i].y/16;
        if (16.0f * y_tile == object[i].y)
        {
            switch (test_tile(x_tile, y_tile, RIGHT))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].x = 16*(x_tile+1);
                object[i].vx = 0;
                break;
            }
        }
        else
        {
            switch (test_tile(x_tile, y_tile, RIGHT))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].x = 16*(x_tile+1);
                object[i].vx = 0;
                break;
            }
            switch (test_tile(x_tile, ++y_tile, RIGHT))
            {
            case 0:
                break;
            case -1:
                message("need to add hurt damage here!\n");
            case 1:
                object[i].x = 16*(x_tile+1);
                object[i].vx = 0;
                break;
            }
        }
    }
    // finally check the space that the sprite is in.
    int y_tile = object[i].y/16;
    if (tile_xy_is_block(object[i].x/16, y_tile))
    {
        object[i].y = 16.0f*(y_tile-1);
        object[i].vy = 0;
        object[i].properties &= ~IN_AIR;
    }

    object_execute_commands:
    if (object[i].wait)
    {
        --object[i].wait;
        return;
    }
    while (object[i].cmd_index < 32)
    {
        uint8_t cmd = sprite_pattern[object[i].sprite_index][object[i].cmd_index++];
        uint8_t param = cmd >> 4;
        switch (cmd & 15)
        {
        case GO_BREAK:
            if (param)
                object[i].wait = param + param*param/2; // tweak as necessary
            else
                object[i].cmd_index = 0;
            return;
        case GO_NOT_MOVE:
            if (object[i].vx || object[i].vy)
            {
                if (!param)
                    param = 16;
                object[i].cmd_index += param;
            }
            break;
        case GO_NOT_RUN:
            if (object[i].properties & RUNNING)
                break;
            if (!param)
                param = 16;
            object[i].cmd_index += param;
            break;
        case GO_NOT_AIR:
            if (object[i].properties & IN_AIR)
                break;
            if (!param)
                param = 16;
            object[i].cmd_index += param;
            break;
        case GO_NOT_FIRE:
            if (!object[i].firing)
            {
                if (!param)
                    param = 16;
                object[i].cmd_index += param;
            }
            break;
        case GO_EXECUTE:
            switch (param)
            {
            case 0: // stop
                object[i].vx = object[i].vy = 0;
                break;
            case 1: // turn CCW
            {
                object[i].sprite_frame = (object[i].sprite_frame + 2)%8;
                float vx = object[i].vx;
                object[i].vx = object[i].vy;
                object[i].vy = -vx;
                break;
            }
            case 2: // turn 180
                object[i].sprite_frame = (object[i].sprite_frame + 4)%8;
                object[i].vx = -object[i].vx;
                object[i].vy = -object[i].vy;
                break;
            case 3: // turn 270 = CW
            {
                object[i].sprite_frame = (object[i].sprite_frame + 6)%8;
                float vx = object[i].vx;
                object[i].vx = -object[i].vy;
                object[i].vy = vx;
                break;
            }
            case 4: // walk
                switch (object[i].sprite_frame/2)
                {
                case RIGHT:
                    object[i].vy = 0;
                    object[i].vx = (object[i].speed_jump&15)*SPEED_MULTIPLIER;
                    break;
                case UP:
                    object[i].vy = -(object[i].speed_jump&15)*SPEED_MULTIPLIER;
                    object[i].vx = 0;
                    // TODO: in platformer only:
                    object[i].properties |= IN_AIR;
                    break;
                case LEFT:
                    object[i].vy = 0;
                    object[i].vx = -(object[i].speed_jump&15)*SPEED_MULTIPLIER;
                    break;
                case DOWN:
                    object[i].vy = (object[i].speed_jump&15)*SPEED_MULTIPLIER;
                    object[i].vx = 0;
                    break;
                }
                break;
            case 5: // toggle run
                if (object[i].properties & RUNNING)
                    object[i].properties &= ~RUNNING;
                else
                    object[i].properties |= RUNNING;
                break;
            case 6: // do a jump
                object[i].vy = -(object[i].speed_jump >> 4)*JUMP_MULTIPLIER;
                object[i].properties |= IN_AIR;
                break;
            case 7: // toggle ghost
                if (object[i].properties & GHOSTING)
                    object[i].properties &= ~GHOSTING;
                else
                    object[i].properties |= GHOSTING;
                break;
            default:
                // the rest are edge behaviors
                object[i].edge_accel &= 240;
                object[i].edge_accel |= param;
            }
            break;
        case GO_LOOK:
            break;
        case GO_DIRECTION:
        {
            int p = param/8;
            if (param == 4)
            {
                // need special treatment here, random movement when pressing down
                if (!(gamepad_buttons[p] & (gamepad_up | gamepad_down | gamepad_left | gamepad_right)))
                {
                    object[i].vx /= (1 + (object[i].edge_accel>>6));
                    if (0) // for non-platformer
                    {
                        object[i].vy /= (1 + (object[i].edge_accel>>6));
                    }
                    break;
                }
                switch (rand()%4)
                {
                    case 0:
                    {
                        object[i].vx -= (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*LEFT;
                        float vx_limit = -(object[i].speed_jump&15)*SPEED_MULTIPLIER;
                        if (object[i].vx < vx_limit)
                            object[i].vx = vx_limit;
                        break;
                    }
                    case 1:
                    {
                        object[i].vx += (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*RIGHT;
                        float vx_limit = (object[i].speed_jump&15)*SPEED_MULTIPLIER;
                        if (object[i].vx > vx_limit)
                            object[i].vx = vx_limit;
                        break;
                    }
                    case 2:
                    {
                        object[i].vy -= (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*UP;
                        float vy_limit = -(object[i].speed_jump>>4)*SPEED_MULTIPLIER;
                        if (object[i].vy < vy_limit)
                            object[i].vy = vy_limit;
                        break;
                    }
                    case 3:
                    {
                        object[i].vy += (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = 2*DOWN;
                        // only check this for non-platformer,
                        // a platformer will fix vy automatically
                        if (0)
                        {
                            float vy_limit = (object[i].speed_jump>>4)*SPEED_MULTIPLIER;
                            if (object[i].vy > vy_limit)
                                object[i].vy = vy_limit;
                        }
                        break;
                    }
                }

                break;
            }
            if (param & 1)
            {
                int motion = 0;
                if (GAMEPAD_PRESSED(p, left))
                    --motion;
                if (GAMEPAD_PRESSED(p, right))
                    ++motion;

                if (motion)
                {
                    if (param & 4)
                        motion *= -1;
                    if (motion < 0)
                    {
                        object[i].vx -= (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = LEFT;
                        float vx_limit = -(object[i].speed_jump&15)*SPEED_MULTIPLIER;
                        if (object[i].vx < vx_limit)
                            object[i].vx = vx_limit;
                    }
                    else
                    {
                        object[i].vx += (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = RIGHT;
                        float vx_limit = (object[i].speed_jump&15)*SPEED_MULTIPLIER;
                        if (object[i].vx > vx_limit)
                            object[i].vx = vx_limit;
                    }
                }
                else
                {
                    object[i].vx /= (1 + (object[i].edge_accel>>6));
                }
            }
            if (param & 2)
            {
                int motion = 0;
                if (GAMEPAD_PRESSED(p, up))
                    --motion;
                if (GAMEPAD_PRESSED(p, down))
                    ++motion;

                if (motion)
                {
                    if (param & 4)
                        motion *= -1;
                    if (motion < 0)
                    {
                        object[i].vy -= (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = UP;
                        float vy_limit = -(object[i].speed_jump>>4)*SPEED_MULTIPLIER;
                        if (object[i].vy < vy_limit)
                            object[i].vy = vy_limit;
                    }
                    else
                    {
                        object[i].vy += (1+((object[i].edge_accel>>4)&3))*ACCELERATION_MULTIPLIER;
                        object[i].sprite_frame = DOWN;
                        // only check this for non-platformer,
                        // a platformer will fix vy automatically
                        if (0)
                        {
                            float vy_limit = (object[i].speed_jump>>4)*SPEED_MULTIPLIER;
                            if (object[i].vy > vy_limit)
                                object[i].vy = vy_limit;
                        }
                    }
                }
                else if (0) // for non-platformer
                {
                    object[i].vy /= (1 + (object[i].edge_accel>>6));
                }
            }
            break;
        }
        case GO_SPECIAL_INPUT:
        {
            int p = param/8;
            if (param & 1) // run
            {
                if ((gamepad_buttons[p] & (gamepad_Y | gamepad_A)))
                {
                    object[i].properties |= RUNNING;
                }
                else
                {
                    object[i].properties &= ~RUNNING;
                }
            }
            if (param & 2) // jump
            {
                if (GAMEPAD_PRESSED(p, B))
                {
                    object[i].vy = -(object[i].speed_jump >> 4)*JUMP_MULTIPLIER;
                    object[i].properties |= IN_AIR;
                }
            }
            if (param & 4) // fire 
            {
                if (GAMEPAD_PRESSED(p, X))
                {
                    object[i].firing = 16;  // or some other number?
                    // TODO: needs help, spawn a sprite, or change how jumping around
                    // works for fire mode
                }
            }
            break;
        }
        case GO_SPAWN_TILE:
            break;
        case GO_SPAWN_SPRITE:
            break;
        case GO_ACCELERATION:
            object[i].edge_accel &= 15; // keep edge behavior
            object[i].edge_accel |= param << 4;
            break;
        case GO_SPEED:
            if (param & 8) // jump speed
            {
                object[i].speed_jump &= 15; // keep run speed
                object[i].speed_jump |= (param-7) << 4;
            }
            else // run speed
            {
                object[i].speed_jump &= 240;
                object[i].speed_jump |= 1 + param;
            }
            break;
        case GO_NOISE:
            break;
        case GO_RANDOMIZE:
            if (object[i].cmd_index >= 32)
                goto end_run_commands;
            uint8_t *memory = &sprite_pattern[object[i].sprite_index][object[i].cmd_index];
            *memory = ((*memory)&15) | (randomize(param)<<4);
            break;
        case GO_QUAKE:
            break;
        }
    }
    // only way to get here is to get to object[i].cmd_index >= 32
    end_run_commands:
    object[i].cmd_index = 0;
}

void update_object(uint8_t i)
//int16_t x, int16_t y), object[index].x, object[index].y
{
    if (object[i].draw_order_index < 255) // object was visible...
    {
        if (on_screen(object[i].x, object[i].y))
        {
            // object is still visible, need to sort draw_order.. but do it later!
            object[i].iy = object[i].y - tile_map_y;
            object[i].ix = object[i].x - tile_map_x;
            if (object[i].z)
                object_run_commands(i);
        }
        else // object is no longer visible, but still exists
        {
            remove_object_from_view(i);
        }
    }
    else // wasn't visible
    {
        if (on_screen(object[i].x, object[i].y))
        {
            // object has become visible
            make_unseen_object_viewable(i);
            if (object[i].z)
                object_run_commands(i);
        }
        else // object is still not visible
        {
            // do nothing!
        }
    }
}

void update_object_image(uint8_t i)
//int16_t x, int16_t y), object[index].x, object[index].y
{
    if (object[i].draw_order_index < 255) // object was visible...
    {
        if (on_screen(object[i].x, object[i].y))
        {
            // object is still visible, need to sort draw_order.. but do it later!
            object[i].iy = object[i].y - tile_map_y;
            object[i].ix = object[i].x - tile_map_x;
        }
        else // object is no longer visible, but still exists
        {
            remove_object_from_view(i);
        }
    }
    else // wasn't visible
    {
        if (on_screen(object[i].x, object[i].y))
        {
            // object has become visible
            make_unseen_object_viewable(i);
        }
        else // object is still not visible
        {
            // do nothing!
        }
    }
}

void remove_object(uint8_t index)
{
    // free the object by linking previous indices to next indices and vice versa:
    uint8_t previous_index = object[index].previous_object_index;
    uint8_t next_index = object[index].next_object_index;
    if (previous_index == 255)
        // we were at the head of the list, update root:
        first_used_object_index = next_index;
    else
        object[previous_index].next_object_index = next_index;
    if (next_index < 255)
        // update the next object's link backwards:
        object[next_index].previous_object_index = previous_index;
    // put index at the head of the free list:
    object[index].previous_object_index = 255;
    object[index].next_object_index = first_free_object_index;
    first_free_object_index = index;
    // and remove object from view
    if (object[index].draw_order_index < 255)
        remove_object_from_view(index);
}

void remove_object_from_view(uint8_t i)
{
    for (int k=object[i].draw_order_index+1; k<drawing_count; ++k)
    {
        // move the pointers around for the objects draw_order_index:
        // object at draw_order k should go to spot k-1:
        int object_k = draw_order[k];
        // move down draw_order:
        object[object_k].draw_order_index = k-1;
        draw_order[k-1] = object_k;
    }
    object[i].iy = SCREEN_H;
    object[i].ix = SCREEN_W;
    object[i].draw_order_index = -1;
    --drawing_count;
}

void update_objects()
{
    uint8_t index = first_used_object_index;
    while (index < 255)
    {
        update_object(index);
        index = object[index].next_object_index;
    }
}

void sprites_line()
{
    if (!drawing_count) // none to draw...
        return;
    int16_t vga16 = (int16_t) vga_line;
    // add any new sprites to the draw line:
    while (last_drawing_index < drawing_count)
    {
        if (object[draw_order[last_drawing_index]].iy <= vga16) 
            ++last_drawing_index;
        else
            break;
    }
    // subtract any early sprites no longer on the line
    vga16-=16;
    while (first_drawing_index < drawing_count)
    {
        if (object[draw_order[first_drawing_index]].iy <= vga16) 
            ++first_drawing_index;
        else
            break;
    }
    // reset vga16
    vga16+=16;
    // draw on-screen and currently visible (on this line) sprites
    for (int k=first_drawing_index; k<last_drawing_index; ++k)
    {
        struct object *o = &object[draw_order[k]];
        if (!o->z)
            continue; // hidden object
        int sprite_draw_row = vga16 - o->iy;
        if (o->ix < 0) // object left of screen but still visible
        {
            uint16_t *dst = draw_buffer;
            uint8_t *src = &sprite_draw[o->sprite_index][o->sprite_frame][sprite_draw_row][-o->ix/2];
            uint8_t invisible_color = sprite_info[o->sprite_index][o->sprite_frame] & 31;
            if ((-o->ix)%2)
            {
                // if -o->ix == 15
                // this executes no matter what:
                uint8_t color = ((*src)>>4); //&15; //unnecessary!
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst;
                // if (o->ix == -13)
                // we run from 7 to <8, which is just once:
                for (int pxl=(-o->ix+1)/2; pxl<16/2; ++pxl) // 
                {
                    color = ((*(++src)))&15;
                    if (color != invisible_color)
                        *dst = palette[color]; // &65535; // unnecessary...
                    ++dst; 
                    color = ((*src)>>4); //&15; //unnecessary!
                    if (color != invisible_color)
                        *dst = palette[color]; // &65535; // unnecessary...
                    ++dst; 
                }
            }
            else // not odd
            {
                --src;
                for (int pxl=-o->ix/2; pxl<16/2; ++pxl)
                {
                    uint8_t color = (*(++src))&15;
                    if (color != invisible_color)
                        *dst = palette[color]; // &65535; // unnecessary...
                    ++dst; 
                    color = ((*src)>>4); //&15; //unnecessary!
                    if (color != invisible_color)
                        *dst = palette[color]; // &65535; // unnecessary...
                    ++dst; 
                }
            }
        }
        else if (o->ix > SCREEN_W-16)
        {
            // object right of screen but still visible
            int num_pxls = 1 + (SCREEN_W-1) - o->ix;
            uint16_t *dst = draw_buffer + o->ix;
            uint8_t *src = &sprite_draw[o->sprite_index][o->sprite_frame][sprite_draw_row][0]-1;
            uint8_t invisible_color = sprite_info[o->sprite_index][o->sprite_frame] & 31;
            uint8_t color;
            for (int pxl=0; pxl<num_pxls/2; ++pxl)
            {
                color = (*(++src))&15;
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst; 
                color = ((*src)>>4); //&15; //unnecessary!
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst; 
            }
            if (num_pxls%2)
            {
                color = (*(++src))&15;
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
            }
        }
        else
        {
            // object is somewhere in the middle of the screen
            uint16_t *dst = draw_buffer + o->ix;
            uint8_t *src = &sprite_draw[o->sprite_index][o->sprite_frame][sprite_draw_row][0]-1;
            uint8_t invisible_color = sprite_info[o->sprite_index][o->sprite_frame] & 31;
            for (int pxl=0; pxl<8; ++pxl)
            {
                uint8_t color = (*(++src))&15;
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst; 
                color = ((*src)>>4); //&15; //unnecessary!
                if (color != invisible_color)
                    *dst = palette[color]; // &65535; // unnecessary...
                ++dst; 
            }
        }
    }
}

static inline void swap_draw_order_k_kminus1(int k)
{
    int object_kminus1 = draw_order[k-1];
    int object_k = draw_order[k];

    object[object_kminus1].draw_order_index = k;
    object[object_k].draw_order_index = k-1;

    draw_order[k-1] = object_k;
    draw_order[k] = object_kminus1;
}

void sprites_frame()
{
    // insertion sort all the drawing objects on the map
    for (int j=0; j<drawing_count-1; ++j)
    {
        // move object k as far up as it should be.
        // lower z is drawn earlier, z=0 means the object is hidden inside a tile.
        for (int k=j+1; k > 0 && ((object[draw_order[k-1]].iy > object[draw_order[k]].iy) ||
            ((object[draw_order[k-1]].iy == object[draw_order[k]].iy) &&
             (object[draw_order[k-1]].z > object[draw_order[k]].z)) ); --k)
        {
            swap_draw_order_k_kminus1(k);
        }
    }
    
    first_drawing_index = 0;
    last_drawing_index = 0;
}

void sprites_reset()
{
    // create some random sprites...
    uint8_t *sc = sprite_draw[0][0][0];
    int color_index = 0;
    for (int tile=0; tile<15; ++tile)
    {
        for (int frame=0; frame<8; ++frame)
        {
            sprite_info[tile][frame] = 16;
            int forward = frame % 2;
            for (int line=0; line<16; ++line)
            {
                for (int col=0; col<8; ++col)
                    *sc++ = (((color_index+1)>>(tile/4))&15)|((((color_index - forward)>>(tile/4))&15)<<4);
                ++color_index;
            }
            color_index -= 16;
        }
        ++color_index;
    }
    // 16th sprite is random
    for (int l=0; l<8; ++l)
    {
        sprite_info[15][l] = 0; // with black as invisible for all frames
        for (int k=0; k<256/2; ++k)
            *sc++ = rand()%256;
    }
}

void update_object_images()
{
    uint8_t index = first_used_object_index;
    while (index < 255)
    {
        update_object_image(index);
        index = object[index].next_object_index;
    }
}
