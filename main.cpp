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

#include "CPU\DynarecCPU.h"
#include "CPU\InterpCPU.h"

int main(int argc, char** argv) {
	bool useDynarec = false;
	Chip16::CPU* chip16;
	if(useDynarec)
		chip16 = new Chip16::DynarecCPU();
	else
		chip16 = new Chip16::InterpCPU();
	chip16->Init();
	chip16->LoadROM(argv[1]);
	// TODO: Timing
	while(chip16->IsReady()) {
		chip16->Execute();
	}
	chip16->Clear();
	return 0;
}