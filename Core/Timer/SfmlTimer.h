/*
	Mash16 - an open-source C++ Chip16 emulator
    Copyright (C) 2011-12 Tim Kelsall

    Mash16 is free software: you can redistribute it and/or modify it under the terms 
	of the GNU General Public License as published by the Free Software Foundation, 
	either version 3 of the License, or  (at your option) any later version.

    Mash16 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
	PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program.
	If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SFML_TIMER_H
#define SFML_TIMER_H

#include <SFML/System.hpp>
#include "Timer.h"

namespace Chip16 {
    class SfmlTimer : public Timer {
        private:
            sf::Clock m_timer;

        public:
            SfmlTimer();
            ~SfmlTimer();
            
            uint32 GetDt(); 
            void Reset();
    };
}

#endif

