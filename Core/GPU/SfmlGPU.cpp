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

Chip16::SfmlGPU::SfmlGPU(void)
{
    m_screen = new sf::Image(320,240);
    m_screen.setSmooth(false);
}


Chip16::SfmlGPU::~SfmlGPU(void)
{
    delete m_screen;
}

void Chip16::SfmlGPU::Blit(spr_info* si) {
    // Address the buffer 32 bits at a time
    uint32* buffer32 = (uint32*)buffer;
    // Get the base location to blit
    uint32 base = (si->y * 320) + si->x;
	// Draw a sprite to the screen
    for(int i=0; i<m_state.sz; ++i) {
        // Get the color pair to blit
        uint8 col2 = *(si->data + i);
        // Offset to this pair of pixels
        uint32 offs = (i/m_state->w)*320 + i;
        // High-bit pixel
        buffer32[base + offs] = m_colors[col2 >> 4];
        // Low-bit pixel
        buffer32[base + offs + 1] = m_colors[col2 & 0x0F];  
    }
}

void Chip16::SfmlGPU::Draw() {
	// Refresh the screen
    m_screen.LoadFromPixels(320,240,m_buffer);
}

void Chip16::SfmlGPU::Clear() {
	// Clear the screen to the bg color
    m_screen.Create(320,240);
    // Get the color indexed as the bg
    uint32 bg = m_colors[m_state->bg];
    // Address the buffer 32 bits at a time
    uint32* buffer32 = (uint32*)buffer;
    // Reset the frame buffer
    for(int i=0; i<320*240; ++i)
        buffer32[i] = bg;
}