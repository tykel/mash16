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
#include "Opcodes.h"

#include <cstdlib>
#include <ctime>

Chip16::InterpCPU::InterpCPU(void)
{
}


Chip16::InterpCPU::~InterpCPU(void)
{
}

// Apparently switch/case is faster than function pointers... so here goes
void Chip16::InterpCPU::Execute() {
	// Fetch
	m_instr = (cpu_instr*)((uint8*)(m_mem +_PC));
	// Increment PC
	_PC += 4;
	// Execute
	switch(m_instr->op) {
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
	case SND1:		snd1(); break;
	case SND2:		snd2(); break;
	case SND3:		snd3();	break;
	case JMP_I:		jmp_i(); break;
	case JMC:		jmc(); break;
	case JME:		jme(); break;
	case JMP_R:		jmp_r(); break;
	case CALL_I:	call_i(); break;
	case RET:		ret(); break;
	case Cx:		cx(); break;
	case CALL_R:	call_r(); break;
	case LDI_R:		ldi_r(); break;
	case LDI_SP:	ldi_sp(); break;
	case LDM_I:		ldm_i(); break;
	case LDM_R:		ldm_r(); break;
	case MOV:		mov(); break;
	case STM_I:		stm_i(); break;
	case STM_R:		stm_r(); break;
	case ADDI:		addi(); break;
	case ADD_R2:	add_r2(); break;
	case ADD_R3:	add_r3(); break;
	case SUBI:		subi(); break;
	case SUB_R2:	sub_r2(); break;
	case SUB_R3:	sub_r3(); break;
	case CMPI:		cmpi(); break;
	case CMP:		cmp(); break;
	case ANDI:		andi(); break;
	case AND_R2:	and_r2(); break;
	case AND_R3:	and_r3(); break;
	case TSTI:		tsti(); break;
	case TST:		tst(); break;
	case ORI:		ori(); break;
	case OR_R2:		or_r2(); break;
	case OR_R3:		or_r3(); break;
	case XORI:		xori(); break;
	case XOR_R2:	xor_r2(); break;
	case XOR_R3:	xor_r3(); break;
	case MULI:		muli(); break;
	case MUL_R2:	mul_r2(); break;
	case MUL_R3:	mul_r3(); break;
	case DIVI:		divi(); break;
	case DIV_R2:	div_r2(); break;
	case DIV_R3:	div_r3(); break;
	case SHL_N:		shl_n(); break;
	case SHR_N:		shr_n(); break;
	case SAR_N:		sar_n(); break;
	case SHL_R:		shl_r(); break;
	case SHR_R:		shr_r(); break;
	case SAR_R:		sar_r(); break;
	case PUSH:		push(); break;
	case POP:		pop(); break;
	case PUSHALL:	pushall(); break;
	case POPALL:	popall(); break;
	case PUSHF:		pushf(); break;
	case POPF:		popf(); break;

	default:
		break;
	}
}

void Chip16::InterpCPU::Init(const uint8* mem) {
	m_mem = (uint8*)mem;
	srand((uint32)time(NULL));
}

void Chip16::InterpCPU::Clear() {

}

