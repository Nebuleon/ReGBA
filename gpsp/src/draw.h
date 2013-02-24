/* unofficial gameplaySP kai
 *
 * Copyright (C) 2007 NJ
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

/******************************************************************************
 * draw.h
 * 基本描画の処理
 ******************************************************************************/
#ifndef DRAW_H
#define DRAW_H

/******************************************************************************
 * マクロ等の定義
 ******************************************************************************/
#define COLOR16(red, green, blue) ((blue << 10) | (green << 5) | red)
#define GET_R16(color) (color & 0x1f)
#define GET_G16(color) ((color >> 5) & 0x1f)
#define GET_B16(color) ((color >> 10)& 0x1f)
#define COLOR32(red, green, blue) (0xff000000 | ((blue & 0xff) << 16) | ((green & 0xff) << 8) | (red & 0xff))

#define RGB24_15(pixel) ((((*pixel) & 0xF8) << 7) |\
                        (((*(pixel+1)) & 0xF8) << 2) |\
                        (((*(pixel+2)) & 0xF8)>>3))

#if 0
#define PRINT_STRING_EXT_BG(str, fg_color, bg_color, x, y, dest_ptr, pitch)                                            \
  fbm_printVRAM( dest_ptr, pitch, x, y, str, fg_color, bg_color, FBM_FONT_FILL | FBM_BACK_FILL)                        \

#define PRINT_STRING(str, fg_color, x, y)                                                                              \
  fbm_printVRAM( screen_address, screen_pitch, x, y, str, fg_color, 0, FBM_FONT_FILL)                                  \

#define PRINT_STRING_BG(str, fg_color, bg_color, x, y)                                                                 \
  fbm_printVRAM( screen_address, screen_pitch, x, y, str, fg_color, bg_color, FBM_FONT_FILL | FBM_BACK_FILL)           \

#define PRINT_STRING_SHADOW(str, fg_color, x, y)                                                                       \
  fbm_printVRAM( screen_address, screen_pitch, x + 1, y + 1, str, 0, 0, FBM_FONT_FILL);                                \
  fbm_printVRAM( screen_address, screen_pitch, x, y, str, fg_color, 0, FBM_FONT_FILL)                                  \

#define PRINT_STRING_BG_SJIS(utf8, str, fg_color, bg_color, x, y)                                                      \
  sjis_to_utf8(str, utf8);                                                                                             \
  fbm_printVRAM( screen_address, screen_pitch, x, y, utf8, fg_color, bg_color, FBM_FONT_FILL | FBM_BACK_FILL)          \

#ifdef NDS_LAYER
#define PRINT_STRING_BG_SJIS_GBA(utf8, str, fg_color, bg_color, x, y) \
  sjis_to_utf8(str, utf8); \
  fbm_printVRAM( gba_screen_address-16*256-8, screen_pitch, x, y, utf8, fg_color, bg_color, FBM_FONT_FILL | FBM_BACK_FILL)
#else
#define PRINT_STRING_BG_SJIS_GBA
#endif
#endif

#define PRINT_STRING(str, fg_color, x, y)                                      \
  BDF_render_string((char*)screen_address, x, y, COLOR_TRANS, fg_color, str)   \

#define PRINT_STRING_GBA(str, fg_color, x, y)                                      \
  BDF_render_string((char*)(gba_screen_address -16*256 -8), x, y, COLOR_TRANS, fg_color, str)   \

#define PRINT_STRING_SHADOW(str, fg_color, x, y)                               \
  BDF_render_string((char*)screen_address, x+1, y+1, 0, 0, str);               \
  BDF_render_string((char*)screen_address, x, y, 0, 0, str)                    \

#define PRINT_STRING_BG(str, fg_color, bg_color, x, y)                         \
  BDF_render_string((char*)screen_address, x, y, bg_color, fg_color, str)      \

#define PRINT_STRING_BG_SJIS(utf8, str, fg_color, bg_color, x, y)              \
  BDF_render_mix((char*)screen_address, SCREEN_WIDTH, x, y, 0, bg_color, fg_color, str)      \

#define PRINT_STRING_BG_SJIS_GBA(utf8, str, fg_color, bg_color, x, y)          \
  BDF_render_mix(gba_screen_address-16*256-8, SCREEN_WIDTH, x, y, 2, bg_color, fg_color, str)\

#define PRINT_STRING_BG_UTF8(utf8, fg_color, bg_color, x, y)                   \
  BDF_render_mix((char*)screen_address, SCREEN_WIDTH, x, y, 0, bg_color, fg_color, utf8)     \
  
#define PRINT_STRING_GBA_UTF8(utf8, fg_color, bg_color, x, y)                   \
  BDF_render_mix((char*)(gba_screen_address -16*256 -8), SCREEN_WIDTH, x, y, 0, bg_color, fg_color, utf8)     \

// 基本カラーの設定
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
#define COLOR_INACTIVE_ITEM COLOR16(13, 20, 18)
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
/******************************************************************************
 *
 ******************************************************************************/
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

extern struct background back_ground;
extern struct gui_iconlist gui_icon_list[];

