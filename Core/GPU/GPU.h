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

#ifndef GPU_H
#define GPU_H

#include "../Common.h"

// Default color palette
enum color {
    BLACK_TR = 0x0, BLACK, GRAY, RED, PINK, DK_BROWN, BROWN, ORANGE,
    YELLOW, GREEN, LT_GREEN, DK_BLUE, BLUE, LT_BLUE, SKY_BLUE, WHITE
}
// Sprite flip states
enum flip {
	NONE = 0, VERT = 1, HORZ = 2, BOTH = 4
};
// GPU state info
struct gpu_state {
	uint32	bg;				// Background color
	uint32	fp;				// Flip orientation mask
	union {
		uint32 sz;			// Sprite size (aggr.)
		struct {
			uint16	h;		// Sprite height (in cols)
			uint16	w;		// Sprite width (in B)
		};
	};
};

namespace Chip16 {

	class GPU
	{
	public:
		gpu_state m_state;
        uint32 m_colors[16];
		GPU(void);
		~GPU(void);

		virtual void Blit(spr_info* si) = 0;
		virtual void Draw() = 0;
		virtual void Clear() = 0;
	};

}

#endif
