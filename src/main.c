/*
 *  mash16 -- a chip16 emulator
 *  Copyright (C) 2011-2, T. Kelsall
 *
 *  main.c : program entry point 
 */

#include "consts.h"
#include "header/header.h"
#include "core/cpu.h"
#include "core/gpu.h"

#include <SDL/SDL.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void print_state(cpu_state* state)
{
    printf("---------------------------------------\n");
    printf("pc: %x\t\tsp: %x\t\tflags: %d%d%d%d\n",
        state->pc,state->sp,state->f.c,state->f.z,state->f.o,state->f.n);
    printf("spr: %dx%d\t\tbgc: %d\t\tinstr: %08x\n",state->sw,state->sh,state->bgc,state->i.dword);
    for(int i=0; i<4; ++i)
        printf("r%d: %d\t\tr%d: %d\t\tr%d: %d\t\tr%d: %d\n",
            i,state->r[i],i+4,state->r[i+4],i+8,state->r[i+8],i+12,state->r[i+12]);
    if(getchar() == 'q')
        exit(1);
}

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

    printf("argc >= 2\n");
    
    /* Read our rom file into memory */
    uint8_t* buf = calloc(MEM_SIZE+sizeof(ch16_header),sizeof(uint8_t));
    int len = read_file(argv[1],buf);
    if(!len)
        return 1;

    printf("Read file\n");

    /* Check if a rom header is present; if so, verify it */
    int use_header = 0;
    if((char)buf[0] == 'C' && (char)buf[1] == 'H' &&
       (char)buf[2] == '1' && (char)buf[3] == '6')
    {
        use_header = 1;
        if(!verify_header(buf,len))
            return 1;
    }

    printf("Verified header\n");

    /* Get a buffer without header. */
    uint8_t* mem = malloc(MEM_SIZE);
    memcpy(mem,(uint8_t*)(buf + use_header*sizeof(ch16_header)),
           len - use_header*sizeof(ch16_header));
    free(buf);

    printf("Copied ROM\n");

    /* Initialise SDL target. */
    SDL_Surface* screen;
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr,"Failed to initialise SDL: %s\n",SDL_GetError());
        return 1;
    }
    if((screen = SDL_SetVideoMode(640,480,32,SDL_SWSURFACE|SDL_DOUBLEBUF)) == NULL)
    {
        fprintf(stderr,"Failed to init. video mode (320x240,32bpp): %s\n",SDL_GetError());
        return 1;
    }

    printf("SDL initialised\nScreen surface format: %dbpp w=%d h=%d\n",
            screen->format->BitsPerPixel,screen->w,screen->h);
    
    SDL_WM_SetCaption("mash16","mash16");

    /* Initialise the Chip16 processor state. */
    cpu_state* state = NULL;
    cpu_init(&state,mem);
    init_pal(state);
    
    uint32_t t = 0, oldt = 0;
    int exit = 0;
    
    /* Emulation loop. */
    while(!exit)
    {
        while(!state->meta.wait_vblnk && state->meta.cycles < FRAME_CYCLES)
            cpu_step(state);
        //print_state(state);
        /* Handle input. */
        //cpu_io_reset(state);
        SDL_Event evt;
        while(SDL_PollEvent(&evt))
        {
            switch(evt.type)
            {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
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
        while((t = SDL_GetTicks()) - oldt < FRAME_DT) ;
            //SDL_Delay(0);
        oldt = t;
        /* Draw. */
        blit_screen2x(screen,state);
        /* Reset vblank flag. */
        state->meta.wait_vblnk = 0;
        state->meta.cycles = 0;
    }

    printf("\n");

    /* Tidy up before exit. */
    cpu_free(state);
    free(mem);
    SDL_FreeSurface(screen);
    SDL_Quit();

    return 0;
}
