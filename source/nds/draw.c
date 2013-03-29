/* draw.c
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
//v1.1

/******************************************************************************
 * draw.cpp
 * basic program to draw some graphic
 ******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "ds2_malloc.h"
#include "ds2_cpu.h"
#include "bdf_font.h"
#include "gui.h"
#include "bitmap.h"
#include "draw.h"
#include "input.h"

#include "gu.h"

/******************************************************************************
 * macro definition
 ******************************************************************************/
#define progress_sx (screen_width2 - SCREEN_WIDTH / 3)  // Center -160/-80
#define progress_ex (screen_width2 + SCREEN_WIDTH / 3)  // Center +160/+80
#define progress_sy (screen_height2 + 3)                // Center +3
#define progress_ey (screen_height2 + 13)               // Center +13
#define yesno_sx    (screen_width2 - SCREEN_WIDTH / 3)  // Center -160/-80
#define yesno_ex    (screen_width2 + SCREEN_WIDTH / 3)  // Center +160/+80
#define yesno_sy    (screen_height2 + 3)                // Center +3
#define yesno_ey    (screen_height2 + 13)               // Center +13
#define progress_color COLOR16(15,15,15)

//#define progress_wait (0.5 * 1000 * 1000)
#define progress_wait (OS_TICKS_PER_SEC/2)				//0.5S

#define FONTS_HEIGHT    14

#define SCREEN_PITCH	256

#define VRAM_POS(screen, x, y)  ((unsigned short*)screen + (x + (y) * SCREEN_PITCH))

#define BOOTLOGO "SYSTEM/GUI/boot.bmp"
#define GUI_SOURCE_PATH "SYSTEM/GUI"
#define GUI_PIC_BUFSIZE 1024*512

char gui_picture[GUI_PIC_BUFSIZE];

struct gui_iconlist gui_icon_list[]= {
    //file system
    /* 00 */ {"zipfile", 16, 16, NULL},
    /* 01 */ {"directory", 16, 16, NULL},
    /* 02 */ {"gbafile", 16, 16, NULL},

	//title
	/* 03 */ {"stitle", 256, 33, NULL},
	//main menu
	/* 04 */ {"savo", 52, 52, NULL},
	/* 05 */ {"ssaveo", 52, 52, NULL},
	/* 06 */ {"stoolo", 52, 52, NULL},
	/* 07 */ {"scheato", 52, 52, NULL},
	/* 08 */ {"sother", 52, 52, NULL},
	/* 09 */ {"sexito", 52, 52, NULL},
	/* 10 */ {"smsel", 79, 15, NULL},
	/* 11 */ {"smnsel", 79, 15, NULL},

	/* 12 */ {"snavo", 52, 52, NULL},
	/* 13 */ {"snsaveo", 52, 52, NULL},
	/* 14 */ {"sntoolo", 52, 52, NULL},
	/* 15 */ {"sncheato", 52, 52, NULL},
	/* 16 */ {"snother", 52, 52, NULL},
	/* 17 */ {"snexito", 52, 52, NULL},

	/* 18 */ {"sunnof", 16, 16, NULL},
	/* 19 */ {"smaini", 85, 38, NULL},
	/* 20 */ {"snmaini", 85, 38, NULL},
	/* 21 */ {"smaybgo", 256, 192, NULL},

	/* 22 */ {"sticon", 29, 13, NULL},
	/* 23 */ {"ssubbg", 256, 192, NULL},

	/* 24 */ {"subsela", 245, 22, NULL},
	/* 25 */ {"sfullo", 12, 12, NULL},
	/* 26 */ {"snfullo", 12, 12, NULL},
	/* 27 */ {"semptyo", 12, 12, NULL},
	/* 28 */ {"snemptyo", 12, 12, NULL},
	/* 29 */ {"fdoto", 16, 16, NULL},
	/* 30 */ {"backo", 19, 13, NULL},
	/* 31 */ {"nbacko", 19, 13, NULL},
	/* 32 */ {"chtfile", 16, 15, NULL},
	/* 33 */ {"smsgfr", 193, 111, NULL},
	/* 34 */ {"sbutto", 76, 16, NULL}
                        };


/*
*	Drawing string aroud center
*/
void print_string_center(void* screen_addr, u32 sy, u32 color, u32 bg_color, char *str)
{
	int width = 0;//fbm_getwidth(str);
	u32 sx = (SCREEN_WIDTH - width) / 2;

	PRINT_STRING_BG(screen_addr, str, color, bg_color, sx, sy);
}

/*
*	Drawing string with shadow around center
*/
void print_string_shadow_center(void* screen_addr, u32 sy, u32 color, char *str)
{
	int width = 0;//fbm_getwidth(str);
	u32 sx = (SCREEN_WIDTH - width) / 2;

	PRINT_STRING_SHADOW(screen_addr, str, color, sx, sy);
}

/*
*	Drawing horizontal line
*/
void drawhline(void* screen_addr, u32 sx, u32 ex, u32 y, u32 color)
{
	u32 x;
	u32 width  = (ex - sx) + 1;
	u16 *dst = VRAM_POS(screen_addr, sx, y);

	for (x = 0; x < width; x++)
		*dst++ = (u16)color;
}

