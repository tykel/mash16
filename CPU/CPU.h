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

#ifndef _CPU_H
#define _CPU_H

#include "..\Common.h"
#include "..\GPU\GPU.h"

// Used to represent CPU state
struct cpu_state {
	uint16* r;			// Registers R0..F
	uint16	pc;			// Program counter
	uint16	sp;			// Stack pointer
	uint16	fl;			// Flags register
};
// Used to map instruction segments
struct cpu_instr {
	uint8	op;			// Opcode byte
	uint8	yx;			// Registers Y, X
	union {
		uint16 addr;	// Address (HHLL)
		uint16 n;		// Nibble (000N)
		uint16 z;		// Register Z
		struct {
			uint8 res;	// Reserved, = 0
			uint8 fp;	// Flip type
		};
	};
};

namespace Chip16 {

	class CPU
	{
	protected:
		uint8*		m_mem;			// Ptr to memory
		cpu_state	m_state;		// CPU state info
		spr_info	m_sprinfo;		// Sprite info
		cpu_instr*	m_instr;		// Current instruction
		GPU*		m_gpu;			// Ptr to GPU
		bool		m_isReady;		// is the system ready?
	
		void Draw(int16 x, int16 y, uint16 start);

	public:
		CPU(void);
		~CPU(void);

		virtual void Execute() = 0;
		virtual void Init() = 0;
		virtual void Clear() = 0;
		void LoadROM(const char*);
		bool IsReady();

	};

}

#endif