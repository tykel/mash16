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

#include "System.h"
#include "CPU/InterpCPU.h"
#include "CPU/DynarecCPU.h"
#include "GPU/SfmlGPU.h"
#include "Timer/SfmlTimer.h"

Chip16::System::System() {
// Allocate library-dependant components
#ifdef BACKEND_SFML
	m_gpu = new Chip16::SfmlGPU();
	//m_spu = new Chip16::SfmlSPU();
	//m_pad = new Chip16::SfmlPad();
	m_timer = new Chip16::SfmlTimer();
#else
	// No other backends for now
	m_gpu = NULL;
#endif
	m_mem = NULL;
// Initialize CPU core
#ifdef CORE_DYNAREC
	m_cpu = new Chip16::DynarecCPU();
#else
	m_cpu = new Chip16::InterpCPU();
#endif
}

Chip16::System::~System() { }

void Chip16::System::Init() {
    m_cpu->Init(m_mem, this);
}

uint32 Chip16::System::GetCurDt() {
	return m_timer->GetDt();
}

void Chip16::System::ResetDt() {
    m_timer->Reset();
    m_cpu->UnWaitVblnk();
}

void Chip16::System::LoadRom(uint8* mem) {
	m_mem = mem;
}

void Chip16::System::ExecuteStep() {
	m_cpu->Execute();
}

void Chip16::System::Clear() {
    m_cpu->Clear();
    m_gpu->Clear();
}

Chip16::CPU* Chip16::System::getCPU() {
    return m_cpu;
}

Chip16::GPU* Chip16::System::getGPU() {
    return m_gpu;
}

Chip16::Timer* Chip16::System::getTimer() {
    return m_timer;
}
