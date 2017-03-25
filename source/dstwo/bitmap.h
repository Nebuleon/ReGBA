/* bitmap.c
 *
 * Copyright (C) 2010 dking <dking024@gmail.com>
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

#ifndef __BITMAP_H__
#define __BITMAP_H__
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BMP_OK              0
#define BMP_ERR_OPEN        1
#define BMP_ERR_FORMAT      2
#define BMP_ERR_UNSUPPORTED 3
#define BMP_ERR_MEM         4

/*
 * Reads a bitmap file into the given buffer, converting pixels to BGR 555.
 *
 * In:
 *   filename: Full path to the file to be read.
 *   width: Expected width of the data.
 *   height: Expected height of the data.
 * Out:
 *   buf: The buffer to be filled with BGR 555 pixels from the bitmap file.
 *     'width' pixels from row 0 are first, then row 1, and so on until row
 *     'height - 1'.
 * Input assertions:
 *   (width * height) 16-bit quantities are valid and writable at 'buf'.
 * Output assertions:
 * - If the width of the bitmap file exceeds 'width', only the leftmost
 *   'width' pixels of each row are written to 'buf'.
 * - If the height of the bitmap file exceeds 'height', only the topmost
 *   'height' rows of the file are written to 'buf'.
 * - If the width of the bitmap file is less than 'width', the rightmost
 *   pixels in excess in each row are filled with black in 'buf'.
 * - If the height of the bitmap file is less than 'height', the bottommost
 *   rows in excess of the file are filled with black in 'buf'.
 * Returns:
 *   BMP_OK on success.
 *   BMP_ERR_OPEN: The file could not be opened, and errno is set accordingly.
 *   BMP_ERR_FORMAT: The file may not be a .bmp file.
 *   BMP_ERR_UNSUPPORTED: The file is a .bmp file with unsupported attributes.
 *   BMP_ERR_MEM: An auxiliary memory allocation was required but could not be
 *   made.
 */
extern int BMP_Read(const char* filename, uint16_t* buf, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#endif //__BITMAP_H__
