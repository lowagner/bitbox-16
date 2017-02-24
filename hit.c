#include "bitbox.h"
#include "hit.h"
#include "common.h"
#include "go-sprite.h"
#include "unlocks.h"
#include "chiptune.h"
#include "run.h"

#include <math.h>
#include <stdlib.h> // rand

#define FIRE_COUNT 32 // number of times GO_NOT_FIRE needs to be called before you can fire again
#define SPEED_MULTIPLIER 0.5f
#define THROW_MULTIPLIER 0.6f
#define JUMP_MULTIPLIER 1.75f
#define ACCELERATION_DIVIDEND 4.0f
#define DECELERATION_DIVIDEND 4.0f
#define MAX_VY 10.0f


void set_checkpoint(int i)
{
    if (i != camera_index)
        return;
    message("checkpoint reached... TODO: do something\n");
}

static inline int test_inside_tile(int x, int y)
{
    int index = y*tile_map_width + x;
    uint8_t tile;
    if (index % 2)
        tile = tile_map[index/2] >> 4;
    else
        tile = tile_map[index/2] & 15;
    uint32_t info = tile_info[tile_translator[tile]];
    if (!(info & 8))
        return 0; // TODO: check for warps
    // check if blocked on each side:
    if (((info >> 16)&15) &&
        ((info >> 20)&15) &&
        ((info >> 24)&15) &&
        ((info >> 28)))
        return 1;
    return 0;
}

static inline int test_tile(int x, int y, int dir)
{
    int index = y*tile_map_width + x;
    uint8_t tile;
    if (index % 2)
        tile = tile_map[index/2] >> 4;
    else
        tile = tile_map[index/2] & 15;
    uint32_t info = tile_info[tile_translator[tile]];
    if (!(info & 8))
        return 0; // TODO: check for warps
    return (info >> (16 + 4*dir))&15;
}

static inline int damage_sprite(struct object *o, float damage)
{
    if (o->properties & PROJECTILE)
        goto kill_sprite;
    if (o->blink) // blinking protection
        return 0;
    
    int d = (int)damage;
    if (!d)
    {
        // roll for damage
        if (rand()%16 < (int)(16*damage))
        {
            // damage hit!
            if ((--o->health) == 0)
                goto kill_sprite;
        
            o->blink = 16;
        }
        return 0;
    }
    if (o->health <= d)
    {
        kill_sprite:
        o->health = 0;
        o->blink = 16 - (o->properties & PROJECTILE);
        o->vx = -3.75f+(rand()%16)*0.5f;
        o->vy = -gravity*8.0f -1.875f+(rand()%16)*0.25f;
        return 1; // sprite is dead
    }
    o->health -= d;
    o->blink = 16 + d/2;
    return 0; // sprite is ok, but blinking
}

int hit_tile(int i, float momentum, int hit)
{
    // return 0 if you want to cancel momentum, otherwise 1 or 2 (for bounce/superbounce), -1 for death
    switch (hit)
    {
        case Slippery:
            object[i].properties |= SLIPPING;
            break;
        case SuperSlippery:
            object[i].properties |= SUPER_SLIPPING;
            break;
        case Sticky:
            object[i].vx /= (1.0f + 0.5f*momentum);
            object[i].vy /= (1.0f + 0.5f*momentum);
            object[i].properties |= STICKING | CAN_JUMP;
            object[i].properties &= ~RUNNING;
            break;
        case SuperSticky:
            object[i].vx /= (1.0f + 2.0f*momentum);
            object[i].vy /= (1.0f + 2.0f*momentum);
            object[i].properties |= SUPER_STICKING | CAN_JUMP;
            object[i].properties &= ~RUNNING;
            break;
        case Bouncy:
            message("got bouncy\n");
            return 1;
        case SuperBouncy:
            message("got super bouncy\n");
            return 2;
        case Damaging:
            if (damage_sprite(&object[i], momentum * damage_multiplier))
                return -1;
            break;
        case SuperDamaging:
            if (damage_sprite(&object[i], momentum * damage_multiplier * 2.0f))
                return -1;
            break;
        case Unlock0:
            set_unlock(i, 0);
            break;
        case Unlock1:
            set_unlock(i, 1);
            break;
        case Unlock2:
            set_unlock(i, 2);
            break;
        case Unlock3:
            set_unlock(i, 3);
            break;
        case Checkpoint:
            set_checkpoint(i);
            break;
        case Win:
            unlocks_win(i);
            break;
        default:
            break;
    }
    return 0;
}

static inline int compare_hit(int i, float y_momentum, float x_delta, int hit_left, int hit_right)
{
    // return 0 if you want to cancel y momentum, otherwise 1 or 2 (for bounce/superbounce), -1 for death
    if (x_delta < 8.0f)
        return hit_tile(i, y_momentum, hit_left);
    else
        return hit_tile(i, y_momentum, hit_right);
}

