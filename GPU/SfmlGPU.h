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

#ifndef _SFML_GPU_H
#define _SFML_GPU_H

#include "GPU.h"
#include "..\Common.h"

namespace Chip16 {

	class SfmlGPU : public GPU
	{
	public:
		SfmlGPU(void);
		~SfmlGPU(void);

		void Blit(spr_info* si);
		void Draw();
		void Clear();
	};

}

#endif