/*
*	Drawing vertical line
*/
void drawvline(void* screen_addr, u32 x, u32 sy, u32 ey, u32 color)
{
	int y;
	int height = (ey - sy) + 1;
	u16 *dst = VRAM_POS(screen_addr, x, sy);

	for (y = 0; y < height; y++)
	{
		*dst = (u16)color;
		dst += SCREEN_PITCH;
	}
}

/*
*	Drawing rectangle
*/
void drawbox(void* screen_addr, u32 sx, u32 sy, u32 ex, u32 ey, u32 color)
{
	drawhline(screen_addr, sx, ex - 1, sy, color);
	drawvline(screen_addr, ex, sy, ey - 1, color);
	drawhline(screen_addr, sx + 1, ex, ey, color);
	drawvline(screen_addr, sx, sy + 1, ey, color);
}

/*
*	Filling a rectangle
*/
void drawboxfill(void* screen_addr, u32 sx, u32 sy, u32 ex, u32 ey, u32 color)
{
	u32 x, y;
	u32 width  = (ex - sx) + 1;
	u32 height = (ey - sy) + 1;
	u16 *dst = VRAM_POS(screen_addr, sx, sy);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			dst[x + y * SCREEN_PITCH] = (u16)color;
		}
	}
}

/*
*	Drawing a selection item
- active    0 not fill
-           1 fill with gray
-           2 fill with color
-           3 fill with color and most brithness
- color     0 Red
-           1 Green
-           2 Blue
------------------------------------------------------*/
void draw_selitem(void* screen_addr, u32 x, u32 y, u32 color, u32 active)
{
    u32 size;
    u32 color0, color1, color2, color3;

    size= 10;

    switch(active)
    {
        case 1:
            color0 = COLOR16(12, 12, 12);
            color1 = COLOR16(2, 2, 2);
            color2 = COLOR16(7, 7, 7);
            color3 = COLOR16(22, 22, 22);
          break;
        case 2:
            switch(color)
            {
                case 0: //Red
                    color0 = COLOR16(12, 12, 12);
                    color1 = COLOR16(8, 0, 0);
                    color2 = COLOR16(16, 0, 0);
                    color3 = COLOR16(24, 0, 0);
                  break;
                case 1: //Green
                    color0 = COLOR16(12, 12, 12);
                    color1 = COLOR16(0, 8, 0);
                    color2 = COLOR16(0, 16, 0);
                    color3 = COLOR16(0, 24, 0);
                  break;
                case 2: //Blue
                    color0 = COLOR16(12, 12, 12);
                    color1 = COLOR16(0, 0, 8);
                    color2 = COLOR16(0, 0, 16);
                    color3 = COLOR16(0, 0, 24);
                  break;
                default:
                    color0 = COLOR16(12, 12, 12);
                    color1 = COLOR16(0, 8, 0);
                    color2 = COLOR16(0, 16, 0);
                    color3 = COLOR16(0, 24, 0);
                  break;
            }
          break;
        case 3:
            switch(color)
            {
                case 0: //Red
                    color0 = COLOR16(31, 31, 31);
                    color1 = COLOR16(16, 0, 0);
                    color2 = COLOR16(22, 0, 0);
                    color3 = COLOR16(31, 0, 0);
                  break;
                case 1: //Green
                    color0 = COLOR16(31, 31, 31);
                    color1 = COLOR16(0, 16, 0);
                    color2 = COLOR16(0, 22, 0);
                    color3 = COLOR16(0, 31, 0);
                  break;
                case 2: //Blue
                    color0 = COLOR16(31, 31, 31);
                    color1 = COLOR16(0, 0, 16);
                    color2 = COLOR16(0, 0, 22);
                    color3 = COLOR16(0, 0, 31);
                  break;
                default:
                    color0 = COLOR16(31, 31, 31);
                    color1 = COLOR16(0, 16, 0);
                    color2 = COLOR16(0, 22, 0);
                    color3 = COLOR16(0, 31, 0);
                  break;
            }
          break;
        default:
            color0= COLOR16(18, 18, 18);
            color1= color2= color3= COLOR16(18, 18, 18);
          break;
    }

    drawbox(screen_addr, x, y, x+size-1, y+size-1, color0);

    if(active >0)
    {
        drawbox(screen_addr, x+1, y+1, x+size-2, y+size-2, color1);
        drawbox(screen_addr, x+2, y+2, x+size-3, y+size-3, color2);
        drawboxfill(screen_addr, x+3, y+3, x+size-4, y+size-4, color3);
    }
}

