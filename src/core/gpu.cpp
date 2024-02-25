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
#include <emmintrin.h>
#include <tmmintrin.h>

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
        state->pal[i] = col;
        state->pal_b[i] = pal[i*b];
        state->pal_g[i] = pal[i*b + 1];
        state->pal_r[i] = pal[i*b + 2];
    }
    state->pal_b0 = pal[0];
    state->pal_g0 = pal[1];
    state->pal_r0 = pal[2];
}

/* (Public) blitting functions. */
void blit_screen(GLuint sfc, cpu_state* state, int scale)
{
    blit_screen1x(state);
    glBindTexture(GL_TEXTURE_2D, sfc);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 240, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    //SDL_UpdateTexture(sfc, NULL, pixels, 320*sizeof(uint32_t));
}

/* Internal blitters. */
void blit_screen1x(cpu_state* state)
{
#if 1
   __m128i *vm = reinterpret_cast<__m128i*>(state->vm);
   __m128i *out = reinterpret_cast<__m128i*>(pixels);
   __m128i vZero = _mm_set_epi32(0, 0, 0, 0);
   __m128i vPalB = _mm_loadu_si128(reinterpret_cast<__m128i*>(state->pal_b));
   __m128i vPalG = _mm_loadu_si128(reinterpret_cast<__m128i*>(state->pal_g));
   __m128i vPalR = _mm_loadu_si128(reinterpret_cast<__m128i*>(state->pal_r));
   for (int i = 0; i < 320*240; i += 16) {
      __m128i vPix16 = _mm_load_si128(vm);
      
      __m128i vRPix16 = _mm_shuffle_epi8(vPalR, vPix16);
      __m128i vGPix16 = _mm_shuffle_epi8(vPalG, vPix16);
      __m128i vBPix16 = _mm_shuffle_epi8(vPalB, vPix16);

      // 0..7
      __m128i vGRPix8 = _mm_unpacklo_epi8(vRPix16, vGPix16);
      __m128i vABPix8 = _mm_unpacklo_epi8(vBPix16, vZero);
      // 0..3
      __m128i vABGRPix4 = _mm_unpacklo_epi16(vGRPix8, vABPix8);
      _mm_store_si128(out, vABGRPix4);
      // 4..7
      vABGRPix4 = _mm_unpackhi_epi16(vGRPix8, vABPix8);
      _mm_store_si128(out + 1, vABGRPix4);

      // 8..15
      vGRPix8 = _mm_unpackhi_epi8(vRPix16, vGPix16);
      vABPix8 = _mm_unpackhi_epi8(vBPix16, vZero);
      // 8..11
      vABGRPix4 = _mm_unpacklo_epi16(vGRPix8, vABPix8);
      _mm_store_si128(out + 2, vABGRPix4);
      // 12..15
      vABGRPix4 = _mm_unpackhi_epi16(vGRPix8, vABPix8);
      _mm_store_si128(out + 3, vABGRPix4);
      
      vm += 1;
      out += 4;
   }
   
#else
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
#endif
}
