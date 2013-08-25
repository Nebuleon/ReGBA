/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 * Copyright (C) 2007 ????? <?????>
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

#include "common.h"

void trigger_key(u32 key);

void trigger_key(u32 key)
{
  u32 p1_cnt = io_registers[REG_P1CNT];

  if((p1_cnt >> 14) & 0x01)
  {
    u32 key_intersection = (p1_cnt & key) & 0x3FF;

    if(p1_cnt >> 15)
    {
      if(key_intersection == (p1_cnt & 0x3FF))
        raise_interrupt(IRQ_KEYPAD);
    }
    else
    {
      if(key_intersection)
        raise_interrupt(IRQ_KEYPAD);
    }
  }
}

// These are the last keys pressed, in the GBA bitfield format.
// They are stored in saved states.
u32 key = 0;

u32 rapidfire_flag = 1;
// 仿真过程输入

u32 update_input()
{
	enum ReGBA_Buttons new_key = ReGBA_GetPressedButtons();
	
	if (new_key & REGBA_BUTTON_MENU)
	{
		return ReGBA_Menu(REGBA_MENU_ENTRY_REASON_MENU_KEY);
	}

	bool RapidFireUsed = false;

	if (new_key & REGBA_BUTTON_RAPID_A)
	{
		if (rapidfire_flag) new_key |=  REGBA_BUTTON_A;
		else                new_key &= ~REGBA_BUTTON_A;

		RapidFireUsed = true;
	}

	if (new_key & REGBA_BUTTON_RAPID_B)
	{
		if (rapidfire_flag) new_key |=  REGBA_BUTTON_B;
		else                new_key &= ~REGBA_BUTTON_B;

		RapidFireUsed = true;
	}

	if (RapidFireUsed)
	{
		rapidfire_flag = !rapidfire_flag;
	}

	if((new_key | key) != key)
		trigger_key(new_key);

	key = new_key;
	io_registers[REG_P1] = (~key) & 0x3FF;

	return 0;
}

// type = READ / WRITE_MEM
#define input_savestate_body(type)                                            \
{                                                                             \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, key);                            \
}                                                                             \

void input_read_mem_savestate()
input_savestate_body(READ_MEM);

void input_write_mem_savestate()
input_savestate_body(WRITE_MEM);
