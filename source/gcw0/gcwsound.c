#include <SDL/SDL.h>

#include "common.h"
#include "../sound.h"

void feed_buffer(void *udata, Uint8 *buffer, int len)
{
	s16* stream = (s16*) buffer;
	u32 Samples = ReGBA_GetAudioSamplesAvailable();
	u32 Requested = len / (2 * sizeof(s16));

	if (Samples < Requested)
		return;

	u32 i;
	for (i = 0; i < Requested; i++)
	{
		s16 Left, Right;
		if (ReGBA_LoadNextAudioSample(&Left, &Right))
		{
			/* The GBA outputs in 12-bit sound. Make it louder. */
			stream[i * 2    ] = Left  << 4;
			stream[i * 2 + 1] = Right << 4;
		}
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
