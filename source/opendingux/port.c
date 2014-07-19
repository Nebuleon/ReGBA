/* Per-platform code - gpSP on GCW Zero
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
#include <stdarg.h>

uint32_t PerGameBootFromBIOS;
uint32_t BootFromBIOS;
uint32_t PerGameShowFPS;
uint32_t ShowFPS;
uint32_t PerGameUserFrameskip;
uint32_t UserFrameskip;

void ReGBA_Trace(const char* Format, ...)
{
	char* line = malloc(82);
	va_list args;
	int linelen;

	va_start(args, Format);
	if ((linelen = vsnprintf(line, 82, Format, args)) >= 82)
	{
		va_end(args);
		va_start(args, Format);
		free(line);
		line = malloc(linelen + 1);
		vsnprintf(line, linelen + 1, Format, args);
	}
	printf(line);
	va_end(args);
	free(line);
	printf("\r\n");
}

void ReGBA_BadJump(u32 SourcePC, u32 TargetPC)
{
	printf("GBA segmentation fault");
	printf("The game tried to jump from %08X to %08X", SourcePC, TargetPC);
	exit(1);
}

void ReGBA_MaxBlockExitsReached(u32 BlockStartPC, u32 BlockEndPC, u32 Exits)
{
	ReGBA_Trace("Native code exit limit reached");
	ReGBA_Trace("%u exits in the block of GBA code from %08X to %08X", Exits, BlockStartPC, BlockEndPC);
}

void ReGBA_MaxBlockSizeReached(u32 BlockStartPC, u32 BlockEndPC, u32 BlockSize)
{
	ReGBA_Trace("Native code block size reached");
	ReGBA_Trace("%u instructions in the block of GBA code from %08X to %08X", BlockSize, BlockStartPC, BlockEndPC);
}

timespec TimeDifference(timespec Past, timespec Present)
{
	timespec Result;
	Result.tv_sec = Present.tv_sec - Past.tv_sec;

	if (Present.tv_nsec >= Past.tv_nsec)
		Result.tv_nsec = Present.tv_nsec - Past.tv_nsec;
	else
	{
		Result.tv_nsec = 1000000000 - (Past.tv_nsec - Present.tv_nsec);
		Result.tv_sec--;
	}
	return Result;
}

void ReGBA_DisplayFPS(void)
{
	u32 Visible = ResolveSetting(ShowFPS, PerGameShowFPS);
	if (Visible)
	{
		timespec Now;
		clock_gettime(CLOCK_MONOTONIC, &Now);
		timespec Duration = TimeDifference(Stats.LastFPSCalculationTime, Now);
		if (Duration.tv_sec >= 2)
		{
			Visible = 0;
			StatsStopFPS();
			Stats.LastFPSCalculationTime = Now;
		}
		else if (Duration.tv_sec >= 1)
		{
			uint32_t Nanoseconds = (uint32_t) Duration.tv_sec * 1000000000 + Duration.tv_nsec;
			Stats.RenderedFPS = (uint64_t) Stats.RenderedFrames * 1000000000 / Nanoseconds;
			Stats.RenderedFrames = 0;
			Stats.EmulatedFPS = (uint64_t) Stats.EmulatedFrames * 1000000000 / Nanoseconds;
			Stats.EmulatedFrames = 0;
			Stats.LastFPSCalculationTime = Now;
		}
		else
			Visible = Stats.RenderedFPS != -1 && Stats.EmulatedFPS != -1;
	}

	if (Visible)
	{
		char line[512];
		sprintf(line, "%2u/%3u", Stats.RenderedFPS, Stats.EmulatedFPS);
		// White text, black outline
		ScaleModeUnapplied();
		PrintStringOutline(line, RGB888_TO_RGB565(255, 255, 255), RGB888_TO_RGB565(0, 0, 0), OutputSurface->pixels, OutputSurface->pitch, 7, 3, OutputSurface->w - 14, OutputSurface->h - 6, LEFT, BOTTOM);
	}
}

void ReGBA_LoadRTCTime(struct ReGBA_RTC* RTCData)
{
	time_t GMTTime = time(NULL);
	struct tm* Time = localtime(&GMTTime);

	RTCData->year = Time->tm_year;
	RTCData->month = Time->tm_mon + 1;
	RTCData->day = Time->tm_mday;
	// Weekday conforms to the expectations (0 = Sunday, 6 = Saturday).
	RTCData->weekday = Time->tm_wday;
	RTCData->hours = Time->tm_hour;
	RTCData->minutes = Time->tm_min;
	RTCData->seconds = Time->tm_sec;
}

bool ReGBA_IsRenderingNextFrame()
{
	uint32_t ResolvedUserFrameskip = ResolveSetting(UserFrameskip, PerGameUserFrameskip);
	if (FastForwardFrameskip != 0) /* fast-forwarding */
	{
		if (ResolvedUserFrameskip != 0)  /* fast-forwarding on user frameskip */
			return FastForwardFrameskipControl == 0 && UserFrameskipControl == 0;
		else /* fast-forwarding on automatic frameskip */
			return FastForwardFrameskipControl == 0 && AudioFrameskipControl == 0;
	}
	else
	{
		if (ResolvedUserFrameskip != 0) /* user frameskip */
			return UserFrameskipControl == 0;
		else /* automatic frameskip */
			return AudioFrameskipControl == 0;
	}
}

const char* GetFileName(const char* Path)
{
	const char* Result = strrchr(Path, '/');
	if (Result)
		return Result + 1;
	return Path;
}

void RemoveExtension(char* Result, const char* FileName)
{
	strcpy(Result, FileName);
	char* Dot = strrchr(Result, '.');
	if (Dot)
		*Dot = '\0';
}

void GetFileNameNoExtension(char* Result, const char* Path)
{
	const char* FileName = GetFileName(Path);
	RemoveExtension(Result, FileName);
}

bool ReGBA_GetBackupFilename(char* Result, const char* GamePath)
{
	char FileNameNoExt[MAX_PATH + 1];
	GetFileNameNoExtension(FileNameNoExt, GamePath);
	if (strlen(main_path) + strlen(FileNameNoExt) + 5 /* / .sav */ > MAX_PATH)
		return false;
	sprintf(Result, "%s/%s.sav", main_path, FileNameNoExt);
	return true;
}

bool ReGBA_GetSavedStateFilename(char* Result, const char* GamePath, uint32_t SlotNumber)
{
	if (SlotNumber >= 100)
		return false;

	char FileNameNoExt[MAX_PATH + 1];
	char SlotNumberString[11];
	GetFileNameNoExtension(FileNameNoExt, GamePath);
	sprintf(SlotNumberString, "%02u", SlotNumber);
	
	if (strlen(main_path) + strlen(FileNameNoExt) + strlen(SlotNumberString) + 2 /* / . */ > MAX_PATH)
		return false;
	sprintf(Result, "%s/%s.s%s", main_path, FileNameNoExt, SlotNumberString);
	return true;
}

bool ReGBA_GetBundledGameConfig(char* Result)
{
	if (executable_path[0] == '\0')
		return false;

	if (strlen(executable_path) + strlen(CONFIG_FILENAME) + 1 /* "/" */ > MAX_PATH)
		return false;

	sprintf(Result, "%s/%s", executable_path, CONFIG_FILENAME);
	return true;
}

void ReGBA_OnGameLoaded(const char* GamePath)
{
}
