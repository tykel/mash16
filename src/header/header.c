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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "header.h"
#include "crc.h"

int read_header(ch16_header* header, uint32_t size, uint8_t* data)
{
    if(header->magic != 0x36314843)
    {
        fprintf(stderr,"Invalid magic number\n");
        fprintf(stderr,"Found: 0x%x, Expected: 0x%x\n",
                header->magic, 0x36314843);
        return 0;
    }
    if(header->reserved != 0)
    {
        fprintf(stderr,"Reserved not 0\n");
        return 0;
    }
    if(header->rom_size != size - sizeof(ch16_header))
    {
        fprintf(stderr,"Incorrect size reported\n");
        fprintf(stderr,"Found: 0x%x, Expected: 0x%x\n",
                header->rom_size, (uint32_t)(size - sizeof(ch16_header)));
        return 0;
    }
    crc_t crc = crc_init();
    crc = crc_update(crc,data,size-sizeof(ch16_header));
    crc = crc_finalize(crc);
    if(header->crc32_sum != crc)
    {
        fprintf(stderr,"Incorrect CRC32 checksum\n");
        fprintf(stderr,"Found: 0x%x, Expected: 0x%x\n",
                header->crc32_sum, crc);
        return 0;
    }
    return 1;
}

