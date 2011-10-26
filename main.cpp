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

#include "Core/Common.h"
#include "Core/System.h"

#include <fstream>
#include <iostream>

bool readFile(const char* fp, uint8* dest) {	
	std::fstream rom(fp,std::ios::binary);
	if(rom.is_open()) {
		// Find file length
		rom.seekg (0, std::ios::end);
		uint32 len = (uint32) rom.tellg();
		rom.seekg (0, std::ios::beg);
		// Copy it to memory
		rom.read((char*)dest,len);
		// Clean up
		rom.close();
		return true;
	}
	std::clog << "Could not open file" << std::endl;
	return false;	
}

int main(int argc, char** argv) {
	Chip16::System chip16;
	uint8* mem = new uint8[MEMORY_SIZE]();
	if(argc > 1)
		readFile(argv[1],mem);
	else 
		return 1;
	chip16.LoadRom(mem);
	chip16.Run();
	chip16.Clear();
	// All the windowing is missing
	
	delete mem;
	return 0;
}