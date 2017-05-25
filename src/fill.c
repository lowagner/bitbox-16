#include "bitbox.h"
#include "common.h"
#include "fill.h"
#include <stdint.h>
#include <stdlib.h> // rand
#include <string.h> // memset

#define MAX_STACK_INDEX (32*2-1)

uint8_t *fill_memory CCM_MEMORY;
int fill_width, fill_height;

int fill_old_color, fill_new_color;
uint8_t fill_matrix[2*TILE_MAP_MEMORY/8] CCM_MEMORY;
// 0 - not checked
// 1 - checked

uint32_t fill_stack[MAX_STACK_INDEX+1];
// 2^9 2^9 2^8
// x_min + y*SCREEN_W + (x_max<<17)
int fill_stack_index = -1;
// value = fill_x + fill_y * SCREEN_W

void fill_set_matrix(int x, int y) 
{
    int index = y*fill_width + x;
    fill_matrix[index/8] |= 1 << (index % 8);
}

int fill_get_matrix(int x, int y)
{
    int index = y*fill_width + x;
    return (fill_matrix[index/8] & (1 << (index % 8)));
}

void fill_set_color(int x, int y, int c)
{
    int index = y*fill_width + x;
    if (index % 2)
        fill_memory[index/2] = (fill_memory[index/2] & 15) | (c<<4);
    else
        fill_memory[index/2] = c | (fill_memory[index/2] & 240);
}

int fill_get_color(int x, int y)
{
    int index = y*fill_width + x;
    if (index % 2)
        return (fill_memory[index/2] >> 4);
    else
        return (fill_memory[index/2] & 15);
}

void fill_add_row_to_stack(int y, int xmin, int xmax)
{
    if (fill_stack_index >= MAX_STACK_INDEX)
    {
        message("ran out of memory\n");
        // shuffle the fill stack and drop the last half of it
        int i=0;
        for (; i<=MAX_STACK_INDEX/2; ++i) // don't shuffle all the way up, you drop it anyway
        {
            int j = i + rand()%(MAX_STACK_INDEX+1 - i);
            int old_value_i = fill_stack[i];
            fill_stack[i] = fill_stack[j];
            fill_stack[j] = old_value_i;
        }
        fill_stack_index = i-1;
    }
    fill_stack[++fill_stack_index] = xmin + (y<<10) + (xmax<<20);
}

void fill_row(int y, int xmin, int xmax)
{
    if (xmax - xmin >= 4)
    {
        int index_min = y*fill_width + xmin;
        int index_max = y*fill_width + xmax;
        if (index_min % 2)
        {
            fill_memory[index_min/2] = (fill_memory[index_min/2] & 15) | (fill_new_color<<4);
            fill_set_matrix(xmin, y);
            ++index_min;
        }
        if (index_max % 2 == 0)
        {
            fill_memory[index_max/2] = fill_new_color | (fill_memory[index_max/2] & 240);
            fill_set_matrix(xmax, y);
            --index_max;
        }
        // for example, if index_min == 3 and index_max == 6
        // then index_min -> 4 and index_max -> 5
        // then we need to set row[index_min/2 = 2] to row[index_max/2 = 2], inclusive
        // so we need to set 1 byte now

        // another example, index_min = 3 and index_max == 9
        // then index_min -> 4, index_min/2 -> 2, and index_max/2 -> 4.
        // need to set bytes 2, 3, and 4, so counting 3 in total
        uint8_t c = (fill_new_color) | ((fill_new_color) << 4);
        memset(&fill_memory[index_min/2], c, 1 + index_max/2-index_min/2);
        
        while (index_min <= index_max && (index_min % 8)) 
        {
            //fill_set_matrix(xmin, y);
            fill_matrix[index_min/8] |= 1 << (index_min % 8);
            ++index_min;
        }
        while (index_min <= index_max && ((index_max % 8)<7)) 
        {
            fill_matrix[index_max/8] |= 1 << (index_max % 8);
            --index_max;
        }
        // suppose index_min == 0 and index_max == 15
        // we need to fill two bytes worth
        if (index_min < index_max)
        {
            memset(&fill_matrix[index_min/8], 255, (index_max+1-index_min)/8);
        }
    }
    else
    {
        for (int x=xmin; x<=xmax; ++x)
        {
            fill_set_color(x, y, fill_new_color);
            fill_set_matrix(x, y);
        }
    }
}

int fill_can_start()
{
    return fill_stack_index < 0; 
}

void fill_stop()
{
    fill_stack_index = -1; 
}

