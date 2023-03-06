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

#ifndef GPU_H
#define GPU_H

/* Default palette colours. */
#define PAL_TRANS   0x00000000
#define PAL_BLACK   0x00000000
#define PAL_GRAY    0x00888888
#define PAL_RED     0x003239bf
#define PAL_PINK    0x00ae7ade
#define PAL_DKBROWN 0x00213d4c
#define PAL_BROWN   0x00259f90
#define PAL_ORANGE  0x005294e4
#define PAL_YELLOW  0x0079d9ea
#define PAL_GREEN   0x003b7a53
#define PAL_LTGREEN 0x004ad5ab
#define PAL_DKBLUE  0x00382e25
#define PAL_BLUE    0x007f4600
#define PAL_LTBLUE  0x00ccab68
#define PAL_SKYBLUE 0x00e4debc
#define PAL_WHITE   0x00ffffff
/*
#define PAL_TRANS   0x00000000
#define PAL_BLACK   0x00000000
#define PAL_GRAY    0x00888888
#define PAL_RED     0x00bf3932
#define PAL_PINK    0x00de7aae
#define PAL_DKBROWN 0x004c3d21
#define PAL_BROWN   0x00905f25
#define PAL_ORANGE  0x00e49452
#define PAL_YELLOW  0x00ead979
#define PAL_GREEN   0x00537a3b
#define PAL_LTGREEN 0x00abd54a
#define PAL_DKBLUE  0x00252e38
#define PAL_BLUE    0x0000467f
#define PAL_LTBLUE  0x0068abcc
#define PAL_SKYBLUE 0x00bcdee4
#define PAL_WHITE   0x00ffffff
*/

#include "cpu.h"

#include <SDL2/SDL.h>

void init_pal(cpu_state*);
void load_pal(uint8_t*,int,cpu_state*);
void blit_screen(SDL_Texture*,cpu_state*,int);
void blit_screen1x(cpu_state*);

#endif
