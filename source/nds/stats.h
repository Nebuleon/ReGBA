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
};

extern struct GPSP_STATS Stats;

extern void StatsInit(void);
extern void StatsDisplayFPS(void);

#endif // !__GPSP_STATS_H__
