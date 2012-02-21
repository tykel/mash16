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

#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;

const uint32 MEMORY_SIZE = 0x10000;
const uint32 MAX_ADDRESS = 0x0FFFF;
const uint16 MAX_VALUE16 = 0xFFFF;
const int16  MAX_SVALUE16 = 32767;
const int16	 MAX_NVALUE16 = -32768;
const uint8  MAX_VALUE8	 = 0xFF;
const uint16 NEG_BIT16   = 0x8000;
const uint32 REGS_SIZE	 = 16;

const uint32 CYCLE_DT = 1;
const uint32 FRAME_DT = 16667;

// Structure shared by CPU and CPU for fast sprite blitting
struct spr_info {
	int32 x;			// x coordinate of the sprite
	int32 y;			// y coordinate of the sprite
	const uint8* data;	// ptr to start of sprite data
};

#endif
