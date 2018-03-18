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
#include <stdio.h>
#include <SDL2/SDL.h>

static audio_state as;
static int use_audio;
extern int use_verbose;

double triangle_lerp_table[4][2] = {
    { 0.0,  1.0},
    { 1.0,  0.0},
    { 0.0, -1.0},
    {-1.0,  0.0},
};

double sawtooth_lerp_table[4][2] = {
    { 0.0,  0.5},
    { 0.5,  1.0},
    {-1.0, -0.5},
    {-0.5,  0.0},
};

double pulse_lerp_table[4][2] = {
    { 1.0,  1.0},
    { 1.0,  1.0},
    { 0.0,  0.0},
    { 0.0,  0.0},
};

double noise_table[1] = {
    1.0,
};

double lerp(double v1, double v2, double w)
{
    return w * v1 + (1.0 - w) * v2;
}

/* Initialise the SDL audio system. */
void audio_init(cpu_state *state, program_opts *opts)
{
	SDL_AudioSpec spec;
    SDL_AudioSpec actual_spec;

	memset(&as,0,sizeof(audio_state));
	as.wf = WF_TRIANGLE;
	as.f = 100;
	as.max_vol = opts->audio_volume << 7;
	as.vol = as.max_vol;
	as.buffer_size = opts->audio_buffer_size;
	as.sample_rate = opts->audio_sample_rate;
	use_audio = opts->use_audio;
	if(!use_audio)
		return;

	spec.freq = as.sample_rate;
	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	spec.samples = (uint16_t)as.buffer_size;
	spec.callback = audio_callback;
	spec.userdata = state;

	if(SDL_OpenAudio(&spec, &actual_spec) < 0)
	{
		fprintf(stderr,"error: could not open audio: %s",SDL_GetError());
		exit(1);
	}
    if (use_verbose) {
        printf("audio: reqstd spec: freq = %d Hz, samples = %d, format = %d\n"
               "audio: actual spec: freq = %d Hz, samples = %d, format = %d\n",
               spec.freq, spec.samples, spec.format,
               actual_spec.freq, actual_spec.samples, actual_spec.format);
    }
}

/* Clean up after the sound system. */
void audio_free()
{
	if(use_audio)
	{
		SDL_LockAudio();
		SDL_CloseAudio();
		SDL_UnlockAudio();
	}
}