void object_run_commands(int i) 
{
    // update position here, too
    object[i].properties &= ~(CAN_JUMP|SLIPPING|SUPER_SLIPPING|STICKING|SUPER_STICKING); // assume you're not slipping or sticking, until we find out as much
    object[i].blocked = 0;
    if (object[i].blink)
    {
        if (vga_frame % 2)
            --object[i].blink;
        if (!object[i].health) // no health
        {
            if (!object[i].blink) // end death blink process
            {
                if (camera_index == i)
                    kill_player();
                return remove_object(i);
            }
            object[i].vy += gravity;
            object[i].x += object[i].vx;
            object[i].y += object[i].vy;
            return;
        }
    }
    if (object[i].properties & GHOSTING)
    {
        object[i].y += object[i].vy;
        if (object[i].y > tile_map_height*16 - 32)
        {
            object[i].y = tile_map_height*16 - 32;
            object[i].vy = 0;
            object[i].blocked |= BLOCKED_DOWN;
        }
        
        object[i].x += object[i].vx;
        if (object[i].x < 16)
        {
            object[i].x = 16;
            object[i].vx = 0;
            object[i].blocked |= BLOCKED_LEFT;
        }
        else if (object[i].x > tile_map_width*16-32)
        {
            object[i].x = tile_map_width*16-32;
            object[i].vx = 0;
            object[i].blocked |= BLOCKED_RIGHT;
        }

        goto object_execute_commands;
    }
    // run physics and hit tests
    //   e.g. if colliding against ground, set vy = 0

    // avoid adding in gravity for non-platformers:
    if (gravity)
    {
        object[i].vy += gravity;
        if (object[i].vy > MAX_VY)
            object[i].vy = MAX_VY;
    }
    object[i].y += object[i].vy;
    if (object[i].y < -16) {}
    else if (object[i].y > tile_map_height*16 - 32)
    {
        object[i].y = tile_map_height*16 - 32;
        object[i].vy = 0;
        object[i].properties |= CAN_JUMP;
        object[i].blocked |= BLOCKED_DOWN;
        // TODO: maybe test object right below player here.
    }
    else if (object[i].vy == 0.0f || (16.0f*((int)(object[i].y/16)) == object[i].y)) {}
    else if (object[i].vy > 0)
    {
        // player is falling downwards.
        // test collision onto some tile's UP side.
        int y_tile = object[i].y/16 + 1;
        int x_tile = object[i].x/16;
        float x_delta = object[i].x - 16.0f*x_tile;
        int hit_left = test_tile(x_tile, y_tile, UP);
        int hit_right;
        if (x_delta == 0.0f)
        {
            hit_right = Passable;
            if (hit_left)
            {
                object[i].y = 16*(y_tile-1);
                object[i].properties |= CAN_JUMP;
                object[i].blocked |= BLOCKED_DOWN;
                goto test_top_left_block_only;
            }
        }
        // else gotta test left and right tiles
        else if ((hit_right = test_tile(x_tile+1, y_tile, UP)))
        {
            object[i].y = 16*(y_tile-1);
            object[i].properties |= CAN_JUMP;
            object[i].blocked |= BLOCKED_DOWN;
            if (hit_left)
            switch (compare_hit(i, object[i].vy, x_delta, hit_left, hit_right))
            {
                case -1:
                    return;
                case 0:
                    object[i].vy = 0.0f;
                    break;
                case 1: // bounce
                    object[i].vy *= -0.5f;
                    break;
                default: // super bounce
                    object[i].vy *= -1.0f;
                    break;
            }
            else if (x_delta < 3.0f && object[i].vx < 0)
            // about to fall off to the left:
            // decide what to do based on edge behavior
            switch (object[i].edge_behavior)
            {
                case 9:
                case 10:
                case 11:
                    // turn around, if the current field doesn't look terrible
                    if (hit_right/2 == Damaging/2 && !(sprite_info[object[i].sprite_index]&(1<<(12+DOWN))))
                        goto fall_through_left;
                    object[i].x = x_tile*16.0f + 3.0f;
                    object[i].vx *= -1;
                    object[i].sprite_index = (object[i].sprite_index/8)*8 + 
                        (object[i].sprite_index + 4)%8;
                    object[i].vy = 0;
                    break;
                case 12:
                    // jump
                    message("oh jump left!\n");
                    object[i].vy -= object[i].jump_speed*JUMP_MULTIPLIER/
                        (1.0f+((object[i].properties&(STICKING|SUPER_STICKING))>>5));
                    break;
                case 13:
                    // turn around if low enough speed
                    if (hit_right/2 == Damaging/2 && !(sprite_info[object[i].sprite_index]&(1<<(12+DOWN))))
                        goto fall_through_left;
                    if (object[i].vx > -SPEED_MULTIPLIER)
                    {
                        object[i].x = x_tile*16.0f + 3.0f;
                        object[i].vx *= -1; 
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 
                            (object[i].sprite_index + 4)%8;
                    }
                    else
                        object[i].vx /= (1.0f + (object[i].acceleration>>2)/
                            (DECELERATION_DIVIDEND + ((object[i].properties&(SUPER_SLIPPING|SLIPPING)))));
                    object[i].vy = 0;
                    break;
                case 14:
                    // stop if small enough speed
                    if (hit_right/2 == Damaging/2 && !(sprite_info[object[i].sprite_index]&(1<<(12+DOWN))))
                        goto fall_through_left;
                    if (object[i].vx > -SPEED_MULTIPLIER)
                    {
                        object[i].x = x_tile*16.0f + 3.0f;
                        object[i].vx = 0; 
                    }
                    else
                        object[i].vx /= (1.0f + (object[i].acceleration>>2)/
                            (DECELERATION_DIVIDEND + ((object[i].properties&(SUPER_SLIPPING|SLIPPING)))));
                    object[i].vy = 0;
                    break;
                case 15:
                    if (hit_right/2 == Damaging/2 && !(sprite_info[object[i].sprite_index]&(1<<(12+DOWN))))
                        goto fall_through_left;
                    // stop no matter what
                    object[i].x = x_tile*16.0f + 3.0f;
                    object[i].vx = 0;
                    object[i].vy = 0;
                    break;
                default: // 8 or lower
                    fall_through_left:
                    object[i].x = x_tile*16.0f;
                    if (object[i].vx > -1.0f)
                        object[i].vx = 0;
            }
            else switch (hit_tile(i, object[i].vy, hit_right))
            {
                case -1:
                    return;
                case 0:
                    object[i].vy = 0.0f;
                    break;
                case 1: // bounce
                    object[i].vy *= -0.5f;
                    break;
                default: // super bounce
                    object[i].vy *= -1.0f;
                    break;
            }
        }
        else if (hit_left) // but not hit_right
        {
            object[i].y = 16*(y_tile-1);
            object[i].properties |= CAN_JUMP;
            object[i].blocked |= BLOCKED_DOWN;
            if (x_delta > 13.0f && object[i].vx > 0)
            // about to fall off to the right:
            // decide what to do based on edge behavior, and what is currently here
            switch (object[i].edge_behavior)
            {
                case 9:
                case 10:
                case 11:
                    // turn around, if not in a damage field
                    if (hit_left/2 == Damaging/2 && !(sprite_info[object[i].sprite_index]&(1<<(12+DOWN))))
                        goto fall_through_right;
                    object[i].x = (x_tile+1)*16.0f - 3.0f;
                    object[i].vx *= -1;
                    object[i].sprite_index = (object[i].sprite_index/8)*8 + 
                        (object[i].sprite_index + 4)%8;
                    object[i].vy = 0;
                    break;
                case 12:
                    // jump
                    message("oh jump right!\n");
                    object[i].vy -= object[i].jump_speed*JUMP_MULTIPLIER/
                        (1.0f+((object[i].properties&(STICKING|SUPER_STICKING))>>5));
                    break;
                case 13:
                    // turn around if low enough speed
                    if (hit_left/2 == Damaging/2 && !(sprite_info[object[i].sprite_index]&(1<<(12+DOWN))))
                        goto fall_through_right;
                    if (object[i].vx < SPEED_MULTIPLIER)
                    {
                        object[i].x = (x_tile+1)*16.0f - 3.0f;
                        object[i].vx *= -1; 
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 
                            (object[i].sprite_index + 4)%8;
                    }
                    else
                        object[i].vx /= (1.0f + (object[i].acceleration>>2)/
                            (DECELERATION_DIVIDEND + ((object[i].properties&(SUPER_SLIPPING|SLIPPING)))));
                    object[i].vy = 0;
                    break;
                case 14:
                    // stop if small enough speed
                    if (hit_left/2 == Damaging/2 && !(sprite_info[object[i].sprite_index]&(1<<(12+DOWN))))
                        goto fall_through_right;
                    if (object[i].vx < SPEED_MULTIPLIER)
                    {
                        object[i].x = (x_tile+1)*16.0f - 3.0f;
                        object[i].vx = 0; 
                    }
                    else
                        object[i].vx /= (1.0f + (object[i].acceleration>>2)/
                            (DECELERATION_DIVIDEND + ((object[i].properties&(SUPER_SLIPPING|SLIPPING)))));
                    object[i].vy = 0;
                    break;
                case 15:
                    if (hit_left/2 == Damaging/2 && !(sprite_info[object[i].sprite_index]&(1<<(12+DOWN))))
                        goto fall_through_right;
                    // stop no matter what
                    object[i].x = (x_tile+1)*16.0f - 3.0f;
                    object[i].vx = 0;
                    object[i].vy = 0;
                    break;
                default: // 8 or lower
                    fall_through_right:
                    object[i].x = (x_tile+1)*16.0f;
                    if (object[i].vx < 1.0f)
                        object[i].vx = 0;
            }
            else
            {
                test_top_left_block_only:
                switch (hit_tile(i, object[i].vy, hit_left))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vy = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vy *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vy *= -1.0f;
                        break;
                }
            }
        }
    }
    else if (object[i].y < 0) {}
    else // vy < 0
    {
        // check bumping head
        int y_tile = object[i].y/16;
        int x_tile = object[i].x/16;
        float x_delta = object[i].x - 16.0f*x_tile;
        
        int hit_left = test_tile(x_tile, y_tile, DOWN);
        int hit_right;
        if (x_delta == 0.0f)
        {
            hit_right = Passable;
            if (hit_left)
                goto test_bottom_left_block_only;
        }
        // gotta test left and right tiles
        else if ((hit_right = test_tile(x_tile+1, y_tile, DOWN)))
        {
            if (hit_left)
            {
                object[i].y = 16*(y_tile+1);
                object[i].blocked |= BLOCKED_UP;
                switch (compare_hit(i, -object[i].vy, x_delta, hit_left, hit_right))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vy = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vy *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vy *= -1.0f;
                        break;
                }
            }
            else if (x_delta < 3.0f) // hit right but not left
            {
                // be nice, jump into hole
                object[i].x = 16.0f*x_tile;
                message("jumping up into left hole\n");
            }
            else 
            {
                object[i].y = 16*(y_tile+1);
                object[i].blocked |= BLOCKED_UP;
                switch (hit_tile(i, -object[i].vy, hit_right))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vy = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vy *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vy *= -1.0f;
                        break;
                }
            }
        }
        else if (hit_left) // but not hit_right
        {
            if (x_delta > 13.0f)
            {
                // be nice, jump up into hole
                object[i].x = 16.0f*(x_tile+1);
                message("jumping up into right hole\n");
            }
            else
            {
                test_bottom_left_block_only:
                object[i].y = 16*(y_tile+1);
                object[i].blocked |= BLOCKED_UP;
                switch (hit_tile(i, -object[i].vy, hit_left))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vy = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vy *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vy *= -1.0f;
                        break;
                }
            }
        }
    }
    object[i].x += object[i].vx;
    if (object[i].x < 16)
    {
        object[i].x = 16;
        object[i].vx = 0;
        object[i].blocked |= BLOCKED_LEFT;
    }
    else if (object[i].x > tile_map_width*16-32)
    {
        object[i].x = tile_map_width*16-32;
        object[i].vx = 0;
        object[i].blocked |= BLOCKED_RIGHT;
    }
    else if (object[i].y < 0 || object[i].vx == 0.0f || ( ((int)(object[i].x/16)*16.0f) == object[i].x)) {}
    else if (object[i].vx > 0)
    {
        // test colliding into something's LEFT side
        float old_vx = object[i].vx;
        int x_tile = object[i].x/16 + 1;
        int y_tile = object[i].y/16;
        float y_delta = object[i].y - 16.0f*y_tile;
        int hit_up = test_tile(x_tile, y_tile, LEFT);
        int hit_down;
        if (y_delta == 0.0f)
        {
            hit_down = Passable;
            if (hit_up)
                goto test_right_top_block_only;
        }
        else if ((hit_down = test_tile(x_tile, y_tile+1, LEFT)))
        {
            if (hit_up)
            {
                object[i].blocked |= BLOCKED_RIGHT;
                object[i].x = 16*(x_tile-1);
                switch (compare_hit(i, object[i].vx, y_delta, hit_up, hit_down))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vx = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vx *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vx *= -1.0f;
                        break;
                }
            }
            else if (y_delta < 3.0f) // nothing above, squeeze up there
                object[i].y = 16*(y_tile);
            else 
            {
                object[i].x = 16*(x_tile-1);
                object[i].blocked |= BLOCKED_RIGHT;
                switch (hit_tile(i, object[i].vx, hit_down))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vx = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vx *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vx *= -1.0f;
                        break;
                }
            }
        }
        else if (hit_up)
        {
            if (y_delta > 13.0f) // nothing below, squeeze down there
                object[i].y = 16*(y_tile+1);
            else
            {
                test_right_top_block_only:
                object[i].x = 16*(x_tile-1);
                object[i].blocked |= BLOCKED_RIGHT;
                switch (hit_tile(i, object[i].vx, hit_up))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vx = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vx *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vx *= -1.0f;
                        break;
                }
            }
        }
        if (object[i].vx == 0.0f)
        switch (object[i].edge_behavior)
        {
            case 8: // fall off edges
            case 14: // stop or fall
            case 15: // stop at edges
                break;
            case 9: // turn CCW
                object[i].vy = -old_vx;
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*UP;
                break;
            case 11: // turn CW
                object[i].vy = old_vx;
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*DOWN;
                break;
            default: // turn 180 (at 10), plus other things
                object[i].vx = -old_vx;
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*LEFT;
                break;
        }
    }
    else // vx < 0
    {
        float old_vx = object[i].vx;
        int x_tile = object[i].x/16;
        int y_tile = object[i].y/16;
        float y_delta = object[i].y - 16.0f*y_tile;
        int hit_up = test_tile(x_tile, y_tile, RIGHT);
        int hit_down;
        if (y_delta == 0.0f)
        {
            hit_down = Passable;
            if (hit_up)
                goto test_left_top_block_only;
        }
        else if ((hit_down = test_tile(x_tile, y_tile+1, RIGHT)))
        {
            if (hit_up)
            {
                object[i].x = 16*(x_tile+1);
                object[i].blocked |= BLOCKED_LEFT;
                switch (compare_hit(i, -object[i].vx, y_delta, hit_up, hit_down))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vx = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vx *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vx *= -1.0f;
                        break;
                }
            }
            else if (y_delta < 3.0f)
                object[i].y = 16*(y_tile);
            else
            {
                object[i].x = 16*(x_tile+1);
                object[i].blocked |= BLOCKED_LEFT;
                switch (hit_tile(i, -object[i].vx, hit_down))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vx = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vx *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vx *= -1.0f;
                        break;
                }
            }
        }
        else if (hit_up)
        {
            if (y_delta > 13.0f)
                object[i].y = 16*(y_tile+1);
            else
            {
                test_left_top_block_only:
                object[i].x = 16*(x_tile+1);
                object[i].blocked |= BLOCKED_LEFT;
                switch (hit_tile(i, object[i].vx, hit_up))
                {
                    case -1:
                        return;
                    case 0:
                        object[i].vx = 0.0f;
                        break;
                    case 1: // bounce
                        object[i].vx *= -0.5f;
                        break;
                    default: // super bounce
                        object[i].vx *= -1.0f;
                        break;
                }
            }

        }
        if (object[i].vx == 0.0f)
        switch (object[i].edge_behavior)
        {
            case 8: // fall off edges
            case 14: // stop or fall
            case 15: // stop at edges
                break;
            case 9: // turn CCW
                object[i].vy = old_vx;
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*DOWN;
                break;
            case 11: // turn CW
                object[i].vy = -old_vx;
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*UP;
                break;
            default: // turn 180 (at 10), plus other things
                object[i].vx = -old_vx;
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*RIGHT;
                break;
        }
    }
    // finally check the space that the sprite is in.
    /*
    int y_tile = object[i].y;
    if (y_tile%16)
    {
        y_tile = y_tile/16 + 1;
        if (tile_xy_is_block(object[i].x/16, y_tile))
        {
            object[i].y = 16.0f*(y_tile-1);
            message("internal hit %f\n", object[i].y);
            object[i].vy = 0;
            object[i].properties |= CAN_JUMP;
        }
    }
    */

    object_execute_commands:
    if (object[i].wait)
    {
        switch (object[i].sprite_index%8/2)
        {
            case RIGHT:
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*RIGHT + ((int)object[i].x/(8+8*(object[i].properties&RUNNING)))%2;
                break;
            case UP:
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*UP + ((int)object[i].y/(8+8*(object[i].properties&RUNNING)))%2;
                break;
            case LEFT:
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*LEFT + ((int)object[i].x/(8+8*(object[i].properties&RUNNING)))%2;
                break;
            case DOWN:
                object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*DOWN + ((int)object[i].y/(8+8*(object[i].properties&RUNNING)))%2;
                break;
        }
        --object[i].wait;
        return;
    }
    while (object[i].cmd_index < 32)
    {
        uint8_t cmd = sprite_pattern[object[i].sprite_index/8][object[i].cmd_index++];
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
                break;
            if (!param)
                param = 16;
            object[i].cmd_index += param;
        case GO_NOT_RUN:
            if (object[i].properties & RUNNING)
                break;
            if (!param)
                param = 16;
            object[i].cmd_index += param;
            break;
        case GO_NOT_AIR:
            if (!(object[i].properties & CAN_JUMP))
                break;
            if (!param)
                param = 16;
            object[i].cmd_index += param;
            break;
        case GO_NOT_FIRE:
            if (object[i].firing && (--object[i].firing == FIRE_COUNT-1))
                // continue executing at next index if firing was FIRE_COUNT
                break;
            // otherwise, jump forward a few indices:
            if (!param)
                param = 16;
            object[i].cmd_index += param;
            break;
        case GO_EXECUTE:
            switch (param)
            {
            case 0: // stop
                object[i].vx = object[i].vy = 0;
                break;
            case 1: // turn CCW
            {
                object[i].sprite_index = (object[i].sprite_index/8)*8 + (object[i].sprite_index+2)%8;
                float vx = object[i].vx;
                object[i].vx = object[i].vy;
                object[i].vy = -vx;
                break;
            }
            case 2: // turn 180
                object[i].sprite_index = (object[i].sprite_index/8)*8 + (object[i].sprite_index+4)%8;
                object[i].vx = -object[i].vx;
                object[i].vy = -object[i].vy;
                break;
            case 3: // turn 270 = CW
            {
                object[i].sprite_index = (object[i].sprite_index/8)*8 + (object[i].sprite_index+6)%8;
                float vx = object[i].vx;
                object[i].vx = -object[i].vy;
                object[i].vy = vx;
                break;
            }
            case 4: // walk
                switch ((object[i].sprite_index%8)/2)
                {
                case RIGHT:
                    object[i].vy = 0;
                    object[i].vx = object[i].speed*SPEED_MULTIPLIER;
                    object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*RIGHT + ((int)object[i].x/(8+8*(object[i].properties&RUNNING)))%2;
                    break;
                case UP:
                    object[i].vy = -object[i].speed*SPEED_MULTIPLIER;
                    object[i].vx = 0;
                    if (gravity)
                        object[i].properties &= ~CAN_JUMP;
                    object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*UP + ((int)object[i].y/(8+8*(object[i].properties&RUNNING)))%2;
                    break;
                case LEFT:
                    object[i].vy = 0;
                    object[i].vx = -object[i].speed*SPEED_MULTIPLIER;
                    object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*LEFT + ((int)object[i].x/(8+8*(object[i].properties&RUNNING)))%2;
                    break;
                case DOWN:
                    object[i].vy = object[i].speed*SPEED_MULTIPLIER;
                    object[i].vx = 0;
                    object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*DOWN + ((int)object[i].y/(8+8*(object[i].properties&RUNNING)))%2;
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
                object[i].vy -= object[i].jump_speed*JUMP_MULTIPLIER/
                    (1.0f+((object[i].properties&(STICKING|SUPER_STICKING))>>5));
                object[i].properties &= ~CAN_JUMP;
                break;
            case 7: // toggle ghost
                if (object[i].properties & GHOSTING)
                    object[i].properties &= ~GHOSTING;
                else
                    object[i].properties |= GHOSTING;
                break;
            default:
                // the rest are edge behaviors
                object[i].edge_behavior = param;
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
                    object[i].vx /= (1.0f + (object[i].acceleration>>2)/
                        (DECELERATION_DIVIDEND + ((object[i].properties&(SUPER_SLIPPING|SLIPPING)))));
                    if ((object[i].properties & GHOSTING) || !gravity) // for non-platformer
                    {
                        object[i].vy /= (1.0f + (object[i].acceleration>>2)/
                            (DECELERATION_DIVIDEND + ((object[i].properties&(SUPER_SLIPPING|SLIPPING)))));
                    }
                    break;
                }
                switch (rand()%4)
                {
                    case 0:
                    {
                        object[i].vx -= (1+(object[i].acceleration&3))/
                            (ACCELERATION_DIVIDEND+((object[i].properties&(SUPER_SLIPPING|SLIPPING))));
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*LEFT;
                        float vx_limit = -object[i].speed*SPEED_MULTIPLIER;
                        if (object[i].vx < vx_limit)
                            object[i].vx = vx_limit;
                        break;
                    }
                    case 1:
                    {
                        object[i].vx += (1+(object[i].acceleration&3))/
                            (ACCELERATION_DIVIDEND+((object[i].properties&(SUPER_SLIPPING|SLIPPING))));
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*RIGHT;
                        float vx_limit = object[i].speed*SPEED_MULTIPLIER;
                        if (object[i].vx > vx_limit)
                            object[i].vx = vx_limit;
                        break;
                    }
                    case 2:
                    if ((object[i].properties & GHOSTING) || !gravity) // non-platformer
                    {
                        object[i].vy -= (1+(object[i].acceleration&3))/
                            (ACCELERATION_DIVIDEND+((object[i].properties&(SUPER_SLIPPING|SLIPPING))));
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*UP;
                        float vy_limit = -object[i].jump_speed*SPEED_MULTIPLIER;
                        if (object[i].vy < vy_limit)
                            object[i].vy = vy_limit;
                    }
                    break;
                    case 3:
                    {
                        object[i].vy += (1+(object[i].acceleration&3))/
                            (ACCELERATION_DIVIDEND+((object[i].properties&(SUPER_SLIPPING|SLIPPING))));
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*DOWN;
                        // only check this for non-platformer,
                        // a platformer will fix vy automatically
                        if ((object[i].properties & GHOSTING) || !gravity)
                        {
                            float vy_limit = object[i].jump_speed*SPEED_MULTIPLIER;
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
                        object[i].vx -= (1+(object[i].acceleration&3))/
                            (ACCELERATION_DIVIDEND+((object[i].properties&(SUPER_SLIPPING|SLIPPING))));
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*LEFT + ((int)object[i].x/(8+8*(object[i].properties&RUNNING)))%2;
                        float vx_limit = -object[i].speed*SPEED_MULTIPLIER;
                        if (object[i].vx < vx_limit)
                            object[i].vx = vx_limit;
                    }
                    else
                    {
                        object[i].vx += (1+(object[i].acceleration&3))/
                            (ACCELERATION_DIVIDEND+((object[i].properties&(SUPER_SLIPPING|SLIPPING))));
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*RIGHT + ((int)object[i].x/(8+8*(object[i].properties&RUNNING)))%2;
                        float vx_limit = object[i].speed*SPEED_MULTIPLIER;
                        if (object[i].vx > vx_limit)
                            object[i].vx = vx_limit;
                    }
                }
                else
                {
                    object[i].vx /= (1.0f + (object[i].acceleration>>2)/
                        (DECELERATION_DIVIDEND + ((object[i].properties&(SUPER_SLIPPING|SLIPPING)))));
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
                        object[i].vy -= (1+(object[i].acceleration&3))/
                            (ACCELERATION_DIVIDEND+((object[i].properties&(SUPER_SLIPPING|SLIPPING))));
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*UP + ((int)object[i].y/(8+8*(object[i].properties&RUNNING)))%2;
                        if ((object[i].properties & GHOSTING) || !gravity) // check for non-platformer
                        {
                            float vy_limit = -object[i].jump_speed*SPEED_MULTIPLIER;
                            if (object[i].vy < vy_limit)
                                object[i].vy = vy_limit;
                        }
                    }
                    else
                    {
                        object[i].vy += (1+(object[i].acceleration&3))/
                            (ACCELERATION_DIVIDEND+((object[i].properties&(SUPER_SLIPPING|SLIPPING))));
                        object[i].sprite_index = (object[i].sprite_index/8)*8 + 2*DOWN + ((int)object[i].y/(8+8*(object[i].properties&RUNNING)))%2;
                        // only check this for non-platformer,
                        // a platformer will fix vy automatically
                        if ((object[i].properties & GHOSTING) || !gravity)
                        {
                            float vy_limit = object[i].jump_speed*SPEED_MULTIPLIER;
                            if (object[i].vy > vy_limit)
                                object[i].vy = vy_limit;
                        }
                    }
                }
                else if ((object[i].properties & GHOSTING) || !gravity) // non-platformer
                {
                    object[i].vy /= (1.0f + (object[i].acceleration>>2)/
                        (DECELERATION_DIVIDEND + ((object[i].properties&(SUPER_SLIPPING|SLIPPING)))));
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
                    object[i].properties |= RUNNING;
                else
                    object[i].properties &= ~RUNNING;
            }
            if (param & 2) // jump
            {
                if (GAMEPAD_PRESSED(p, B))
                {
                    object[i].vy = -object[i].jump_speed*JUMP_MULTIPLIER/
                        (1.0f+((object[i].properties&(STICKING|SUPER_STICKING))>>5));
                    object[i].properties &= ~CAN_JUMP;
                }
            }
            if (param & 4) // fire 
            {
                if (GAMEPAD_PRESSED(p, X))
                {
                    if (!object[i].firing)
                        object[i].firing = FIRE_COUNT;
                }
                else if (object[i].firing == FIRE_COUNT)
                {
                    // haven't encountered a "GO_NOT_FIRE" yet,
                    // assume we need to turn off the fire power.
                    object[i].firing = 0;
                }
            }
            break;
        }
        case GO_SPAWN_TILE:
        {
            int32_t y, x;
            float delta_y, delta_x;
            y = object[i].y/16;
            delta_y = object[i].y - 16*y;
            x = object[i].x/16;
            delta_x = object[i].x - 16*x;
            switch ((object[i].sprite_index%8)/2)
            {
                case RIGHT:
                    if (delta_x < 6.0f)
                        object[i].x = (x++)*16;
                    else
                        x += 2;
                    if (delta_y > 10.0f)
                        ++y;
                    break;
                case UP:
                    if (delta_x > 10.0f)
                        ++x;
                    if (delta_y > 10.0f)
                        object[i].y = (y+1)*16;
                    else
                        --y;
                    break;
                case LEFT:
                    if (delta_x > 10.0f)
                        object[i].x = (x+1)*16;
                    else
                        --x;
                    if (delta_y > 10.0f)
                        ++y;
                    break;
                case DOWN:
                    if (delta_x > 10.0f)
                        ++x;
                    if (delta_y < 6.0f)
                        object[i].y = (y++)*16;
                    else
                        y+=2;
                    break;
            }
            int index = y*tile_map_width + x;
            if (index % 2)
            {
                tile_map[index/2] &= 15;
                tile_map[index/2] |= param<<4;
            }
            else
            {
                tile_map[index/2] &= 240;
                tile_map[index/2] |= param;
            }
            break;
        }
        case GO_SPAWN_SPRITE:
        {
            uint8_t j = 255;
            float vx = object[i].vx, vy = object[i].vy;
            int ty = object[i].y/16, tx = object[i].x/16;
            switch ((object[i].sprite_index%8)/2)
            {
                case RIGHT:
                    if (16.0f*ty == object[i].y)
                    {
                        if (test_inside_tile(tx+1, ty))
                            break;
                    }
                    else if (test_inside_tile(tx+1, ty) || test_inside_tile(tx+1, ty+1))
                        break;
                    j = create_object(8*param + RIGHT*2, object[i].x+16, object[i].y, 1);
                    vx += object[i].speed*THROW_MULTIPLIER;
                    if (gravity) // platformer
                        vy -= object[i].jump_speed*THROW_MULTIPLIER;
                    break;
                case UP:
                    if (16.0f*tx == object[i].x)
                    {
                        if (test_inside_tile(tx, ty-1))
                            break;
                    }
                    else if (test_inside_tile(tx, ty-1) || test_inside_tile(tx+1, ty-1))
                        break;
                    j = create_object(8*param + UP*2, object[i].x, object[i].y-16, 1);
                    vy -= object[i].speed*THROW_MULTIPLIER;
                    if (gravity) // platformer
                        vy -= object[i].jump_speed*THROW_MULTIPLIER/2;
                    break;
                case LEFT:
                    if (16.0f*ty == object[i].y)
                    {
                        if (test_inside_tile(tx-1, ty))
                            break;
                    }
                    else if (test_inside_tile(tx-1, ty) || test_inside_tile(tx-1, ty+1))
                        break;
                    j = create_object(8*param + LEFT*2, object[i].x-16, object[i].y, 1);
                    vx -= object[i].speed*THROW_MULTIPLIER;
                    if (gravity) // platformer
                        vy -= object[i].jump_speed*THROW_MULTIPLIER;
                    break;
                case DOWN:
                    if (16.0f*tx == object[i].x)
                    {
                        if (test_inside_tile(tx, ty+1))
                            break;
                    }
                    else if (test_inside_tile(tx, ty+1) || test_inside_tile(tx+1, ty+1))
                        break;
                    j = create_object(8*param + DOWN*2, object[i].x, object[i].y+16, 1);
                    vy += object[i].speed*THROW_MULTIPLIER;
                    break;
            }
            if (j == 255)
            {
                // reset, couldn't spawn a sprite.
                object[i].cmd_index = 0;
                return;
            }
            object[j].vx = vx;
            object[j].vy = vy;
            object[j].properties |= PROJECTILE;
            break;
        }
        case GO_ACCELERATION:
            object[i].acceleration = param;
            break;
        case GO_SPEED:
            if (param & 8) // jump speed
                object[i].jump_speed = param-7;
            else // run speed
                object[i].speed = 1 + param;
            break;
        case GO_NOISE:
        {
            chip_note(
                rand()%4, // random player
                param, // instrument
                rand()%16, // random note
                255 // full volume 
            ); 
            break;
        }
        case GO_RANDOMIZE:
            if (object[i].cmd_index >= 32)
                goto end_run_commands;
            uint8_t *memory = &sprite_pattern[object[i].sprite_index/8][object[i].cmd_index];
            *memory = ((*memory)&15) | (randomize(param)<<4);
            break;
        case GO_QUAKE:
            if (param)
                camera_shake += param + param*param;
            else
                camera_shake = 0;
            break;
        }
    }
    // only way to get here is to get to object[i].cmd_index >= 32
    end_run_commands:
    object[i].cmd_index = 0;
}