/*
*	Drawing message box
*	Note if color_fg is transparent, screen_bg can't be transparent
*/
void draw_message(void* screen_addr, u16 *screen_bg, u32 sx, u32 sy, u32 ex, u32 ey,
        u32 color_fg)
{
    if(!(color_fg & 0x8000))
    {
//        drawbox(screen_addr, sx, sy, ex, ey, COLOR16(12, 12, 12));
//        drawboxfill(screen_addr, sx+1, sy+1, ex-1, ey-1, color_fg);
		show_icon(screen_addr, &ICON_MSG, (NDS_SCREEN_WIDTH - ICON_MSG.x) / 2, (NDS_SCREEN_HEIGHT - ICON_MSG.y) / 2);
    }
    else
    {
        u16 *screenp, *screenp1;
        u32 width, height, i, k;
        u32 tmp, tmp1, tmp2;
        u32 r, g, b;

        width= ex-sx;
        height= ey-sy;
        r= ((color_fg >> 10) & 0x1F) * 6/7;
        g= ((color_fg >> 5) & 0x1F) * 6/7;
        b= (color_fg & 0x1F) * 6/7;
        for(k= 0; k < height; k++)
        {
            screenp = VRAM_POS(screen_addr, sx, sy+k);
            screenp1 = screen_bg + sx + (sy + k) * SCREEN_PITCH;
            for(i= 0; i < width; i++)
            {
                tmp = *screenp1++;
                tmp1 = ((tmp >> 10) & 0x1F) *1/7 + r;
                tmp2 = (tmp1 > 31) ? 31 : tmp1;
                tmp1 = ((tmp >> 5) & 0x1F) *1/7 + g;
                tmp2 = (tmp2 << 5) | ((tmp1 > 31) ? 31 : tmp1);
                tmp1 = (tmp & 0x1F) *1/7 + b;
                tmp2 = (tmp2 << 5) | ((tmp1 > 31) ? 31 : tmp1);
                *screenp++ = tmp2;
            }
        }
    }
}

/*
*	Drawing string horizontal center aligned
*/
void draw_string_vcenter(void* screen_addr, u32 sx, u32 sy, u32 width, u32 color_fg, char *string)
{
    u32 x, num, i, m;
    u16 *screenp;
    u16 unicode[256];

    num= 0;
    while(*string)
    {
        string= utf8decode(string, unicode+num);
        num++;
    }

    if(num== 0) return;

    screenp = (unsigned short*)screen_addr + sx + sy*SCREEN_WIDTH;
    i= 0;
    while(i < num)
    {
        m= BDF_cut_unicode(&unicode[i], num-i, width, 1);
        x= (width - BDF_cut_unicode(&unicode[i], m, 0, 3)) / 2;
        while(m--)
        {
            x += BDF_render16_ucs(screenp+x, SCREEN_WIDTH, 0, COLOR_TRANS, 
                color_fg, unicode[i++]);
        }
        if (i < num && (unicode[i] == 0x0D || unicode[i] == 0x0A))
            i++;
	else {
            while (i < num && (unicode[i] == ' ')) i++;
        }
        screenp += FONTS_HEIGHT * SCREEN_WIDTH;
    }
}

/*------------------------------------------------------
	Drawing a scroll string
------------------------------------------------------*/
//limited
// < 256 Unicodes
// width < 256+128
//#define MAX_SCROLL_STRING   8

/*------------------------------------------------------
- scroll_val    < 0     scroll toward left
-               > 0     scroll toward right
------------------------------------------------------*/
struct scroll_string_info{
    u16     *screenp;
    u32     sx;
    u32     sy;
    u32     width;
    u32     height;
    u16     *unicode;
    u32     color_bg;
    u32     color_fg;
    u16     *buff_fonts;
    u32     buff_width;
    u16     *buff_bg;
    s32     pos_pixel;
    u32     str_start;
    u32     str_end;
    u32     str_len;
};

static struct scroll_string_info    scroll_strinfo[MAX_SCROLL_STRING];
static u32  scroll_string_num= 0;

/*
 * Initialises a text scroller to display a certain string.
 * Input assertions: sx + width < NDS_SCREEN_WIDTH &&
 *   sy + [text height] < NDS_SCREEN_HEIGHT && string != NULL &&
 *   screen_addr != NULL.
 * Input: 'screen_addr', the address of the upper-left corner of the screen.
 *        'sx' and 'sy', the X and Y coordinates of the upper-left corner of
 *          the text.
 *        'width', the width of the scroller's viewport.
 *        'color_bg', the RGB15 color of the background around the text, or
 *          COLOR_TRANS for transparency.
 *        'color_fg', the RGB15 color of the text.
 *        'string', the text to be scrolled, encoded as UTF-8.
 * Output: the scroller's handle, to be used to scroll the text in
 *   draw_hscroll.
 */
