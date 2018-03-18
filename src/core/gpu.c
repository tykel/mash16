/*
 *   mash16 - the chip16 emulator
 *   Copyright (C) 2012-2013 tykel
 *
 *   mash16 is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   mash16 is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with mash16.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gpu.h"
#include <SDL2/SDL.h>

uint32_t pixels[320*240];

void init_pal(cpu_state* state)
{
    uint32_t pal[] =
    {
        PAL_TRANS,  PAL_BLACK,   PAL_GRAY,    PAL_RED,
        PAL_PINK,   PAL_DKBROWN, PAL_BROWN,   PAL_ORANGE,
        PAL_YELLOW, PAL_GREEN,   PAL_LTGREEN, PAL_DKBLUE,
        PAL_BLUE,   PAL_LTBLUE,  PAL_SKYBLUE, PAL_WHITE
    };
    load_pal((uint8_t*)&pal[0],1,state);
}

void load_pal(uint8_t* pal, int alpha, cpu_state* state)
{
    int i, b = 3 + alpha;
    for(i=0; i<16; ++i)
    {
        uint32_t col = (pal[i*b] << 16) | (pal[i*b + 1] << 8) | (pal[i*b + 2]);
        *((uint32_t*)&state->pal[i]) = col;
    }
}

/* (Public) blitting functions. */
void blit_screen(SDL_Texture* sfc, cpu_state* state, int scale)
{
    blit_screen1x(sfc,state);
    SDL_UpdateTexture(sfc, NULL, pixels, 320*sizeof(uint32_t));
}

/* Internal blitters. */
void blit_screen1x(SDL_Texture* sfc, cpu_state* state)
{
    int y, x;
    uint8_t rgb;
    
    for(y=0; y<240; ++y)
    {
        for(x=0; x<320; ++x)
        {
            rgb = state->vm[y*320 + x];
            pixels[y*320 + x] = !rgb ? state->pal[state->bgc] : state->pal[rgb];
        }
    }
}
