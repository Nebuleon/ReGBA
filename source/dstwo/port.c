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

#include "common.h"
#include <stdarg.h>

void ReGBA_Trace(const char* Format, ...)
{
	{
		char timestamp[14];
		unsigned int Now = getSysTime();
		sprintf(timestamp, "%6u.%03u", Now / 23437, (Now % 23437) * 16 / 375);
		serial_puts(timestamp);
	}
	serial_putc(':');
	serial_putc(' ');

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
	serial_puts(line);
	va_end(args);
	free(line);
	serial_putc('\r');
	serial_putc('\n');
}

extern char gamepak_filename[MAX_FILE];

void ReGBA_BadJump(u32 SourcePC, u32 TargetPC)
{
	ds2_clearScreen(gba_screen_num, COLOR16(15, 0, 0));
	ds2_flipScreen(gba_screen_num, 2);
	char Line[512];

	draw_string_vcenter(*gba_screen_addr_ptr, 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Jump to unmapped address %08X", TargetPC);
	BDF_render_mix(*gba_screen_addr_ptr, NDS_SCREEN_WIDTH, 0, 32, 0, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at address %08X", SourcePC);
	BDF_render_mix(*gba_screen_addr_ptr, NDS_SCREEN_WIDTH, 0, 48, 0, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(*gba_screen_addr_ptr, 0, 80, 256, COLOR_WHITE, "The game has encountered an unrecoverable error. Please restart the emulator to load another game.");
	ds2_flipScreen(gba_screen_num, 2);

	while (1)
		pm_idle(); // This is goodbye...
}

void ReGBA_MaxBlockExitsReached(u32 BlockStartPC, u32 BlockEndPC, u32 Exits)
{
	ds2_clearScreen(gba_screen_num, COLOR16(0, 0, 15));
	ds2_flipScreen(gba_screen_num, 2);
	char Line[512];

	draw_string_vcenter(*gba_screen_addr_ptr, 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Native code exit limit reached (%u)", Exits);
	BDF_render_mix(*gba_screen_addr_ptr, NDS_SCREEN_WIDTH, 0, 32, 0, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at addresses %08X .. %08X", BlockStartPC, BlockEndPC);
	BDF_render_mix(*gba_screen_addr_ptr, NDS_SCREEN_WIDTH, 0, 48, 0, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(*gba_screen_addr_ptr, 0, 80, 256, COLOR_WHITE, "The game has encountered a recoverable error. It has not crashed, but due to this, it soon may.");
	ds2_flipScreen(gba_screen_num, 2);

	wait_Anykey_press(0);
	wait_Allkey_release(0);
}

void ReGBA_MaxBlockSizeReached(u32 BlockStartPC, u32 BlockEndPC, u32 BlockSize)
{
	ds2_clearScreen(gba_screen_num, COLOR16(0, 0, 15));
	ds2_flipScreen(gba_screen_num, 2);
	char Line[512];

	draw_string_vcenter(*gba_screen_addr_ptr, 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Native code block size reached (%u)", BlockSize);
	BDF_render_mix(*gba_screen_addr_ptr, NDS_SCREEN_WIDTH, 0, 32, 0, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at addresses %08X .. %08X", BlockStartPC, BlockEndPC);
	BDF_render_mix(*gba_screen_addr_ptr, NDS_SCREEN_WIDTH, 0, 48, 0, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(*gba_screen_addr_ptr, 0, 80, 256, COLOR_WHITE, "The game has encountered a recoverable error. It has not crashed, but due to this, it soon may.");
	ds2_flipScreen(gba_screen_num, 2);

	wait_Anykey_press(0);
	wait_Allkey_release(0);
}

void ReGBA_DisplayFPS(void)
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
			Visible = Stats.RenderedFPS != -1 && Stats.EmulatedFPS != -1;
	}

	// Blacken the bottom bar
	memset((u16*) *gba_screen_addr_ptr + 177 * 256, 0, 15 * 256 * sizeof(u16));
	if (Visible)
	{
		char line[512];
		sprintf(line, msg[FMT_STATUS_FRAMES_PER_SECOND], Stats.RenderedFPS, Stats.EmulatedFPS);
		PRINT_STRING_BG_UTF8(*gba_screen_addr_ptr, line, 0x7FFF, 0x0000, 1, 177);
	}
}
