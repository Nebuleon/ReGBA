/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
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

#ifndef INPUT_H
#define INPUT_H

#define MAX_GAMEPAD_CONFIG_MAP 16

enum ReGBA_Buttons
{
	REGBA_BUTTON_MENU = 0x1000,
	REGBA_BUTTON_RAPID_B = 0x0800,
	REGBA_BUTTON_RAPID_A = 0x0400,
	REGBA_BUTTON_L = 0x0200,
	REGBA_BUTTON_R = 0x0100,
	REGBA_BUTTON_DOWN = 0x0080,
	REGBA_BUTTON_UP = 0x0040,
	REGBA_BUTTON_LEFT = 0x0020,
	REGBA_BUTTON_RIGHT = 0x0010,
	REGBA_BUTTON_START = 0x0008,
	REGBA_BUTTON_SELECT = 0x0004,
	REGBA_BUTTON_B = 0x0002,
	REGBA_BUTTON_A = 0x0001,
};

enum ReGBA_MenuEntryReason
{
	/* ReGBA is initialising and has no ROM. The menu is shown to select it.
	 * It must stay up until the user selects a ROM. */
	REGBA_MENU_ENTRY_REASON_NO_ROM,
	/* The user pressed ReGBA's menu key. The menu can be dismissed by the
	 * user. */
	REGBA_MENU_ENTRY_REASON_MENU_KEY,
	/* The GBA game entered suspend mode. The menu is shown until the user
	 * wants to exit suspend mode or selects a new ROM. */
	REGBA_MENU_ENTRY_REASON_SUSPENDED,
};

uint32_t update_input();
void input_read_mem_savestate();
void input_write_mem_savestate();

// Tilt sensor on the GBA side. It's mapped... somewhere... in the GBA address
// space. See the read_backup function in memory.c for more information.
extern uint32_t tilt_sensor_x;
extern uint32_t tilt_sensor_y;

#endif

