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

	uint8_t *audio_data = NULL;
	if(!(audio_data = malloc(AUDIO_SAMPLES)))
	{
		fprintf(stderr, "error: malloc failed (audio_data)\n");
		exit(1);
	}

	SDL_AudioSpec spec;
	spec.freq = AUDIO_FREQUENCY;
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

void audio_free()
{
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	free(buffer);
}

void audio_update(cpu_state *state)
{
	astate.wf = (waveform)state->type;
	astate.atk = atk_ms[state->atk];
	astate.dec = dec_ms[state->dec];
	astate.sus = INT16_MAX / state->sus;
	astate.rls = rls_ms[state->rls];
	astate.vol = INT16_MAX / 2;
	astate.tone = state->tone;
}

/* Callback function provided to SDL for copying sound data. */
void audio_callback(void* data, uint8_t* stream, int len)
{
	t = SDL_GetTicks();
	if(t > astate.dt)
	{
		return;	
	}
	cpu_state *state = (cpu_state*)data;
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
	len /= 2;
	for(int i=0; i<len; ++i)
	{
		buffer[i] = (*f_sample)();
	}
}

/* (Re)populate the audio buffer and start playing. */
void audio_play(int16_t f, int16_t dt)
{
	/* Pause while we set up parameters. */
	SDL_PauseAudio(1);
	/* Set things up. */
	astate.f = f;
	astate.dt = dt;
	/* Unset pause, start playing. */
	SDL_PauseAudio(0);
}

/* Stop playing audio. */
void audio_stop()
{
	SDL_PauseAudio(1);
}

int16_t audio_gen_snd1_sample()
{
	++spos;
	if(spos >= SND1_SAMPLES)
		spos = 0;
	if(2*spos < SND1_SAMPLES)
		return INT16_MIN;
	return INT16_MAX;
}

int16_t audio_gen_snd2_sample()
{
	++spos;
	if(spos >= SND2_SAMPLES)
		spos = 0;
	if(2*spos < SND2_SAMPLES)
		return INT16_MIN;
	return INT16_MAX;
}

int16_t audio_gen_snd3_sample()
{
	++spos;
	if(spos >= SND3_SAMPLES)
		spos = 0;
	if(2*spos < SND3_SAMPLES)
		return INT16_MIN;
	return INT16_MAX;
}

int16_t audio_gen_sample()
{
	++spos;
	switch(astate.wf)
	{
		case WF_TRIANGLE:
		case WF_SAWTOOTH:
		case WF_PULSE:
			break;
		case WF_NOISE:
			return rand();
		default:
			break;
	}
	return 0;
}