u32 hscroll_init(void* screen_addr, u32 sx, u32 sy, u32 width, 
        u32 color_bg, u32 color_fg, char *string)
{
    u32 index, x, textWidth, num, len, i;
    u16 *unicode, *screenp;

    // 1. Which scroller should we use for this request?
    for(i= 0; i < MAX_SCROLL_STRING; i++)
    {
        if(scroll_strinfo[i].screenp == NULL)
            break;
    }

    if(i >= MAX_SCROLL_STRING)
        return -1;

    index= i;

    // 2. Convert to Unicode while calculating the width of the text.
    unicode= (u16*)malloc(strlen(string)*sizeof(u16));
    if(unicode == NULL)
    {
        scroll_strinfo[index].str_len = 0;
        return -3;
    }

    num= 0;
    textWidth = 0;
    while(*string)
    {
        string= utf8decode(string, unicode+num);
        if(unicode[num] != 0x0D && unicode[num] != 0x0A) {
            textWidth += BDF_width16_ucs(unicode[num]);
            num++;
        }
    }
    if (textWidth < width)
        textWidth = width;

    // 3. Allocate a rectangle of pixels for drawing the entire text into.
    screenp= (u16*)malloc(textWidth*FONTS_HEIGHT*sizeof(u16));
    if(screenp == NULL)
    {
        scroll_strinfo[index].str_len = 0;
        free((void*)unicode);
        return -2;
    }

    if(color_bg == COLOR_TRANS)
        memset(screenp, 0, textWidth*FONTS_HEIGHT*sizeof(u16));

    scroll_string_num += 1;
    scroll_strinfo[index].screenp = (unsigned short*)screen_addr;
    scroll_strinfo[index].sx= sx;
    scroll_strinfo[index].sy= sy;
    scroll_strinfo[index].color_bg= color_bg;
    scroll_strinfo[index].color_fg= color_fg;
    scroll_strinfo[index].width= width;
    scroll_strinfo[index].height= FONTS_HEIGHT;
    scroll_strinfo[index].unicode= unicode;
    scroll_strinfo[index].buff_fonts= screenp;
    scroll_strinfo[index].buff_bg= 0;
    scroll_strinfo[index].buff_width= textWidth;
    scroll_strinfo[index].pos_pixel= 0;
    scroll_strinfo[index].str_start= 0;
    scroll_strinfo[index].str_end= len-1;

    scroll_strinfo[index].str_len= num;
    if(num == 0)
        return index; // (1. Which scroller?)

    // 4. Render text into the allocation.
    i= 0;
    x= 0;
    while(i < num)
    {
        x += BDF_render16_ucs(screenp + x, textWidth, 0, color_bg, color_fg, unicode[i++]);
    }

    return index; // (1. Which scroller?)
}

u32 draw_hscroll_init(void* screen_addr, u32 sx, u32 sy, u32 width, 
        u32 color_bg, u32 color_fg, char *string)
{
	u32 ret = hscroll_init(screen_addr, sx, sy, width, color_bg, color_fg, string);

	draw_hscroll(ret, 0 /* stay on the left */);

	return ret;
}

/*
 * Scrolls an initialised scroller's text.
 * A scroller is never allowed to go past the beginning of the text when
 * scrolling to the left, or to go past the end when scrolling to the right.
 * Input assertions: index was returned by a previous call to
 *   draw_hscroll_init and not used in a call to draw_hscroll_over.
 * Input: 'index', the scroller's handle.
 *        'scroll_val', the number of pixels to scroll. The sign affects the
 *          direction. If scroll_val > 0, the scroller's viewport is moved to
 *          the left; if < 0, the scroller's viewport is moved to the right.
 * Output: the number of pixels still available to scroll in the direction
 *   specified by the sign of 'scroll_val'.
 *
 * Example: (assume each letter is 1 pixel; this won't be true in reality)
 *           [some lengthy text shown in ]         |
 * val -5 -> |    [lengthy text shown in a scr]xxxxx -> to right, returns 5
 * val -5 -> |         [hy text shown in a scroller] -> to right, returns 0
 * val  3 -> xxxxxxx[ngthy text shown in a scrol]  | -> to left,  returns 7
 * val  3 -> xxxx[ lengthy text shown in a sc]     | -> to left,  returns 4
 */
u32 draw_hscroll(u32 index, s32 scroll_val)
{
    u32 color_bg, color_fg, i, width, height;
    s32 xoff;

    if(index >= MAX_SCROLL_STRING) return -1;
    if(scroll_strinfo[index].screenp == NULL) return -2;
    if(scroll_strinfo[index].str_len == 0) return 0;
    
    width= scroll_strinfo[index].width;
    height= scroll_strinfo[index].height;
    color_bg= scroll_strinfo[index].color_bg;
    color_fg= scroll_strinfo[index].color_fg;

    // 1. Shift the scroller.
    scroll_strinfo[index].pos_pixel -= scroll_val;
    if (scroll_strinfo[index].pos_pixel < 0) // Reached the beginning
        scroll_strinfo[index].pos_pixel = 0;
    else if (scroll_strinfo[index].pos_pixel > scroll_strinfo[index].buff_width - width) // Reached the end
        scroll_strinfo[index].pos_pixel = scroll_strinfo[index].buff_width - width;

    // 2. Draw the scroller's text at its new position.
    u32 x, sx, sy, pixel;
    u16 *screenp, *screenp1;

    sx= scroll_strinfo[index].sx;
    sy= scroll_strinfo[index].sy;

    if(color_bg == COLOR_TRANS)
    {
        for(i= 0; i < height; i++)
        {
            screenp= scroll_strinfo[index].screenp + sx + (sy + i) * SCREEN_WIDTH;
            screenp1= scroll_strinfo[index].buff_fonts + scroll_strinfo[index].pos_pixel + i*scroll_strinfo[index].buff_width;
            for(x= 0; x < width; x++)
            {
                pixel= *screenp1++;
				if(pixel) *screenp = pixel;
				screenp ++;
            }
        }
    }
    else
    {
        for(i= 0; i < height; i++)
        {
            screenp= scroll_strinfo[index].screenp + sx + (sy + i) * SCREEN_WIDTH;
            screenp1= scroll_strinfo[index].buff_fonts + scroll_strinfo[index].pos_pixel + i*scroll_strinfo[index].buff_width;
            for(x= 0; x < width; x++)
                *screenp++ = *screenp1++;
        }
    }

    // 3. Return how many more pixels we can scroll in the same direction.
    if(scroll_val > 0)
        // Scrolling to the left: Return the number of pixels we can still go
        // to the left.
        return scroll_strinfo[index].pos_pixel;
    else
        // Scrolling to the right: Return the number of pixels we can still go
        // to the right.
        return scroll_strinfo[index].buff_width - scroll_strinfo[index].pos_pixel - width;
}

