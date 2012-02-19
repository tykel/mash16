/*
	Mash16 - an open-source C++ Chip16 emulator
    Copyright (C) 2011 Tim Kelsall

    Mash16 is free software: you can redistribute it and/or modify it under the terms 
	of the GNU General Public License as published by the Free Software Foundation, 
	either version 3 of the License, or  (at your option) any later version.

    Mash16 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
	PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program.
	If not, see <http://www.gnu.org/licenses/>.
*/

#include "SfmlGPU.h"

#include <iostream>

Chip16::SfmlGPU::SfmlGPU(void)
{
    m_screen.Create(320,240);
    m_screen.SetSmooth(false);
}


Chip16::SfmlGPU::~SfmlGPU(void)
{
}

void Chip16::SfmlGPU::Blit(spr_info* si) {
    //std::clog << "spr: x=" << si->x << " y=" << si->y 
    //    << " w=" << m_state.w*2 << "=" << m_state.w << "B"
    //    << " h=" << m_state.h << " sz=" << m_state.w*m_state.h
    //    << std::endl;
    // Address the buffer 32 bits at a time
    uint32* buffer32 = (uint32*)m_buffer;
    uint32 base = (si->y * 320) + si->x;
    //std::clog << std::hex << " badd=0x" << base << " w=" << m_state.w
    //    << " h=" << m_state.h;
    for(int i=0; i<m_state.w*m_state.h; ++i) {
        // Get the color pair to blit
        uint8 col2 = *(si->data + i);
        // Offset to this pair of pixels
        uint32 offs = (i/m_state.w)*320 + 2*(i % m_state.w);
        //std::clog << " [0x" << buffer32[base+offs]
        //    << " -> 0x" << m_colors[col2 >> 4];
        buffer32[base + offs] = m_colors[col2 >> 4];
        //std::clog << "] [0x" << buffer32[base+offs+1]
        //    << " -> 0x" << m_colors[col2 & 0x0F];
        buffer32[base + offs + 1] = m_colors[col2 & 0x0F];  
    }
    //std::clog << "]" << std::dec << std::endl;
}

void Chip16::SfmlGPU::Draw() {
	// Refresh the screen
    m_screen.LoadFromPixels(320,240,m_buffer);
}

void Chip16::SfmlGPU::Clear() {
    // Get the color indexed as the bg
    uint32 bg = m_colors[m_state.bg];
    // Address the buffer 32 bits at a time
    uint32* buffer32 = (uint32*)m_buffer;
    // Reset the frame buffer
    for(int i=0; i<320*240; ++i)
        buffer32[i] = bg;
}

void Chip16::SfmlGPU::ResetPalette() {
    // Restore the color table to the default palette
    m_colors[BLACK_TR]  = 0x00000000;
    m_colors[BLACK]     = 0xFF000000;
    m_colors[GRAY]      = 0xFF888888;
    m_colors[RED]       = 0xFF3239BF;//0xBF393200;
    m_colors[PINK]      = 0xFFAE7ADE;//0xDE7AAE00;
    m_colors[DK_BROWN]  = 0xFF213D4C;//0x4C3D2100;
    m_colors[BROWN]     = 0xFF255F90;//0x905F2500;
    m_colors[ORANGE]    = 0xFF5294E4;//0xE4945200;
    m_colors[YELLOW]    = 0xFF79D9EA;//0xEAD97900;
    m_colors[GREEN]     = 0xFF3B7A53;//0x537A3B00;
    m_colors[LT_GREEN]  = 0xFF4AD5AB;//0xABD54A00;
    m_colors[DK_BLUE]   = 0xFF382E25;//0x252E3800;
    m_colors[BLUE]      = 0xFF7F4600;//0x00467F00;
    m_colors[LT_BLUE]   = 0xFFCCAB68;//0x68ABCC00;
    m_colors[SKY_BLUE]  = 0xFFE4DEBC;//0xBCDEE400;
    m_colors[WHITE]     = 0xFFFFFFFF;
}

void Chip16::SfmlGPU::UpdateBg(uint32 newcol) {
    uint32 ncol = m_colors[newcol];
    uint32* buffer32 = (uint32*)m_buffer;
    for(int i=0; i<320*240; ++i) {
        if(buffer32[i] == m_colors[m_state.bg])
            buffer32[i] = ncol;
    }
    m_state.bg = newcol;
}

void* Chip16::SfmlGPU::getBuffer() {
    return (void*)(&m_screen);
}

void Chip16::SfmlGPU::Dump() {
    Chip16::GPU::Dump();
    for(int j=0; j<240; ++j) {
        for(int i=0; i<320; ++i) {
            if(m_buffer[i] != m_colors[m_state.bg])
                std::clog << "#";
            else
                std::clog << ".";
        }
        std::clog << std::endl;
    }
}