int combine_properties(int prop1, int prop2)
{
    switch (prop1)
    {
        case Slippery:
            switch (prop2)
            {
                case SuperSlippery:
                    return SuperSlippery;
                case Sticky:
                    return Normal;
                case SuperSticky:
                    return Sticky;
                case Bouncy:
                    return Bouncy;
                case SuperBouncy:
                    return SuperBouncy;
                default:
                    return Slippery;
            }
        case SuperSlippery:
            switch (prop2)
            {
                case Sticky:
                    return Slippery;
                case SuperSticky:
                    return Normal;
                case Bouncy:
                    return Bouncy;
                case SuperBouncy:
                    return SuperBouncy;
                default:
                    return SuperSlippery;
            }
        case Sticky:
            switch (prop2)
            {
                case Slippery:
                case Bouncy:
                    return Normal;
                case SuperSlippery:
                    return Slippery;
                case SuperBouncy:
                    return Bouncy;
                default:
                    return Sticky;
            }
        case SuperSticky:
            switch (prop2)
            {
                case Slippery:
                case Bouncy:
                    return Sticky;
                case SuperSlippery:
                case SuperBouncy:
                    return Normal;
                default:
                    return SuperSticky;
            }
        case Bouncy:
            switch (prop2)
            {
                case Sticky:
                    return Normal;
                case SuperSticky:
                    return Sticky;
                case SuperBouncy:
                    return SuperBouncy;
                default:
                    return Bouncy;
            }
        case SuperBouncy:
            switch (prop2)
            {
                case Sticky:
                    return Bouncy;
                case SuperSticky:
                    return Normal;
                default:
                    return SuperBouncy;
            }
        default:
            return prop2;
    }
}

