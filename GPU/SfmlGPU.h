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
