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

#ifndef AUDIO_H
#define AUDIO_H

#include "../options.h"
#include "cpu.h"

#define SND1_FREQ		500
#define SND2_FREQ		1000
#define SND3_FREQ		1500

/* Data types and structures. */
typedef enum 
{  
	WF_TRIANGLE = 0,
	WF_SAWTOOTH,
	WF_PULSE,
	WF_NOISE
} waveform_t;

/* Lookup tables for envelope durations. */
static const int atk_ms[16] = 
{
	2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000
};

static const int dec_ms[16] = 
{
	6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000
};

static const int rls_ms[16] =
{
	6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000
};

/* Structure to track how the sound should be generated. */
typedef struct
{
	/* PCM data relevant. */
	int sample_rate, buffer_size;
	/* Current sample index, and number of samples to generate. */
	int s_index, s_total;
	/* Current position in the waveform period. */
	int s_period_index, s_period_total;
	/* Temporary storage for returning samples. */
	double sample;
	/* Sample length of different parts of the sound. */
	int atk_samples, dec_samples, sus_samples, rls_samples;

	/* ADSR relevant. */
	/* Waveform type. */
	waveform_t wf;
	/* Fequency, duration, tone, ADSR, volume. */
	int f;
	int dt;
    int tone;
    int atk, dec, sus, rls;
    int vol, max_vol;
    /* Should the ADSR envelope be used (snd0-3 v. sng/p). */
	int use_envelope;

} audio_state;

/* Function declarations. */
void audio_init(cpu_state*,program_opts*);
void audio_free();
void audio_update(cpu_state*);
void audio_play(int16_t,int16_t,int);
void audio_stop();
void audio_callback(void*,uint8_t*,int);
int16_t audio_gen_sample();
int16_t audio_gen_snd1_sample();
int16_t audio_gen_snd2_sample();
int16_t audio_gen_snd3_sample();

#endif