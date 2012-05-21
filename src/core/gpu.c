#include "gpu.h"
#include <SDL/SDL.h>

void init_pal(cpu_state* state)
{
    uint32_t pal[16] =
    {
        PAL_TRANS, PAL_BLACK, PAL_GRAY, PAL_RED,
        PAL_PINK, PAL_DKBROWN, PAL_BROWN, PAL_ORANGE,
        PAL_YELLOW, PAL_GREEN, PAL_LTGREEN, PAL_DKBLUE,
        PAL_BLUE, PAL_LTBLUE, PAL_SKYBLUE, PAL_WHITE
    };
    load_pal((uint8_t*)pal,1,state);
}

void load_pal(uint8_t* pal, int alpha, cpu_state* state)
{
    uint32_t col = 0;
    int b = alpha ? 4 : 3;
    for(int i=0; i<16; ++i)
    {
        col = (pal[i*b] << 24) | (pal[i*b + 1] << 16) | (pal[i*b + 2] << 8);
        *((uint32_t*)&state->pal[i]) = col;
    }
}

void blit_screen(SDL_Surface* sfc, cpu_state* state)
{
    SDL_LockSurface(sfc);
    
    for(int y=0; y<240; ++y)
    {
        for(int x=0; x<160; ++x)
        {
            uint8_t rgb = state->vm[y*160 + x];
            uint32_t* dst = (uint32_t*)sfc->pixels;
            if((rgb >> 4) != 0x0)
            {
                dst[y*320 + x*2] = state->pal[rgb >> 4];
                printf("Drew pixel index %d -> rgba 0x%x ... ",rgb>>4,state->pal[rgb>>4]);
            }
            if((rgb & 0x0f) != 0x0)
            {
                dst[y*320 + x*2 + 1] = state->pal[rgb & 0x0f];
                printf("Drew pixel index %d -> rgba 0x%x ... ",rgb&0x0f,state->pal[rgb&0x0f]);
            }
        }
    }

    SDL_UnlockSurface(sfc);
}
