#ifndef _DYNAREC_CPU_H
#define _DYNAREC_CPU_H

#include "CPU.h"

namespace Chip16 {

	class DynarecCPU : public CPU
	{
	public:
		DynarecCPU(void);
		~DynarecCPU(void);

		void Execute();
		void Init();
		void Clear();
	};

}

#endif