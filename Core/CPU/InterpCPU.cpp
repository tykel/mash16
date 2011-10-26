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

#define _RX		(m_state.r[m_instr->yx & 0x0F])
#define _RY		(m_state.r[m_instr->yx >> 4])
#define _RZ		(m_state.r[m_instr->z])
#define _IMM	(m_instr->hhll)
#define _N		(m_instr->n)
#define _SP		(m_state.sp)
#define _PC		(m_state.pc)


Chip16::InterpCPU::InterpCPU(void)
{
}


Chip16::InterpCPU::~InterpCPU(void)
{
}

void Chip16::InterpCPU::Execute() {
	_PC += 4;
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
	m_instr = (cpu_instr*)((uint8*)(m_mem +_PC));
}

void Chip16::InterpCPU::Init(const uint8* mem) {
	m_mem = mem;
	srand((uint32)time(NULL));
}

void Chip16::InterpCPU::Clear() {

}

void Chip16::InterpCPU::nop() { }
void Chip16::InterpCPU::cls() { m_gpu->Clear(); }
void Chip16::InterpCPU::vblnk() { /* stop execution until next vblnk */ }
void Chip16::InterpCPU::bgc() { m_gpu->m_state.bg = _N; }
void Chip16::InterpCPU::spr() { m_gpu->m_state.sz = _IMM; }
void Chip16::InterpCPU::drw_i() { 
	m_sprinfo.x = _RX;
	m_sprinfo.y = _RX;
	m_sprinfo.data = (uint8*)(m_mem + _IMM); 
	m_gpu->Blit(&m_sprinfo); 
}
void Chip16::InterpCPU::drw_r() {
	m_sprinfo.x = _RX;
	m_sprinfo.y = _RY;
	m_sprinfo.data = (uint8*)(m_mem + _RZ); 
	m_gpu->Blit(&m_sprinfo); 
}
void Chip16::InterpCPU::rnd() { 
	_RX = rand() % (_IMM + 1); 
}
void Chip16::InterpCPU::flip() { m_gpu->m_state.fp = (uint32) m_instr->fp; }
void Chip16::InterpCPU::snd0() { /* SPU: TODO*/ }
void Chip16::InterpCPU::snd1() { /* SPU: TODO*/ }
void Chip16::InterpCPU::snd2() { /* SPU: TODO*/ }
void Chip16::InterpCPU::snd3() { /* SPU: TODO*/ }
void Chip16::InterpCPU::jmp_i() { m_state.pc = _IMM; }
void Chip16::InterpCPU::jmc() { }
void Chip16::InterpCPU::jx() {
	switch(m_instr->yx & 0x0F) {
	case C_Z:
		if(m_state.fl & FLAG_Z) _PC = _IMM;
		break;
	case C_NZ:
		if(!(m_state.fl & FLAG_Z)) _PC = _IMM;
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
	if(_RX == _RY)
		_PC = _IMM;
}
void Chip16::InterpCPU::jmp_r() { m_state.pc = m_state.r[m_instr->yx & 0x0F]; }
void Chip16::InterpCPU::call_i() {
	uint16* _sp = (uint16*)&m_mem[_SP]; *_sp =_PC;
	m_state.sp += 2;
	m_state.pc = _IMM;
}
void Chip16::InterpCPU::ret() {
	_SP -= 2;
	m_state.pc = m_mem[_SP];
}
void Chip16::InterpCPU::cx() { 
	uint16* _sp = (uint16*)&m_mem[_SP]; *_sp = m_state.pc;
	m_state.sp += 2;
	jx();
}
void Chip16::InterpCPU::call_r() { 
	uint16* _sp = (uint16*)&m_mem[_SP]; *_sp = m_state.pc;
	m_state.sp += 2;
	m_state.pc = _RX;
}
void Chip16::InterpCPU::ldi_r() {
	int16* rx = &_RX; *rx = _IMM;
}
void Chip16::InterpCPU::ldi_sp() { m_state.sp = _IMM; }
void Chip16::InterpCPU::ldm_i() { 
	int16* rx = &_RX; *rx = m_mem[_IMM]; 
}
void Chip16::InterpCPU::ldm_r() { 
	int16* rx = &_RX; *rx = m_mem[_RY];
}
void Chip16::InterpCPU::mov() { 
	int16* rx = &_RX; *rx =_RY; 
}
void Chip16::InterpCPU::stm_i() { 
	uint16* _pm = (uint16*)&m_mem[_IMM];
	*_pm =  _RX;
}
void Chip16::InterpCPU::stm_r() { 
	uint16* _pm = (uint16*)&m_mem[_RY];
	*_pm =  _RX;
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
	int16* rx = &_RX; int16 imm = _IMM;
	*rx += imm;
	flags_add((int32)*rx,(int32)imm);
}
void Chip16::InterpCPU::add_r2() {	
	int16* rx = &_RX; int16 ry = _RY;
	*rx += ry;
	flags_add((int32)*rx,(int32)ry);
}
void Chip16::InterpCPU::add_r3() {
	int16 rx = _RX; int16 ry = _RY; int16* rz = &_RZ;
	*rz = rx + ry;
	flags_add((int32)rx,(int32)ry);
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
	int16* rx = &_RX; int16 imm = _IMM;
	*rx -= imm;
	flags_sub((int32)*rx,(int32)imm);
}
void Chip16::InterpCPU::sub_r2() {
	int16* rx = &_RX; int16 ry = _RY;
	*rx -= ry;
	flags_sub((int32)*rx,(int32)ry);
}
void Chip16::InterpCPU::sub_r3() {
	int16 rx = _RX; int16 ry = _RY; int16* rz = &_RZ;
	*rz = rx - ry;
	flags_sub((int32)rx,(int32)ry);
}
void Chip16::InterpCPU::cmpi() {
	int16 rx = _RX; int16 imm = _IMM;
	flags_sub((int32)rx,(int32)imm);
}
void Chip16::InterpCPU::cmp() {
	int16 rx = _RX; int16 ry = _RY;
	flags_sub((int32)rx,(int32)ry);
}
void Chip16::InterpCPU::flags_and(int32 rx, int32 ry) {
	int32 res = rx & ry;
	m_state.fl = 0;
	if(!res) {
		m_state.fl |= FLAG_Z;
	}
	else if(res < 0) {
		m_state.fl |= FLAG_N;
	}
}
void Chip16::InterpCPU::andi() {
	int16* rx = &_RX; int16 imm = _IMM;
	*rx &= imm;
	flags_and((int32)*rx,(int32)imm);
}
void Chip16::InterpCPU::and_r2() {
	int16* rx = &_RX; int16  ry = _RY;
	*rx &= ry;
	flags_and((int32)*rx,(int32)ry);
}
void Chip16::InterpCPU::and_r3() {
	int16 rx = _RX; int16 ry = _RY; int16* rz = &_RZ;
	*rz = rx & ry;
	flags_and((int32)rx,(int32)ry);
}
void Chip16::InterpCPU::tsti() {
	int16 rx  = _RX; int16 imm = _IMM;
	flags_and((int32)rx,(int32)imm);
}
void Chip16::InterpCPU::tst() {
	int16 rx = _RX; int16 ry = _RY;
	flags_and((int32)rx,(int32)ry);
}
// And so on, TODO
