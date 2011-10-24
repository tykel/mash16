#ifndef _GPU_H
#define _GPU_H

#include "..\Common.h"

enum flip {
	NONE = 0, VERT = 1, HORZ = 2, BOTH = 4
};

struct gpu_state {
	uint16	sz;		// Sprite size
	uint16	bg;		// Background color
	uint32	fp;		// Flip orientation mask
};

namespace Chip16 {

	class GPU
	{
	public:
		gpu_state m_state;

		GPU(void);
		~GPU(void);

		virtual void Blit(spr_info* si) = 0;
		virtual void Draw() = 0;
		virtual void Clear() = 0;
	};

}

#endif
