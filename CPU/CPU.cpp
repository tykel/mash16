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

#include "CPU.h"

#include <iostream>
#include <fstream>

// Global register file to avoid dynalloc
uint16 g_regs[REGS_SIZE] = {0};
// Global memory file to avoid dynalloc
uint8 g_mem[MEMORY_SIZE] = {0};

Chip16::CPU::CPU(void)
{
	m_state.r = g_regs;
	m_state.pc = 0;
	m_state.sp = 0;

	m_sprinfo.data = NULL;
	m_sprinfo.len = 0;
	m_sprinfo.x = 0;
	m_sprinfo.y = 0;

	m_mem = g_mem;
	// TODO: Initalize m_gpu

	bool m_isReady = true;
}


Chip16::CPU::~CPU(void)
{
}

void Chip16::CPU::LoadROM(const char* fp) {
	std::fstream rom(fp,std::ios::binary);
	if(rom.is_open()) {
		// Find file length
		rom.seekg (0, std::ios::end);
		uint32 len = (uint32) rom.tellg();
		rom.seekg (0, std::ios::beg);
		// Copy it to memory
		rom.read((char*)m_mem,len);
		// Clean up
		rom.close();
		m_isReady = true;
	}
	else {
		std::clog << "Could not open file" << std::endl;
		m_isReady = false;
	}
}

void Chip16::CPU::Draw(int16 x, int16 y, uint16 start) {
	// Offset = rows * rowlen + column, / 2 because 2 spr/byte
	m_sprinfo.data = (uint8*) ((uint32)m_mem + ((y*320 + x)>>1));
	// If SPR HHLL, then len = HH * LL
	m_sprinfo.len = (m_gpu->m_state.sz >> 8) * (m_gpu->m_state.sz & 0x00FF);
	// Provide coords
	m_sprinfo.x = x, m_sprinfo.y = y;
	// Send to GPU for drawing!
	m_gpu->Blit(&m_sprinfo);
}

bool Chip16::CPU::IsReady() {
	return m_isReady;
}
