/*
 *  mash16 -- a chip16 emulator
 *  Copyright (C) 2011-2, T. Kelsall
 *
 *  main.c : program entry point 
 */

#include "header/header.h"

int verify_header(uint8_t* bin)
{
    ch16_header* header = (ch16_header*)bin;
    uint8_t* data = (uint8_t*)(bin + sizeof(ch16_header));
    if(read_header(header,len,data))
        return 1;
    return 0;
}

int main(int argc, char* argv[])
{

    return 0;
}
