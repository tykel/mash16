/*
 *   mash16 - the chip16 emulator
 *   Copyright (C) 2012-2013 tykel
 *
 *   mash16 is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   mash16 is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with mash16.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONSTS_H
#define CONSTS_H

#define CLOCK_RATE		1000000

#define MEM_SIZE        0x10000
#define MAX_ADDR        0xFFFF
#define STACK_ADDR      0xFDF0
#define IO_PAD1_ADDR    0xFFF0
#define IO_PAD2_ADDR    0xFFF2

#define FRAME_CYCLES    (CLOCK_RATE/60.0)
#define FRAME_DT        16

#define PAD_UP          0x01
#define PAD_DOWN        0x02
#define PAD_LEFT        0x04
#define PAD_RIGHT       0x08
#define PAD_SELECT      0x10
#define PAD_START       0x20
#define PAD_A           0x40
#define PAD_B           0x80

#define AUDIO_RATE		48000
#define AUDIO_SAMPLES   1024	

#include <stdint.h>

#endif
