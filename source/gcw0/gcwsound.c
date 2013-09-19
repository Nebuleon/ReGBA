#include <SDL/SDL.h>

#include "common.h"
#include "../sound.h"

volatile uint_fast8_t AudioFastForwarded;

#define DAMPEN_SAMPLE_COUNT (AUDIO_OUTPUT_BUFFER_SIZE / 32)

#ifdef SOUND_TO_FILE
FILE* WaveFile;
#endif

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
	uint32_t i, j;
	for (i = 0; i < Requested / 2; i++)
	{
		s16 Left = 0, Right = 0, LeftPart, RightPart;
		for (j = 0; j < OUTPUT_FREQUENCY_DIVISOR; j++) {
			ReGBA_LoadNextAudioSample(&LeftPart, &RightPart);

			/* The GBA outputs in 12-bit sound. Make it louder. */
			if      (LeftPart >  2047) LeftPart =  2047;
			else if (LeftPart < -2048) LeftPart = -2048;
			Left += LeftPart / OUTPUT_FREQUENCY_DIVISOR;

			if      (RightPart >  2047) RightPart =  2047;
			else if (RightPart < -2048) RightPart = -2048;
			Right += RightPart / OUTPUT_FREQUENCY_DIVISOR;
		}

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
		s16 Left = 0, Right = 0, LeftPart, RightPart;
		for (j = 0; j < OUTPUT_FREQUENCY_DIVISOR; j++) {
			ReGBA_LoadNextAudioSample(&LeftPart, &RightPart);

			/* The GBA outputs in 12-bit sound. Make it louder. */
			if      (LeftPart >  2047) LeftPart =  2047;
			else if (LeftPart < -2048) LeftPart = -2048;
			Left += LeftPart / OUTPUT_FREQUENCY_DIVISOR;

			if      (RightPart >  2047) RightPart =  2047;
			else if (RightPart < -2048) RightPart = -2048;
			Right += RightPart / OUTPUT_FREQUENCY_DIVISOR;
		}

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

#ifdef SOUND_TO_FILE
	if (WaveFile != NULL)
	{
		size_t Size = (size_t) Requested * 2 * sizeof(s16);
		if (fwrite(stream, 1, Size, WaveFile) < Size && errno != 0)
		{
			ReGBA_Trace("W: Could not write to the trace wave file: %s", strerror(errno));

			// Go update the rest-of-chunk size field in the trace wave file.
			uint32_t Size = ftell(WaveFile);
			Size -= 8;
			fseek(WaveFile, 4, SEEK_SET);
			fwrite(&Size, 1, sizeof(uint32_t), WaveFile);
			// Also update subchunk 2, 'data'.
			Size -= 36;
			fseek(WaveFile, 40, SEEK_SET);
			fwrite(&Size, 1, sizeof(uint32_t), WaveFile);

			fclose(WaveFile);
			WaveFile = NULL;
		}
	}
#endif
}

#ifdef SOUND_TO_FILE
static uint8_t WaveHeader[] = {
	'R', 'I', 'F', 'F',
	0, 0, 0, 0,                 // Rest-of-chunk size, unknown as of yet
	'W', 'A', 'V', 'E',
	'f', 'm', 't', ' ',         // Subchunk 1 identification
	16, 0, 0, 0,                // Subchunk 1 rest-of-chunk size
	1, 0,                       // PCM
	2, 0,                       // Channel count
	((uint32_t) OUTPUT_SOUND_FREQUENCY) & 0xFF,
	(((uint32_t) OUTPUT_SOUND_FREQUENCY) >> 8) & 0xFF,
	(((uint32_t) OUTPUT_SOUND_FREQUENCY) >> 16) & 0xFF,
	(((uint32_t) OUTPUT_SOUND_FREQUENCY) >> 24) & 0xFF,  // Sample rate
	((uint32_t) OUTPUT_SOUND_FREQUENCY * 2 * sizeof(s16)) & 0xFF,
	(((uint32_t) OUTPUT_SOUND_FREQUENCY * 2 * sizeof(s16)) >> 8) & 0xFF,
	(((uint32_t) OUTPUT_SOUND_FREQUENCY * 2 * sizeof(s16)) >> 16) & 0xFF,
	(((uint32_t) OUTPUT_SOUND_FREQUENCY * 2 * sizeof(s16)) >> 24) & 0xFF,  // Byte rate
	2 * sizeof(s16), 0,         // Bytes per sample for all channels
	8 * sizeof(s16), 0,             // Bits per sample
	'd', 'a', 't', 'a',         // Subchunk 2 identification
	0, 0, 0, 0,                 // Subchunk 2 rest-of-chunk size, unknown as of yet
};
#endif

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

#ifdef SOUND_TO_FILE
	char WaveFileName[MAX_PATH];
	sprintf(WaveFileName, "%s/%s", main_path, "trace.wav");
	WaveFile = fopen(WaveFileName, "wb");
	if (WaveFile == NULL)
	{
		ReGBA_Trace("W: Could not open the trace wave file: %s", strerror(errno));
	}
	else
	{
		if (fwrite(WaveHeader, 1, sizeof(WaveHeader), WaveFile) < sizeof(WaveHeader) && errno != 0)
		{
			ReGBA_Trace("W: Could not write to the trace wave file: %s", strerror(errno));
			fclose(WaveFile);
			WaveFile = NULL;
		}
	}
#endif

	SDL_PauseAudio(0);
}

signed int ReGBA_AudioUpdate()
{
	return 0;
}