#if 0
struct gui_iconlist gui_icon_list[]= {
    //file system
    /* 00 */ {"filesys.bmp", 10, 10},
    /* 01 */ {"gbafile.bmp", 16, 16},
    /* 02 */ {"zipfile.bmp", 16, 16},
    /* 03 */ {"directory.bmp", 16, 16},
    /* 04 */ {"vscrollslider.bmp", 2, 103},
    /* 05 */ {"vscrollbar.bmp", 8, 13},
    /* 06 */ {"vscrolluparrow.bmp", 9, 9},
    /* 07 */ {"vscrolldwarrow.bmp", 9, 9},
    /* 08 */ {"visolator.bmp", 213, 3},
    /* 09 */ {"fileselected.bmp", 201, 21},
    //main menu
    /* 10 */ {"newgame.bmp", 70, 37},
    /* 11 */ {"resetgame.bmp", 98, 37},
    /* 12 */ {"returngame.bmp", 88, 37},
    /* 13 */ {"newgameselect.bmp", 70, 37},
    /* 14 */ {"resetgameselect.bmp", 98, 37},
    /* 15 */ {"returngameselect.bmp", 88, 37},
    /* 16 */ {"menuindex.bmp", 5, 7},
    /* 17 */ {"menuisolator.bmp", 98, 5},
    //sub-menu
    /* 18 */ {"subisolator.bmp", 221, 11}
                        };
#endif

#define ICON_FILEPATH       gui_icon_list[0]
#define ICON_GBAFILE        gui_icon_list[1]
#define ICON_ZIPFILE        gui_icon_list[2]
#define ICON_DIRECTORY      gui_icon_list[3]
#define ICON_VSCROL_SLIDER  gui_icon_list[4]
#define ICON_VSCROL_BAR     gui_icon_list[5]
#define ICON_VSCROL_UPAROW  gui_icon_list[6]
#define ICON_VSCROL_DWAROW  gui_icon_list[7]
#define ICON_FILE_ISOLATOR  gui_icon_list[8]
#define ICON_FILE_SELECT_BG gui_icon_list[9]
#define ICON_NEWGAME        gui_icon_list[10]
#define ICON_RESTGAME       gui_icon_list[11]
#define ICON_RETGAME        gui_icon_list[12]
#define ICON_NEWGAME_SLT    gui_icon_list[13]
#define ICON_RESTGAME_SLT   gui_icon_list[14]
#define ICON_RETGAME_SLT    gui_icon_list[15]
#define ICON_MENUINDEX      gui_icon_list[16]
#define ICON_MENU_ISOLATOR  gui_icon_list[17]
#define ICON_SUBM_ISOLATOR  gui_icon_list[18]
#define ICON_BINFILE        gui_icon_list[19]

/******************************************************************************
 *
 ******************************************************************************/
extern void print_string_center(u32 sy, u32 color, u32 bg_color, char *str);
extern void print_string_shadow_center(u32 sy, u32 color, char *str);
extern void hline(u32 sx, u32 ex, u32 y, u32 color);
extern void hline_alpha(u32 sx, u32 ex, u32 y, u32 color, u32 alpha);
extern void vline(u32 x, u32 sy, u32 ey, u32 color);
extern void vline_alpha(u32 x, u32 sy, u32 ey, u32 color, u32 alpha);
extern void box(u32 sx, u32 sy, u32 ex, u32 ey, u32 color);
extern void drawbox(u16 *screen_address, u32 sx, u32 sy, u32 ex, u32 ey, u32 color);
extern void boxfill(u32 sx, u32 sy, u32 ex, u32 ey, u32 color);
extern void drawboxfill(u16 *screen_address, u32 sx, u32 sy, u32 ex, u32 ey, u32 color);
extern void draw_selitem(u16 *screen_address, u32 x, u32 y, u32 color, u32 active);
extern void draw_message(u16 *screen_address, u16 *screen_bg, u32 sx, u32 sy, u32 ex, u32 ey,
                u32 color_fg);
extern void draw_string_vcenter(u16* screen_address, u32 sx, u32 sy, u32 width, 
        u32 color_fg, char *string);
extern u32 draw_hscroll_init(u16 *screen_address, u16 *buff_bg, u32 sx, u32 sy, u32 width, 
        u32 color_bg, u32 color_fg, char *string);
extern u32 draw_hscroll(u32 index, s32 scroll_val);
extern void draw_hscroll_over(u32 index);
extern void boxfill_alpha(u32 sx, u32 sy, u32 ex, u32 ey, u32 color, u32 alpha);
extern void init_progress(u32 total, char *text);
extern void update_progress(void);
extern void show_progress(char *text);
extern void scrollbar(u32 sx, u32 sy, u32 ex, u32 ey, u32 all,u32 view,u32 now);
extern u32 yesno_dialog(char *text);
extern u32 draw_yesno_dialog(u16 *screen_address, u32 sy, char *yes, char *no);
extern void msg_screen_init(const char *title);
extern void msg_screen_draw();
extern void msg_printf(const char *text, ...);
extern void msg_screen_clear(void);
extern void msg_set_text_color(u32 color);

extern int gui_init(u32 language_id);
extern int gui_change_icon(u32 language_id);
extern int show_background(void *screen, char *bgname);
extern void show_icon(u16* screen, struct gui_iconlist icon, u32 x, u32 y);
extern void show_Vscrollbar(char *screen, u32 x, u32 y, u32 part, u32 total);

extern void show_log();
extern void show_err();
#endif

