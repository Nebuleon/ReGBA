/* draw.h
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

#ifndef __DRAW_H__
#define __DRAW_H__

#include <stdbool.h>
#include <stdint.h>
#include <ds2/ds.h>
#include "bdf_font.h"


#define PRINT_STRING(screen, str, fg_color, x, y)							   \
  BDF_RenderUTF8s(screen, DS_SCREEN_WIDTH, x, y, COLOR_TRANS, fg_color, str)				   \

#define PRINT_STRING_SHADOW(screen, str, fg_color, x, y)                       \
  BDF_RenderUTF8s(screen, DS_SCREEN_WIDTH, x+1, y+1, 0, 0, str);             				   \
  BDF_RenderUTF8s(screen, DS_SCREEN_WIDTH, x, y, 0, 0, str)                  				   \

#define PRINT_STRING_BG(screen, str, fg_color, bg_color, x, y)                 \
  BDF_RenderUTF8s(screen, DS_SCREEN_WIDTH, x, y, bg_color, fg_color, str)					   \

#define COLOR_TRANS         0xFFFF
#define COLOR_WHITE         BGR555(31, 31, 31)
#define COLOR_BLACK         BGR555( 0,  0,  0)

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t COLOR_BG;
extern uint16_t COLOR_INACTIVE_ITEM;
extern uint16_t COLOR_ACTIVE_ITEM;
extern uint16_t COLOR_MSSG;
extern uint16_t COLOR_INACTIVE_MAIN;
extern uint16_t COLOR_ACTIVE_MAIN;

struct gui_icon {
	const char *name;      /* Base name of the icon */
	uint32_t x;            /* Width */
	uint32_t y;            /* Height */
	const uint16_t *data;  /* Image data (BGR555) */
};

extern struct gui_icon gui_icons[];

#define ICON_ZIPFILE      gui_icons[0]
#define ICON_DIRECTORY    gui_icons[1]
#define ICON_GBAFILE      gui_icons[2]
#define ICON_TITLE        gui_icons[3]

#define ICON_AVO          gui_icons[4]
#define ICON_SAVO         gui_icons[5]
#define ICON_TOOL         gui_icons[6]
#define ICON_CHEAT        gui_icons[7]
#define ICON_OTHER        gui_icons[8]
#define ICON_EXIT         gui_icons[9]
#define ICON_MSEL         gui_icons[10]
#define ICON_MNSEL        gui_icons[11]
#define ICON_NAVO         gui_icons[12]
#define ICON_NSAVO        gui_icons[13]
#define ICON_NTOOL        gui_icons[14]
#define ICON_NCHEAT       gui_icons[15]
#define ICON_NOTHER       gui_icons[16]
#define ICON_NEXIT        gui_icons[17]

#define ICON_UNKNOW       gui_icons[18]
#define ICON_MAINITEM     gui_icons[19]
#define ICON_NMAINITEM    gui_icons[20]
#define ICON_MAINBG       gui_icons[21]

#define ICON_TITLEICON    gui_icons[22]
#define ICON_SUBBG        gui_icons[23]

#define ICON_SUBSELA      gui_icons[24]
#define ICON_STATEFULL    gui_icons[25]
#define ICON_NSTATEFULL   gui_icons[26]
#define ICON_STATEEMPTY   gui_icons[27]
#define ICON_NSTATEEMPTY  gui_icons[28]
#define ICON_DOTDIR       gui_icons[29]
#define ICON_BACK         gui_icons[30]
#define ICON_NBACK        gui_icons[31]
#define ICON_CHTFILE      gui_icons[32]
#define ICON_MSG          gui_icons[33]
#define ICON_BUTTON       gui_icons[34]

/*
* Draws a message box. Note that this only shows the message box itself, and
* not any of its contents.
*/
extern void draw_message_box(uint16_t* screen);

