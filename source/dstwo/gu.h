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

#define FONT_WIDTH  6
#define FONT_HEIGHT 10

//16bit per/pixel
#define GBA_SCREEN_BUFF_SIZE (GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT)

#define NDS_SCREEN_WIDTH 256
#define NDS_SCREEN_HEIGHT 192

void init_video();
void video_resolution(u32 mode);
void clear_screen(u16 color);
void clear_gba_screen(u16 color);
void blit_to_screen(u16 *src, u32 w, u32 h, u32 x, u32 y);
void copy_screen(u16 *buffer);

extern void** gba_screen_addr_ptr;
extern u32 gba_screen_num;

#endif
