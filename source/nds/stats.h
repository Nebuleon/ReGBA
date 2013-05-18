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

#ifndef USE_C_CORE
	/* Performance statistics collectors. This set does not impact
	 * performance of the emulator much. */
	/* How many bytes of MIPS code have we discarded since the current
	 * game started running, in which code caches? */
	u32	TranslationBytesFlushed[TRANSLATION_REGION_COUNT];
	/* How many times have we had to discard MIPS code since the current
	 * game started running, in which code caches and for what reasons? */
	u32	TranslationFlushCount[TRANSLATION_REGION_COUNT][CACHE_FLUSH_REASON_COUNT];
	/* Up to how many bytes of MIPS code must we hold for this game? */
	u32	TranslationBytesPeak[TRANSLATION_REGION_COUNT];
	/* How many times have we gone through a Partial Flush? */
	u32	PartialFlushCount;
	/* How many times have we had to clear entire metadata areas since the
	 * current game started running, which areas and for what reasons? */
	u32	MetadataClearCount[METADATA_AREA_COUNT][METADATA_CLEAR_REASON_COUNT];
#endif
	/* How many times have we detected an underrun of the sound buffer? */
	u32	SoundBufferUnderrunCount;
	/* How many frames have we emulated since the current game started
	 * running? */
	u32	TotalEmulatedFrames;

#ifdef PERFORMANCE_IMPACTING_STATISTICS
	/* Performance statistics collectors. This set impacts normal
	 * performance of the emulator. */
	/* How many times have we recompiled a block anew since the current
	 * game started running? */
	u32	BlockRecompilationCount;
	/* And how many opcodes was that? */
	u32	OpcodeRecompilationCount;
	/* How many times have we reused a cached block of native code since
	 * the current game started running? */
	u32	BlockReuseCount;
	/* And how many opcodes was that? */
	u32	OpcodeReuseCount;
#endif

	/* Are we in a sound buffer underrun? If we are, ignore underrunning
	 * until the current underrun is done. */
	u8	InSoundBufferUnderrun;
};

extern struct GPSP_STATS Stats;

extern void StatsInit(void);
extern void StatsDisplayFPS(void);

#endif // !__GPSP_STATS_H__