/*
 * Draws some text centered horizontally on the screen.
 *
 * In:
 *   screen: The address of the upper-left corner of the screen.
 *   sx, sy: The X and Y coordinates of the upper-left corner of the text.
 *   width: The width of the area that may contain the text to the right of
 *     'sx'.
 *   color: The BGR 555 color of the text.
 *   string: The text to be scrolled, encoded as UTF-8.
*/
extern void draw_string_vcenter(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width,
	uint32_t color, const char* string);

/*
 * Initialises a text scroller to display a certain string.
 *
 * In:
 *   screen: The address of the upper-left corner of the screen.
 *   sx, sy: The X and Y coordinates of the upper-left corner of the text.
 *   width: The width of the scroller's viewport.
 *   bg_color: The BGR 555 color of the background around the text, or
 *     COLOR_TRANS for transparency.
 *   fg_color: The BGR 555 color of the text.
 *   string: The text to be scrolled, encoded as UTF-8.
 * Input assertions: sx + width < DS_SCREEN_WIDTH &&
 *   sy + [text height] < DS_SCREEN_HEIGHT && string != NULL &&
 *   screen != NULL.
 * Returns: the scroller's handle, to be used to scroll the text in
 *   draw_hscroll.
 */
extern void* hscroll_init(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width,
	uint16_t bg_color, uint16_t fg_color, const char* string);
extern void* draw_hscroll_init(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width,
	uint16_t bg_color, uint16_t fg_color, const char* string);

/*
 * Scrolls an initialised scroller's text.
 *
 * A scroller is never allowed to go past the beginning of the text when
 * scrolling to the left, or to go past the end when scrolling to the right.
 *
 * In:
 *   handle: The scroller's handle.
 *   scroll_val: The number of pixels to scroll. The sign affects the
 *     direction. If scroll_val > 0, the scroller's viewport is moved to
 *     the left; if < 0, the scroller's viewport is moved to the right.
 * Input assertions: handle was returned by a previous call to
 *   draw_hscroll_init and not used in a call to draw_hscroll_over.
 * Returns: The number of pixels still available to scroll in the direction
 *   specified by the sign of 'scroll_val'.
 *
 * Example: (assume each letter is 1 pixel; this won't be true in reality)
 *           [some lengthy text shown in ]         |
 * val -5 -> |    [lengthy text shown in a scr]xxxxx -> to right, returns 5
 * val -5 -> |         [hy text shown in a scroller] -> to right, returns 0
 * val  3 -> xxxxxxx[ngthy text shown in a scrol]  | -> to left,  returns 7
 * val  3 -> xxxx[ lengthy text shown in a sc]     | -> to left,  returns 4
 */
extern uint32_t draw_hscroll(void* handle, int32_t scroll_val);
extern void draw_hscroll_over(void* handle);

/*
 * Draws the buttons for a yes/no dialog and returns true if yes was selected,
 * or false if no was selected.
 */
extern bool draw_yesno_dialog(enum DS_Engine engine, const char* yes, const char* no);

/*
 * Draws the buttons for a hotkey dialog (with Clear and Cancel buttons)
 * and returns a bitmask containing the DS buttons pressed as in <ds2/ds.h>.
 */
extern uint16_t draw_hotkey_dialog(enum DS_Engine engine, const char* clear, const char* cancel);

/*
* Loads GUI icons corresponding to the given language_id.
*
* In:
*   language_id: The sequence number of the language to load language-specific
*     icons for.
* Returns:
*   0 on success.
*   1 if there is insufficient space in gui_picture for all image data
*   (this is an internal error).
*   -(n + 1) if an error occurred while loading any icon, with 'n' being the
*   index of the first icon for which an error occurred.
*/
extern int gui_change_icon(uint32_t language_id);
extern int icon_init(uint32_t language_id);
extern int color_init(void);
extern void show_icon(uint16_t* screen, const struct gui_icon *icon, uint32_t x, uint32_t y);

/*
* Displays the boot logo. No error is returned if the logo is not present;
* the screen is simply not overwritten.
*/
extern void show_logo(void);

#ifdef __cplusplus
}
#endif

#endif //__DRAW_H__

