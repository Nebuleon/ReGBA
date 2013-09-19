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
#include "gui.h"

void** gba_screen_addr_ptr = &up_screen_addr;
u32 gba_screen_num = UP_SCREEN;

u16* GBAScreen;
u32  GBAScreenPitch = 256;

void init_video()
{
	u8* ptr = (u8*) up_screen_addr;
	memset(ptr, 0, 256 * 192 * sizeof(u16));

	ds2_flipScreen(UP_SCREEN, UP_SCREEN_UPDATE_METHOD);

	GBAScreen = (u16*) *gba_screen_addr_ptr + (16 * 256 + 8);
}

void flip_screen(void)
{
	ds2_flipScreen(DOWN_SCREEN, DOWN_SCREEN_UPDATE_METHOD);
}

void ReGBA_RenderScreen(void)
{
	if (ReGBA_IsRenderingNextFrame())
	{
		Stats.RenderedFrames++;
		ReGBA_DisplayFPS();
		ds2_flipScreen(gba_screen_num, UP_SCREEN_UPDATE_METHOD);

		// Update screen address each flip otherwise flickering occurs
		GBAScreen = (u16*) *gba_screen_addr_ptr + (16 * 256 + 8);
		to_skip = 0;
	}
	else
		to_skip++;

	skip_next_frame_flag = to_skip < SKIP_RATE;
}

void clear_screen(u16 color)
{
	ds2_clearScreen(DOWN_SCREEN, color);
}

void clear_gba_screen(u16 color)
{
	ds2_clearScreen(gba_screen_num, color);
	ds2_flipScreen(gba_screen_num, UP_SCREEN_UPDATE_METHOD);

	// Update screen address each flip otherwise flickering occurs
	GBAScreen = (u16*) *gba_screen_addr_ptr + (16 * 256 + 8);
}

void copy_screen(u16 *buffer)
{
	u32  y;
	u16* ptr;

	for (y = 0; y < GBA_SCREEN_HEIGHT; y++)
	{
		ptr= (u16*) *gba_screen_addr_ptr + (y + 16) * 256 + 8;
		memcpy(buffer, ptr, GBA_SCREEN_WIDTH * sizeof(u16));
		buffer += GBA_SCREEN_WIDTH;
	}
}

void blit_to_screen(u16 *src, u32 w, u32 h, u32 dest_x, u32 dest_y)
{
	u32 y;
	u16* dst;

	if(w > NDS_SCREEN_WIDTH) w = NDS_SCREEN_WIDTH;
	if(h > NDS_SCREEN_HEIGHT) h = NDS_SCREEN_HEIGHT;
	if(dest_x == -1)    //align center
		dest_x = (NDS_SCREEN_WIDTH - w) / 2;
	if(dest_y == -1)
		dest_y = (NDS_SCREEN_HEIGHT - h) / 2;

	u16* screenp = *gba_screen_addr_ptr;
	for (y = 0; y < h; y++)
	{
		dst= screenp + (y + dest_y) * 256 + dest_x;
		memcpy(dst, src, w * sizeof(u16));
		src += GBA_SCREEN_WIDTH;
	}
}
