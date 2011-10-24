#ifndef _CPU_H
#define _CPU_H

#include "..\Common.h"
#include "..\GPU\GPU.h"

// Used to represent CPU state
struct cpu_state {
	uint16* r;		// Registers R0..F
	uint16	pc;		// Program counter
	uint16	sp;		// Stack pointer
	uint16	fl;		// Flags register
};
// Used to map instruction segments
struct cpu_instr {
	uint8	op;
	uint8	yx;
	union {
		uint16 addr;
		uint16 n;
		uint16 z;
		struct {
			uint8 res;
			uint8 fp;
		};
	};
};

namespace Chip16 {

	class CPU
	{
	protected:
		uint8*		m_mem;
		cpu_state	m_state;
		spr_info	m_sprinfo;
		cpu_instr*	m_instr;
		GPU*		m_gpu;
		bool		m_isReady;
	
		void Draw(int16 x, int16 y, uint16 start);

	public:
		CPU(void);
		~CPU(void);

		virtual void Execute() = 0;
		virtual void Init() = 0;
		virtual void Clear() = 0;
		void LoadROM(const char*);
		bool IsReady();

	};

}

#endif