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

#include "ds2_types.h"
#include "ds2io.h"
#include "bdf_font.h"

#define NDS_SCREEN_WIDTH 256
#define NDS_SCREEN_HEIGHT 192
#define NDS_SCREEN_SIZE	(NDS_SCREEN_WIDTH*NDS_SCREEN_HEIGHT)

#define COLOR16(red, green, blue) ((blue << 10) | (green << 5) | red)
#define GET_R16(color) (color & 0x1f)
#define GET_G16(color) ((color >> 5) & 0x1f)
#define GET_B16(color) ((color >> 10)& 0x1f)
#define COLOR32(red, green, blue) (0xff000000 | ((blue & 0xff) << 16) | ((green & 0xff) << 8) | (red & 0xff))

#define RGB24_15(pixel) ((((*pixel) & 0xF8) << 7) |\
                        (((*(pixel+1)) & 0xF8) << 2) |\
                        (((*(pixel+2)) & 0xF8)>>3))

#define RGB16_15(pixel) ((((*pixel)>>10) & 0x1F) |\
						(((*pixel) & 0x1F) << 10) |\
						((*pixel) & 0x83E0))


#define PRINT_STRING(screen, str, fg_color, x, y)							   \
  BDF_render_mix(screen, SCREEN_WIDTH, x, y, 0, COLOR_TRANS, fg_color, str)				   \

#define PRINT_STRING_SHADOW(screen, str, fg_color, x, y)                       \
  BDF_render_mix(screen, SCREEN_WIDTH, x+1, y+1, 0, 0, 0, str);             				   \
  BDF_render_mix(screen, SCREEN_WIDTH, x, y, 0, 0, 0, str)                  				   \

#define PRINT_STRING_BG(screen, str, fg_color, bg_color, x, y)                 \
  BDF_render_mix(screen, SCREEN_WIDTH, x, y, 0, bg_color, fg_color, str)					   \

#define PRINT_STRING_BG_UTF8(screen, utf8, fg_color, bg_color, x, y)           \
   BDF_render_mix(screen, SCREEN_WIDTH, x, y, 0, bg_color, fg_color, utf8)	   \


//colors
#define COLOR_TRANS         COLOR16(31, 31, 63)
#define COLOR_WHITE         COLOR16(31, 31, 31)
#define COLOR_BLACK         COLOR16( 0,  0,  0)
#define COLOR_TEXT          COLOR16(31, 31, 31)
#define COLOR_PROGRESS_TEXT COLOR16( 0,  0,  0)
#define COLOR_PROGRESS_BAR  COLOR16(15, 15, 15)
#define COLOR_ERROR         COLOR16(31,  0,  0)
#define COLOR_BG            COLOR16(2,  4,  10)
#define COLOR_BG32          COLOR32(2*8,  4*8,  10*8)
#define COLOR_ROM_INFO      COLOR16(22, 18, 26)
#define COLOR_ACTIVE_ITEM   COLOR16(31, 31, 31)
#define COLOR_INACTIVE_ITEM COLOR16(11,  7, 19)
#define COLOR_HELP_TEXT     COLOR16(16, 20, 24)
#define COLOR_DIALOG        COLOR16(31, 31, 31)
#define COLOR_DIALOG_SHADOW COLOR16( 0,  2,  8)
#define COLOR_FRAME         COLOR16( 0,  0,  0)
#define COLOR_YESNO_TEXT    COLOR16( 0,  0,  0)
#define COLOR_GREEN         COLOR16( 0, 31, 0 )
#define COLOR_GREEN1        COLOR16( 0, 24, 0 )
#define COLOR_GREEN2        COLOR16( 0, 18, 0 )
#define COLOR_GREEN3        COLOR16( 0, 12, 0 )
#define COLOR_GREEN4        COLOR16( 0, 6, 0 )
#define COLOR_RED           COLOR16( 31, 0, 0 )
#define COLOR_MSSG			COLOR16( 16, 8, 29)
/******************************************************************************
 *
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

struct background{
    char bgname[128];
    char bgbuffer[256*192*2];
};

struct gui_iconlist{
    const char *iconname;     //icon name
    u32 x;                    //picture size
    u32 y;
    char *iconbuff;
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
extern void print_string_center(void* screen_addr, u32 sy, u32 color, u32 bg_color, char *str);
extern void print_string_shadow_center(void* screen_addr, u32 sy, u32 color, char *str);
extern void hline(u32 sx, u32 ex, u32 y, u32 color);
extern void hline_alpha(u32 sx, u32 ex, u32 y, u32 color, u32 alpha);
extern void vline(u32 x, u32 sy, u32 ey, u32 color);
extern void vline_alpha(u32 x, u32 sy, u32 ey, u32 color, u32 alpha);
extern void drawbox(void* screen_address, u32 sx, u32 sy, u32 ex, u32 ey, u32 color);
extern void drawboxfill(void* screen_address, u32 sx, u32 sy, u32 ex, u32 ey, u32 color);
extern void draw_selitem(void* screen_address, u32 x, u32 y, u32 color, u32 active);
extern void draw_message(void* screen_address, u16 *screen_bg, u32 sx, u32 sy, u32 ex, u32 ey,
                u32 color_fg);
extern void draw_string_vcenter(void* screen_address, u32 sx, u32 sy, u32 width, 
        u32 color_fg, char *string);

#define MAX_SCROLL_STRING   16
extern u32 draw_hscroll_init(void* screen_address, u32 sx, u32 sy, u32 width, 
        u32 color_bg, u32 color_fg, char *string);
extern u32 draw_hscroll(u32 index, s32 scroll_val);
extern void draw_hscroll_over(u32 index);
extern void boxfill_alpha(u32 sx, u32 sy, u32 ex, u32 ey, u32 color, u32 alpha);
extern void init_progress(enum SCREEN_ID screen, u32 total, char *text);
extern void update_progress(void);
extern void show_progress(char *text);
extern void scrollbar(void* screen_addr, u32 sx, u32 sy, u32 ex, u32 ey, u32 all, u32 view, u32 now);
extern u32 yesno_dialog(char *text);
extern u32 draw_yesno_dialog(enum SCREEN_ID screen, u32 sy, char *yes, char *no);
extern u32 draw_hotkey_dialog(enum SCREEN_ID screen, u32 sy, char *clear, char *cancel);
extern void msg_screen_init(const char *title);
extern void msg_screen_draw();
extern void msg_printf(const char *text, ...);
extern void msg_screen_clear(void);
extern void msg_set_text_color(u32 color);

extern int icon_init(u32 language_id);
extern int gui_change_icon(u32 language_id);
extern int show_background(void *screen, char *bgname);
extern void show_icon(void* screen, struct gui_iconlist *icon, u32 x, u32 y);
extern void show_Vscrollbar(char *screen, u32 x, u32 y, u32 part, u32 total);

extern void show_log(void* screen_addr);
extern void err_msg(enum SCREEN_ID screen, char *msg);

#ifdef __cplusplus
}
#endif

#endif //__DRAW_H__

