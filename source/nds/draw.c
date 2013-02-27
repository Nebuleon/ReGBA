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
		show_icon(screen_addr, &ICON_MSG, 34, 48);
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
        if(unicode[num] != 0x0D && unicode[num] != 0x0A) num++;
    }

    if(num== 0) return;

    i= BDF_cut_unicode(unicode, num, width, 1);
    if(i == num)
    {
        x= BDF_cut_unicode(unicode, num, 0, 3);
        sx += (width - x)/2;
    }

    screenp = (unsigned short*)screen_addr + sx + sy*SCREEN_WIDTH;
    i= 0;
    while(i < num)
    {
        m= BDF_cut_unicode(&unicode[i], num-i, width, 1);
        x= 0;
        while(m--)
        {
            x += BDF_render16_ucs(screenp+x, SCREEN_WIDTH, 0, COLOR_TRANS, 
                color_fg, unicode[i++]);
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

u32 draw_hscroll_init(void* screen_addr, u32 sx, u32 sy, u32 width, 
        u32 color_bg, u32 color_fg, char *string)
{
    u32 index, x, num, len, i;
    u16 *unicode, *screenp;

    for(i= 0; i < MAX_SCROLL_STRING; i++)
    {
        if(scroll_strinfo[i].screenp == NULL)
            break;
    }

    if(i >= MAX_SCROLL_STRING)
        return -1;

    index= i;
    screenp= (u16*)malloc((256+128)*FONTS_HEIGHT*2);
    if(screenp == NULL)
    {
        scroll_strinfo[index].str_len = 0;
        return -2;
    }

    unicode= (u16*)malloc(256*2);
    if(unicode == NULL)
    {
        scroll_strinfo[index].str_len = 0;
        free((void*)screenp);
        return -3;
    }

    if(color_bg == COLOR_TRANS)
        memset(screenp, 0, (256+128)*FONTS_HEIGHT*2);

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

    num= 0;
    while(*string)
    {
        string= utf8decode(string, unicode+num);
        if(unicode[num] != 0x0D && unicode[num] != 0x0A) num++;
    }

    scroll_strinfo[index].str_len= num;
    if(num == 0)
        return index;

    len= BDF_cut_unicode(unicode, num, 256+128, 1);
    i= 0;
    x= 0;
    while(i < len)
    {
        x += BDF_render16_ucs(screenp + x, 256+128, 0, color_bg, color_fg, unicode[i++]);
    }

    scroll_strinfo[index].buff_width= x;
    scroll_strinfo[index].pos_pixel= 0;
    scroll_strinfo[index].str_start= 0;
    scroll_strinfo[index].str_end= len-1;

    num= scroll_strinfo[index].height;
    len= width;

    u16 *screenp1;

    if(color_bg == COLOR_TRANS)
    {
        u16 pixel;

        for(i= 0; i < num; i++)
        {
            screenp= (unsigned short*)screen_addr + sx + (sy + i) * SCREEN_WIDTH;
            screenp1= scroll_strinfo[index].buff_fonts + i*(256+128);
            for(x= 0; x < len; x++)
            {
                pixel= *screenp1++;
				if(pixel) *screenp = pixel;
				screenp ++;
            }
        }
    }
    else
    {
        screenp= (unsigned short*)screen_addr + sx + sy * SCREEN_WIDTH;
        screenp1= scroll_strinfo[index].buff_fonts;

        for(i= 0; i < num; i++)
        {
            memcpy((char*)screenp, (char*)screenp1, len*2);
            screenp += SCREEN_WIDTH;
            screenp1 += (256+128);
        }
    }

    return index;
}

u32 draw_hscroll(u32 index, s32 scroll_val)
{
    u32 color_bg, color_fg, i, width, height;
    s32 xoff;

//static int flag= 0;

    if(index >= MAX_SCROLL_STRING) return -1;
    if(scroll_strinfo[index].screenp == NULL) return -2;
    if(scroll_strinfo[index].str_len == 0) return 0;
    
    width= scroll_strinfo[index].width;
    height= scroll_strinfo[index].height;
    xoff= scroll_strinfo[index].pos_pixel - scroll_val;
    color_bg= scroll_strinfo[index].color_bg;
    color_fg= scroll_strinfo[index].color_fg;

    if(scroll_val > 0)    //shift right
    {
        if(xoff <= 0)
        {
            if(scroll_strinfo[index].str_start > 0)
            {
                u32 x, y, len;
                u16 *unicode;
                u32 *ptr;
                //we assume the malloced memory are 4 bytes aligned, or else this method is wrong
                y= height*width;
                ptr= (u32*)scroll_strinfo[index].buff_fonts;
                y= ((256+128)*FONTS_HEIGHT*2+3)/4;
                x= 0;
                while(x<y)  ptr[x++] = 0;
    
                unicode= scroll_strinfo[index].unicode + scroll_strinfo[index].str_end;
                len= scroll_strinfo[index].str_end +1;
                x= (scroll_val > SCREEN_WIDTH/4) ? scroll_val : SCREEN_WIDTH/4;
                y= BDF_cut_unicode(unicode, len, x, 0);
                if(y < len) y += 1;
    
                if(y < scroll_strinfo[index].str_start)
                    scroll_strinfo[index].str_start -= y;
                else
                {
                    y= scroll_strinfo[index].str_start;
                    scroll_strinfo[index].str_start = 0;
                }
    
                len= scroll_strinfo[index].str_len - scroll_strinfo[index].str_start;
                unicode= scroll_strinfo[index].unicode + scroll_strinfo[index].str_start;
                x= 0;
                i= 0;
                while(i < y)
                {
                    x += BDF_render16_ucs(scroll_strinfo[index].buff_fonts + x, 256+128, 0, 
                        color_bg, color_fg, unicode[i++]);
                    if(x >= (256+128-14)) break;
                }
    
                y= x;
                while(i < len)
                {
                    x += BDF_render16_ucs(scroll_strinfo[index].buff_fonts + x, 256+128, 0, 
                        color_bg, color_fg, unicode[i++]);
                    if(x >= (256+128-14)) break;
                }
    
                scroll_strinfo[index].pos_pixel += y - scroll_val;
                if((scroll_strinfo[index].pos_pixel + width) > (256+128))
                    scroll_strinfo[index].pos_pixel= 0;
                scroll_strinfo[index].buff_width= x;
                scroll_strinfo[index].str_end = scroll_strinfo[index].str_start + i -1;
            }
            else
            {
                if(scroll_strinfo[index].pos_pixel > 0)
                    scroll_strinfo[index].pos_pixel= 0;
                else
                    return 0;
            }
    
            xoff= scroll_strinfo[index].pos_pixel;
        }
        else
            scroll_strinfo[index].pos_pixel= xoff;
    }
    else if(xoff < (s32)scroll_strinfo[index].buff_width)   //shift left
    {
        if((scroll_strinfo[index].buff_width + width) > (256+128))
        if((xoff + width) > scroll_strinfo[index].buff_width)
        {
            u32 x, y, len;
            u16 *unicode;
            u32 *ptr;
            //we assume the malloced memory are 4 bytes aligned, or else this method is wrong
            y= height*width;
            ptr= (u32*)scroll_strinfo[index].buff_fonts;
            y= ((256+128)*FONTS_HEIGHT*2+3)/4;
            x= 0;
            while(x<y)  ptr[x++] = 0;

            unicode= scroll_strinfo[index].unicode + scroll_strinfo[index].str_start;
            len= scroll_strinfo[index].str_len - scroll_strinfo[index].str_start;
            x= (scroll_val > SCREEN_WIDTH/4) ? scroll_val : SCREEN_WIDTH/4;
            x= ((s32)x < xoff) ? x : xoff;
            y= BDF_cut_unicode(unicode, len, x, 1);

            scroll_strinfo[index].str_start += y;
            len= scroll_strinfo[index].str_len - scroll_strinfo[index].str_start;
            y= scroll_strinfo[index].str_end - scroll_strinfo[index].str_start +1;
            unicode= scroll_strinfo[index].unicode + scroll_strinfo[index].str_start;
            x= 0;
            i= 0;
            while(i < y)
            {
                x += BDF_render16_ucs(scroll_strinfo[index].buff_fonts + x, 256+128, 0, 
                    color_bg, color_fg, unicode[i++]);
            }

            xoff -= scroll_strinfo[index].buff_width - x;

            while(i < len)
            {
                x += BDF_render16_ucs(scroll_strinfo[index].buff_fonts + x, 256+128, 0, 
                    color_bg, color_fg, unicode[i++]);
                if(x >= (256+128-14)) break;
            }

            scroll_strinfo[index].buff_width= x;
            scroll_strinfo[index].str_end = scroll_strinfo[index].str_start + i -1;
        }

        scroll_strinfo[index].pos_pixel= xoff;
    }
    else
        return 0;

    u32 x, sx, sy, pixel;
    u16 *screenp, *screenp1;

    color_bg = scroll_strinfo[index].color_bg;
    sx= scroll_strinfo[index].sx;
    sy= scroll_strinfo[index].sy;

    if(color_bg == COLOR_TRANS)
    {
        for(i= 0; i < height; i++)
        {
            screenp= scroll_strinfo[index].screenp + sx + (sy + i) * SCREEN_WIDTH;
            screenp1= scroll_strinfo[index].buff_fonts + xoff + i*(256+128);
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
            screenp1= scroll_strinfo[index].buff_fonts + xoff + i*(256+128);
            for(x= 0; x < width; x++)
                *screenp++ = *screenp1++;
        }
    }

    u32 ret;
    if(scroll_val > 0)
        ret= scroll_strinfo[index].pos_pixel;
    else
        ret= scroll_strinfo[index].buff_width - scroll_strinfo[index].pos_pixel;

    return ret;
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

    i= SCREEN_WIDTH/2 - box_width - 2;
	show_icon((unsigned short*)screen_addr, &ICON_BUTTON, 49, 128);
    draw_string_vcenter((unsigned short*)screen_addr, 51, 130, 73, COLOR_WHITE, yes);

    i= SCREEN_WIDTH/2 + 3;
	show_icon((unsigned short*)screen_addr, &ICON_BUTTON, 136, 128);
    draw_string_vcenter((unsigned short*)screen_addr, 138, 130, 73, COLOR_WHITE, no);

	ds2_flipScreen(screen, DOWN_SCREEN_UPDATE_METHOD);

    gui_action_type gui_action = CURSOR_NONE;
    while((gui_action != CURSOR_SELECT)  && (gui_action != CURSOR_BACK))
    {
        gui_action = get_gui_input();
	if (gui_action == CURSOR_TOUCH)
	{
		struct key_buf inputdata;
		ds2_getrawInput(&inputdata);
		// Turn it into a SELECT (A) or BACK (B) if the button is touched.
		if (inputdata.y >= 128 && inputdata.y < 128 + ICON_BUTTON.y)
		{
			if (inputdata.x >= 49 && inputdata.x < 49 + ICON_BUTTON.x)
				gui_action = CURSOR_SELECT;
			else if (inputdata.x >= 136 && inputdata.x < 136 + ICON_BUTTON.x)
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

/*
*	Drawing progress bar
*/
static enum SCREEN_ID _progress_screen_id;
static int progress_total;
static int progress_current;
static char progress_message[256];

//	progress bar initialize
void init_progress(enum SCREEN_ID screen, u32 total, char *text)
{
	void* screen_addr;

	_progress_screen_id = screen;
	if(_progress_screen_id & UP_MASK)
		screen_addr = up_screen_addr;
	else
		screen_addr = down_screen_addr;

	progress_current = 0;
	progress_total   = total;
//  strcpy(progress_message, text);

//  draw_dialog(progress_sx - 8, progress_sy -29, progress_ex + 8, progress_ey + 13);

//  boxfill(progress_sx - 1, progress_sy - 1, progress_ex + 1, progress_ey + 1, 0);

//  if (text[0] != '\0')
//    print_string_center(progress_sy - 21, COLOR_PROGRESS_TEXT, COLOR_DIALOG, text);

    drawboxfill((unsigned short*)screen_addr, progress_sx, progress_sy, progress_ex, 
		progress_ey, COLOR16(15, 15, 15));

	ds2_flipScreen(_progress_screen_id, DOWN_SCREEN_UPDATE_METHOD);
}

//	update progress bar
void update_progress(void)
{
	void* screen_addr;

	if(_progress_screen_id & UP_MASK)
		screen_addr = up_screen_addr;
	else
		screen_addr = down_screen_addr;

  int width = (int)( ((float)++progress_current / (float)progress_total) * ((float)SCREEN_WIDTH / 3.0 * 2.0) );

//  draw_dialog(progress_sx - 8, progress_sy -29, progress_ex + 8, progress_ey + 13);

//  boxfill(progress_sx - 1, progress_sy - 1, progress_ex + 1, progress_ey + 1, COLOR_BLACK);
//  if (progress_message[0] != '\0')
//    print_string_center(progress_sy - 21, COLOR_PROGRESS_TEXT, COLOR_DIALOG, progress_message);

	drawboxfill(screen_addr, progress_sx, progress_sy, progress_sx+width, progress_ey, COLOR16(30, 19, 7));

	ds2_flipScreen(_progress_screen_id, DOWN_SCREEN_UPDATE_METHOD);
}

//	display progress string
void show_progress(char *text)
{
	void* screen_addr;

	if(_progress_screen_id & UP_MASK)
		screen_addr = up_screen_addr;
	else
		screen_addr = down_screen_addr;

//  draw_dialog(progress_sx - 8, progress_sy -29, progress_ex + 8, progress_ey + 13);
//  boxfill(progress_sx - 1, progress_sy - 1, progress_ex + 1, progress_ey + 1, COLOR_BLACK);

	if (progress_current)
	{
		int width = (int)( (float)(++progress_current / progress_total) * (float)(SCREEN_WIDTH / 3.0 * 2.0) );
		drawboxfill(screen_addr, progress_sx, progress_sy, progress_sx+width, progress_ey, COLOR16(30, 19, 7));
	}

//  if (text[0] != '\0')
//    print_string_center(progress_sy - 21, COLOR_PROGRESS_TEXT, COLOR_DIALOG, text);

	ds2_flipScreen(_progress_screen_id, DOWN_SCREEN_UPDATE_METHOD);

//  OSTimeDly(progress_wait);
	mdelay(500);
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
	ConsoleInit(RGB15(31, 31, 31), RGB15(0, 0, 0), UP_SCREEN, 512);
	printf(msg);
}
