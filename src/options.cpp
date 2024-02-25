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

#include "options.h"
#include "strings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Fallback values to remove warnings */
#ifndef VERSION
#define VERSION "version"
#endif

#ifndef BUILD
#define BUILD "build"
#endif

extern int use_verbose;

int read_palette(char const *filename, uint32_t *palette)
{
    int i;
    FILE *palf = NULL;

    palf = fopen(filename,"r");
    if(palf == NULL)
        return 0;
    
    if(use_verbose)
        printf("palette loaded:\n>");
    
    for(i = 0; i < 16; ++i)
    {
        fscanf(palf,"%x",&palette[i]);
        if(use_verbose)
            printf("%s%06x",(i && !(i%4)) ? "\n> " : " ",palette[i]);
    }
    
    if(use_verbose)
        printf("\r\n\n");

    return 1;
}

void options_parse(int argc, char** argv, program_opts* opts)
{
    int i, len;
    
    if(argc < 2)
    {
        fprintf(stderr,"warning: no filename supplied, exiting...\n");
        exit(0);
    }
    else if(!strncmp(argv[1],"--help",MAX_STRING))
    {
        printf("%s",str_help[0]);
        printf("%s",str_help[1]);
        exit(0);
    }
    else if(!strncmp(argv[1],"--version",MAX_STRING))
    {
        printf("mash16 %s (%s) -- the chip16 emulator\n",VERSION,BUILD);
        exit(0);
    }
    else if(argv[1][0] == '-')
    {
        fprintf(stderr,"warning: no filename supplied, exiting...\n");
        exit(0);
    }
    
	len = strlen(argv[1]);
    if(len > 235)
	{
		fprintf(stderr,"error: filename too long (%d chars., max. 235), exiting...\n",len);
		exit(1);
	}
	opts->filename = argv[1];
    if(argc > 2)
    {
        for(i=2; i<argc; ++i)
        {
            if(!strncmp(argv[i],"--no-audio", MAX_STRING))
            {
                opts->use_audio = 0;
            }
            else if(!strncmp(argv[i],"--audio-sample-rate",19))
            {
                char *num;
                long int rate;

                if(strlen(argv[i]) > 20 && argv[i][19] == '=')
                    num = &argv[i][20];
                else if(i+1 < argc)
                    num = argv[++i];
                else
                {
                    fprintf(stderr,"error: no audio sample rate provided\n");
                    continue;
                }

                rate = strtol(num,NULL,0);
                if(!rate)
                    fprintf(stderr,"error: invalid input '%s'\n",num);
                else
                    opts->audio_sample_rate = rate;
            }
            else if(!strncmp(argv[i],"--audio-buffer",14))
            {
                long int size;
                char *num;

                if(strlen(argv[i]) > 15 && argv[i][14] == '=')
                    num = &argv[i][15];
                else if(i+1 < argc)
                    num = argv[++i];
                else
                {
                    fprintf(stderr,"error: no audio buffer size provided\n");
                    continue;
                }

                size = strtol(num,NULL,0);
                if(!size)
                    fprintf(stderr,"error: invalid input '%s'\n",num);
                else
                    opts->audio_buffer_size = size;

            }
            else if(!strncmp(argv[i],"--audio-volume",14))
            {
                long int vol;
                char *num;

                if(strlen(argv[i]) > 15 && argv[i][14] == '=')
                    num = &argv[i][15];
                else if(i+1 < argc)
                    num = argv[++i];
                else
                {
                    fprintf(stderr,"error: no volume amount provided\n");
                    continue;
                }

                vol = strtol(num,NULL,0);
                if(!vol)
                    fprintf(stderr,"error: invalid input '%s'\n",num);
                else
                    opts->audio_volume = vol;

            }
            else if(!strncmp(argv[i],"--verbose",MAX_STRING))
            {
                opts->use_verbose = 1;
            }
            else if(!strncmp(argv[i],"--break",7))
            {
                char *n, *nums;
                
                if(!strncmp(argv[i],"--break-all",MAX_STRING))
                {
                    opts->use_breakall = 1;
                    continue;
                }
               
                if(strlen(argv[i]) > 8 && argv[i][7] == '@')
                {
                    nums = &argv[i][8];
                    n = strtok(nums,", ");
                    while(n != NULL)
                    {
                        opts->breakpoints[opts->num_breakpoints++] = n;
                        n = strtok(NULL,", ");
                    }
                }
                else
                {
                    fprintf(stderr,"error: no breakpoint specified\n");
                    exit(1);
                }
            }
            else if (!strncmp(argv[i],"--watch", 7)) {
                char *n, *nums;
                
                if (strlen(argv[i]) > 8 && argv[i][7] == '@') {
                    nums = &argv[i][8];
                    n = strtok(nums, ", ");
                    while (n != NULL) {
                        opts->watchpoints[opts->num_watchpoints++] = n;
                        n = strtok(NULL, ", ");
                    }
                } else {
                    fprintf(stderr, "error: no breakpoint specified\n");
                    exit(1);
                }
            }
            else if(!strncmp(argv[i],"--fullscreen",MAX_STRING))
            {
                opts->use_fullscreen = 1;
            }
            else if(!strncmp(argv[i],"--video-scaler",14))
            {
                long int scale;
                char *num;

                if(strlen(argv[i]) > 15 && argv[i][14] == '=')
                    num = &argv[i][15];
                else if(i+1 < argc)
                    num = argv[++i];
                else
                {
                    fprintf(stderr,"error: no video scale provided\n");
                    continue;
                }
                
                scale = strtol(num,NULL,0);
                if(!scale)
                    fprintf(stderr,"error: invalid input '%s'\n",num);
                else
                    opts->video_scaler = scale;
            }
            else if(!strncmp(argv[i],"--palette",9))
            {
                if(strlen(argv[i]) > 10 && argv[i][9] == '=')
                    opts->pal_filename = &argv[i][10];
                else if(i+1 < argc)
                    opts->pal_filename = argv[++i];
                else
                {
                    fprintf(stderr,"error: no palette file provided\n");
                    continue;
                }
            }
            else if(!strncmp(argv[i],"--symbols",9))
            {
                if(strlen(argv[i]) > 10 && argv[i][9] == '=')
                    opts->sym_filename = &argv[i][10];
                else if(i+1 < argc)
                    opts->sym_filename = argv[++i];
                else
                {
                    fprintf(stderr,"error: no symbol file provided\n");
                    continue;
                }
            }
            else if(!strncmp(argv[i],"--no-cpu-limit",MAX_STRING))
            {
                opts->use_cpu_limit = 0;
            }
            else if(!strncmp(argv[i],"--cpu-rec",MAX_STRING))
            {
                opts->use_cpu_rec = 1;
            }
            else if(!strncmp(argv[i],"--cpu-rec-1bblk-per-op",MAX_STRING))
            {
                opts->cpu_rec_1bblk_per_op = 1;
            }
            else if (!strncmp(argv[i],"--debugger",10))
            {
                char *debugger = NULL;
                if(strlen(argv[i]) > 11 && argv[i][10] == '=')
                {
                    debugger = &argv[i][11];
                }
                else if(i+1 < argc)
                {
                    debugger = argv[++i];
                }
                else
                {
                    fprintf(stderr,"error: no debugger type supplied\n");
                    continue;
                }
                if(!strncmp(debugger,"ui",2))
                {
                    opts->debug_ui = 1;
                    opts->debug_stdout = 0;
                }
                else if(!strncmp(debugger,"stdout",6))
                {
                    opts->debug_ui = 0;
                    opts->debug_stdout = 1;
                }
                else if(!strncmp(debugger,"both",4))
                {
                    opts->debug_ui = 1;
                    opts->debug_stdout = 1;
                }
                else
                   fprintf(stderr,"error: invalid debugger type \"%s\"\n",debugger);
            }
            else if(!strncmp(argv[i],"--help",MAX_STRING))
            {
                printf("%s",str_help[0]);
                printf("%s",str_help[1]);
            }
            else if(!strncmp(argv[i],"--version",MAX_STRING))
            {
                printf("mash16 %s (%s) -- the chip16 emulator\n",VERSION,BUILD);
            }
            else
            {
                fprintf(stderr,"warning: invalid option '%s', ignoring\n",argv[i]);
            }
        }    
    }
}