void draw_hscroll_over(u32 index)
{
    if(scroll_strinfo[index].screenp== NULL)
        return;

    if(index < MAX_SCROLL_STRING && scroll_string_num > 0)
    {
        if(scroll_strinfo[index].unicode)
        {
            free((void*)scroll_strinfo[index].unicode);
            scroll_strinfo[index].unicode= NULL;
        }
        if(scroll_strinfo[index].buff_fonts)
        {
            free((void*)scroll_strinfo[index].buff_fonts);
            scroll_strinfo[index].buff_fonts= NULL;
        }
        scroll_strinfo[index].screenp= NULL;
        scroll_strinfo[index].str_len= 0;
    
        scroll_string_num -=1;
    }
}

/*
*	Drawing dialog
*/
void draw_dialog(void* screen_addr, u32 sx, u32 sy, u32 ex, u32 ey)
{
	drawboxfill(screen_addr, sx + 5, sy + 5, ex + 5, ey + 5, COLOR_DIALOG_SHADOW);

	drawhline(screen_addr, sx, ex - 1, sy, COLOR_FRAME);
	drawvline(screen_addr, ex, sy, ey - 1, COLOR_FRAME);
	drawhline(screen_addr, sx + 1, ex, ey, COLOR_FRAME);
	drawvline(screen_addr, sx, sy + 1, ey, COLOR_FRAME);

	sx++;
	ex--;
	sy++;
	ey--;

	drawhline(screen_addr, sx, ex - 1, sy, COLOR_FRAME);
	drawvline(screen_addr, ex, sy, ey - 1, COLOR_FRAME);
	drawhline(screen_addr, sx + 1, ex, ey, COLOR_FRAME);
	drawvline(screen_addr, sx, sy + 1, ey, COLOR_FRAME);

	sx++;
	ex--;
	sy++;
	ey--;

	drawboxfill(screen_addr, sx, sy, ex, ey, COLOR_DIALOG);
}

/*
*	Draw yes or no dialog
*/
u32 draw_yesno_dialog(enum SCREEN_ID screen, u32 sy, char *yes, char *no)
{
    u16 unicode[8];
    u32 len, width, box_width, i;
    char *string;
	void* screen_addr;

    len= 0;
    string= yes;
    while(*string)
    {
        string= utf8decode(string, &unicode[len]);
        if(unicode[len] != 0x0D && unicode[len] != 0x0A)
        {
            if(len < 8) len++;
            else break;
        }
    }
    width= BDF_cut_unicode(unicode, len, 0, 3);
    
    len= 0;
    string= no;
    while(*string)
    {
        string= utf8decode(string, &unicode[len]);
        if(unicode[len] != 0x0D && unicode[len] != 0x0A)
        {
            if(len < 8) len++;
            else    break;
        }
    }
    i= BDF_cut_unicode(unicode, len, 0, 3);

    if(width < i)   width= i;
    box_width= 64;
    if(box_width < (width +6)) box_width = width +6;

	if(screen & UP_MASK)
		screen_addr = up_screen_addr;
	else
		screen_addr = down_screen_addr;

	sy = (NDS_SCREEN_HEIGHT + ICON_MSG.y) / 2 - 8 - ICON_BUTTON.y;

	u32 left_sx = NDS_SCREEN_WIDTH / 2 - 8 - ICON_BUTTON.x,
	    right_sx = NDS_SCREEN_WIDTH / 2 + 8;

	show_icon((unsigned short*)screen_addr, &ICON_BUTTON, left_sx, sy);
    draw_string_vcenter((unsigned short*)screen_addr, left_sx + 2, sy, ICON_BUTTON.x - 4, COLOR_WHITE, yes);

	show_icon((unsigned short*)screen_addr, &ICON_BUTTON, right_sx, sy);
    draw_string_vcenter((unsigned short*)screen_addr, right_sx + 2, sy, ICON_BUTTON.x - 4, COLOR_WHITE, no);

	ds2_flipScreen(screen, 2);

    gui_action_type gui_action = CURSOR_NONE;
    while((gui_action != CURSOR_SELECT)  && (gui_action != CURSOR_BACK))
    {
        gui_action = get_gui_input();
	if (gui_action == CURSOR_TOUCH)
	{
		struct key_buf inputdata;
		ds2_getrawInput(&inputdata);
		// Turn it into a SELECT (A) or BACK (B) if the button is touched.
		if (inputdata.y >= sy && inputdata.y < sy + ICON_BUTTON.y)
		{
			if (inputdata.x >= left_sx && inputdata.x < left_sx + ICON_BUTTON.x)
				gui_action = CURSOR_SELECT;
			else if (inputdata.x >= right_sx && inputdata.x < right_sx + ICON_BUTTON.x)
				gui_action = CURSOR_BACK;
		}
	}
	mdelay(16);
    }

    if (gui_action == CURSOR_SELECT)
        return 1;
    else
        return 0;
}

