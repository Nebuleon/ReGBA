/* Statistics gathered during emulation in gpSP
 *
 * Copyright (C) 2013 GBATemp user Nebuleon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
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
#include "stats.h"
#include "gui.h"
#include "draw.h"
#include "message.h"

struct GPSP_STATS Stats;

void StatsInit(void)
{
	Stats.RenderedFrames = 0;
	Stats.EmulatedFrames = 0;
	Stats.LastFPSCalculationTime = 0;
	Stats.RenderedFPS = -1;
	Stats.EmulatedFPS = -1;
}

void StatsInitGame(void)
{
#ifndef USE_C_CORE
	u32 cache, reason;
	for (cache = 0; cache < TRANSLATION_REGION_COUNT; cache++)
	{
		Stats.TranslationBytesFlushed[cache] = 0;
		Stats.TranslationBytesPeak[cache] = 0;
		for (reason = 0; reason < FLUSH_REASON_COUNT; reason++)
		{
			Stats.TranslationFlushCount[cache][reason] = 0;
		}
	}
	Stats.PartialFlushCount = 0;
#endif
	Stats.SoundBufferUnderrunCount = 0;
	Stats.InSoundBufferUnderrun = 0;
	Stats.TotalEmulatedFrames = 0;
#ifdef PERFORMANCE_IMPACTING_STATISTICS
	Stats.ARMOpcodesDecoded = 0;
	Stats.ThumbOpcodesDecoded = 0;
#endif
}

void StatsDisplayFPS(void)
{
	u32 Visible = gpsp_persistent_config.DisplayFPS;
	if (Visible)
	{
		unsigned int Now = getSysTime(), Duration = Now - Stats.LastFPSCalculationTime;
		if (Duration >= 23437 /* 1 second */)
		{
			Stats.RenderedFPS = Stats.RenderedFrames * 23437 / Duration;
			Stats.RenderedFrames = 0;
			Stats.EmulatedFPS = Stats.EmulatedFrames * 23437 / Duration;
			Stats.EmulatedFrames = 0;
			Stats.LastFPSCalculationTime = Now;
		}
		else
			Visible = Stats.RenderedFPS != -1 && Stats.EmulatedFrames != -1;
	}

	// Blacken the bottom bar
	memset((u16*) up_screen_addr + 177 * 256, 0, 15 * 256 * sizeof(u16));
	if (Visible)
	{
		char line[512];
		sprintf(line, msg[FMT_STATUS_FRAMES_PER_SECOND], Stats.RenderedFPS, Stats.EmulatedFPS);
		PRINT_STRING_BG_UTF8(up_screen_addr, line, 0x7FFF, 0x0000, 1, 177);
	}
}
