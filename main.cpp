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

#include "Core/Common.h"
#include "Core/System.h"
#include "Gui/SfmlGui.h"

#include <SFML/System.hpp>

#include <fstream>
#include <iostream>

const char* title = "mash16";

bool readFile(const char* fp, uint8* dest) {	
	std::ifstream rom(fp,std::ios::binary);
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
	// Emulation core
    Chip16::System chip16;
    
    // Read in file
	uint8* mem = new uint8[MEMORY_SIZE]();
	if(argc > 1) {
		if(!readFile(argv[1],mem))
            return 1;
    }
	else 
		return 1;
    std::clog << "read file: " << argv[1] << std::endl;
	chip16.LoadRom(mem);
    std::clog << "loaded rom\n";
    
    //Initialize it now we have memory occupied
    chip16.Init();
	
    // Windowing system
    Chip16::SfmlGui window;
    window.Init(title,&chip16);
    std::clog << "using 2X scale (640x480)\n";
    window.setScale(SCALE_2X);

    int cycles; 
    // Start emulation
	while(window.IsOpen()) {
        // Process one frame's worth of CPU cycles
		cycles = 0;
        while(cycles++ < FRAME_DT) {
			if(!chip16.getCPU()->IsWaitingVblnk()) 
                chip16.ExecuteStep();
		}
        // Sleep the remaining frame time
		if(chip16.GetCurDt() < FRAME_DT)
            // Move to Chip16::Timer and Chip16::System
            sf::Sleep((FRAME_DT-chip16.GetCurDt())/2000000);
        // Let the GPU push screen changes 
        chip16.getGPU()->Draw();
        // Update the window contents
		window.Update();
        // Start timer for new frame
        chip16.ResetDt();
	}
    
	// Emulation is over
    chip16.Clear();
	delete mem;

    std::clog << "exiting\n";
	return 0;
}
