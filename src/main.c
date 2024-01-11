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

/* A global used in other files. */
int use_verbose;

#include "options.h"
#include "consts.h"
#include "strings.h"
#include "header/header.h"
#include "core/cpu.h"
#include "core/gpu.h"
#include "core/audio.h"

#include <SDL2/SDL.h>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#pragma pack(push,1)
typedef struct sym_entry
{
    uint16_t addr;
    uint16_t str_offs;
} sym_entry_t;
#pragma pack(pop)

/* Globals used within the file. */
static program_opts opts;
static cpu_state* state;
static char *symbol_strs;
static char *symbols[0x10000];
static SDL_Renderer *renderer;
static SDL_Window *window;
static SDL_Texture* screen;
static char strfps[256];

void (*cpu_exec)(cpu_state* state);

/* Timing variables. */
static int t = 0, oldt = 0;
static int fps = 0, lastsec = 0;
static int stop = 0, paused = 0;

/* State printing options. */
static int hex = 1;

void print_state(cpu_state* state)
{
    instr ni;
    ni.dword = state->i.dword; //*(uint32_t *)&state->m[state->pc];
    int i;
    const char *sym_imm = symbols[i_hhll(ni)];
    const char *sym_imm_pt = sym_imm ? sym_imm : "";
    const char sym_lp = sym_imm ? '(' : ' ';
    const char sym_rp = sym_imm ? ')' : ' ';

    printf("state @ cycle %ld:",state->meta.target_cycles);
    printf("   %04x [ %s%s ", state->pc, str_ops[i_op(ni)],
            i_op(ni)==0x12 || i_op(ni)==0x17 ? str_cond[i_yx(ni)&0xf]:"");
    switch(state->meta.type)
    {
        case OP_HHLL:
            printf("$%04x",i_hhll(ni));
            printf(" %c%s%c", sym_lp, sym_imm_pt, sym_lp);
            break;
        case OP_N:
            printf("%x",i_n(ni));
            break;
        case OP_R:
            printf("r%x",i_yx(ni)&0xf);
            break;
        case OP_R_N:
            printf("r%x, %x",i_yx(ni)&0xf,i_n(ni));
            break;
        case OP_R_R:
            printf("r%x, r%x",i_yx(ni)&0xf, i_yx(ni) >> 4);
            break;
        case OP_R_R_R:
            printf("r%x, r%x, r%x",i_yx(ni)&0xf, i_yx(ni) >> 4, i_z(ni));
            break;
        case OP_N_N:
            printf("%x, %x",i_yx(ni)&0xf, i_yx(ni) >> 4);
            break;
        case OP_R_HHLL:
            printf("r%x, $%04x",i_yx(ni)&0xf, i_hhll(ni));
            printf(" %c%s%c", sym_lp, sym_imm_pt, sym_rp);
            break;
        case OP_R_R_HHLL:
            printf("r%x, r%x, $%04x",i_yx(ni)&0xf, i_yx(ni) >> 4, i_hhll(ni));
            printf(" %c%s%c", sym_lp, sym_imm_pt, sym_rp);
            break;
        case OP_HHLL_HHLL:
            printf("$%02x, $%04x",i_yx(ni),i_hhll(ni));
            break;
        case OP_SP_HHLL:
            printf("sp, $%04x",i_hhll(ni));
            break;
        case OP_NONE:
        default:
            break;
    }
    printf(" ]      %c%s%c\n",
            symbols[state->pc] ? '(' : ' ',
            symbols[state->pc] ? symbols[state->pc] : "",
            symbols[state->pc] ? ')' : ' ');
    printf("--------------------------------------------------------------\n");
    printf("| pc:   0x%04x     |    sp:  0x%04x     |    flags: %c%c%c%c     | \n",
        state->pc,state->sp,state->f.c?'C':'_',state->f.z?'Z':'_',state->f.o?'O':'_',state->f.n?'N':'_');
    printf("| spr: %3dx%3d     |    bg:     0x%x     |    instr: %02x%02x%02x%02x |\n",
        state->sw,state->sh,state->bgc,i_op(ni),i_yx(ni),i_z(ni),i_res(ni));
    printf("--------------------------------------------------------------\n");
    for(i=0; i<4; ++i)
    {
        if(hex)
            printf("| r%x: 0x%04x   |  r%x: 0x%04x   |  r%x: 0x%04x   |  r%x: 0x%04x |\n",
                   i,((uint16_t*)state->r)[i],
                   i+4,((uint16_t*)state->r)[i+4],
                   i+8,((uint16_t*)state->r)[i+8],
                   i+12,((uint16_t*)state->r)[i+12]);
        else
            printf("| r%x: % 6d   |  r%x: % 6d   |  r%x: % 6d   |  r%x: % 6d |\n",
                   i,state->r[i],i+4,state->r[i+4],i+8,state->r[i+8],i+12,state->r[i+12]);
    }
    printf("--------------------------------------------------------------\n");
}

