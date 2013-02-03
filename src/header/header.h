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

#define CH16_HEADER_SIZE 0x10

#include <stdint.h>

#pragma pack(push,1)
typedef struct ch16_header
{
    uint32_t magic;
    uint8_t  reserved;
    uint8_t  spec_ver;
    uint32_t rom_size;
    uint16_t start_addr;
    uint32_t crc32_sum;

} ch16_header;

typedef struct ch16_rom
{
    ch16_header header;
    uint8_t*    data;

} ch16_rom;
#pragma pack(pop)


int read_header(ch16_header*, uint32_t, uint8_t*);

