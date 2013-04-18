/*
 * GBA fatal error screens
 * (c) 2013 GBAtemp user Nebuleon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "ds2_types.h"
#include "ds2io.h"
#include "ds2_cpu.h"
#include "input.h"
#include "bdf_font.h"
#include "draw.h"
#include "gu.h"
#include "gui.h"
#include "stddef.h"
#include "string.h"

extern char gamepak_filename[MAX_FILE];

void ARMBadJump(unsigned int SourcePC, unsigned int TargetPC)
{
	ds2_clearScreen(UP_SCREEN, COLOR16(15, 0, 0));
	ds2_flipScreen(UP_SCREEN, 2);
	char Line[512];

	draw_string_vcenter(up_screen_addr, 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Jump to unmapped address %08X", TargetPC);
	BDF_render_mix(up_screen_addr, NDS_SCREEN_WIDTH, 0, 32, 0, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at address %08X", SourcePC);
	BDF_render_mix(up_screen_addr, NDS_SCREEN_WIDTH, 0, 48, 0, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(up_screen_addr, 0, 80, 256, COLOR_WHITE, "The game has encountered an unrecoverable error. Please restart the emulator to load another game.");
	ds2_flipScreen(UP_SCREEN, 2);

	while (1)
		pm_idle(); // This is goodbye...
}

void RecompilerMaxExitsReached(unsigned int BlockStartPC, unsigned int BlockEndPC, unsigned int Exits)
{
	ds2_clearScreen(UP_SCREEN, COLOR16(0, 0, 15));
	ds2_flipScreen(UP_SCREEN, 2);
	char Line[512];

	draw_string_vcenter(up_screen_addr, 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Native code exit limit reached (%u)", Exits);
	BDF_render_mix(up_screen_addr, NDS_SCREEN_WIDTH, 0, 32, 0, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at addresses %08X .. %08X", BlockStartPC, BlockEndPC);
	BDF_render_mix(up_screen_addr, NDS_SCREEN_WIDTH, 0, 48, 0, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(up_screen_addr, 0, 80, 256, COLOR_WHITE, "The game has encountered a recoverable error. It has not crashed, but due to this, it soon may.");
	ds2_flipScreen(UP_SCREEN, 2);

	wait_Anykey_press(0);
	wait_Allkey_release(0);
}

void RecompilerMaxBlockSizeReached(unsigned int BlockStartPC, unsigned int BlockEndPC, unsigned int BlockSize)
{
	ds2_clearScreen(UP_SCREEN, COLOR16(0, 0, 15));
	ds2_flipScreen(UP_SCREEN, 2);
	char Line[512];

	draw_string_vcenter(up_screen_addr, 0, 0, 256, COLOR_WHITE, "Guru Meditation");
	sprintf(Line, "Native code block size reached (%u)", BlockSize);
	BDF_render_mix(up_screen_addr, NDS_SCREEN_WIDTH, 0, 32, 0, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at addresses %08X .. %08X", BlockStartPC, BlockEndPC);
	BDF_render_mix(up_screen_addr, NDS_SCREEN_WIDTH, 0, 48, 0, COLOR_TRANS, COLOR_WHITE, Line);

	draw_string_vcenter(up_screen_addr, 0, 80, 256, COLOR_WHITE, "The game has encountered a recoverable error. It has not crashed, but due to this, it soon may.");
	ds2_flipScreen(UP_SCREEN, 2);

	wait_Anykey_press(0);
	wait_Allkey_release(0);
}
