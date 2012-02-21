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

#include "CPU.h"

#include <iostream>
#include <fstream>

// Global register file to avoid dynalloc
int16 g_regs[REGS_SIZE]  = {0};

Chip16::CPU::CPU(void)
{
	m_state.r = g_regs;
	m_state.pc = 0;
	m_state.sp = 0;
    m_state.fl = 0;

	m_sprinfo.data = NULL;
	m_sprinfo.x = 0;
	m_sprinfo.y = 0;

	m_mem = NULL;
    m_gpu = NULL; 
    m_isWaitVblnk = false;
}


Chip16::CPU::~CPU(void) { }

void Chip16::CPU::Execute() {}
void Chip16::CPU::Init(const uint8* mem, System* sys) {}
void Chip16::CPU::Clear() {}

bool Chip16::CPU::IsWaitingVblnk() {
	return m_isWaitVblnk;
}

void Chip16::CPU::WaitVblnk() {
	m_isWaitVblnk = true;
}

void Chip16::CPU::UnWaitVblnk() {
	m_isWaitVblnk = false;
}
