/*
 * Not bothering to copyright this header, so there you go, it's now in the
 * public domain. Deal with it.
 * -- Nebuleon, 2013-04-02
 */

#ifndef __GPSP_STATS_H__
#define __GPSP_STATS_H__

#include "common.h"

struct GPSP_STATS {
	/* For FPS display. The first 3 variables are updated by the
	 * emulation and read by StatsDisplayFPS. The last 2 variables are
	 * updated and read by StatsDisplayFPS. */
	u32	RenderedFrames;
	u32	EmulatedFrames;
	u32	LastFPSCalculationTime;
	s32	RenderedFPS;
	s32	EmulatedFPS;

	/* Performance statistics collectors. This set does not impact
	 * performance of the emulator much. */
	/* How many bytes of MIPS code have we discarded since the current
	 * game started running? */
	u32	ROMTranslationBytesFlushed;
	u32	RAMTranslationBytesFlushed;
	u32	BIOSTranslationBytesFlushed;
	/* How many times have we had to discard MIPS code since the current
	 * game started running? */
	u32	ROMTranslationFlushCount;
	u32	RAMTranslationFlushCount;
	u32	BIOSTranslationFlushCount;
	/* How many times have we detected an underrun of the sound buffer? */
	u32	SoundBufferUnderrunCount;
	/* How many frames have we emulated since the current game started
	 * running? */
	u32	TotalEmulatedFrames;

#ifdef PERFORMANCE_IMPACTING_STATISTICS
	/* Performance statistics collectors. This set impacts normal
	 * performance of the emulator. */
	/* Up to how many bytes of MIPS code must we hold for this game? */
	u32	ROMTranslationBytesPeak;
	u32	RAMTranslationBytesPeak;
	u32	BIOSTranslationBytesPeak;
	/* How many times have we had to decode an ARM or a Thumb opcode from
	 * scratch since the current game started running? */
	u32	ARMOpcodesDecoded;
	u32	ThumbOpcodesDecoded;
#endif

	/* Are we in a sound buffer underrun? If we are, ignore underrunning
	 * until the current underrun is done. */
	u8	InSoundBufferUnderrun;
};

extern struct GPSP_STATS Stats;

extern void StatsInit(void);
extern void StatsDisplayFPS(void);

#endif // !__GPSP_STATS_H__
