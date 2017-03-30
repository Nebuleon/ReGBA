/* Per-platform code - gpSP on DSTwo
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

#include <ds2/pm.h>
#include "common.h"
#include <stdarg.h>

void ReGBA_Trace(const char* Format, ...)
{
	clock_t now = clock();
	printf("%6u.%03u: ", now / CLOCKS_PER_SEC, (now % CLOCKS_PER_SEC) * 1000 / CLOCKS_PER_SEC);

	va_list args;
	va_start(args, Format);
	vfprintf(stdout, Format, args);
	va_end(args);

	putc('\n', stdout);
}

void ReGBA_BadJump(uint32_t SourcePC, uint32_t TargetPC)
{
	DS2_FillScreen(DS_ENGINE_MAIN, BGR555(0, 0, 15));
	char Line[512];

	draw_string_vcenter(DS2_GetMainScreen(), 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Jump to unmapped address %08" PRIX32, TargetPC);
	BDF_RenderUTF8s(DS2_GetMainScreen(), DS_SCREEN_WIDTH, 0, 32, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at address %08" PRIX32, SourcePC);
	BDF_RenderUTF8s(DS2_GetMainScreen(), DS_SCREEN_WIDTH, 0, 48, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(DS2_GetMainScreen(), 0, 80, 256, COLOR_WHITE, "The game has encountered an unrecoverable error. Please restart the emulator to load another game.");
	DS2_FlipMainScreen();

	while (1) {
		DS2_AwaitInterrupt();
	}
}

void ReGBA_MaxBlockExitsReached(uint32_t BlockStartPC, uint32_t BlockEndPC, uint32_t Exits)
{
	DS2_FillScreen(DS_ENGINE_MAIN, BGR555(15, 0, 0));
	char Line[512];

	draw_string_vcenter(DS2_GetMainScreen(), 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Native code exit limit reached (%" PRIu32 ")", Exits);
	BDF_RenderUTF8s(DS2_GetMainScreen(), DS_SCREEN_WIDTH, 0, 32, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at addresses %08" PRIX32 " .. %08" PRIX32, BlockStartPC, BlockEndPC);
	BDF_RenderUTF8s(DS2_GetMainScreen(), DS_SCREEN_WIDTH, 0, 48, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(DS2_GetMainScreen(), 0, 80, 256, COLOR_WHITE, "The game has encountered a recoverable error. It has not crashed, but due to this, it soon may.");
	DS2_FlipMainScreen();
	UpdateBorder();

	DS2_AwaitAnyButtons();
	DS2_AwaitNoButtons();
}

void ReGBA_MaxBlockSizeReached(uint32_t BlockStartPC, uint32_t BlockEndPC, uint32_t BlockSize)
{
	DS2_FillScreen(DS_ENGINE_MAIN, BGR555(15, 0, 0));
	char Line[512];

	draw_string_vcenter(DS2_GetMainScreen(), 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Native code block size reached (%" PRIu32 ")", BlockSize);
	BDF_RenderUTF8s(DS2_GetMainScreen(), DS_SCREEN_WIDTH, 0, 32, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at addresses %08" PRIX32 " .. %08" PRIX32, BlockStartPC, BlockEndPC);
	BDF_RenderUTF8s(DS2_GetMainScreen(), DS_SCREEN_WIDTH, 0, 48, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(DS2_GetMainScreen(), 0, 80, 256, COLOR_WHITE, "The game has encountered a recoverable error. It has not crashed, but due to this, it soon may.");
	DS2_FlipMainScreen();
	UpdateBorder();

	DS2_AwaitAnyButtons();
	DS2_AwaitNoButtons();
}

void ReGBA_DisplayFPS(void)
{
	uint32_t Visible = gpsp_persistent_config.DisplayFPS;
	if (Visible) {
		clock_t now = clock(), duration = now - Stats.LastFPSCalculationTime;
		if (duration >= CLOCKS_PER_SEC /* 1 second */) {
			Stats.RenderedFPS = Stats.RenderedFrames * CLOCKS_PER_SEC / duration;
			Stats.RenderedFrames = 0;
			Stats.EmulatedFPS = Stats.EmulatedFrames * CLOCKS_PER_SEC / duration;
			Stats.EmulatedFrames = 0;
			Stats.LastFPSCalculationTime = now;
			UpdateBorder();
		} else
			Visible = Stats.RenderedFPS != -1 && Stats.EmulatedFPS != -1;
	}

	// Blacken the bottom bar
	memset(DS2_GetMainScreen() + DS_SCREEN_WIDTH * 177, 0, DS_SCREEN_WIDTH * 15 * sizeof(uint16_t));
	if (Visible) {
		char line[512];
		sprintf(line, msg[FMT_STATUS_FRAMES_PER_SECOND], Stats.RenderedFPS, Stats.EmulatedFPS);
		PRINT_STRING_BG(DS2_GetMainScreen(), line, COLOR_WHITE, COLOR_BLACK, 1, 177);
	}
}

