/*
 *  mash16 -- a chip16 emulator
 *  Copyright (C) 2011-2, T. Kelsall
 *
 *  main.c : program entry point 
 */

#include "consts.h"
#include "header/header.h"
#include "core/cpu.h"

#include <SDL/SDL.h>

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

    /* Initialise SDL target. */
    if(!SDL_Init(SDL_INIT_VIDEO))
        return 1;
    if(!SDL_SetVideoMode(320,240,32,SDL_SWSURFACE))
        return 1;

    /* Declare our variable. */
    cpu_state* state;
    cpu_init(state);
    uint32_t t = 0, oldt = 0;
    SDL_Event evt;
    int exit = 0;

    /* Emulation loop. */
    while(!exit)
    {
        while(state->meta.wait_vblnk && state->meta.cycles < FRAME_CYCLES)
            cpu_step(state);
        /* Handle input. */
        while(SDL_PollEvent(&evt))
        {
            switch(evt.type)
            {
                case SDL_KEYDOWN:
                    cpu_io_update(&evt.key,state);
                    break;
                case SDL_QUIT:
                    exit = 1;
                    break;
                default:
                    break;
            }
        }

        /* Timing for cycle times. */
        t = SDL_GetTicks();
        if(t - oldt < FRAME_DT)
            continue;
        /* Draw. */
    }

    /* Tidy up before exit. */
    SDL_Quit();
    return 0;
}
