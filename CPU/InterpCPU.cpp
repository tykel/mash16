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

#include <cstdlib>

Chip16::InterpCPU::InterpCPU(void)
{
}


Chip16::InterpCPU::~InterpCPU(void)
{
}

void Chip16::InterpCPU::Execute() {
	m_state.pc += 4;
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
	//...
	default:
		break;
	}
	m_instr = (cpu_instr*)((uint8*)(m_mem + m_state.pc));
}

void Chip16::InterpCPU::Init() {

}

void Chip16::InterpCPU::Clear() {

}

void Chip16::InterpCPU::nop() { }
void Chip16::InterpCPU::cls() { m_gpu->Clear(); }
void Chip16::InterpCPU::vblnk() { /* stop execution until next vblnk */ }
void Chip16::InterpCPU::bgc() { m_gpu->m_state.bg = m_instr->n; }
void Chip16::InterpCPU::spr() { m_gpu->m_state.sz = m_instr->hhll; }
void Chip16::InterpCPU::drw_i() { 
	m_sprinfo.x = m_state.r[m_instr->yx & 0x0F];
	m_sprinfo.y = m_state.r[m_instr->yx >> 4];
	m_sprinfo.data = (uint8*)(m_mem + m_state.r[m_instr->hhll]); 
	m_gpu->Blit(&m_sprinfo); 
}
void Chip16::InterpCPU::drw_r() {
	m_sprinfo.x = m_state.r[m_instr->yx & 0x0F];
	m_sprinfo.y = m_state.r[m_instr->yx >> 4];
	m_sprinfo.data = (uint8*)(m_mem + m_state.r[m_instr->z]); 
	m_gpu->Blit(&m_sprinfo); 
}
void Chip16::InterpCPU::rnd() { 
	m_state.r[m_instr->yx & 0x0F] = rand() % (m_instr->hhll + 1); 
}
void Chip16::InterpCPU::flip() { m_gpu->m_state.fp = (uint32) m_instr->fp; }
void Chip16::InterpCPU::snd0() { /* SPU: TODO*/ }
void Chip16::InterpCPU::snd1() { /* SPU: TODO*/ }
void Chip16::InterpCPU::snd2() { /* SPU: TODO*/ }
void Chip16::InterpCPU::snd3() { /* SPU: TODO*/ }
void Chip16::InterpCPU::jmp_i() { m_state.pc = m_instr->hhll; }
void Chip16::InterpCPU::jmc() { }
void Chip16::InterpCPU::jx() {
	switch(m_instr->yx & 0x0F) {
	case C_Z:
		if(m_state.fl & FLAG_Z)
			m_state.pc = m_instr->hhll;
		break;
	case C_NZ:
		if(!(m_state.fl & FLAG_Z))
			m_state.pc = m_instr->hhll;
		break;
	case C_N:
		if(m_state.fl & FLAG_N)
			m_state.pc = m_instr->hhll;
		break;
	case C_NN:
		if(!(m_state.fl & FLAG_N))
			m_state.pc = m_instr->hhll;
		break;
	case C_P:
		if(!(m_state.fl & FLAG_N & FLAG_Z))
			m_state.pc = m_instr->hhll;
		break;
	case C_O:
		if(m_state.fl & FLAG_O)
			m_state.pc = m_instr->hhll;
		break;
	case C_NO:
		if(!(m_state.fl & FLAG_O))
			m_state.pc = m_instr->hhll;
		break;
	case C_A:
		if(!(m_state.fl & FLAG_C & FLAG_Z))
			m_state.pc = m_instr->hhll;
		break;
	case C_AE:
		if(!(m_state.fl & FLAG_C))
			m_state.pc = m_instr->hhll;
		break;
	case C_B:
		if(m_state.fl & FLAG_C)
			m_state.pc = m_instr->hhll;
		break;
	case C_BE:
		if(m_state.fl & FLAG_C & FLAG_Z)
			m_state.pc = m_instr->hhll;
		break;
	case C_G:
		if(!(m_state.fl & FLAG_Z) && (((m_state.fl & FLAG_O) >> 6) == ((m_state.fl & FLAG_N) >> 7)))
			m_state.pc = m_instr->hhll;
		break;
	case C_GE:
		if(((m_state.fl & FLAG_O) >> 6) == ((m_state.fl & FLAG_N) >> 7))
			m_state.pc = m_instr->hhll;
		break;
	case C_L:
		if((((m_state.fl & FLAG_O) >> 6) != ((m_state.fl & FLAG_N) >> 7)))
			m_state.pc = m_instr->hhll;
		break;
	case C_LE:
		if((m_state.fl & FLAG_Z) && (((m_state.fl & FLAG_O) >> 6) != ((m_state.fl & FLAG_N) >> 7)))
			m_state.pc = m_instr->hhll;
		break;
	default:
		break;
	}
}
void Chip16::InterpCPU::jme() { 
	if(m_state.r[m_instr->yx & 0x0F] == m_state.r[m_instr->yx >> 4])
		m_state.pc = m_instr->hhll;
}
void Chip16::InterpCPU::jmp_r() { m_state.pc = m_state.r[m_instr->yx & 0x0F]; }
void Chip16::InterpCPU::call_i() {
	m_mem[m_state.sp] = (uint8)(m_state.pc >> 8);
	m_mem[m_state.sp] = (uint8)(m_state.pc & 0xFF);
	m_state.sp += 2;
	m_state.pc = m_instr->hhll;
}
void Chip16::InterpCPU::ret() {
	m_state.sp -= 2;
	m_state.pc = m_mem[m_state.sp];
}
void Chip16::InterpCPU::cx() { 
	m_mem[m_state.sp] = (uint8)(m_state.pc >> 8);
	m_mem[m_state.sp] = (uint8)(m_state.pc & 0xFF);
	m_state.sp += 2;
	jx();
}
void Chip16::InterpCPU::call_r() { 
	m_mem[m_state.sp] = (uint8)(m_state.pc >> 8);
	m_mem[m_state.sp] = (uint8)(m_state.pc & 0xFF);
	m_state.sp += 2;
	m_state.pc = m_state.r[m_instr->yx & 0x0F];
}
void Chip16::InterpCPU::ldi_r() { m_state.r[m_instr->yx & 0x0F] = m_instr->hhll; }
void Chip16::InterpCPU::ldi_sp() { m_state.sp = m_instr->hhll; }
void Chip16::InterpCPU::ldm_i() { m_state.r[m_instr->yx & 0x0F] = m_mem[m_instr->hhll]; }
void Chip16::InterpCPU::ldm_r() { m_state.r[m_instr->yx & 0x0F] = m_mem[m_state.r[m_instr->yx >> 4]]; }
void Chip16::InterpCPU::mov() { m_state.r[m_instr->yx & 0x0F] = m_state.r[m_instr->yx >> 4]; }
void Chip16::InterpCPU::stm_i() { 
	m_mem[m_instr->hhll] = m_state.r[m_instr->yx & 0x0F] >> 8;
	m_mem[m_instr->hhll+1] = m_state.r[m_instr->yx & 0x0F] & 0xFF;
}
void Chip16::InterpCPU::stm_r() { 
	m_mem[m_state.r[m_instr->yx >> 4]] = m_state.r[m_instr->yx & 0x0F] >> 8;
	m_mem[m_state.r[m_instr->yx >> 4]+1] = m_state.r[m_instr->yx & 0x0F] & 0xFF;
}
void Chip16::InterpCPU::flags_add(int32 rx, int32 ry) {
	int32 res = rx + ry;
	if(res > MAX_VALUE16)
		m_state.fl |= FLAG_C;
	else
		m_state.fl &= ~FLAG_C;
	if(res == 0)
		m_state.fl |= FLAG_Z;
	else
		m_state.fl &= ~FLAG_Z;
	if((rx < 0 && ry < 0 && res >= 0) || (rx > 0 && ry > 0 && res < 0))
		m_state.fl |= FLAG_O;
	else
		m_state.fl &= ~FLAG_O;
	if(res < 0)
		m_state.fl |= FLAG_N;
	else
		m_state.fl &= ~FLAG_N;
}
void Chip16::InterpCPU::addi() {
	int16* rx = (int16*)&m_state.r[m_instr->yx & 0x0F];
	int16 imm = m_instr->hhll;
	*rx += imm;
	flags_add((int32)*rx,(int32)imm);
}
void Chip16::InterpCPU::add_r2() {	
	int16* rx = (int16*)&m_state.r[m_instr->yx & 0x0F];
	int16* ry = (int16*)&m_state.r[m_instr->yx >> 4];
	*rx += *ry;
	flags_add((int32)*rx,(int32)*ry);
}
void Chip16::InterpCPU::add_r3() {
	int16* rx = (int16*)&m_state.r[m_instr->yx & 0x0F];
	int16* ry = (int16*)&m_state.r[m_instr->yx >> 4];
	m_state.r[m_instr->z] = *rx + *ry;
	flags_add((int32)*rx,(int32)*ry);
}
void Chip16::InterpCPU::flags_sub(int32 rx, int32 ry) {
	int32 res = rx - ry;
	if(res < MAX_NVALUE16)
		m_state.fl |= FLAG_C;
	else
		m_state.fl &= ~FLAG_C;
	if(res == 0)
		m_state.fl |= FLAG_Z;
	else
		m_state.fl &= ~FLAG_Z;
	if((rx < 0 && ry >= 0 && res >= 0) || (rx >= 0 && ry < 0 && res < 0))
		m_state.fl |= FLAG_O;
	else
		m_state.fl &= ~FLAG_O;
	if(res < 0)
		m_state.fl |= FLAG_N;
	else
		m_state.fl &= ~FLAG_N;
}
void Chip16::InterpCPU::subi() {
	int16* rx = (int16*)&m_state.r[m_instr->yx & 0x0F];
	int16 imm = m_instr->hhll;
	*rx -= imm;
	flags_sub((int32)*rx,(int32)imm);
}
// And so on, TODO
