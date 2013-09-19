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

extern u32 game_fast_forward;
extern u32 temporary_fast_forward;
  
#define DAMPEN_SAMPLE_COUNT  (AUDIO_LEN / 32)

static s16 LastFastForwardedLeft[DAMPEN_SAMPLE_COUNT];
static s16 LastFastForwardedRight[DAMPEN_SAMPLE_COUNT];

// Each pointer should be towards the last N samples output for a channel
// during fast-forwarding. This procedure stores these samples for damping
// the next 8.
void EndFastForwardedSound(s16* left, s16* right)
{
	memcpy(LastFastForwardedLeft,  left,  DAMPEN_SAMPLE_COUNT * sizeof(s16));
	memcpy(LastFastForwardedRight, right, DAMPEN_SAMPLE_COUNT * sizeof(s16));
}

// Each pointer should be towards the first N samples which are about to be
// output for a channel during fast-forwarding. Using the last emitted
// samples, this sound is dampened.
void StartFastForwardedSound(s16* left, s16* right)
{
	// Start by bringing the start sample of each channel much closer
	// to the last sample written, to avoid a loud pop.
	// Subsequent samples are paired with an earlier sample. In a way,
	// this is a form of cross-fading.
	s32 index;
	for (index = 0; index < DAMPEN_SAMPLE_COUNT; index++)
	{
		left[index] = (s16) (
			((LastFastForwardedLeft[DAMPEN_SAMPLE_COUNT - index - 1])
				* (DAMPEN_SAMPLE_COUNT - index) / (DAMPEN_SAMPLE_COUNT + 1))
			+ ((left[index])
				* (index + 1) / (DAMPEN_SAMPLE_COUNT + 1))
			);
		right[index] = (s16) (
			((LastFastForwardedRight[DAMPEN_SAMPLE_COUNT - index - 1])
				* (DAMPEN_SAMPLE_COUNT - index) / (DAMPEN_SAMPLE_COUNT + 1))
			+ ((right[index])
				* (index + 1) / (DAMPEN_SAMPLE_COUNT + 1))
			);
	}
}

signed int ReGBA_AudioUpdate()
{
	u32 i, j;
	s16* audio_buff;
	s16 *dst_ptr, *dst_ptr1;
	u32 n = ds2_checkAudiobuff();
	int ret;

	u8 WasInUnderrun = Stats.InSoundBufferUnderrun;
	Stats.InSoundBufferUnderrun = n == 0 && ReGBA_GetAudioSamplesAvailable() < AUDIO_LEN * 2;
	if (Stats.InSoundBufferUnderrun && !WasInUnderrun)
		Stats.SoundBufferUnderrunCount++;

	// On auto frameskip, sound buffers being full or empty determines
	// whether we're late.
	if (AUTO_SKIP)
	{
		if (n >= 2)
		{
			// We're in no hurry, because 2 buffers are still full.
			// Minimum skip 1
			if(SKIP_RATE > 1)
			{
#if defined TRACE || defined TRACE_FRAMESKIP
				ReGBA_Trace("I: Decreasing automatic frameskip: %u..%u", SKIP_RATE, SKIP_RATE - 1);
#endif
				SKIP_RATE--;
			}
		}
		else
		{
			// Maximum skip 9
			if(SKIP_RATE < 8)
			{
#if defined TRACE || defined TRACE_FRAMESKIP
				ReGBA_Trace("I: Increasing automatic frameskip: %u..%u", SKIP_RATE, SKIP_RATE + 1);
#endif
				SKIP_RATE++;
			}
		}
	}

	/* There must be AUDIO_LEN * 2 samples generated in order for the first
	 * AUDIO_LEN to be valid. Some sound is generated in the past from the
	 * future, and if the first AUDIO_LEN is grabbed before the core has had
	 * time to generate all of it (at AUDIO_LEN * 2), the end may still be
	 * silence, causing crackling. */
	if (ReGBA_GetAudioSamplesAvailable() < AUDIO_LEN * 2)
	{
		// Generate more sound first, please!
		return -1;
	}

	// We have enough sound. Complete this update.
	if (game_fast_forward || temporary_fast_forward)
	{
		if (n >= AUDIO_BUFFER_COUNT)
		{
			// Drain the buffer down to a manageable size, then exit.
			// This needs to be high to avoid audible crackling/bubbling,
			// but not so high as to require all of the sound to be emitted.
			// gpSP synchronises on the sound, after all. -Neb, 2013-03-23
			ReGBA_DiscardAudioSamples(ReGBA_GetAudioSamplesAvailable() - AUDIO_LEN);
			return 0;
		}
	}
	else
	{
		// Wait for at least one buffer to be free for audio.
		// Output assertion: The return value is between 0, inclusive,
		// and AUDIO_BUFFER_COUNT, inclusive, but can also be
		// 4294967295; that's (unsigned int) -1. (DSTWO SPECIFIC HACK)
		unsigned int n2;
		while ((n2 = ds2_checkAudiobuff()) >= AUDIO_BUFFER_COUNT && (int) n2 >= 0);
	}

	audio_buff = ds2_getAudiobuff();
	if (audio_buff == NULL) {
#if defined TRACE || defined TRACE_SOUND
		ReGBA_Trace("Recovered from the lack of a buffer");
#endif
		return -1;
	}

	dst_ptr = audio_buff; // left (stereo)
	dst_ptr1 = dst_ptr + (int) (AUDIO_LEN / OUTPUT_FREQUENCY_DIVISOR); // right (stereo)

	for(i= 0; i < AUDIO_LEN; i += OUTPUT_FREQUENCY_DIVISOR)
	{
		s16 Left = 0, Right = 0, LeftPart, RightPart;
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
		*dst_ptr1++ = Right << 4;
	}

	if (game_fast_forward || temporary_fast_forward)
	{
		// Dampen the sound with the previous samples written
		// (or unitialised data if we just started the emulator)
		StartFastForwardedSound(audio_buff,
			&audio_buff[(int) (AUDIO_LEN / OUTPUT_FREQUENCY_DIVISOR)]);
		// Store the end for the next time
		EndFastForwardedSound(&audio_buff[(int) (AUDIO_LEN / OUTPUT_FREQUENCY_DIVISOR) - DAMPEN_SAMPLE_COUNT],
			&audio_buff[(int) (AUDIO_LEN / OUTPUT_FREQUENCY_DIVISOR) * 2 - DAMPEN_SAMPLE_COUNT]);
	}

	Stats.InSoundBufferUnderrun = 0;
	ds2_updateAudio();
	return 0;
}
