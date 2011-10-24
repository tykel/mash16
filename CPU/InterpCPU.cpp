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

#include "InterpCPU.h"
#include "..\Opcodes.h"

Chip16::InterpCPU::InterpCPU(void)
{
}


Chip16::InterpCPU::~InterpCPU(void)
{
}

void Chip16::InterpCPU::Execute() {
	//m_instr = (cpu_instr*) &m_mem[m_state.pc++];
	switch(m_instr++->op) {
	case NOP:		nop(); break;
	case CLS:		cls(); break;
	case VBLNK:		vblnk(); break;
	case BGC:		bgc(); break;
	case SPR:		spr(); break;
	case DRW_I:		drw_i(); break;
	case DRW_R:		drw_r(); break;
	case RND:		rnd(); break;
	case FLIP:		flip(); break;
	case SND0:		snd0(); break;
	//...
	default:
		break;
	}
}

void Chip16::InterpCPU::Init() {

}

void Chip16::InterpCPU::Clear() {

}

void Chip16::InterpCPU::nop() { }
void Chip16::InterpCPU::cls() { m_gpu->Clear(); }
void Chip16::InterpCPU::vblnk() { }
void Chip16::InterpCPU::bgc() { }
void Chip16::InterpCPU::spr() { }
void Chip16::InterpCPU::drw_i() { }
void Chip16::InterpCPU::drw_r() { }
void Chip16::InterpCPU::rnd() { }
void Chip16::InterpCPU::flip() { m_gpu->m_state.fp = (uint32) m_instr->fp; }
void Chip16::InterpCPU::snd0() { }
void Chip16::InterpCPU::snd1() { }
void Chip16::InterpCPU::snd2() { }
void Chip16::InterpCPU::snd3() { }
void Chip16::InterpCPU::jmp_i() { }
void Chip16::InterpCPU::jmc() { }
void Chip16::InterpCPU::jx() { }
void Chip16::InterpCPU::jme() { }
void Chip16::InterpCPU::jmp_r() { }
void Chip16::InterpCPU::call_i() { }
void Chip16::InterpCPU::ret() { }
void Chip16::InterpCPU::cx() { }
void Chip16::InterpCPU::call_r() { }
void Chip16::InterpCPU::ldi_r() { }
void Chip16::InterpCPU::ldi_sp() { }
void Chip16::InterpCPU::ldm_i() { }
void Chip16::InterpCPU::ldm_r() { }
void Chip16::InterpCPU::mov() { }
void Chip16::InterpCPU::stm_i() { }
void Chip16::InterpCPU::stm_r() { }
void Chip16::InterpCPU::addi() { }
// And so on, TODO