char*
get_symbol(uint16_t a)
{
   return symbols[a];
}

static int verify_header(uint8_t* bin, int len)
{
    ch16_header* header;
    uint8_t *data;
    
    header = (ch16_header*)bin;
    data = (uint8_t*)(bin + sizeof(ch16_header));
    if(read_header(header,len,data))
        return 1;
    return 0;
}

/* Return length of file if success; otherwise 0 */
static int read_file(char* fp, uint8_t* buf)
{
    int len, read;
    FILE* romf;
    
    romf = fopen(fp,"rb");
    if(romf == NULL)
        return 0;
    
    fseek(romf,0,SEEK_END);
    len = ftell(romf);
    fseek(romf,0,SEEK_SET);

    read = fread(buf,sizeof(uint8_t),len,romf);
    fclose(romf);
    
    return (read == len) ? len : 0;
}

/* Populate the symbol list. */
int read_symbols(char *fp)
{
    FILE *symf;
    uint32_t stroffs;
    size_t strblksz;
    int i;

    memset(symbols, 0, sizeof(symbols));

    symf = fopen(fp, "rb");
    if(symf == NULL)
    {
        fprintf(stderr, "error: could not open \"%s\"\n", fp);
        return 0;
    }

    fread(&stroffs, sizeof(stroffs), 1, symf);
    fseek(symf, 0, SEEK_END);
    strblksz = ftell(symf) - stroffs;
    symbol_strs = malloc(strblksz);
    fseek(symf, stroffs, SEEK_SET);
    fread(symbol_strs, strblksz, 1, symf);

    fseek(symf, sizeof(stroffs), SEEK_SET);
    if(use_verbose)
        printf("found string table offset: %d bytes\n", stroffs);
    for(i = 0; i < stroffs - sizeof(sym_entry_t); i += sizeof(sym_entry_t))
    {
        sym_entry_t sym;
        fread(&sym, sizeof(sym), 1, symf);
        symbols[sym.addr] = &symbol_strs[sym.str_offs];
        if(use_verbose)
            printf("> found symbol: %s -> 0x%04x\n",
                    symbols[sym.addr], sym.addr);
    }

    return 1;
}

int parse_breakpoints(program_opts* opts)
{
    int i;
   
    for(i = 0; i < opts->num_breakpoints; i++) {
        char *s = opts->breakpoints[i];
        int v = strtol(s, NULL, 0);
        /* If strtol returns 0 from a string which isn't some form of 0, it is
         * relatively safe to say it must be a symbol. */
        if(v == 0 &&
            strncmp(s, "0", 6) &&
            strncmp(s, "0x0", 6) &&
            strncmp(s, "0x00", 6) &&
            strncmp(s, "0x0000", 6))
        {
            int j;
            for(j = 0; j < 0x10000; j++)
            {
                if(symbols[j] && !strncmp(s, symbols[j], 100))
                {
                    v = j;
                    break;
                }
            }
            if(j == 0x10000)
            {
                fprintf(stderr, "error: symbol \"%s\" not found in exports\n", s); 
                return 0;
            }
        }
        opts->bpoffs[i] = v;
    }
    return 1;
}

/* Sanitize the input. */
void sanitize_options(program_opts* opts)
{
    int input_errors = 0;
    
    if(opts->video_scaler <= 0 || opts->video_scaler > 3)
    {
        fprintf(stderr,"error: scaler %dx not supported\n",opts->video_scaler);
        ++input_errors;
    }
    if(opts->audio_sample_rate != 8000 && opts->audio_sample_rate != 11025 &&
       opts->audio_sample_rate != 22050 && opts->audio_sample_rate != 44100 &&
       opts->audio_sample_rate != 48000)
    {
        fprintf(stderr,"error: %dHz sample rate not supported\n",opts->audio_sample_rate);
        ++input_errors;
    }
    if(opts->audio_buffer_size < 128)
    {
        fprintf(stderr,"error: audio buffer size (%d B) too small\n",opts->audio_buffer_size);
        ++input_errors;
    }
    if(opts->audio_volume < 0  || opts->audio_volume > 255)
    {
        fprintf(stderr, "error: volume %d not valid (range is 0-255)\n",opts->audio_volume);
        ++input_errors;
    }
    if(opts->use_cpu_rec)
    {
        cpu_exec = cpu_rec_1bblk;
        printf("> using experimental recompiler core\n");
    }

    if(input_errors)
        exit(1);
}

