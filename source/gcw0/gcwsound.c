#include <SDL/SDL.h>

#include "common.h"
#include "../sound.h"

void feed_buffer(void *udata, Uint8 *buffer, int len)
{
	s16* stream = (s16*) buffer;
	u32 Samples = ReGBA_GetAudioSamplesAvailable();
	u32 Requested = len / (2 * sizeof(s16));

	/* There must be AUDIO_BUFFER_OUTPUT_SIZE * 2 samples generated in order
	 * for the first AUDIO_BUFFER_OUTPUT_SIZE to be valid. Some sound is
	 * generated in the past from the future, and if the first
	 * AUDIO_BUFFER_OUTPUT_SIZE is grabbed before the core has had time to
	 * generate all of it (at AUDIO_BUFFER_OUTPUT_SIZE * 2), the end may
	 * still be silence, causing crackling. */
	if (Samples < Requested * 2)
		return; // Generate more sound first, please!

	u32 i;
	for (i = 0; i < Requested; i++)
	{
		s16 Left, Right;
		ReGBA_LoadNextAudioSample(&Left, &Right);
		/* The GBA outputs in 12-bit sound. Make it louder. */
		if      (Left >  4095) Left =  4095;
		else if (Left < -4096) Left = -4096;

		if      (Right >  4095) Right =  4095;
		else if (Right < -4096) Right = -4096;

		stream[i * 2    ] = Left  << 3;
		stream[i * 2 + 1] = Right << 3;
	}
}

void init_sdlaudio()
{
	SDL_AudioSpec spec;

	spec.freq = 88200;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = AUDIO_OUTPUT_BUFFER_SIZE;
	spec.callback = feed_buffer;
	spec.userdata = NULL;

	if (SDL_OpenAudio(&spec, NULL) < 0) {
		printf("debug:: failed to open audio: %s\n", SDL_GetError());
		return;
	}

	SDL_PauseAudio(0);
}

signed int ReGBA_AudioUpdate()
{
	return 0;
}
