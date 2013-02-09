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

#define MAX_STRING	100
#define BREAKPOINTS 100

typedef struct program_opts
{
	char *filename;
	int use_audio;
	int audio_sample_rate;
	int audio_buffer_size;
	int use_verbose;
	int video_scaler;
	int use_cpu_limit;
	int use_cpu_rec;
	int breakpoints[BREAKPOINTS];
	int num_breakpoints;
} program_opts;

void options_parse(int,char**,program_opts*);

#endif
