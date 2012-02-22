/*
	Mash16 - an open-source C++ Chip16 emulator
    Copyright (C) 2011-12 Tim Kelsall

    Mash16 is free software: you can redistribute it and/or modify it under the terms 
	of the GNU General Public License as published by the Free Software Foundation, 
	either version 3 of the License, or  (at your option) any later version.

    Mash16 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
	PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program.
	If not, see <http://www.gnu.org/licenses/>.
*/

#include "GPU.h"

#include <iostream>

Chip16::GPU::GPU(void)
{
	m_state.bg = 0x0;
	m_state.sz = 0x0000;
    // Initialize the color table to the default palette
    m_colors[BLACK_TR]  = 0xFF000000;
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


Chip16::GPU::~GPU(void)
{
}

void Chip16::GPU::Init() { }
void Chip16::GPU::Blit(spr_info* si) { }
void Chip16::GPU::Draw() { }
void Chip16::GPU::Clear() { }
void Chip16::GPU::UpdateBg(uint32 newcol) { }
void* Chip16::GPU::getBuffer() { }

void Chip16::GPU::LoadPalette(uint8* start) {
    for(int i=0; i<16; ++i) {
        m_colors[i] = (uint32)(*(start + i*4));
    }
}

void Chip16::GPU::Dump() {
    std::clog << "BGC: " << m_state.bg
        << "\nSIZE: " << m_state.sz << std::endl;
}
