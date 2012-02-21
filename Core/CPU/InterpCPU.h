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

#ifndef INTERP_CPU_H
#define INTERP_CPU_H

#define _RX		(m_state.r[m_instr->yx & 0x0F])
#define _RY		(m_state.r[m_instr->yx >> 4])
#define _RZ		(m_state.r[m_instr->z])
#define _IMM	(m_instr->hhll)
#define _N		(m_instr->n)
#define _SP		(m_state.sp)
#define _PC		(m_state.pc)

#include "CPU.h"

namespace Chip16 {

	class InterpCPU : public CPU
	{
	private:

	public:
		InterpCPU(void);
		~InterpCPU(void);

		void Execute();
		void Init(const uint8* mem, System* sys);
		void Clear();
		// Flags modifiers
		void flags_add(int32 rx, int32 ry);
		void flags_sub(int32 rx, int32 ry);
		void flags_and(int32 rx, int32 ry);
		void flags_or(int32 rx, int32 ry);
		void flags_xor(int32 rx, int32 ry);
		void flags_mul(int32 rx, int32 ry);
		void flags_div(int32 rx, int32 ry);
		void flags_shl(int32 rx, int32 ry);
		void flags_shr(int32 rx, int32 ry);
		void flags_sar(int32 rx, int32 ry);
		// Big pile of instructions >:D
		void nop();
		void cls();
		void vblnk();
		void bgc();
		void spr();
		void drw_i();
		void drw_r();
		void rnd();
		void flip();
		void snd0();
		void snd1();
        void snd2();
        void snd3();
		void snp();
		void sng();
		void jmp_i();
		void jmc();
		void jx();
		void jme();
		void jmp_r();
		void call_i();
		void ret();
		void cx();
		void call_r();
		void ldi_r();
		void ldi_sp();
		void ldm_i();
		void ldm_r();
		void mov();
		void stm_i();
		void stm_r();
		void addi();
		void add_r2();
		void add_r3();
		void subi();
		void sub_r2(); 
		void sub_r3();
		void cmpi();
		void cmp();
		void andi();
		void and_r2();
		void and_r3();
		void tsti();
		void tst();
		void ori();
		void or_r2();
		void or_r3();
		void xori();
		void xor_r2();
		void xor_r3();
		void muli();
		void mul_r2();
		void mul_r3();
		void divi();
		void div_r2();
		void div_r3();
		void shl_n();
		void shr_n();
		void sar_n();
		void shl_r();
		void shr_r();
		void sar_r();
		void push();
		void pop();
		void pushall();
		void popall();
		void pushf();
		void popf(); 
		void pal_i();
		void pal_r();
	};

}

#endif
