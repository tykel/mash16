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

#ifndef SYSTEM_H
#define SYSTEM_H

#include "Common.h"
#include "CPU/CPU.h"
#include "GPU/GPU.h"
//#include "SPU/SPU.h"
//#include "Pad/Pad.h"

// Uncomment to use Dynamic recompiler core
//#define CORE_DYNAREC

// Use SFML Rendering backend
#define BACKEND_SFML

namespace Chip16 {

	class System {
	private:
		// (Base) components of the Chip16 system - use implementations
		Chip16::CPU* m_cpu;
		Chip16::GPU* m_gpu;
		//Chip16::SPU* m_spu;
		//Chip16::Pad* m_pad;
		Chip16::Timer* m_timer;
		
		// System RAM
		uint8* m_mem;
		// Last frame counter/timestamp
		uint32 m_lastT;
		
    public:
		System();
		~System();
		// Load a rom file
		void LoadRom(uint8* mem);
		// Runs a 'step' of the CPU's execution
		// (Single instruction for the InterpCPU)
		// (Basic block/fixed block for the DynarecCPU)
		void ExecuteStep();
		// Clean up after use
		void Clear();
		// Return the dt since the last frame (ms)
		uint32 GetCurDt();
        // Reset timer
        void ResetDt();	

        // Getters for intercomponent communication
        Chip16::CPU* getCPU();
        Chip16::GPU* getGPU();
        Chip16::Timer* getTimer();
	};

}

#endif
