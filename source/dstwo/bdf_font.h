/* bdf_font.h
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

#ifndef __BDF_FONT_H__
#define __BDF_FONT_H__

#ifdef __cplusplus
extern "C" {
#endif

struct bdffont {
	uint32_t dwidth;    //byte 3:2 x-distance; 1:0 y-distance
	uint32_t bbx;       //byte 3 x-width; 2 y-height; 1 x-offset; 0 y-offset
	uint8_t *bitmap;
};

struct bdflibinfo {
	uint32_t width;
	uint32_t height;
	uint32_t start;
	uint32_t span;
	uint32_t maplen;
	uint8_t* mapmem;
	struct bdffont* fonts;
};


/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
extern int BDF_font_init(void);
extern void BDF_font_release(void);

extern uint32_t BDF_GetFontHeight(void);

/*
 * Renders a single character given its Unicode codepoint.
 *
 * In:
 *   screen_w: The width of the screen, which is added to the 'screen' address
 *     between rows.
 *   bg_color: The color value to apply for unset bits in the glyph's bitmap
 *     or space around its bounding box. If bit 15 is set (0x8000), nothing is
 *     drawn.
 *   fg_color: The color value to apply for set bits in the glyph's bitmap.
 *   ch: The Unicode codepoint, between U+0000 and U+FFFF, to be drawn.
 * Out:
 *   screen: The address of the upper-left corner of the screen part to draw
 *     the character to. X and Y offsets have been applied to this address.
 * Returns:
 *   The width of the character that was just drawn.
 */
extern uint32_t BDF_RenderUCS2(uint16_t* screen, size_t screen_w, uint16_t bg_color,
	uint16_t fg_color, uint16_t ch);

/*
 * Renders a string of characters encoded in UTF-8.
 *
 * In:
 *   screen_w: The width of the screen, which is added to the 'screen' address
 *     between rows.
 *   x: The column at which to start rendering in 'screen'.
 *   y: The row at which to start rendering in 'screen'.
 *   bg_color: The color value to apply for unset bits in the glyph's bitmap
 *     or space around its bounding box. If bit 15 is set (0x8000), nothing is
 *     drawn.
 *   fg_color: The color value to apply for set bits in the glyph's bitmap.
 *   string: The string to be drawn. If drawing this string wraps around the
 *     right side of the screen, it will continue 1 row of pixels below. A
 *     newline character in this string will reset 'x' and increase 'y' by
 *     BDF_GetFontHeight().
 * Out:
 *   screen: The address of the upper-left corner of the screen. X and Y
 *     offsets have NOT been applied to this address.
 */
extern void BDF_RenderUTF8s(uint16_t* screen, size_t screen_w, uint32_t x,
    uint32_t y, uint16_t bg_color, uint16_t fg_color, const char* string);

extern const char* utf8decode(const char *utf8, unsigned short *ucs);

/*
 * Cuts up a run of Unicode codepoints to fit in 'width' pixels.
 *
 * In:
 *   ucs2s: The run of Unicode codepoints to be used.
 *   len: The length of the run of Unicode codepoints.
 *   width: The number of pixels to fit Unicode codepoints into.
 * Input assertions:
 * - 'len' elements at and after 'ucs2s' are valid and readable.
 * Returns:
 *   The number of Unicode codepoints starting at 'ucs2s' that fit in 'width'
 *   pixels.
 */
extern size_t BDF_CutUCS2s(const uint16_t* ucs2s, size_t len, uint32_t width);

/*
 * Returns the width, in pixels, of a character given its Unicode codepoint,
 * which must be U+FFFF or below.
 */
extern uint32_t BDF_WidthUCS2(uint16_t ch);

/*
 * Returns the total width, in pixels, of all characters in 'ucs2s', whose
 * length is specified by 'len'.
 *
 * Input assertions:
 * - 'len' entries are valid and readable at and after 'ucs2s'.
 */
extern uint32_t BDF_WidthUCS2s(const uint16_t* ucs2s, size_t len);

/*
 * Returns the total width, in pixels, of all characters contained in the
 * given string, which is a zero-terminated UTF-8 string.
 */
uint32_t BDF_WidthUTF8s(const char* utf8);

#ifdef __cplusplus
}
#endif

#endif //__BDF_FONT_H__
