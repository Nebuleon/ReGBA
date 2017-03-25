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

#ifndef GU_H
#define GU_H

//16bit per/pixel
#define GBA_SCREEN_BUFF_SIZE (GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT)

void init_video(void);
void video_resolution(uint32_t mode);
void clear_screen(uint16_t color);
void clear_gba_screen(uint16_t color);
void blit_to_screen(const uint16_t *src, uint32_t w, uint32_t h, int32_t x, int32_t y);
void copy_screen(uint16_t *buffer);

/*
 * Called by the GUI to tell the renderer that 3 frames need to be sent fully
 * to the Nintendo DS before returning to sending the usual 160 rows of pixel
 * data.
 */
extern void UpdateBorder(void);

#endif
