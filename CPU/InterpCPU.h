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