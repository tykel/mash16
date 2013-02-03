#include "audio.h"
#include <SDL/SDL.h>

static int cur_sample = 0;
static int total_samples = 0;
static int spos = 0;
static audio_state astate;

typedef int16_t (*fptr)();

/* Initialise the SDL audio system. */
void audio_init(cpu_state *state)
{
	astate = (audio_state){0};
	astate.wf = WF_TRIANGLE;
	astate.f = 100;
	astate.vol = INT16_MAX / 2;

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
	SDL_PauseAudio(1);
	SDL_CloseAudio();
}

/* (Re)populate the audio buffer and start playing. */
void audio_play(int16_t f, int16_t dt, int adsr)
{
	/* Pause while we set up parameters. */
	SDL_PauseAudio(1);
	/* Set things up. */
	astate.f = f;
	astate.dt = dt;
	astate.use_envelope = adsr;
	/* Number of samples for the whole sound. */
	total_samples = (int)dt * AUDIO_RATE/1000;
	/* Number of sampls for the sustain period. */
	astate.sus_samples = total_samples - astate.atk_samples
									   - astate.dec_samples
									   - astate.rls_samples;
	if(astate.sus_samples < 0)
		astate.sus_samples = 0;
	printf("A=%ds\tD=%ds\tS=%ds\tR=%ds\tTOTAL=%d\n",
		astate.atk_samples,astate.dec_samples,astate.sus_samples,astate.rls_samples,total_samples);
	cur_sample = 0;
	/* Unset pause, start playing. */
	SDL_PauseAudio(0);
}

/* Stop playing audio. */
void audio_stop()
{
	SDL_PauseAudio(1);
}

void audio_update(cpu_state *state)
{
	/* Copy over the envelope parameters. */
	astate.wf = (waveform)state->type;
	astate.atk = atk_ms[state->atk];
	astate.dec = dec_ms[state->dec];
	astate.sus = INT16_MAX / (2 * (16 - state->sus));
	astate.rls = rls_ms[state->rls];
	astate.vol = INT16_MAX / (2 * (16 - state->vol));
	astate.tone = state->tone;
	/* Get number of samples from duration for the perods. */
	astate.atk_samples = (AUDIO_RATE * astate.atk) / 1000;
	astate.dec_samples = (AUDIO_RATE * astate.dec) / 1000;
	astate.rls_samples = (AUDIO_RATE * astate.rls) / 1000;
}

/* Callback function provided to SDL for copying sound data. */
void audio_callback(void* data, uint8_t* stream, int len)
{
	if(cur_sample >= total_samples)
		return;

	fptr f_sample = NULL;
	if(astate.use_envelope)
		f_sample = &audio_gen_sample;
	else if(astate.f == 500)
		f_sample = &audio_gen_snd1_sample;
	else if(astate.f == 1000)
		f_sample = &audio_gen_snd2_sample;
	else if(astate.f == 1500)
		f_sample = &audio_gen_snd3_sample;

	int16_t *buffer = (int16_t*)stream;
	/* We are dealing with 16 bit samples. */
	len /= 2;

	for(int i=0; i<len; ++i, ++cur_sample)
		buffer[i] = (*f_sample)();
}

/* Simple square tone generators. */
int16_t audio_gen_snd1_sample()
{
	++spos;
	if(spos >= SND1_SAMPLES)
		spos = 0;
	if(2*spos < SND1_SAMPLES)
		return -astate.vol;
	return astate.vol;
}

int16_t audio_gen_snd2_sample()
{
	++spos;
	if(spos >= SND2_SAMPLES)
		spos = 0;
	if(2*spos < SND2_SAMPLES)
		return -astate.vol;
	return astate.vol;
}

int16_t audio_gen_snd3_sample()
{
	++spos;
	if(spos >= SND3_SAMPLES)
		spos = 0;
	if(2*spos < SND3_SAMPLES)
		return -astate.vol;
	return astate.vol;
}

/* Our more advanced envelope generators. */
int16_t audio_gen_sample()
{
	++spos;
	/* Number of samples for oscillation period at given frequency. */
	double samples = (double)AUDIO_RATE / (double)(astate.f + 1);
	if((double)spos >= samples)
		spos = 0;
	/* Our output sample. */
	double s = 0.0;
	switch(astate.wf)
	{
		case WF_TRIANGLE:
			s = 2.0 * (double)spos / (double)samples - 1.0;
			break;
		case WF_SAWTOOTH:
			if(4*spos < samples) 
				s = (double)(4*spos)/(double)(samples);
			else
				s = (double)(samples)/(double)(4*spos);
			break;
		case WF_PULSE:
			s = 1.0; 
			break;
		case WF_NOISE:
			s = (double)rand() / (double)RAND_MAX;
			break;
		default:
			fprintf(stderr, "error: invalid ADSR envelope type (%d)", astate.wf);
			exit(1);
	}
	/* Scale the amplitude according to position in envelope. */
	if(cur_sample < astate.atk_samples)
		s *= astate.vol * (double)cur_sample / astate.atk_samples;
	else if(cur_sample < astate.dec_samples + astate.atk_samples)
		s *= astate.sus + (astate.vol - astate.sus)*(astate.atk_samples - cur_sample)/(astate.dec_samples);
	else if(cur_sample < astate.atk_samples + astate.dec_samples + astate.sus_samples)
		s *= astate.sus;
	else
		s *= astate.sus * (1.0 - (double)(cur_sample - astate.sus_samples - astate.dec_samples - astate.atk_samples) /
												(double)astate.rls_samples);

	/* Positive or negative? */
	if((double)(2 * spos) < samples && astate.wf != WF_NOISE)
		return (int16_t)-s;
	return (int16_t)s;
}