void ReGBA_OnGameLoaded(const char* GamePath)
{
	char tempPath[PATH_MAX];
	strcpy(tempPath, GamePath);

	char *dirEnd = strrchr(tempPath, '/');

	if(!dirEnd)
		return;

	//then strip filename from directory path and set it
	*dirEnd = '\0';
	strcpy(g_default_rom_dir, tempPath);
}

void ReGBA_LoadRTCTime(struct ReGBA_RTC* RTCData)
{
	time_t rawtime = time(NULL);
	struct tm* tm = localtime(&rawtime);

	// struct tm's year base is 1900; ReGBA_RTC's is 2000.
	RTCData->year = tm->tm_year - 100;
	// struct tm uses 0-based months; ReGBA_RTC uses 1-based months.
	RTCData->month = tm->tm_mon + 1;
	RTCData->day = tm->tm_mday;
	// Weekday conforms to the expectations (0 = Sunday, 6 = Saturday).
	RTCData->weekday = tm->tm_wday;
	RTCData->hours = tm->tm_hour;
	RTCData->minutes = tm->tm_min;
	RTCData->seconds = tm->tm_sec;
}

bool ReGBA_IsRenderingNextFrame()
{
	return !skip_next_frame_flag;
}

const char* GetFileName(const char* Path)
{
	char* Result = strrchr(Path, '/');
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
	char FileNameNoExt[PATH_MAX];
	GetFileNameNoExtension(FileNameNoExt, GamePath);
	if (strlen(DEFAULT_SAVE_DIR) + strlen(FileNameNoExt) + 5 /* / .sav */ > PATH_MAX)
		return false;
	sprintf(Result, "%s/%s.sav", DEFAULT_SAVE_DIR, FileNameNoExt);
	return true;
}

bool ReGBA_GetSavedStateFilename(char* Result, const char* GamePath, uint32_t SlotNumber)
{
	char FileNameNoExt[PATH_MAX];
	char SlotNumberString[11];
	GetFileNameNoExtension(FileNameNoExt, GamePath);
	sprintf(SlotNumberString, "%" PRIu32, (uint32_t) (SlotNumber + 1));
	
	if (strlen(DEFAULT_SAVE_DIR) + strlen(FileNameNoExt) + strlen(SlotNumberString) + 6 /* / _ .rts */ > PATH_MAX)
		return false;
	sprintf(Result, "%s/%s_%s.rts", DEFAULT_SAVE_DIR, FileNameNoExt, SlotNumberString);
	return true;
}

bool ReGBA_GetBundledGameConfig(char* Result)
{
	return false;
}

size_t FILE_LENGTH(FILE_TAG_TYPE File)
{
	size_t pos, size;
	pos = ftell(File);
	fseek(File, 0, SEEK_END);
	size = ftell(File);
	fseek(File, pos, SEEK_SET);

	return size;
}
