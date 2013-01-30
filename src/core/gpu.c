#include "gpu.h"
#include <SDL/SDL.h>

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
    int b = 3 + alpha;
    for(int i=0; i<16; ++i)
    {
        uint32_t col = (pal[i*b] << 16) | (pal[i*b + 1] << 8) | (pal[i*b + 2]);
        *((uint32_t*)&state->pal[i]) = col;
    }
}

void blit_screen1x(SDL_Surface* sfc, cpu_state* state)
{
    SDL_LockSurface(sfc);
	uint32_t* dst = (uint32_t*)sfc->pixels;
    
    for(int y=0; y<240; ++y)
    {
        for(int x=0; x<320; ++x)
        {
            uint8_t rgb = state->vm[y*320 + x];
            dst[y*320 + x] = !rgb ? state->pal[state->bgc] : state->pal[rgb];
        }
    }
    SDL_UnlockSurface(sfc);
    /* Force screen update. */
    SDL_UpdateRect(sfc,0,0,0,0);
}

void blit_screen2x(SDL_Surface* sfc, cpu_state* state)
{
    SDL_LockSurface(sfc);
    uint32_t* dst = (uint32_t*)sfc->pixels;
    
    for(int y=0; y<240; ++y)
    {
        /* Access memory in rows for better cache usage. */
        for(int x=0; x<320; ++x)
        {
            uint8_t rgb = state->vm[y*320 + x];
            uint32_t p = !rgb ? state->pal[state->bgc] : state->pal[rgb];
            dst[y*2*640 + x*2] = p;
            dst[y*2*640 + x*2+1] = p;
        }
        for(int x=0; x<320; ++x)
        {
            uint8_t rgb = state->vm[y*320 + x];
            uint32_t p = !rgb ? state->pal[state->bgc] : state->pal[rgb];
            dst[(y*2+1)*640 + x*2] = p;
            dst[(y*2+1)*640 + x*2+1] = p;
        }
    }
    SDL_UnlockSurface(sfc);
    /* Force screen update. */
    SDL_UpdateRect(sfc,0,0,0,0);
}