/*
*	Draw hotkey dialog
*	Returns DS keys pressed, as in ds2io.h.
*/
u32 draw_hotkey_dialog(enum SCREEN_ID screen, u32 sy, char *clear, char *cancel)
{
    u16 unicode[8];
    u32 len, width, box_width, i;
    char *string;
	void* screen_addr;

    len= 0;
    string= clear;
    while(*string)
    {
        string= utf8decode(string, &unicode[len]);
        if(unicode[len] != 0x0D && unicode[len] != 0x0A)
        {
            if(len < 8) len++;
            else break;
        }
    }
    width= BDF_cut_unicode(unicode, len, 0, 3);
    
    len= 0;
    string= cancel;
    while(*string)
    {
        string= utf8decode(string, &unicode[len]);
        if(unicode[len] != 0x0D && unicode[len] != 0x0A)
        {
            if(len < 8) len++;
            else    break;
        }
    }
    i= BDF_cut_unicode(unicode, len, 0, 3);

    if(width < i)   width= i;
    box_width= 64;
    if(box_width < (width +6)) box_width = width +6;

	if(screen & UP_MASK)
		screen_addr = up_screen_addr;
	else
		screen_addr = down_screen_addr;

    i= SCREEN_WIDTH/2 - box_width - 2;
	show_icon((unsigned short*)screen_addr, &ICON_BUTTON, 49, 128);
    draw_string_vcenter((unsigned short*)screen_addr, 51, 130, 73, COLOR_WHITE, clear);

    i= SCREEN_WIDTH/2 + 3;
	show_icon((unsigned short*)screen_addr, &ICON_BUTTON, 136, 128);
    draw_string_vcenter((unsigned short*)screen_addr, 138, 130, 73, COLOR_WHITE, cancel);

	ds2_flipScreen(screen, DOWN_SCREEN_UPDATE_METHOD);

	// This function has been started by a key press. Wait for it to end.
	struct key_buf inputdata;
	do {
		mdelay(1);
		ds2_getrawInput(&inputdata);
	} while (inputdata.key != 0);

	// While there are no keys pressed, wait for keys.
	do {
		mdelay(1);
		ds2_getrawInput(&inputdata);
	} while (inputdata.key == 0);

	// Now, while there are keys pressed, keep a tally of keys that have
	// been pressed. (IGNORE TOUCH AND LID! Otherwise, closing the lid or
	// touching to get to the menu will do stuff the user doesn't expect.)
	u32 TotalKeys = 0;

	do {
		TotalKeys |= inputdata.key & ~(KEY_TOUCH | KEY_LID);
		// If there's a touch on either button, turn it into a
		// clear (A) or cancel (B) request.
		if (inputdata.key & KEY_TOUCH)
		{
			if (inputdata.y >= 128 && inputdata.y < 128 + ICON_BUTTON.y)
			{
				if (inputdata.x >= 49 && inputdata.x < 49 + ICON_BUTTON.x)
					return KEY_A;
				else if (inputdata.x >= 136 && inputdata.x < 136 + ICON_BUTTON.x)
					return KEY_B;
			}
		}
		mdelay(1);
		ds2_getrawInput(&inputdata);
	} while (inputdata.key != 0 || TotalKeys == 0);

	return TotalKeys;
}

/**** PROGRESS BAR FUNCTIONS are present for gpSP but empty ****/

//	progress bar initialize
void init_progress(enum SCREEN_ID screen, u32 total, char *text)
{
}

//	update progress bar
void update_progress(void)
{
}

//	display progress string
void show_progress(char *text)
{
}

/*
*	Drawing scroll bar
*/
#define SCROLLBAR_COLOR1 COLOR16( 0, 2, 8)
#define SCROLLBAR_COLOR2 COLOR16(15,15,15)