void collide_vertically(struct object *o, struct object *p)
{
    // REQUIRED: o.y < p.y
    const float rel_velocity = o->vy - p->vy;
    if (rel_velocity == 0.0)
        return;
    int o_bottom = get_sprite_property(o, DOWN);
    int p_top = get_sprite_property(p, UP);
    int op_combined = combine_properties(o_bottom, p_top); 
    if (rel_velocity > 0.0 && (p->blocked & BLOCKED_DOWN))
    {
        o->iy = p->iy - 16;
        o->y = p->y - 16.0;
        o->blocked |= BLOCKED_DOWN;
        switch (op_combined)
        {
            case Bouncy:
                o->vy *= -0.5;
                break;
            case SuperBouncy:
                o->vy *= -1.0;
                break;
            case Sticky:
                p->vx += 0.5*o->vx;
                o->vx *= 0.5;
                o->vy = 0.0;
                break;
            case SuperSticky:
                p->vx += 0.25*o->vx;
                o->vx *= 0.75;
                o->vy = 0.0;
                break;
            default:
                o->vy = 0.0;    
        }
    }
    else if (rel_velocity < 0.0 && (o->blocked & BLOCKED_UP))
    {
        p->iy = o->iy + 16;
        o->y = p->y + 16.0;
        p->blocked |= BLOCKED_UP;
        switch (op_combined)
        {
            case Bouncy:
                p->vy *= -0.5;
                break;
            case SuperBouncy:
                p->vy *= -1.0;
                break;
            case Sticky:
                o->vx += 0.5*p->vx;
                p->vx *= 0.5;
                p->vy = 0.0;
                break;
            case SuperSticky:
                o->vx += 0.25*p->vx;
                p->vx *= 0.75;
                p->vy = 0.0;
                break;
            default:
                p->vy = 0.0;    
        }
    }
    else
    {
        float avg = (p->y+o->y)/2;
        float rel = (p->y-o->y)/2 - 8;
        o->y = avg-8;
        o->iy += rel;
        p->y = avg+8;
        p->iy -= rel;
        avg = (p->vy + o->vy)/2;
        o->vy = p->vy = avg;
        switch (op_combined)
        {
            case Bouncy:
                o->vy -= 0.25*rel_velocity;
                p->vy += 0.25*rel_velocity;
                break;
            case SuperBouncy:
                o->vy -= 0.5*rel_velocity;
                p->vy += 0.5*rel_velocity;
                break;
            case Sticky:
                rel = 0.25*(o->vx - p->vx);
                o->vx = p->vx = 0.5*(o->vx + p->vx);
                o->vx += rel;
                p->vx -= rel;
                break;
            case SuperSticky:
                o->vx = p->vx = 0.5*(o->vx + p->vx);
                break;
        }
    }
    if ((o->blink | p->blink)) // can't do/take damage if you're blinking
        return;
    if (o_bottom/2 == Damaging/2 && (get_sprite_vulnerability(p, UP) & get_sprite_attack(o)))
        damage_sprite(p, fabs(rel_velocity)*damage_multiplier*(o_bottom-Damaging+1));
    if (p_top/2 == Damaging/2 && (get_sprite_vulnerability(o, DOWN) & get_sprite_attack(p)))
        damage_sprite(o, fabs(rel_velocity)*damage_multiplier*(p_top-Damaging+1));
}

