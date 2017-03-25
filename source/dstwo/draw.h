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

#include <stdint.h>
#include <ds2/ds.h>
#include "bdf_font.h"

#define NDS_SCREEN_SIZE	(DS_SCREEN_WIDTH*DS_SCREEN_HEIGHT)

#define RGB24_15(pixel) ((((*pixel) & 0xF8) << 7) |\
                        (((*(pixel+1)) & 0xF8) << 2) |\
                        (((*(pixel+2)) & 0xF8)>>3))

#define RGB16_15(pixel) ((((*pixel)>>10) & 0x1F) |\
						(((*pixel) & 0x1F) << 10) |\
						((*pixel) & 0x83E0))


#define PRINT_STRING(screen, str, fg_color, x, y)							   \
  BDF_render_mix(screen, DS_SCREEN_WIDTH, x, y, COLOR_TRANS, fg_color, str)				   \

#define PRINT_STRING_SHADOW(screen, str, fg_color, x, y)                       \
  BDF_render_mix(screen, DS_SCREEN_WIDTH, x+1, y+1, 0, 0, str);             				   \
  BDF_render_mix(screen, DS_SCREEN_WIDTH, x, y, 0, 0, str)                  				   \

#define PRINT_STRING_BG(screen, str, fg_color, bg_color, x, y)                 \
  BDF_render_mix(screen, DS_SCREEN_WIDTH, x, y, bg_color, fg_color, str)					   \


//colors
#define COLOR_TRANS         0xFFFF
#define COLOR_WHITE         BGR555(31, 31, 31)
#define COLOR_BLACK         BGR555( 0,  0,  0)
/******************************************************************************
 *
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t COLOR_BG;
extern uint16_t COLOR_INACTIVE_ITEM;
extern uint16_t COLOR_ACTIVE_ITEM;
extern uint16_t COLOR_MSSG;
extern uint16_t COLOR_INACTIVE_MAIN;
extern uint16_t COLOR_ACTIVE_MAIN;

struct gui_iconlist {
	const char *iconname;  /* Base name of the icon */
	uint32_t x;            /* Width */
	uint32_t y;            /* Height */
	uint16_t *iconbuff;    /* Image data (BGR555) */
};

//extern struct background back_ground;
extern struct gui_iconlist gui_icon_list[];

#define ICON_ZIPFILE        gui_icon_list[0]
#define ICON_DIRECTORY      gui_icon_list[1]
#define ICON_GBAFILE		gui_icon_list[2]
#define ICON_TITLE			gui_icon_list[3]

#define ICON_AVO			gui_icon_list[4]
#define ICON_SAVO			gui_icon_list[5]
#define ICON_TOOL			gui_icon_list[6]
#define ICON_CHEAT			gui_icon_list[7]
#define ICON_OTHER			gui_icon_list[8]
#define ICON_EXIT			gui_icon_list[9]
#define ICON_MSEL			gui_icon_list[10]
#define ICON_MNSEL			gui_icon_list[11]
#define ICON_NAVO			gui_icon_list[12]
#define ICON_NSAVO			gui_icon_list[13]
#define ICON_NTOOL			gui_icon_list[14]
#define ICON_NCHEAT			gui_icon_list[15]
#define ICON_NOTHER			gui_icon_list[16]
#define ICON_NEXIT			gui_icon_list[17]

#define ICON_UNKNOW			gui_icon_list[18]
#define ICON_MAINITEM			gui_icon_list[19]
#define ICON_NMAINITEM			gui_icon_list[20]
#define ICON_MAINBG			gui_icon_list[21]

#define ICON_TITLEICON		gui_icon_list[22]
#define ICON_SUBBG			gui_icon_list[23]

#define ICON_SUBSELA		gui_icon_list[24]
#define ICON_STATEFULL		gui_icon_list[25]
#define ICON_NSTATEFULL		gui_icon_list[26]
#define ICON_STATEEMPTY		gui_icon_list[27]
#define ICON_NSTATEEMPTY	gui_icon_list[28]
#define ICON_DOTDIR			gui_icon_list[29]
#define ICON_BACK			gui_icon_list[30]
#define ICON_NBACK			gui_icon_list[31]
#define ICON_CHTFILE		gui_icon_list[32]
#define ICON_MSG			gui_icon_list[33]
#define ICON_BUTTON			gui_icon_list[34]

/******************************************************************************
 *
 ******************************************************************************/
extern void print_string_center(uint16_t* screen, uint32_t sy, uint32_t color, uint32_t bg_color, char *str);
extern void print_string_shadow_center(uint16_t* screen, uint32_t sy, uint32_t color, char *str);
extern void drawhline(uint16_t* screen, uint32_t sx, uint32_t ex, uint32_t y, uint32_t color);
extern void drawvline(uint16_t* screen, uint32_t x, uint32_t sy, uint32_t ey, uint32_t color);
extern void drawbox(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t color);
extern void drawboxfill(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t color);
extern void draw_selitem(uint16_t* screen, uint32_t x, uint32_t y, uint32_t color, uint32_t active);
extern void draw_message_box(uint16_t* screen);
extern void draw_string_vcenter(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width,
        uint32_t color_fg, const char* string);

extern void* hscroll_init(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width,
        uint16_t bg_color, uint16_t fg_color, const char* string);
extern void* draw_hscroll_init(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width,
        uint16_t bg_color, uint16_t fg_color, const char* string);
extern uint32_t draw_hscroll(void* handle, int32_t scroll_val);
extern void draw_hscroll_over(void* handle);
extern uint32_t draw_yesno_dialog(enum DS_Engine engine, uint32_t sy, const char* yes, const char* no);
extern uint32_t draw_hotkey_dialog(enum DS_Engine engine, uint32_t sy, const char* clear, const char* cancel);
extern void scrollbar(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t all, uint32_t view, uint32_t now);
extern int gui_change_icon(uint32_t language_id);
extern int icon_init(uint32_t language_id);
extern int color_init(void);
extern void show_icon(uint16_t* screen, struct gui_iconlist *icon, uint32_t x, uint32_t y);
extern void show_Vscrollbar(uint16_t* screen, uint32_t x, uint32_t y, uint32_t part, uint32_t total);

extern void show_log(void);

#ifdef __cplusplus
}
#endif

#endif //__DRAW_H__

