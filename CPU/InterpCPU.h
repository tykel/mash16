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

#ifndef _INTERP_CPU_H
#define _INTERP_CPU_H

#include "CPU.h"

namespace Chip16 {

	class InterpCPU : public CPU
	{
	private:

	public:
		InterpCPU(void);
		~InterpCPU(void);

		void Execute();
		void Init();
		void Clear();

		inline void flags_add(int32 rx, int32 ry);
		inline void flags_sub(int32 rx, int32 ry);
		inline void flags_and(int32 rx, int32 ry);
		inline void nop(); inline void cls(); inline void vblnk(); inline void bgc(); inline void spr();
		inline void drw_i(); inline void drw_r(); inline void rnd(); inline void flip(); inline void snd0();
		inline void snd1(); inline void snd2(); inline void snd3(); inline void jmp_i(); inline void jmc();
		inline void jx(); inline void jme(); inline void jmp_r(); inline void call_i(); inline void ret();
		inline void cx(); inline void call_r(); inline void ldi_r(); inline void ldi_sp(); inline void ldm_i();
		inline void ldm_r(); inline void mov(); inline void stm_i(); inline void stm_r(); inline void addi();
		inline void add_r2(); inline void add_r3(); inline void subi(); inline void sub_r2(); 
		inline void sub_r3(); inline void cmpi(); inline void cmp(); inline void andi(); inline void and_r2();
		inline void and_r3(); inline void tsti(); inline void tst();
	};

}

#endif