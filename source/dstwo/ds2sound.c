/* ReGBA DSTwo - Audio output
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 * Copyright (C) 2013 GBAtemp user Nebuleon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"

extern uint32_t game_fast_forward;
extern uint32_t temporary_fast_forward;

#define DAMPEN_SAMPLE_COUNT  (OUTPUT_SOUND_LEN / 32 - 1)

/* The last emitted audio samples, to be used for cross-fading between skipped
 * segments while fast-forwarding. The left and right channels are interleaved
 * with the left channel first. */
static int16_t LastFastForwarded[DAMPEN_SAMPLE_COUNT * 2];

// The pointer should be towards the last N samples output during
// fast-forwarding, interleaved. This procedure stores these samples for
// damping the next 8.
void EndFastForwardedSound(const int16_t* samples)
{
	memcpy(LastFastForwarded, samples, DAMPEN_SAMPLE_COUNT * 2 * sizeof(int16_t));
}

// The pointer should be towards the first N samples which are about to be
// output during fast-forwarding, interleaved. Using the last emitted samples,
// this sound is dampened.
void StartFastForwardedSound(int16_t* samples)
{
	// Start by bringing the start sample of each channel much closer
	// to the last sample written, to avoid a loud pop.
	// Subsequent samples are paired with an earlier sample. In a way,
	// this is a form of cross-fading.
	size_t index;
	for (index = 0; index < DAMPEN_SAMPLE_COUNT; index++)
	{
		samples[index * 2] = (int16_t) (
			((int32_t) LastFastForwarded[(DAMPEN_SAMPLE_COUNT - index) * 2 - 2]
				* (int32_t) (DAMPEN_SAMPLE_COUNT - index) / (DAMPEN_SAMPLE_COUNT + 1))
			+ (int32_t) (samples[index * 2]
				* (int32_t) (index + 1) / (DAMPEN_SAMPLE_COUNT + 1))
			);
		samples[index * 2 + 1] = (int16_t) (
			((int32_t) LastFastForwarded[(DAMPEN_SAMPLE_COUNT - index) * 2 - 1]
				* (int32_t) (DAMPEN_SAMPLE_COUNT - index) / (DAMPEN_SAMPLE_COUNT + 1))
			+ (int32_t) (samples[index * 2 + 1]
				* (int32_t) (index + 1) / (DAMPEN_SAMPLE_COUNT + 1))
			);
	}
}

signed int ReGBA_AudioUpdate()
{
	uint32_t i, j;
	int16_t* audio_buff;
	int16_t* dst_ptr;
	size_t out_free = DS2_GetFreeAudioSamples(),
	       in_avail = ReGBA_GetAudioSamplesAvailable(),
	       out_count;

	uint_fast8_t WasInUnderrun = Stats.InSoundBufferUnderrun;
	Stats.InSoundBufferUnderrun = out_free == OUTPUT_SOUND_LEN;
	if (Stats.InSoundBufferUnderrun && !WasInUnderrun)
		Stats.SoundBufferUnderrunCount++;

	// On auto frameskip, sound buffers being full or empty determines
	// whether we're late.
	if (AUTO_SKIP) {
		if (out_free <= OUTPUT_SOUND_LEN - OUTPUT_SOUND_HIGH_LEN) {
			// We're in no hurry, because the buffer is still full enough.
			// Minimum skip 1
			if(SKIP_RATE > 1) {
#if defined TRACE || defined TRACE_FRAMESKIP
				ReGBA_Trace("I: Decreasing automatic frameskip: %u..%u", SKIP_RATE, SKIP_RATE - 1);
#endif
				SKIP_RATE--;
			}
		} else if (out_free >= OUTPUT_SOUND_LEN - OUTPUT_SOUND_LOW_LEN) {
			// Maximum skip 9
			if(SKIP_RATE < 8) {
#if defined TRACE || defined TRACE_FRAMESKIP
				ReGBA_Trace("I: Increasing automatic frameskip: %u..%u", SKIP_RATE, SKIP_RATE + 1);
#endif
				SKIP_RATE++;
			}
		}
	}

	if (game_fast_forward || temporary_fast_forward) {
		if (out_free <= OUTPUT_SOUND_LEN - OUTPUT_SOUND_FFWD_LEN) {
			// Drain the buffer down to a manageable size, then exit.
			// This needs to be high to avoid audible crackling/bubbling,
			// but not so high as to require all of the sound to be emitted.
			// gpSP synchronises on the sound, after all. -Neb, 2013-03-23
			if (in_avail > 1280 + (OUTPUT_SOUND_LEN - OUTPUT_SOUND_FFWD_LEN) * OUTPUT_FREQUENCY_DIVISOR)
				ReGBA_DiscardAudioSamples(in_avail - (1280 + (OUTPUT_SOUND_LEN - OUTPUT_SOUND_FFWD_LEN) * OUTPUT_FREQUENCY_DIVISOR));
			return 0;
		} else {
			// We will emit audio. However, ensure that we don't wait in
			// DS2_SubmitAudio; drop samples that we have no room for.
			if (in_avail > 1280 + out_free * OUTPUT_FREQUENCY_DIVISOR) {
				ReGBA_DiscardAudioSamples(in_avail - (1280 + out_free * OUTPUT_FREQUENCY_DIVISOR));
				in_avail = ReGBA_GetAudioSamplesAvailable();
			}
		}
	}

	/* Owing to synchronisation problems between Direct Sound and the GBC
	 * beeper sound, the ReGBA core may have up to 1280/88100 of a second
	 * of silence that is filled later. Don't grab that sound until ReGBA
	 * actually fills it, otherwise crackling will result. */
	if (in_avail < 1280 + SOUND_SEND_LEN * OUTPUT_FREQUENCY_DIVISOR) {
		// Generate more sound first, please!
		return -1;
	}
	out_count = (in_avail - 1280) / OUTPUT_FREQUENCY_DIVISOR;

	audio_buff = malloc(out_count * 2 * sizeof(int16_t));
	dst_ptr = audio_buff;

	for (i = 0; i < out_count; i++) {
		int16_t Left = 0, Right = 0, LeftPart, RightPart;
		for (j = 0; j < OUTPUT_FREQUENCY_DIVISOR; j++) {
			ReGBA_LoadNextAudioSample(&LeftPart, &RightPart);

			if      (LeftPart >  2047) LeftPart =  2047;
			else if (LeftPart < -2048) LeftPart = -2048;
			Left += LeftPart / OUTPUT_FREQUENCY_DIVISOR;

			if      (RightPart >  2047) RightPart =  2047;
			else if (RightPart < -2048) RightPart = -2048;
			Right += RightPart / OUTPUT_FREQUENCY_DIVISOR;
		}
		*dst_ptr++ = Left << 4;
		*dst_ptr++ = Right << 4;
	}

	if ((game_fast_forward || temporary_fast_forward) && out_count >= DAMPEN_SAMPLE_COUNT * 2) {
		// Dampen the sound with the previous samples written
		// (or silence if we just started the emulator)
		StartFastForwardedSound(audio_buff);
		// Store the end for the next time
		EndFastForwardedSound(&audio_buff[(out_count - DAMPEN_SAMPLE_COUNT) * 2]);
	}

	Stats.InSoundBufferUnderrun = 0;
	/* This submission of audio will pause with power saving until all the
	 * samples are sent. This is how the DSTwo port synchronises on audio. */
	DS2_SubmitAudio(audio_buff, out_count);
	free(audio_buff);
	return 0;
}
