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

#ifndef OPTIONS_H
#define OPTIONS_H

 #include <stdint.h>

#define MAX_STRING	100
#define BREAKPOINTS 100

typedef struct program_opts
{
    char *filename;
    char *pal_filename;
    char *sym_filename;
    int use_audio;
    int audio_sample_rate;
    int audio_buffer_size;
    int audio_volume;
    int use_verbose;
    int video_scaler;
    int use_fullscreen;
    int use_cpu_limit;
    int use_cpu_rec;
    int cpu_rec_1bblk_per_op;
    char *breakpoints[BREAKPOINTS];
    int bpoffs[BREAKPOINTS];
    int num_breakpoints;
    int use_breakall;

    int rng_seed;
} program_opts;

int read_palette(char const *,uint32_t *);

void options_parse(int,char**,program_opts*);

#endif