void breakpoint_print(cpu_state *state)
{
    int i;
    paused = opts.use_breakall;
    /* Stop at breakpoint if necessary. */
    if(opts.num_breakpoints > 0)
    {
        for(i=0; i<opts.num_breakpoints; ++i)
        {
            if(state->pc == opts.bpoffs[i])
            {
                paused = 1;
                break;
            }
        }
    }
    if(paused)
    {
        print_state(state);
    }
}

/* Emulation loop. */
void emulation_loop()
{ 
    int i;
    SDL_Event evt;
    
    if(paused)
    {
        SDL_Delay(FRAME_DT);
    }
    else
    {
        /* If using strict emulation, limit to 1M cycles / sec. */
        if(opts.use_cpu_limit)
        {
            while(!state->meta.wait_vblnk && state->meta.cycles < FRAME_CYCLES)
            {
                cpu_exec(state);
                breakpoint_print(state);
                if (paused) break;
            }
            /* Avoid hogging the CPU... */
            while((double)(t = SDL_GetTicks()) - oldt < FRAME_DT)
                SDL_Delay(1);
            oldt = t;
            ++fps;
        }
        /* Otherwise, max out in 1/60th sec. */
        else
        {
            while((t = SDL_GetTicks()) - oldt <= FRAME_DT )
            {
                for(i=0; i<600; ++i)
                {
                    cpu_exec(state);
                    /* Don't forget to count our frames! */
                    if(state->meta.wait_vblnk)
                    {
                        state->meta.wait_vblnk = 0;
                        ++fps;
                    }
                    else if(state->meta.cycles >= FRAME_CYCLES)
                    {
                        state->meta.cycles = 0;
                        ++fps;
                    }
                }
            }
            oldt = t;
        }
        /* Update the FPS counter after every second, or second's worth of frames. */
        if((fps >= 60 && opts.use_cpu_limit) || t > lastsec + 1000)
        {
            /* Update the caption. */
            sprintf(strfps,"mash16 (%d fps) - %s",fps,opts.filename);
            SDL_SetWindowTitle(window, strfps);
            /* Reset timing info. */
            lastsec = t;
            fps = 0;
        }
    }
        
    /* Handle input. */
    while(SDL_PollEvent(&evt))
    {
        switch(evt.type)
        {
            case SDL_KEYDOWN:
                cpu_io_update(&evt.key,state);
                if(evt.key.keysym.sym == SDLK_SPACE)
                {
                    if(!paused)
                    {
                        print_state(state);
                        paused = 1;
                    }
                    else
                        paused = 0;
                }
                else if(evt.key.keysym.sym == SDLK_n && paused)
                {
                    cpu_exec(state);
                    print_state(state);
                }
                else if(evt.key.keysym.sym == SDLK_h && paused)
                {
                    hex = !hex;
                    print_state(state);
                }
                else if(evt.key.keysym.sym == SDLK_ESCAPE)
                    stop = 1;
                break;
            case SDL_KEYUP:
                cpu_io_update(&evt.key,state);
                break;
            case SDL_QUIT:
                stop = 1;
                break;
            default:
                break;
        }
    }
    /* Draw. */
    blit_screen(screen,state,opts.video_scaler);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screen, NULL, NULL);
    SDL_RenderPresent(renderer);
    /* Reset vblank flag. */
    state->meta.wait_vblnk = 0;
    state->meta.cycles = 0;
    state->meta.old_pc = state->pc;
}

