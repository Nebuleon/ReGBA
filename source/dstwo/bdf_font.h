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
extern uint32_t BDF_render16_ucs(uint16_t* screen, size_t screen_w, uint16_t bg_color,
	uint16_t fg_color, uint16_t ch);
extern void BDF_render_mix(uint16_t* screen, size_t screen_w, uint32_t x,
    uint32_t y, uint16_t bg_color, uint16_t fg_color, const char* string);
extern const char* utf8decode(const char *utf8, unsigned short *ucs);
extern unsigned char* skip_utf8_unit(unsigned char* utf8, size_t num);

/*
 * Cuts up a run of Unicode codepoints to fit in 'width' pixels or calculates
 * the total width of the run of Unicode codepoints. See the description for
 * the 'direction' parameter.
 *
 * In:
 *   unicodes: The run of Unicode codepoints to be used.
 *   len: The length of the run of Unicode codepoints.
 *   width: The number of pixels to fit Unicode codepoints into. Only used if
 *     'direction' bit 1 (direction & 2) is unset.
 *   direction:
 *     0: Returns the number of codepoints that fit in 'width' pixels, going
 *        backward in 'unicodes'.
 *     1: Returns the number of codepoints that fit in 'width' pixels, going
 *        forward in 'unicodes'.
 *     2: Returns the total width of codepoints going backward in 'unicodes'.
 *     3: Returns the total width of codepoints going forward in 'unicodes'.
 * Input assertions:
 * - If 'direction' bit 0 (direction & 1) is unset, one element at 'unicodes'
 *   and 'len - 1' before it are valid and readable.
 * - If 'direction' bit 0 (direction & 1) is set, 'len' elements at and after
 *   'unicodes' are valid and readable.
 * Returns:
 *   A value depending on 'direction'.
 */
extern uint32_t BDF_cut_unicode(uint16_t *unicodes, size_t len, uint32_t width, uint32_t direction);
extern uint32_t BDF_cut_string(char *string, uint32_t width, uint32_t direction);
extern void BDF_font_release(void);

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
