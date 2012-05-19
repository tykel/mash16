/*
 *  header.c
 *  
 *  tykel (C) 2011-2012
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