void scrollbar(void* screen_addr, u32 sx, u32 sy, u32 ex, u32 ey, u32 all, u32 view, u32 now)
{
	u32 scrollbar_sy;
	u32 scrollbar_ey;
	u32 len;

	len = ey - sy - 2;

	if ((all != 0) && (all > now))
		scrollbar_sy = (u32)((float)len * (float)now / (float)all) +sy + 1;
	else
		scrollbar_sy = sy + 1;

	if ((all > (now + view)) && (all != 0))
		scrollbar_ey = (u32)((float)len * (float)(now + view) / (float)all ) + sy + 1;
	else
		scrollbar_ey = len + sy + 1;

	drawbox(screen_addr, sx, sy, ex, ey, COLOR_BLACK);
	drawboxfill(screen_addr, sx + 1, sy + 1, ex - 1, ey - 1, SCROLLBAR_COLOR1);
	drawboxfill(screen_addr, sx + 1, scrollbar_sy, ex - 1, scrollbar_ey, SCROLLBAR_COLOR2);
}

#if 0
static struct background back_ground = {{0}, {0}};

int show_background(void *screen, char *bgname)
{
    int ret;

    if(strcasecmp(bgname, back_ground.bgname))
    {
        char *buff, *src;
        int x, y;        
        unsigned short *dst;
		unsigned int type;

        buff= (char*)malloc(256*192*4);

        ret= BMP_read(bgname, buff, 256, 192, &type);
        if(ret != BMP_OK)
        {
            free((int)buff);
            return(-1);
        }

        src = buff;

		if(type ==2)		//2 bytes per pixel
		{
			unsigned short *pt;
			pt = (unsigned short*)buff;
//			memcpy((char*)back_ground.bgbuffer, buff, 256*192*2);
			dst=(unsigned short*)back_ground.bgbuffer;
	        for(y= 0; y< 192; y++)
	        {
    	        for(x= 0; x< 256; x++)
    	        {
    	            *dst++= RGB16_15(pt);
    	            pt += 1;
    	        }
    	    }
		}
		else if(type ==3)	//3 bytes per pixel
		{
			dst=(unsigned short*)back_ground.bgbuffer;
	        for(y= 0; y< 192; y++)
	        {
    	        for(x= 0; x< 256; x++)
    	        {
    	            *dst++= RGB24_15(buff);
    	            buff += 3;
    	        }
    	    }
		}
		else
		{
            free((int)buff);
            return(-1);
		}

        free((int)src);
        strcpy(back_ground.bgname, bgname);
    }

    memcpy((char*)screen, back_ground.bgbuffer, 256*192*2);

    return 0;    
}
#endif

/*
*	change GUI icon
*/
int gui_change_icon(u32 language_id)
{
    char path[128];
    char fpath[8];
    u32  i, item;
    int err, ret; 
    char *buff, *src;
    u32 x, y;
    char *icondst;
	unsigned int type;

    item= sizeof(gui_icon_list)/16;
    buff= (char*)malloc(256*192*4);
    if(buff == NULL)
        return -1;

    ret= 0;
    icondst= gui_picture;

    sprintf(fpath, "%d.bmp", language_id);
    for(i= 0; i< item; i++)
    {
        sprintf(path, "%s/%s/%s%s", main_path, GUI_SOURCE_PATH, gui_icon_list[i].iconname, fpath);

	    src= buff; 
        err= BMP_read(path, src, gui_icon_list[i].x, gui_icon_list[i].y, &type);
        if(err != BMP_OK)
        {
            sprintf(path, "%s/%s/%s%s", main_path, GUI_SOURCE_PATH, gui_icon_list[i].iconname, ".bmp");
            err= BMP_read(path, src, gui_icon_list[i].x, gui_icon_list[i].y, &type);
        }

		if(type < 2)	//< 1 byte per pixels, not surpport now
		{
            if(!ret) ret = -(i+1);
            gui_icon_list[i].iconbuff= NULL;
			continue;
		}

        if(err == BMP_OK)
        {
            unsigned short *dst;

            if(icondst >= gui_picture + GUI_PIC_BUFSIZE -1)
            {
                ret = 1;
                break;
            }

			if(type == 2)
			{
				unsigned short *pt;
				pt = (unsigned short*)src;
//				memcpy((char*)icondst, src, 256*192*2);
				dst = (unsigned short*)icondst;
    	        for(y= 0; y< gui_icon_list[i].y; y++)
    	        {
    	            for(x= 0; x < gui_icon_list[i].x; x++)
    	            {
    	                *dst++ = RGB16_15(pt);
    	                pt += 1;
    	            }
	            }
			}

			if(type == 3)
			{
				dst = (unsigned short*)icondst;
    	        for(y= 0; y< gui_icon_list[i].y; y++)
    	        {
    	            for(x= 0; x < gui_icon_list[i].x; x++)
    	            {
    	                *dst++ = RGB24_15(src);
    	                src += 3;
    	            }
	            }
            }

            gui_icon_list[i].iconbuff= icondst;
            icondst += gui_icon_list[i].x*gui_icon_list[i].y*2;
        }
        else
        {
            if(!ret) ret = -(i+1);
            gui_icon_list[i].iconbuff= NULL;
        }
    }

    free((void*)buff);
//printf("icon_buf: %08x\n", icondst - gui_picture );
    return ret;
}

