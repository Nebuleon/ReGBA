#include <SDL/SDL.h>

#include "common.h"
#include "../sound.h"

volatile uint_fast8_t AudioFastForwarded;

#define DAMPEN_SAMPLE_COUNT (AUDIO_OUTPUT_BUFFER_SIZE / 32)

#ifdef SOUND_TO_FILE
FILE* WaveFile;
#endif

static inline void RenderSample(int16_t* Left, int16_t* Right)
{
	int16_t LeftPart, RightPart;
	uint32_t j;
	for (j = 0; j < OUTPUT_FREQUENCY_DIVISOR; j++) {
		ReGBA_LoadNextAudioSample(&LeftPart, &RightPart);

		/* The GBA outputs in 12-bit sound. Make it louder. */
		if      (LeftPart >  2047) LeftPart =  2047;
		else if (LeftPart < -2048) LeftPart = -2048;
		*Left += LeftPart / OUTPUT_FREQUENCY_DIVISOR;

		if      (RightPart >  2047) RightPart =  2047;
		else if (RightPart < -2048) RightPart = -2048;
		*Right += RightPart / OUTPUT_FREQUENCY_DIVISOR;
	}
}

void feed_buffer(void *udata, Uint8 *buffer, int len)
{
	s16* stream = (s16*) buffer;
	u32 Samples = ReGBA_GetAudioSamplesAvailable() / OUTPUT_FREQUENCY_DIVISOR;
	u32 Requested = len / (2 * sizeof(s16));

	/* There must be AUDIO_OUTPUT_BUFFER_SIZE * 2 samples generated in order
	 * for the first AUDIO_OUTPUT_BUFFER_SIZE to be valid. Some sound is
	 * generated in the past from the future, and if the first
	 * AUDIO_OUTPUT_BUFFER_SIZE is grabbed before the core has had time to
	 * generate all of it (at AUDIO_OUTPUT_BUFFER_SIZE * 2), the end may
	 * still be silence, causing crackling. */
	if (Samples < Requested * 2)
		return; // Generate more sound first, please!
		
	s16* Next = stream;

	// Take the first half of the sound.
	uint32_t i;
	for (i = 0; i < Requested / 2; i++)
	{
		s16 Left = 0, Right = 0;
		RenderSample(&Left, &Right);

		*Next++ = Left  << 4;
		*Next++ = Right << 4;
	}
	Samples -= Requested / 2;
	
	// Discard sound until half the requested length is left, if fast-forwarding.
	bool Skipped = false;
	uint_fast8_t VideoFastForwardedCopy = VideoFastForwarded;
	if (VideoFastForwardedCopy != AudioFastForwarded)
	{
		uint32_t SamplesToSkip = Samples - (Requested * 3 - Requested / 2);
		ReGBA_DiscardAudioSamples(SamplesToSkip * OUTPUT_FREQUENCY_DIVISOR);
		Samples -= SamplesToSkip;
		AudioFastForwarded = VideoFastForwardedCopy;
		Skipped = true;
	}

	// Take the second half of the sound now.
	for (i = 0; i < Requested - Requested / 2; i++)
	{
		s16 Left = 0, Right = 0;
		RenderSample(&Left, &Right);

		*Next++ = Left  << 4;
		*Next++ = Right << 4;
	}
	Samples -= Requested - Requested / 2;

	// If we skipped sound, dampen the transition between the two halves.
	if (Skipped)
	{
		for (i = 0; i < DAMPEN_SAMPLE_COUNT; i++)
		{
			uint_fast8_t j;
			for (j = 0; j < 2; j++)
			{
				stream[Requested / 2 + i * 2 + j] = (int16_t) (
					  (int32_t) stream[Requested / 2 - i * 2 - 2 + j] * (DAMPEN_SAMPLE_COUNT - (int32_t) i) / (DAMPEN_SAMPLE_COUNT + 1)
					+ (int32_t) stream[Requested / 2 + i * 2 + j] * ((int32_t) i + 1) / (DAMPEN_SAMPLE_COUNT + 1)
					);
			}
		}
	}
}

void init_sdlaudio()
{
	SDL_AudioSpec spec;

	spec.freq = OUTPUT_SOUND_FREQUENCY;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = AUDIO_OUTPUT_BUFFER_SIZE;
	spec.callback = feed_buffer;
	spec.userdata = NULL;

	if (SDL_OpenAudio(&spec, NULL) < 0) {
		ReGBA_Trace("E: Failed to open audio: %s", SDL_GetError());
		return;
	}

	SDL_PauseAudio(0);
}

signed int ReGBA_AudioUpdate()
{
	return 0;
}
