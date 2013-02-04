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

#include "audio.h"
#include <SDL/SDL.h>

static audio_state as;

typedef int16_t (*fptr)();
fptr f_sample;

/* Initialise the SDL audio system. */
void audio_init(cpu_state *state)
{
	f_sample = NULL;

	as = (audio_state){0};
	as.wf = WF_TRIANGLE;
	as.f = 100;
	as.vol = INT16_MAX;

	SDL_AudioSpec spec;
	spec.freq = AUDIO_RATE;
	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	spec.samples = (uint16_t)AUDIO_SAMPLES;
	spec.callback = audio_callback;
	spec.userdata = state;

	if(SDL_OpenAudio(&spec, NULL) < 0)
	{
		fprintf(stderr,"error: could not open audio: %s",SDL_GetError());
		exit(1);
	}
}

/* Clean up after the sound system. */
void audio_free()
{
	SDL_LockAudio();
	SDL_CloseAudio();
	SDL_UnlockAudio();
}

/* (Re)populate the audio buffer and start playing. */
void audio_play(int16_t f, int16_t dt, int adsr)
{
	/* Pause while we set up parameters. */
	SDL_PauseAudio(1);
	/* Set things up. */
	as.f = f;
	as.dt = dt;
	as.use_envelope = adsr;
	/* Number of samples for the whole sound. */
	as.s_total = (dt * AUDIO_RATE/1000) + as.rls_samples;
	/* Number of samples for an oscillation period. */
	as.s_period_total = AUDIO_RATE / (as.f + 1);
	/* Number of samples for the sustain duration. */
	as.sus_samples = (dt * AUDIO_RATE/1000) - as.atk_samples
											- as.dec_samples;
	if(as.sus_samples < 0)
		as.sus_samples = 0;
	//printf("A=%ds\tD=%ds\tS=%ds\tR=%ds\tTOTAL=%d\n",
	//	as.atk_samples,as.dec_samples,as.sus_samples,as.rls_samples,as.s_total);
	as.s_index = 0;

	/* Set up the right sampling function. */
	if(as.use_envelope)
		f_sample = &audio_gen_sample;
	else if(as.f == 500)
		f_sample = &audio_gen_snd1_sample;
	else if(as.f == 1000)
		f_sample = &audio_gen_snd2_sample;
	else if(as.f == 1500)
		f_sample = &audio_gen_snd3_sample;
	else
	{
		fprintf(stderr,"error: invalid frequency for beeper (%dHz)\n",as.f);
		exit(1);
	}

	/* Unset pause, start playing. */
	SDL_PauseAudio(0);
}

/* Stop playing audio. */
void audio_stop()
{
	SDL_PauseAudio(1);
}

/* Reload the sound parameters. */
void audio_update(cpu_state *state)
{
	/* Copy over the envelope parameters. */
	as.wf = (waveform)state->type;
	as.atk = atk_ms[state->atk];
	as.dec = dec_ms[state->dec];
	as.sus = INT16_MAX / (2*(16 - state->sus));
	as.rls = rls_ms[state->rls];
	as.vol = INT16_MAX / (2*(16 - state->vol));
	as.tone = state->tone;
	/* Get number of as.s_period_total from duration for the perods. */
	as.atk_samples = (AUDIO_RATE * as.atk) / 1000;
	as.dec_samples = (AUDIO_RATE * as.dec) / 1000;
	as.rls_samples = (AUDIO_RATE * as.rls) / 1000;
}

/* Callback function provided to SDL for copying sound data. */
void audio_callback(void* data, uint8_t* stream, int len)
{
	if(as.s_index >= as.s_total)
		return;

	int16_t *buffer = (int16_t*)stream;
	/* We are dealing with 16 bit as.s_period_total. */
	len /= 2;

	for(int i=0; i<len; ++i, ++as.s_index)
		buffer[i] = (*f_sample)();
}

/* Simple square tone generators. */
int16_t audio_gen_snd1_sample()
{
	++as.s_period_index;
	if(as.s_period_index >= SND1_SAMPLES)
		as.s_period_index = 0;
	if(2*as.s_period_index < SND1_SAMPLES)
		return -as.vol;
	return as.vol;
}

int16_t audio_gen_snd2_sample()
{
	++as.s_period_index;
	if(as.s_period_index >= SND2_SAMPLES)
		as.s_period_index = 0;
	if(2*as.s_period_index < SND2_SAMPLES)
		return -as.vol;
	return as.vol;
}

int16_t audio_gen_snd3_sample()
{
	++as.s_period_index;
	if(as.s_period_index >= SND3_SAMPLES)
		as.s_period_index = 0;
	if(2*as.s_period_index < SND3_SAMPLES)
		return -as.vol;
	return as.vol;
}

/* Our more advanced envelope generators. */
int16_t audio_gen_sample()
{
	++as.s_period_index;
	if((double)as.s_period_index >= as.s_period_total)
		as.s_period_index = 0;
	switch(as.wf)
	{
		case WF_TRIANGLE:
			as.sample = 2.0 * (double)as.s_period_index / (double)as.s_period_total - 1.0;
			break;
		case WF_SAWTOOTH:
			if(4*as.s_period_index < as.s_period_total) 
				as.sample = (double)(4*as.s_period_index)/(double)(as.s_period_total);
			else
				as.sample = (double)(as.s_period_total)/(double)(4*as.s_period_index);
			break;
		case WF_PULSE:
			as.sample = 1.0; 
			break;
		case WF_NOISE:
			as.sample = 2.0*(double)(rand() % INT16_MAX)/(double)(INT16_MAX) - 1.0;
			break;
		default:
			fprintf(stderr, "error: invalid ADSR envelope type (%d)", as.wf);
			exit(1);
	}
	/* Scale the amplitude according to position in envelope. */
	/* Attack */
	if(as.s_index < as.atk_samples)
		as.sample *= as.vol * (double)as.s_index / as.atk_samples;
	/* Decay */
	else if(as.s_index < as.dec_samples + as.atk_samples)
		as.sample *= as.sus + (as.vol - as.sus) *
						  (1.0 - (double)(as.s_index - as.atk_samples) /
						  		 (double)as.dec_samples);
	/* Sustain */
	else if(as.s_index < as.atk_samples + as.dec_samples + as.sus_samples)
		as.sample *= as.sus;
	/* Release */
	else
		as.sample *= as.sus * (1.0 - (double)(as.s_index - (as.sus_samples + as.dec_samples + as.atk_samples)) /
								 	 (double)as.rls_samples);

	/* Noise not affected by silly oscillation. */
	if(as.wf == WF_NOISE)
		return (int16_t)as.sample;

	/* Positive or negative? */	
	if((double)(2 * as.s_period_index) < as.s_period_total)
		return (int16_t)-as.sample;
	return (int16_t)as.sample;
}
