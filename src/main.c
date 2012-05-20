/*
 *  mash16 -- a chip16 emulator
 *  Copyright (C) 2011-2, T. Kelsall
 *
 *  main.c : program entry point 
 */

#include "header/header.h"
#include "consts.h"

#include <stdlib.h>
#include <stdio.h>

int verify_header(uint8_t* bin, int len)
{
    ch16_header* header = (ch16_header*)bin;
    uint8_t* data = (uint8_t*)(bin + sizeof(ch16_header));
    if(read_header(header,len,data))
        return 1;
    return 0;
}

/* Return length of file if success; otherwise 0 */
int read_file(char* fp, uint8_t* buf)
{
    FILE* romf = fopen(fp,"rb");
    if(romf == NULL)
        return 0;
    
    fseek(romf,0,SEEK_END);
    int len = ftell(romf);
    fseek(romf,0,SEEK_SET);

    fread(buf,sizeof(uint8_t),len,romf);
    fclose(romf);
    return len;
}

int main(int argc, char* argv[])
{
    /* Until a non-SDL GUI is implemented, exit if no rom specified */
    if(argc < 2)
        return 1;
    
    /* Read our rom file into memory */
    uint8_t* buf = calloc(MEM_SIZE,sizeof(uint8_t));
    int len = read_file(argv[1],buf);
    if(!len)
        return 1;
    /* Check if a rom header is present; if so, verify it */
    int use_header = 0;
    if((char)buf[0] == 'C' && (char)buf[1] == 'H' &&
       (char)buf[2] == '1' && (char)buf[3] == '6')
    {
        use_header = 1;
        if(!verify_header(buf,len))
            return 1;
    }

    /* Start emulation... */
    printf("File read, valid ROM.\nExiting...\n");

    return 0;
}
