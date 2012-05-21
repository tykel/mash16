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
#define PAL_TRANS   0xff000000
#define PAL_BLACK   0xff000000
#define PAL_GRAY    0xff888888
#define PAL_RED     0xff3239bf
#define PAL_PINK    0xffae7ade
#define PAL_DKBROWN 0xff213d4c
#define PAL_BROWN   0xff255f90
#define PAL_ORANGE  0xff5294e4
#define PAL_YELLOW  0xff79d9ea
#define PAL_GREEN   0xff3b7a53
#define PAL_LTGREEN 0xff4ad5ab
#define PAL_DKBLUE  0xff382e25
#define PAL_BLUE    0xff7f4600
#define PAL_LTBLUE  0xffccab68
#define PAL_SKYBLUE 0xffe4debc
#define PAL_WHITE   0xffffffff
*/

#include "cpu.h"

#include <SDL/SDL.h>

void init_pal(cpu_state*);
void load_pal(uint8_t*,int,cpu_state*);
void blit_screen(SDL_Surface*,cpu_state*);

#endif