int main(int argc, char* argv[])
{
    int i, len, use_header, sdl_flags, video_flags;
    uint8_t *buf, *mem;

    /* Ensure STDIO goes to the terminal in Windows. */
#ifdef _WIN32
    freopen( "CON", "w", stdout );
    freopen( "CON", "w", stderr );
#endif

    /* Set up default options, then read them from the command line. */
    opts.filename = NULL;
    opts.pal_filename = NULL;
    opts.sym_filename = NULL;
    opts.use_audio = 1;
    opts.audio_sample_rate = AUDIO_RATE;
    opts.audio_buffer_size = AUDIO_SAMPLES;
    opts.audio_volume = 128;
    opts.use_verbose = 0;
    opts.video_scaler = 2;
    opts.use_fullscreen = 0;
    opts.use_cpu_limit = 1;
    opts.use_cpu_rec = 0;
    opts.num_breakpoints = 0;
    opts.use_breakall = 0;
    opts.rng_seed = time(NULL);
    cpu_exec = cpu_step;

    options_parse(argc,argv,&opts);
    use_verbose = opts.use_verbose;
    
    sanitize_options(&opts);
    if(opts.sym_filename)
    {
        read_symbols(opts.sym_filename);
        if(use_verbose)
            printf("loaded symbols from %s.\n", opts.sym_filename);
    }

    if(!parse_breakpoints(&opts))
    {
        exit(1);
    }
    if(use_verbose)
    {
        printf("total breakpoints: %d\n",opts.num_breakpoints);
        for(i=0; i<opts.num_breakpoints; ++i)
        {
            printf("> bp %d: 0x%x\n",i,opts.bpoffs[i]);
        }
    }

    /* Read our rom file into memory */
    buf = NULL;
    if(!(buf = (uint8_t *)calloc(MEM_SIZE+sizeof(ch16_header),1)))
    {
        fprintf(stderr,"error: calloc failed (buf)\n");
        exit(1);
    }
    len = read_file(opts.filename,buf);
    if(!len)
    {
        fprintf(stderr,"error: file could not be opened\n");
        exit(1);   
    }

    /* Check if a rom header is present; if so, verify it */
    use_header = 0;
    if((char)buf[0] == 'C' && (char)buf[1] == 'H' &&
       (char)buf[2] == '1' && (char)buf[3] == '6')
    {
        use_header = 1;
        if(!verify_header(buf,len))
        {
            fprintf(stderr,"error: header integrity check failed\n");
            exit(1);
        }
        if(opts.use_verbose)
        {
            ch16_header* h = (ch16_header*)buf;
            printf("header integrity check: ok\n");
            printf("> spec. version:  %d.%d\n",h->spec_ver>>4,h->spec_ver&0x0f);
            printf("> rom size:       %d B\n",h->rom_size);
            printf("> start address:  0x%04x\n",h->start_addr);
            printf("> crc32 checksum: 0x%08x\n",h->crc32_sum);
        }
    }
    else if(opts.use_verbose)
    {
        printf("header not found\n");
    }

    /* Get a buffer without header. */
    mem = NULL;
    if(!(mem = (uint8_t *)malloc(MEM_SIZE)))
    {
        fprintf(stderr,"error: malloc failed (mem)\n");
        exit(1);
    }
    memcpy(mem,(uint8_t*)(buf + use_header*sizeof(ch16_header)),
           len - use_header*sizeof(ch16_header));
    free(buf);

    /* Initialise SDL target. */
    sdl_flags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
    if(opts.use_audio)
        sdl_flags |= SDL_INIT_AUDIO;
    if(SDL_Init(sdl_flags) < 0)
    {
        fprintf(stderr,"error: failed to initialise SDL: %s\n",SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

    video_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC |
      (opts.use_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    if(SDL_CreateWindowAndRenderer(opts.video_scaler*320,opts.video_scaler*240,video_flags,&window,&renderer))
    {
        fprintf(stderr,"error: failed to init. video mode (%d x %d x 32 bpp): %s\n",
                opts.video_scaler*320,opts.video_scaler*240,
                SDL_GetError());
        exit(1);
    }
    if(opts.use_verbose)
        printf("sdl initialised: %d x %d x %d bpp%s\n",
                opts.video_scaler*320,opts.video_scaler*240,32,
                opts.use_fullscreen?" (fullscreen)":"");

    sprintf(strfps,"mash16 - %s",opts.filename);
    SDL_SetWindowTitle(window,strfps);

    screen = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,320,240);

    /* Initialise the chip16 processor state. */
    cpu_init(&state,mem,&opts);
    audio_init(state,&opts);
    init_pal(state);
    if(opts.use_verbose)
        printf("chip16 state initialised\n\n");

    if(opts.pal_filename != NULL)
        if(!read_palette(opts.pal_filename, state->pal))
            fprintf(stderr,"error: palette in %s could not be read, potential corruption\n",opts.pal_filename);

    while(!stop)
        emulation_loop();

    /* Tidy up before exit. */
    audio_free();
    cpu_free(state);
    free(mem); 
    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if(opts.use_verbose)
        printf("memory freed, goodbye\n");
    exit(0);
}

void panic(const char* format, ...)
{
   raise(SIGTRAP);
   
   va_list va = { 0 };
   va_start(va, format);
   vfprintf(stderr, format, va);
   va_end(va);

   exit(1);
}
