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
		inline void nop();
		inline void cls();
		inline void vblnk();
		inline void bgc();
		inline void spr();
		inline void drw_i();
		inline void drw_r();
		inline void rnd();
		inline void flip();
		inline void snd0();
		inline void snd1();
        inline void snd2();
        inline void snd3();
		inline void snp();
		inline void sng();
		inline void jmp_i();
		inline void jmc();
		inline void jx();
		inline void jme();
		inline void jmp_r();
		inline void call_i();
		inline void ret();
		inline void cx();
		inline void call_r();
		inline void ldi_r();
		inline void ldi_sp();
		inline void ldm_i();
		inline void ldm_r();
		inline void mov();
		inline void stm_i();
		inline void stm_r();
		inline void addi();
		inline void add_r2();
		inline void add_r3();
		inline void subi();
		inline void sub_r2(); 
		inline void sub_r3();
		inline void cmpi();
		inline void cmp();
		inline void andi();
		inline void and_r2();
		inline void and_r3();
		inline void tsti();
		inline void tst();
		inline void ori();
		inline void or_r2();
		inline void or_r3();
		inline void xori();
		inline void xor_r2();
		inline void xor_r3();
		inline void muli();
		inline void mul_r2();
		inline void mul_r3();
		inline void divi();
		inline void div_r2();
		inline void div_r3();
		inline void shl_n();
		inline void shr_n();
		inline void sar_n();
		inline void shl_r();
		inline void shr_r();
		inline void sar_r();
		inline void push();
		inline void pop();
		inline void pushall();
		inline void popall();
		inline void pushf();
		inline void popf(); 
		inline void pal_i();
		inline void pal_r();
	};

}

#endif