void collide_horizontally(struct object *o, struct object *p)
{
    // REQUIRED: o.x < p.x
    const float rel_velocity = o->vx - p->vx;
    if (rel_velocity == 0.0)
        return;
    int o_right = get_sprite_property(o, RIGHT);
    int p_left = get_sprite_property(p, LEFT);
    int op_combined = combine_properties(o_right, p_left); 
    if (rel_velocity > 0.0 && (p->blocked & BLOCKED_RIGHT))
    {
        o->ix = p->ix - 16;
        o->x = p->x - 16.0;
        o->blocked |= BLOCKED_RIGHT;
        switch (op_combined)
        {
            case Bouncy:
                o->vx *= -0.5;
                break;
            case SuperBouncy:
                o->vx *= -1.0;
                break;
            case Sticky:
                p->vy += 0.5*o->vy;
                o->vy *= 0.5;
                o->vx = 0.0;
                break;
            case SuperSticky:
                p->vy += 0.25*o->vy;
                o->vy *= 0.75;
                o->vx = 0.0;
                break;
            default:
                o->vx = 0.0;    
        }
    }
    else if (rel_velocity < 0.0 && (o->blocked & BLOCKED_LEFT))
    {
        p->ix = o->ix + 16;
        p->x = o->x + 16.0;
        p->blocked |= BLOCKED_LEFT;
        switch (op_combined)
        {
            case Bouncy:
                p->vx *= -0.5;
                break;
            case SuperBouncy:
                p->vx *= -1.0;
                break;
            case Sticky:
                o->vy += 0.5*p->vy;
                p->vy *= 0.5;
                p->vx = 0.0;
                break;
            case SuperSticky:
                o->vy += 0.25*p->vy;
                p->vy *= 0.75;
                p->vx = 0.0;
                break;
            default:
                p->vx = 0.0;    
        }
    }
    else
    {
        float avg = (p->x+o->x)/2;
        float rel = (p->x-o->x)/2-8;
        o->x = avg-8;
        o->ix += rel;
        p->x = avg+8;
        p->ix -= rel;
        avg = (p->vx + o->vx)/2;
        o->vx = p->vx = avg;
        switch (op_combined)
        {
            case Bouncy:
                o->vx -= 0.25*rel_velocity;
                p->vx += 0.25*rel_velocity;
                break;
            case SuperBouncy:
                o->vx -= 0.5*rel_velocity;
                p->vx += 0.5*rel_velocity;
                break;
            case Sticky:
                rel = 0.25*(o->vy - p->vy);
                o->vy = p->vy = 0.5*(o->vy + p->vy);
                o->vy += rel;
                p->vy -= rel;
                break;
            case SuperSticky:
                o->vy = p->vy = 0.5*(o->vy + p->vy);
                break;
        }
    }
    if ((o->blink | p->blink)) // can't do/take damage if you're blinking
        return;
    if (o_right/2 == Damaging/2 && (get_sprite_vulnerability(p, LEFT) & get_sprite_attack(o)))
        damage_sprite(p, fabs(rel_velocity)*damage_multiplier*(o_right-Damaging+1));
    if (p_left/2 == Damaging/2 && (get_sprite_vulnerability(o, RIGHT) & get_sprite_attack(p)))
        damage_sprite(o, fabs(rel_velocity)*damage_multiplier*(p_left-Damaging+1));
}

void sprite_collide(struct object *o, struct object *p)
{
    if (o->health * p->health == 0) // if either sprite is dead, don't collide them.
        return;

    // p is lower than o, i.e. p.iy > o.iy.
    if (o->ix < p->ix)
    {
        if (o->ix + 16 <= p->ix)
            return;
        if (p->iy - o->iy > 8) // large difference in y, collide vertically
            collide_vertically(o, p);
        else // colliding horizontally, recall o left of p
            collide_horizontally(o, p);
    }
    else // p.x < o.x
    {
        if (p->ix + 16 <= o->ix)
            return;
        if (p->iy - o->iy > 8) // large difference in y
            collide_vertically(o, p);
        else // colliding horizontally, p left of o
            collide_horizontally(p, o);
    }
}