void fill_init(uint8_t *memory, int width, int height, int old_c, int x, int y, int new_c)
{
    if (!memory)
        return;
    if (height <= 0 || width <= 0)
        return;
    if (height*width/8 > sizeof(fill_matrix))
        return;
    fill_memory = memory;
    fill_height = height;
    fill_width = width;
    
    old_c &= 15;
    new_c &= 15;

    memset(fill_matrix, 0, height*width/8);
    // so that fill color will work:
    int xmin = x;
    while (xmin > 0 && fill_get_color(xmin-1, y) == old_c)
        --xmin;
    int xmax = x;
    while (xmax < fill_width-1 && fill_get_color(xmax+1, y) == old_c)
        ++xmax;
    message("init spot (%d, %d) got row from %d to %d, color %d to %d\n", x, y, xmin, xmax, old_c, new_c);
    
    if (y > 0)
        fill_add_row_to_stack(y-1, xmin, xmax);
    if (y < fill_height-1)
        fill_add_row_to_stack(y+1, xmin, xmax);
    
    fill_old_color = old_c;
    fill_new_color = new_c;
    
    fill_row(y, xmin, xmax);
}

// // Recursive works fine on emulator, not on bitbox:
//void fill_color(int this_c, int x, int y, int fill_c) 
//{
//    // set anything neighboring (x,y) that is "this_c" to color "fill_c" 
//    // but first check if (x,y) is this_c, otherwise break:
//    if (get_color(x, y) != this_c) 
//        return;
//
//    // set current color
//    set_color(x, y, fill_c);
//
//    // do neighbors recursively
//    if (x > 0)
//        fill_color(this_c, x-1, y, fill_c);
//    if (x < SCREEN_W-1)
//        fill_color(this_c, x+1, y, fill_c);
//    if (y > 0)
//        fill_color(this_c, x, y-1, fill_c);
//    if (y < SCREEN_H-1)
//        fill_color(this_c, x, y+1, fill_c);
//}

int fill_test(int x, int y)
{
    return (fill_get_color(x, y) == fill_old_color && fill_get_matrix(x, y) == 0);
}

void fill_frame()
{
    if (fill_stack_index < 0)
        return;
    /* TODO:
        to make this more efficient, you can specify whether this line was started
        going up or coming down.  (put a 1 in the 31st bit if it was going up.)
        if this line does not extend left of the original xmin or right of the original xmax,
        it does not have to push a new row to the stack going back the direction it came from.
            e.g. if it was going up, no need to send a row down if it doesn't extend.

     */
    int z = fill_stack[fill_stack_index--];
    // unravel the xmin, xmax, and y values in fill_stack:
    int xmax = (z >> 20) & 1023;
    int y = (z >> 10) &1023;
    int xmin = z & 1023;
    #ifdef NDEBUG
    message("checking row %d from %d to %d\n", y, xmin, xmax);
    #endif

    // the rows will be tested from xmin to xmax_current,
    // then you can add [y, xmin, xmax_current] to the stack
    int xmax_current = -1;
    if (fill_test(xmin, y))
    {
        // try moving backwards from here
        xmax_current = xmin;
        while (xmin > 0 && fill_test(xmin-1, y))
            --xmin;
    }
    else
    {
        // try moving forwards
        while (xmin < xmax)
        {
            ++xmin;
            if (fill_test(xmin, y))
                break;
        }
        if (xmin == xmax && fill_test(xmax, y) == 0)
        {
            // this stack value was a bust, nothing paintable in this row
            return;
        }
        else
        {
            // we know for sure that xmin is possible
            xmax_current = xmin; 
        }
    }
    while (xmin <= xmax)
    {
        // xmin to xmax_current is currently a valid paintable row,
        // push the right side of the as far as possible:
        while (xmax_current < fill_width-1 && fill_test(xmax_current+1, y))
            ++xmax_current;
       
        if (xmin == xmax_current)
        {
            // not much of a row to paint
            fill_set_color(xmin, y, fill_new_color);
            fill_set_matrix(xmin, y);
            // but it may leak above and below:
            if (y > 0)
            {
                // do a pre check here since it will save room on the stack:
                if (fill_test(xmin, y-1))
                    fill_add_row_to_stack(y-1, xmin, xmin);
            }
            if (y < fill_height-1)
            {
                if (fill_test(xmin, y+1))
                    fill_add_row_to_stack(y+1, xmin, xmin);
            }
        }
        else
        {
            fill_row(y, xmin, xmax_current);
            // add stuff to the stack
            if (y > 0)
                fill_add_row_to_stack(y-1, xmin, xmax_current);
            if (y < fill_height-1)
                fill_add_row_to_stack(y+1, xmin, xmax_current);
        }

        // since we know that xmax_current+1 is not a valid spot to fill,
        // start searching here:
        xmin = xmax_current + 2;

        if (xmin > xmax)
        {
            // nothing more can be painted, it doesn't connect
            break;
        }
        
        // try moving forwards
        while (xmin < xmax)
        {
            if (fill_test(xmin, y))
                break;
            ++xmin;
        }
        if (xmin == xmax && fill_test(xmax, y) == 0)
        {
            // nothing left paintable in this row
            return;
        }
        else
        {
            // we know for sure that xmin is possible
            xmax_current = xmin; 
            // so repeat the whole shindig
        }
    }
}
