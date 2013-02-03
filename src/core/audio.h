#ifndef AUDIO_H
#define AUDIO_H

#include "cpu.h"

/* Tweak these if sound pops or is delayed. */
#define AUDIO_RATE		 	44100
#define AUDIO_SAMPLES		512

#define SND1_SAMPLES		(int)(AUDIO_RATE / 500)
#define SND2_SAMPLES		(int)(AUDIO_RATE / 1000)
#define SND3_SAMPLES		(int)(AUDIO_RATE / 1500)

/* Data types and structures. */
typedef enum 
{  
	WF_TRIANGLE = 0,
	WF_SAWTOOTH,
	WF_PULSE,
	WF_NOISE
} waveform;

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

/* Tracks how the sound should be generated. */
typedef struct
{
	int f;
	int dt;
    int tone;
    int atk, atk_samples;
    int dec, dec_samples;
    int sus, sus_samples;
    int rls, rls_samples;
    int vol;
	waveform wf;
	int use_envelope;

} audio_state;

/* Function declarations. */
void audio_init();
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