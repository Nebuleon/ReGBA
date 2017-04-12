/*
 * Not bothering to copyright this header, so there you go, it's now in the
 * public domain. Deal with it.
 * -- Nebuleon, 2013-04-02
 */

#ifndef __GPSP_STATS_H__
#define __GPSP_STATS_H__

#include "common.h"

struct ReGBA_Stats {
#ifdef PERFORMANCE_IMPACTING_STATISTICS
	// This needs to be first, because the assembler code needs it to be
	// at the address of Stats (and I can't be bothered to change the
	// structure offset if I move it, so deal with it, ha!). It's also of type
	// uint32_t, not uint64_t, because the code that updates it is short on
	// registers. - Neb
	uint32_t        WrongAddressLineCount;
#endif
	/* For FPS display. The first 3 variables are updated by the
	 * emulation and read by StatsDisplayFPS. The last 2 variables are
	 * updated and read by StatsDisplayFPS. */
	uint32_t        RenderedFrames;
	uint32_t        EmulatedFrames;
	timespec        LastFPSCalculationTime;
	int32_t         RenderedFPS;
	int32_t         EmulatedFPS;

#ifndef USE_C_CORE
	/* Performance statistics collectors. This set does not impact
	 * performance of the emulator much. */
	/* How many bytes of MIPS code have we discarded since the current
	 * game started running, in which code caches? */
	uint64_t        TranslationBytesFlushed[TRANSLATION_REGION_COUNT];
	/* How many times have we had to discard MIPS code since the current
	 * game started running, in which code caches and for what reasons? */
	uint64_t        TranslationFlushCount[TRANSLATION_REGION_COUNT][CACHE_FLUSH_REASON_COUNT];
	/* Up to how many bytes of MIPS code must we hold for this game? */
	uint64_t        TranslationBytesPeak[TRANSLATION_REGION_COUNT];
	/* How many times have we had to clear Metadata Areas for the current
	 * game, which Metadata Areas and for what reasons? */
	uint64_t        MetadataClearCount[METADATA_AREA_COUNT][METADATA_CLEAR_REASON_COUNT];
	/* How many times have we gone through a Partial Flush? */
	uint64_t        PartialFlushCount;
#endif
	/* How many times have we detected an underrun of the sound buffer? */
	uint64_t        SoundBufferUnderrunCount;
	/* How many frames have we emulated since the current game started
	 * running? */
	uint64_t        TotalEmulatedFrames;
	/* How many frames have we rendered since the current game started
	 * running? */
	uint64_t        TotalRenderedFrames;

#ifdef PERFORMANCE_IMPACTING_STATISTICS
	/* Performance statistics collectors. This set impacts normal
	 * performance of the emulator. */
	/* How many times have we had to decode an ARM or a Thumb opcode from
	 * scratch since the current game started running? */
	uint64_t        ARMOpcodesDecoded;
	uint64_t        ThumbOpcodesDecoded;
	/* How many times have we met a Thumb PC-relative LDR instruction that
	 * loads from the ROM? */
	uint64_t        ThumbROMConstants;
	/* How many times have we recompiled a block anew since the current
	 * game started running? */
	uint64_t        BlockRecompilationCount;
	/* And how many opcodes was that? */
	uint64_t        OpcodeRecompilationCount;
	/* How many times have we reused a cached block of native code since
	 * the current game started running? */
	uint64_t        BlockReuseCount;
	/* And how many opcodes was that? */
	uint64_t        OpcodeReuseCount;

#endif

	/* Are we in a sound buffer underrun? If we are, ignore underrunning
	 * until the current underrun is done. */
	uint_fast8_t    InSoundBufferUnderrun;
} __attribute__((aligned));

extern struct ReGBA_Stats Stats;

extern void StatsStopFPS(void);
extern void StatsInit(void);
extern void StatsInitGame(void);

#endif // !__GPSP_STATS_H__