/*************************************************************/
int icon_init(u32 language_id)
{
    u32  i;
    int ret;

//Initial draw_scroll_string function
    scroll_string_num = 0;
    for(i= 0; i < MAX_SCROLL_STRING; i++)
    {
        scroll_strinfo[i].unicode= NULL;
        scroll_strinfo[i].buff_fonts= NULL;
        scroll_strinfo[i].screenp = NULL;
        scroll_strinfo[i].str_len = 0;
    }

    ret= gui_change_icon(language_id);

//#define GUI_INIT_DEBUG
#if 0
    item= sizeof(gui_icon_list)/12;
    buff= (char*)malloc(256*192*4);
    src= buff;
    ret= 0;
    icondst= gui_picture;

    for(i= 0; i< item; i++)
    {
        sprintf(path, "%s\\%s", GUI_SOURCE_PATH, gui_icon_list[i].iconname);
        
        err= BMP_read(path, buff, gui_icon_list[i].x, gui_icon_list[i].y);
        if(err == BMP_OK)
        {
            unsigned short *dst;
            
            if(icondst >= gui_picture + GUI_PIC_BUFSIZE -1)
            {
                ret = 1;
#ifdef GUI_INIT_DEBUG
                printf("GUI Initial overflow\n");
#endif
                break;
            }

            for(y= 0; y< gui_icon_list[i].y; y++)
            {
                dst= (unsigned short*)(icondst + (gui_icon_list[i].y - y -1)*gui_icon_list[i].x*2);
                for(x= 0; x < gui_icon_list[i].x; x++)
                {
                    *dst++ = RGB24_15(buff);
                    buff += 4;
                }
            }                
            
            gui_icon_list[i].iconname= icondst;
            icondst += gui_icon_list[i].x*gui_icon_list[i].y*2;
        }
        else
        if(!ret)
        {
            ret = -(i+1);
            gui_icon_list[i].iconname= NULL;
#ifdef GUI_INIT_DEBUG
            printf("GUI Initial: %s not open\n", path);
#endif
        }
    }

#ifdef GUI_INIT_DEBUG
    printf("GUI buff %d\n", icondst - gui_picture);
#endif

    free((int)src);
#endif

    return ret;
}

/*************************************************************/
void show_icon(void* screen, struct gui_iconlist* icon, u32 x, u32 y)
{
    u32 i, k;
    unsigned short *src, *dst;

    src= (unsigned short*)icon->iconbuff;
    dst = (unsigned short*)screen + y*NDS_SCREEN_WIDTH + x;
	if(NULL == src) return;	//The icon may initialized failure

	if (icon->x == NDS_SCREEN_WIDTH && icon->y == NDS_SCREEN_HEIGHT && x == 0 && y == 0)
	{
		// Don't support transparency for a background.
		memcpy(dst, src, NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT * sizeof(u16));
	}
	else
	{
		for(i= 0; i < icon->y; i++)
		{
			for(k= 0; k < icon->x; k++)
			{
				if(0x03E0 != *src) dst[k]= *src;
				src++;
			}

			dst += NDS_SCREEN_WIDTH;
		}
	}
}

/*************************************************************/
void show_Vscrollbar(char *screen, u32 x, u32 y, u32 part, u32 total)
{
//    show_icon((u16*)screen, ICON_VSCROL_UPAROW, x+235, y+55);
//    show_icon((u16*)screen, ICON_VSCROL_DWAROW, x+235, y+167);
//    show_icon((u16*)screen, ICON_VSCROL_SLIDER, x+239, y+64);
//    if(total <= 1)
//        show_icon((u16*)screen, ICON_VSCROL_BAR, x+236, y+64);
//    else
//        show_icon((u16*)screen, ICON_VSCROL_BAR, x+236, y+64+(part*90)/(total-1));
}

/*
*	display a log
*/
void show_log(void* screen_addr)
{
    char tmp_path[MAX_PATH];
	char *buff;
	int x, y;        
	unsigned short *dst;
	unsigned int type;
	int ret;

    sprintf(tmp_path, "%s/%s", main_path, BOOTLOGO);
	buff= (char*)malloc(256*192*4);

	ret= BMP_read(tmp_path, buff, 256, 192, &type);
	if(ret != BMP_OK)
	{
		free((void*)buff);
		return;
	}

	if(type ==2)		//2 bytes per pixel
	{
		unsigned short *pt;
		pt = (unsigned short*)buff;
		dst=(unsigned short*)screen_addr;
		for(y= 0; y< 192; y++)
		{
			for(x= 0; x< 256; x++)
			{
				*dst++= RGB16_15(pt);
				pt += 1;
			}
		}
	}
	else if(type ==3)	//3 bytes per pixel
	{
		unsigned char *pt;
		pt = (unsigned char*)buff;
		dst=(unsigned short*)screen_addr;
		for(y= 0; y< 192; y++)
		{
			for(x= 0; x< 256; x++)
			{
				*dst++= RGB24_15(pt);
				pt += 3;
			}
		}
	}

	free((void*)buff);
}

/*************************************************************/
void err_msg(enum SCREEN_ID screen, char *msg)
{
	// A wild console appeared!
	ConsoleInit(RGB15(31, 31, 31), RGB15(0, 0, 0), UP_SCREEN, 2);
	printf(msg);
}
