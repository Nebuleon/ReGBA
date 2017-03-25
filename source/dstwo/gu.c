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

uint16_t* GBAScreen;
uint32_t  GBAScreenPitch = 256;

static uint8_t BorderUpdateCount;

static void set_gba_screen()
{
	GBAScreen = DS2_GetMainScreen()
		+ (DS_SCREEN_HEIGHT - GBA_SCREEN_HEIGHT) / 2 * DS_SCREEN_WIDTH
		+ (DS_SCREEN_WIDTH - GBA_SCREEN_WIDTH) / 2;
}

void init_video()
{
	set_gba_screen();
}

void flip_screen(void)
{
	DS2_UpdateScreen(DS_ENGINE_SUB);
}

void ReGBA_RenderScreen(void)
{
	if (ReGBA_IsRenderingNextFrame()) {
		Stats.TotalRenderedFrames++;
		Stats.RenderedFrames++;
		ReGBA_DisplayFPS();
		if (BorderUpdateCount != 0) {
			BorderUpdateCount--;
			DS2_FlipMainScreen();
			DS2_FillScreen(DS_ENGINE_MAIN, COLOR_BLACK);
		} else {
			DS2_FlipMainScreenPart((DS_SCREEN_HEIGHT - GBA_SCREEN_HEIGHT) / 2,
				(DS_SCREEN_HEIGHT - GBA_SCREEN_HEIGHT) / 2 + GBA_SCREEN_HEIGHT);
		}

		// Update screen address each flip otherwise flickering occurs
		set_gba_screen();
		to_skip = 0;
	} else
		to_skip++;

	skip_next_frame_flag = to_skip < SKIP_RATE;
}

void UpdateBorder(void)
{
	/* This flips the whole screen with the wrong border once, but then fills
	 * 3 screens with black. */
	BorderUpdateCount = 4;
}

void clear_screen(uint16_t color)
{
	DS2_FillScreen(DS_ENGINE_SUB, color);
}

void clear_gba_screen(uint16_t color)
{
	DS2_FillScreen(DS_ENGINE_MAIN, color);
	DS2_FlipMainScreen();

	set_gba_screen();
}

void copy_screen(uint16_t *buffer)
{
	uint32_t  y;
	uint16_t* ptr;

	for (y = 0; y < GBA_SCREEN_HEIGHT; y++)
	{
		ptr = DS2_GetMainScreen()
			+ (y + (DS_SCREEN_HEIGHT - GBA_SCREEN_HEIGHT) / 2) * DS_SCREEN_WIDTH
			+ (DS_SCREEN_WIDTH - GBA_SCREEN_WIDTH) / 2;
		memcpy(buffer, ptr, GBA_SCREEN_WIDTH * sizeof(uint16_t));
		buffer += GBA_SCREEN_WIDTH;
	}
}

void blit_to_screen(const uint16_t *src, uint32_t w, uint32_t h, int32_t dest_x, int32_t dest_y)
{
	uint32_t y;
	uint16_t* dst;

	if (w > DS_SCREEN_WIDTH) w = DS_SCREEN_WIDTH;
	if (h > DS_SCREEN_HEIGHT) h = DS_SCREEN_HEIGHT;
	if (dest_x == -1)    //align center
		dest_x = (DS_SCREEN_WIDTH - w) / 2;
	if (dest_y == -1)
		dest_y = (DS_SCREEN_HEIGHT - h) / 2;

	uint16_t* screenp = DS2_GetMainScreen();
	for (y = 0; y < h; y++) {
		dst = screenp + (y + (uint32_t) dest_y) * DS_SCREEN_WIDTH + (uint32_t) dest_x;
		memcpy(dst, src, w * sizeof(uint16_t));
		src += GBA_SCREEN_WIDTH;
	}
}
