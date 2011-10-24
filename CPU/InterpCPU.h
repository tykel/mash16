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

		void nop(); void cls(); void vblnk(); void bgc(); void spr();
		void drw_i(); void drw_r(); void rnd(); void flip(); void snd0();
		void snd1(); void snd2(); void snd3(); void jmp_i(); void jmc();
		void jx(); void jme(); void jmp_r(); void call_i(); void ret();
		void cx(); void call_r(); void ldi_r(); void ldi_sp(); void ldm_i();
		void ldm_r(); void mov(); void stm_i(); void stm_r(); void addi();
	};

}

#endif