/* (Re)populate the audio buffer and start playing. */
void audio_play(int16_t f, int16_t dt, int adsr)
{
	if(!use_audio)
		return;
	/* Pause while we set up parameters. */
	SDL_PauseAudio(1);
	/* Set things up. */
	as.f = f;
	as.dt = dt;
	as.use_envelope = adsr;
    if (!adsr) {
        as.wf = WF_PULSE;
    }
	/* Reset the sample index so we start at the beginning of the buffer. */
	as.s_index = 0;
	/* Number of samples for the whole sound. */
	as.s_total = (dt * as.sample_rate/1000) + as.rls_samples;
	/* Number of samples for an oscillation period. */
	as.s_period_total = as.sample_rate / (as.f + 1);
	/* Check other numbers of samples are correct. */
	if(as.dt * as.sample_rate/1000 < as.atk_samples)
	{
		as.atk_samples = as.dt * as.sample_rate/1000;
		as.dec_samples = 0;
	}
	else if(dt * as.sample_rate/1000 + as.atk_samples < as.dec_samples)
	{
		as.dec_samples = dt * as.sample_rate/1000 + as.atk_samples;	
	}
	/* Number of samples for the sustain duration. */
	as.sus_samples = (dt * as.sample_rate/1000) - as.atk_samples
												- as.dec_samples;
	if(as.sus_samples < 0)
		as.sus_samples = 0;

	/* Log if necessary. */
	if(use_verbose)
	{
		printf("audio: A=%ds\tD=%ds\tS=%ds\tTOTAL=%ds\tR=%ds\n",
				as.atk_samples,as.dec_samples,as.sus_samples,as.s_total,as.rls_samples);
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
	as.wf = (waveform_t)state->type;
	as.atk = atk_ms[state->atk];
	as.dec = dec_ms[state->dec];
	as.sus = as.max_vol / (2*(16 - state->sus));
	as.rls = rls_ms[state->rls];
	as.vol = as.max_vol / (2*(16 - state->vol));
	as.tone = state->tone;
	/* Get number of as.s_period_total from duration for the perods. */
	as.atk_samples = (as.sample_rate * as.atk) / 1000;
	as.dec_samples = (as.sample_rate * as.dec) / 1000;
	as.rls_samples = (as.sample_rate * as.rls) / 1000;
}

/* Callback function provided to SDL for copying sound data. */
void audio_callback(void* data, uint8_t* stream, int len)
{
    int ns;
    int16_t *buffer;

    if(as.s_index >= as.s_total) {
        memset(stream, 0, len);
		return;
    }

	buffer = (int16_t*)stream;
	/* We are dealing with 16 bit as.s_period_total. */
	ns = len / 2;

    audio_gen_samples(buffer, ns); 
}

/* Our more advanced envelope generators. */
void audio_gen_samples(int16_t *buffer, int ns)
{
    double sample;
    double v1, v2, w;
    int i, s, t;
    int16_t *b = buffer;

    for (s = 0; s < ns; s++) {
        ++as.s_period_index;
        if(as.s_period_index >= as.s_period_total)
            as.s_period_index = 0;

        i = as.s_period_index;
        t = as.s_period_total;
        switch(as.wf)
        {
            case WF_TRIANGLE:
                v1 = triangle_lerp_table[4*i/t][0];
                v2 = triangle_lerp_table[4*i/t][1];
                w  = (double)(i % (t/4)) / (t/4);
                sample = lerp(v1, v2, w);
                break;
            case WF_SAWTOOTH:
                v1 = sawtooth_lerp_table[4*i/t][0];
                v2 = sawtooth_lerp_table[4*i/t][1];
                w  = (double)(i % (t/4)) / (t/4);
                sample = lerp(v1, v2, w);
                break;
            case WF_PULSE:
                v1 = pulse_lerp_table[4*i/t][0];
                v2 = pulse_lerp_table[4*i/t][1];
                w  = (double)(i % (t/4)) / (t/4);
                sample = lerp(v1, v2, w);
                break;
            case WF_NOISE:
                if(i == 0)
                    noise_table[0] =
                        (double)(rand() % (2*as.max_vol))/(double)as.max_vol - 1.0;
                sample = noise_table[0];
                break;
            default:
                fprintf(stderr, "error: invalid ADSR envelope type (%d)", as.wf);
                exit(1);
        }

        if (as.use_envelope)
            /* Scale the amplitude according to position in envelope. */
            as.sample = (int16_t) audio_apply_adsr(sample);
        else
            as.sample = (int16_t) (sample * as.vol);

        *b++ = as.sample;
        as.s_index++;
    }
}

/*      ________ _____ Vol
 *     /\
 *    /  \______ _____ Sus Vol
 *   /          \
 *  /            \
 * /              \
 * +---+-+-----+--+
 * | A |D|  S  |R |
 *
 */
double audio_apply_adsr(double sample)
{
	/* Attack */
	if(as.s_index < as.atk_samples)
		sample *= as.vol * (double)as.s_index / as.atk_samples;
	/* Decay */
	else if(as.s_index < as.dec_samples + as.atk_samples)
		sample *= as.sus + (as.vol - as.sus) *
						  (1.0 - (double)(as.s_index - as.atk_samples) /
						  		 (double)as.dec_samples);
	/* Sustain */
	else if(as.s_index < as.atk_samples + as.dec_samples + as.sus_samples)
		sample *= as.sus;
	/* Release */
	else
		sample *= as.sus * (1.0 - (double)(as.s_index - (as.sus_samples + as.dec_samples + as.atk_samples)) /
								 	 (double)as.rls_samples);

    return sample;
}

