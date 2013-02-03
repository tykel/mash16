#include "audio.h"
#include <SDL/SDL.h>

static int t = 0;
static int spos = 0;
static uint8_t *buffer = NULL;
static audio_state astate;

typedef int16_t (*fptr)();

/* Initialise the SDL audio system. */
void audio_init(cpu_state *state)
{
	astate = (audio_state){0};
	astate.wf = WF_TRIANGLE;
	astate.f = 100;
	astate.vol = INT16_MAX / 2;

	uint8_t *audio_data = NULL;
	if(!(audio_data = malloc(AUDIO_SAMPLES)))
	{
		fprintf(stderr, "error: malloc failed (audio_data)\n");
		exit(1);
	}

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
    SDL_AudioQuit();
	free(buffer);
}

/* (Re)populate the audio buffer and start playing. */
void audio_play(int16_t f, int16_t dt)
{
	/* Pause while we set up parameters. */
	SDL_PauseAudio(1);
	/* Set things up. */
	astate.f = f;
	astate.dt = dt;
	t = (int)dt * AUDIO_RATE/1000;
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
	astate.wf = (waveform)state->type;
	astate.atk = atk_ms[state->atk];
	astate.dec = dec_ms[state->dec];
	astate.sus = INT16_MAX / (2 * (16 - state->sus));
	astate.rls = rls_ms[state->rls];
	astate.vol = INT16_MAX / (2 * (16 - state->vol));
	astate.tone = state->tone;
}

/* Callback function provided to SDL for copying sound data. */
void audio_callback(void* data, uint8_t* stream, int len)
{
	if(t <= 0)
		return;	

	fptr f_sample = NULL;
	if(astate.f == 500)
		f_sample = &audio_gen_snd1_sample;
	else if(astate.f == 1000)
		f_sample = &audio_gen_snd2_sample;
	else if(astate.f == 1500)
		f_sample = &audio_gen_snd3_sample;
	else
		f_sample = &audio_gen_sample;

	int16_t *buffer = (int16_t*)stream;
	/* We are dealing with 16 bit samples. */
	len /= 2;
	for(int i=0; i<len; ++i, --t)
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
	int samples = AUDIO_RATE / (astate.f + 1);
	if(spos >= samples)
		spos = 0;
	double s = 0.0;
	/* Get number of samples from duration. */
	int atk = (AUDIO_RATE * astate.atk) / 1000;
	int dec = (AUDIO_RATE * astate.dec) / 1000;
	int dt = (AUDIO_RATE * astate.dt) / 1000;
	int rls = (AUDIO_RATE * astate.rls) / 1000;
	switch(astate.wf)
	{
		case WF_TRIANGLE:
			s = 2*(double)spos/(double)samples - 1.0;
			break;
		case WF_SAWTOOTH:
			/* Triangle centered around (samples/2). */
			if(4*spos < samples) 
				s = (double)(4*spos)/(double)(samples);
			else
				s = (double)(samples)/(double)(4*spos);
			break;
		case WF_PULSE:
			s = 1.0; 
			break;
		case WF_NOISE:
			return rand();
		default:
			fprintf(stderr, "error: invalid ADSR envelope type (%d)", astate.wf);
			exit(1);
	}
	/* Scale the amplitude according to position in envelope. */
	/*if(t < atk)
		s *= astate.vol * (double)t / atk;
	else if(t < dec + atk)
		s *= astate.sus + (astate.vol - astate.sus)*(t - atk);
	else if(t < dt - rls)
		s *= astate.sus;
	else
		s *= astate.sus * (double)rls / (t - dt - dec - atk);*/
	
	s *= astate.vol;

	/* Positive or negative? */
	if(2 * spos < samples)
		return (int16_t)-s;
	return (int16_t)s;
}
