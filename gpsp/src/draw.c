/* unofficial gameplaySP kai
 *
 * Copyright (C) 2007 NJ
 * Copyright (C) 2007 takka <takka@tfact.net>
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

/******************************************************************************
 * draw.c
 * 基本的绘图处理
 ******************************************************************************/

/******************************************************************************
 * 
 ******************************************************************************/
#include "common.h"
#include <stdarg.h>
#include "errinfo.h"

/******************************************************************************
 * 宏定义
 ******************************************************************************/
#define progress_sx (screen_width2 - screen_width / 3)  // 从中心 -160/-80
#define progress_ex (screen_width2 + screen_width / 3)  // 从中心 +160/+80
#define progress_sy (screen_height2 + 3)                // 从中心 +3
#define progress_ey (screen_height2 + 13)               // 从中心 +13
#define yesno_sx    (screen_width2 - screen_width / 3)  // 从中心 -160/-80
#define yesno_ex    (screen_width2 + screen_width / 3)  // 从中心 +160/+80
#define yesno_sy    (screen_height2 + 3)                // 从中心 +3
#define yesno_ey    (screen_height2 + 13)               // 从中心 +13
#define progress_color COLOR16(15,15,15)
//#define progress_wait (0.5 * 1000 * 1000)
#define progress_wait (OS_TICKS_PER_SEC/2)				//0.5S

#define FONTS_HEIGHT    14

#define VRAM_POS(x, y)  (screen_address + (x + (y) * screen_pitch));

#define BOOTLOGO "SYSTEM\\GUI\\boot.bmp"
#define GUI_SOURCE_PATH "SYSTEM\\GUI"
#define GUI_PIC_BUFSIZE 1024*1024

struct background back_ground = {{0}, {0}};
char gui_picture[GUI_PIC_BUFSIZE];
struct gui_iconlist gui_icon_list[]= {
    //file system
    /* 00 */ {"filesys", 10, 10, NULL},
    /* 01 */ {"gbafile", 16, 16, NULL},
    /* 02 */ {"zipfile", 16, 16, NULL},
    /* 03 */ {"directory", 16, 16, NULL},
    /* 04 */ {"vscrollslider", 2, 103, NULL},
    /* 05 */ {"vscrollbar", 8, 13, NULL},
    /* 06 */ {"vscrolluparrow", 9, 9, NULL},
    /* 07 */ {"vscrolldwarrow", 9, 9, NULL},
    /* 08 */ {"visolator", 213, 3, NULL},
    /* 09 */ {"fileselected", 201, 21, NULL},
    //main menu
    /* 10 */ {"newgame", 70, 37, NULL},
    /* 11 */ {"resetgame", 98, 37, NULL},
    /* 12 */ {"returngame", 88, 37, NULL},
    /* 13 */ {"newgameselect", 70, 37, NULL},
    /* 14 */ {"resetgameselect", 98, 37, NULL},
    /* 15 */ {"returngameselect", 88, 37, NULL},
    /* 16 */ {"menuindex", 5, 7, NULL},
    /* 17 */ {"menuisolator", 98, 3, NULL},
    //sub-menu
    /* 18 */ {"subisolator", 221, 5, NULL},
    //post added
    /* 19 */ {"binfile", 16, 16, NULL},
                        };

/******************************************************************************
 * 全局函数声明
 ******************************************************************************/
u32 yesno_dialog(char *text);

/******************************************************************************
 * 局部变量定义
 ******************************************************************************/
static int progress_total;
static int progress_current;
static char progress_message[256];
//static u32 __attribute__((aligned(32))) display_list[2048];

/******************************************************************************
 * 本地函数声明
 ******************************************************************************/
void draw_dialog(u32 sx, u32 sy, u32 ex, u32 ey);

/*------------------------------------------------------
  字符串描绘 / 围绕中心
------------------------------------------------------*/
void print_string_center(u32 sy, u32 color, u32 bg_color, char *str)
{
  int width = 0;//fbm_getwidth(str);
  u32 sx = (screen_width - width) / 2;

  PRINT_STRING_BG(str, color, bg_color, sx, sy);
}

/*------------------------------------------------------
  字符串描绘 / 阴影 / 围绕中心
------------------------------------------------------*/
void print_string_shadow_center(u32 sy, u32 color, char *str)
{
  int width = 0;//fbm_getwidth(str);
  u32 sx = (screen_width - width) / 2;

  PRINT_STRING_SHADOW(str, color, sx, sy);
}

/*------------------------------------------------------
  水平线描绘
------------------------------------------------------*/
void hline(u32 sx, u32 ex, u32 y, u32 color)
{
  u32 x;
  u32 width  = (ex - sx) + 1;
  volatile u16 *dst = VRAM_POS(sx, y);

  for (x = 0; x < width; x++)
    *dst++ = (u16)color;
}

/*------------------------------------------------------
  垂直线描绘
------------------------------------------------------*/
void vline(u32 x, u32 sy, u32 ey, u32 color)
{
  int y;
  int height = (ey - sy) + 1;
  volatile u16 *dst = VRAM_POS(x, sy);

  for (y = 0; y < height; y++)
  {
    *dst = (u16)color;
    dst += screen_pitch;
  }
}

/*------------------------------------------------------
  矩形描绘 (16bit)
------------------------------------------------------*/
void box(u32 sx, u32 sy, u32 ex, u32 ey, u32 color)
{
  hline(sx, ex - 1, sy, color);
  vline(ex, sy, ey - 1, color);
  hline(sx + 1, ex, ey, color);
  vline(sx, sy + 1, ey, color);
}

/*------------------------------------------------------
  填充矩形
------------------------------------------------------*/
void boxfill(u32 sx, u32 sy, u32 ex, u32 ey, u32 color)
{
  u32 x, y;
  u32 width  = (ex - sx) + 1;
  u32 height = (ey - sy) + 1;
  volatile u16 *dst = (u16 *)(screen_address + (sx + sy * screen_pitch));

  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {
      dst[x + y * screen_pitch] = (u16)color;
    }
  }
}

/*------------------------------------------------------
  画水平线
------------------------------------------------------*/
void drawhline(u16 *screen_address, u32 sx, u32 ex, u32 y, u32 color)
{
  u32 x;
  u32 width  = (ex - sx) + 1;
  volatile u16 *dst = VRAM_POS(sx, y);

  for (x = 0; x < width; x++)
    *dst++ = (u16)color;
}

/*------------------------------------------------------
  画垂直线
------------------------------------------------------*/
void drawvline(u16 *screen_address, u32 x, u32 sy, u32 ey, u32 color)
{
  int y;
  int height = (ey - sy) + 1;
  volatile u16 *dst = VRAM_POS(x, sy);

  for (y = 0; y < height; y++)
  {
    *dst = (u16)color;
    dst += screen_pitch;
  }
}

/*------------------------------------------------------
  画矩形 (16bit)
------------------------------------------------------*/
void drawbox(u16 *screen_address, u32 sx, u32 sy, u32 ex, u32 ey, u32 color)
{
  drawhline(screen_address, sx, ex - 1, sy, color);
  drawvline(screen_address, ex, sy, ey - 1, color);
  drawhline(screen_address, sx + 1, ex, ey, color);
  drawvline(screen_address, sx, sy + 1, ey, color);
}

/*------------------------------------------------------
  填充矩形
------------------------------------------------------*/
void drawboxfill(u16 *screen_address, u32 sx, u32 sy, u32 ex, u32 ey, u32 color)
{
  u32 x, y;
  u32 width  = (ex - sx) + 1;
  u32 height = (ey - sy) + 1;
  volatile u16 *dst;

  for (y = 0; y < height; y++)
  {
    dst = VRAM_POS(sx, sy+y);
    for (x = 0; x < width; x++)
      *dst++ = (u16)color;
  }
}

/*------------------------------------------------------
-  画选择项: 口
- active    0 not fill
-           1 fill with gray
-           2 fill with color
-           3 fill with color and most brithness
- color     0 Red
-           1 Green
-           2 Blue
------------------------------------------------------*/
void draw_selitem(u16 *screen_address, u32 x, u32 y, u32 color, u32 active)
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

    drawbox(screen_address, x, y, x+size-1, y+size-1, color0);

    if(active >0)
    {
        drawbox(screen_address, x+1, y+1, x+size-2, y+size-2, color1);
        drawbox(screen_address, x+2, y+2, x+size-3, y+size-3, color2);
        drawboxfill(screen_address, x+3, y+3, x+size-4, y+size-4, color3);
    }
}
/*------------------------------------------------------
  画消息框
- 如果 color_fg = COLOR_TRANS, 需要 screen_bg
------------------------------------------------------*/
void draw_message(u16 *screen_address, u16 *screen_bg, u32 sx, u32 sy, u32 ex, u32 ey,
        u32 color_fg)
{
    if(!(color_fg & 0x8000))
    {
        drawbox(screen_address, sx, sy, ex, ey, COLOR16(12, 12, 12));
        drawboxfill(screen_address, sx+1, sy+1, ex-1, ey-1, color_fg);
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
            screenp = VRAM_POS(sx, sy+k);
            screenp1 = screen_bg + sx + (sy + k) * screen_pitch;
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

/*------------------------------------------------------
  画水平居中对齐的字符串
------------------------------------------------------*/
void draw_string_vcenter(u16* screen_address, u32 sx, u32 sy, u32 width, u32 color_fg, char *string)
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

    screenp = screen_address + sx + sy*SCREEN_WIDTH;
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
  描绘滚动字符串
------------------------------------------------------*/
#define MAX_SCROLL_STRING   6

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

u32 draw_hscroll_init(u16 *screen_address, u16 *buff_bg, u32 sx, u32 sy, u32 width, 
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
        free((int)screenp);
        return -3;
    }


    if(color_bg == COLOR_TRANS)
        memset(screenp, 0, (256+128)*FONTS_HEIGHT*2);

    scroll_string_num += 1;
    scroll_strinfo[index].screenp = screen_address;
    scroll_strinfo[index].sx= sx;
    scroll_strinfo[index].sy= sy;
    scroll_strinfo[index].color_bg= color_bg;
    scroll_strinfo[index].color_fg= color_fg;
    scroll_strinfo[index].width= width;
    scroll_strinfo[index].height= FONTS_HEIGHT;
    scroll_strinfo[index].unicode= unicode;
    scroll_strinfo[index].buff_fonts= screenp;
    scroll_strinfo[index].buff_bg= buff_bg;

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

    if(color_bg == COLOR_TRANS && buff_bg != NULL)
    {
        u16 *screenp2;
        u16 pixel;

        for(i= 0; i < num; i++)
        {
            screenp= screen_address + sx + (sy + i) * SCREEN_WIDTH;
            screenp1= scroll_strinfo[index].buff_fonts + i*(256+128);
            screenp2= buff_bg + sx + (sy + i) * SCREEN_WIDTH;
            for(x= 0; x < len; x++)
            {
                pixel= *screenp1++;
                *screenp++ = (pixel > 0) ? pixel : *screenp2;
                screenp2++;
            }
        }
    }
    else
    {
        screenp= screen_address + sx + sy * SCREEN_WIDTH;
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
/*
if(index== 3)
{
u16 *sp0, *sp1;
u32 m, n;

flag= 1;

printf("xoff %d %d %d\n", scroll_strinfo[index].pos_pixel, xoff, scroll_strinfo[index].buff_width);

for(m= 0; m< height; m++)
{
    sp0= scroll_strinfo[index].screenp + m * SCREEN_WIDTH;
    sp1= scroll_strinfo[index].buff_fonts + m*(256+128);
    for(n= 0; n< 256; n++)
        *sp0++ = *sp1++;

}

for(m= 0; m< height; m++)
{
    sp0= scroll_strinfo[index].screenp + (height + m) * SCREEN_WIDTH;
    sp1= scroll_strinfo[index].buff_fonts + 256 + m*(256+128);
    for(n= 0; n< 128; n++)
        *sp0++ = *sp1++;
}

flip_screen();
printf("dis 1\n");
OSTimeDly(1000);
}
*/
    }
    else if(xoff < scroll_strinfo[index].buff_width)   //shift left
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
            x= (x < xoff) ? x : xoff;
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
/*
if(flag== 1 && index== 3)
{
u16 *sp0, *sp1;
u32 m, n;

printf("xoff %d %d\n", scroll_strinfo[index].pos_pixel, scroll_strinfo[index].buff_width);

for(m= 0; m< height; m++)
{
    sp0= scroll_strinfo[index].screenp + m * SCREEN_WIDTH;
    sp1= scroll_strinfo[index].buff_fonts + m*(256+128);
    for(n= 0; n< 256; n++)
        *sp0++ = *sp1++;

}

for(m= 0; m< height; m++)
{
    sp0= scroll_strinfo[index].screenp + (height + m) * SCREEN_WIDTH;
    sp1= scroll_strinfo[index].buff_fonts + 256 + m*(256+128);
    for(n= 0; n< 128; n++)
        *sp0++ = *sp1++;
}

flip_screen();
printf("dis 2\n");
OSTimeDly(1000);
}
*/
    }
    else
        return 0;

    u32 x, sx, sy, pixel;
    u16 *screenp, *screenp1;

    color_bg = scroll_strinfo[index].color_bg;
    sx= scroll_strinfo[index].sx;
    sy= scroll_strinfo[index].sy;

    if(color_bg == COLOR_TRANS && scroll_strinfo[index].buff_bg != NULL)
    {
        u16 *screenp2;

        for(i= 0; i < height; i++)
        {
            screenp= scroll_strinfo[index].screenp + sx + (sy + i) * SCREEN_WIDTH;
            screenp1= scroll_strinfo[index].buff_fonts + xoff + i*(256+128);
            screenp2= scroll_strinfo[index].buff_bg + sx + (sy + i) * SCREEN_WIDTH;
            for(x= 0; x < width; x++)
            {
                pixel= *screenp1++;
                *screenp++ = (pixel > 0) ? pixel : *screenp2;
                screenp2++;
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

/*
if(index== 3 && flag== 1)
{
printf("dis 3\n");
printf("xoff %d\n", xoff);
flip_screen();
flag= 0;
OSTimeDly(1000);
}
*/
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
//printf("f %d %08x %08x\n", index, scroll_strinfo[index].buff_fonts, scroll_strinfo[index].unicode);
        if(scroll_strinfo[index].unicode)
        {
            free((int)scroll_strinfo[index].unicode);
            scroll_strinfo[index].unicode= NULL;
        }
        if(scroll_strinfo[index].buff_fonts)
        {
            free((int)scroll_strinfo[index].buff_fonts);
            scroll_strinfo[index].buff_fonts= NULL;
        }
        scroll_strinfo[index].screenp= NULL;
        scroll_strinfo[index].str_len= 0;
    
        scroll_string_num -=1;
    }
}

/*------------------------------------------------------
  显示对话框
------------------------------------------------------*/
void draw_dialog(u32 sx, u32 sy, u32 ex, u32 ey)
{
// 阴影显示
  boxfill(sx + 5, sy + 5, ex + 5, ey + 5, COLOR_DIALOG_SHADOW);

  hline(sx, ex - 1, sy, COLOR_FRAME);
  vline(ex, sy, ey - 1, COLOR_FRAME);
  hline(sx + 1, ex, ey, COLOR_FRAME);
  vline(sx, sy + 1, ey, COLOR_FRAME);

  sx++;
  ex--;
  sy++;
  ey--;

  hline(sx, ex - 1, sy, COLOR_FRAME);
  vline(ex, sy, ey - 1, COLOR_FRAME);
  hline(sx + 1, ex, ey, COLOR_FRAME);
  vline(sx, sy + 1, ey, COLOR_FRAME);

  sx++;
  ex--;
  sy++;
  ey--;

  boxfill(sx, sy, ex, ey, COLOR_DIALOG);
}

/*--------------------------------------------------------
  yes/no ダイヤログボックス
  input
    char *text 表示テキスト
  return
    0 YES
    1 NO
--------------------------------------------------------*/
u32 yesno_dialog(char *text)
{
  gui_action_type gui_action = CURSOR_NONE;

  draw_dialog(yesno_sx - 8, yesno_sy -29, yesno_ex + 8, yesno_ey + 13);
  print_string_center(yesno_sy - 16, COLOR_YESNO_TEXT, COLOR_DIALOG, text);
  print_string_center(yesno_sy + 5 , COLOR_YESNO_TEXT, COLOR_DIALOG, "Yes - A / No - B");

  flip_screen();

  while((gui_action != CURSOR_SELECT)  && (gui_action != CURSOR_BACK))
  {
    gui_action = get_gui_input();
//    sceKernelDelayThread(15000); /* 0.0015s */
	OSTimeDly(10);					//Delay 10 ticks
  }
  if (gui_action == CURSOR_SELECT)
    return 0;
  else
    return 1;
}

/*--------------------------------------------------------
--------------------------------------------------------*/
u32 draw_yesno_dialog(u16 *screen_address, u32 sy, char *yes, char *no)
{
    u16 unicode[8];
    u32 len, width, box_width, i;
    char *string;

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

    i= SCREEN_WIDTH/2 - box_width - 2;
    drawbox(screen_address, i, sy-1, i+box_width-1, sy+FONTS_HEIGHT, COLOR16(8, 8, 8));
    drawboxfill(screen_address, i+1, sy, i+box_width-2, sy+FONTS_HEIGHT-1, COLOR16(15, 15, 15));
    draw_string_vcenter(screen_address, i+1, sy+1, box_width, COLOR_WHITE, yes);

    i= SCREEN_WIDTH/2 + 3;
    drawbox(screen_address, i, sy-1, i+box_width-1, sy+FONTS_HEIGHT, COLOR16(8, 8, 8));
    drawboxfill(screen_address, i+1, sy, i+box_width-2, sy+FONTS_HEIGHT-1, COLOR16(15, 15, 15));
    draw_string_vcenter(screen_address, i+1, sy+1, box_width, COLOR_WHITE, no);

    flip_screen();

    gui_action_type gui_action = CURSOR_NONE;
    while((gui_action != CURSOR_SELECT)  && (gui_action != CURSOR_BACK))
    {
        gui_action = get_gui_input();
        OSTimeDly(OS_TICKS_PER_SEC/10);
    }

    if (gui_action == CURSOR_SELECT)
        return 1;
    else
        return 0;
}

/*--------------------------------------------------------
  进度条
--------------------------------------------------------*/
/*--------------------------------------------------------
  初始化进度条
--------------------------------------------------------*/
void init_progress(u32 total, char *text)
{
  progress_current = 0;
  progress_total   = total;
//  strcpy(progress_message, text);

//  draw_dialog(progress_sx - 8, progress_sy -29, progress_ex + 8, progress_ey + 13);

//  boxfill(progress_sx - 1, progress_sy - 1, progress_ex + 1, progress_ey + 1, 0);

//  if (text[0] != '\0')
//    print_string_center(progress_sy - 21, COLOR_PROGRESS_TEXT, COLOR_DIALOG, text);

//    drawbox(screen_address, progress_sx, progress_sy, progress_ex, progress_ey, COLOR16(0, 0, 0));
    boxfill(progress_sx, progress_sy, progress_ex, progress_ey, COLOR16(15, 15, 15));

  flip_screen();
}

/*--------------------------------------------------------
  更新进度条
--------------------------------------------------------*/
void update_progress(void)
{
  int width = (int)( ((float)++progress_current / (float)progress_total) * ((float)screen_width / 3.0 * 2.0) );

//  draw_dialog(progress_sx - 8, progress_sy -29, progress_ex + 8, progress_ey + 13);

//  boxfill(progress_sx - 1, progress_sy - 1, progress_ex + 1, progress_ey + 1, COLOR_BLACK);
//  if (progress_message[0] != '\0')
//    print_string_center(progress_sy - 21, COLOR_PROGRESS_TEXT, COLOR_DIALOG, progress_message);
//  boxfill(progress_sx, progress_sy, progress_sx+width, progress_ey, COLOR_PROGRESS_BAR);
  boxfill(progress_sx, progress_sy, progress_sx+width, progress_ey, COLOR16(13, 20, 18));

  flip_screen();
}

/*--------------------------------------------------------
  プログレスバー結果表示
--------------------------------------------------------*/
void show_progress(char *text)
{
//  draw_dialog(progress_sx - 8, progress_sy -29, progress_ex + 8, progress_ey + 13);

//  boxfill(progress_sx - 1, progress_sy - 1, progress_ex + 1, progress_ey + 1, COLOR_BLACK);
    
  if (progress_current)
  {
    int width = (int)( (float)(++progress_current / progress_total) * (float)(screen_width / 3.0 * 2.0) );
//    boxfill(progress_sx, progress_sy, progress_sx+width, progress_ey, COLOR_PROGRESS_BAR);
    boxfill(progress_sx, progress_sy, progress_sx+width, progress_ey, COLOR16(13, 20, 18));
  }

//  if (text[0] != '\0')
//    print_string_center(progress_sy - 21, COLOR_PROGRESS_TEXT, COLOR_DIALOG, text);

  flip_screen();
//  sceKernelDelayThread(progress_wait);
  OSTimeDly(progress_wait);
}

/*--------------------------------------------------------
  显示滚动条（只菜单）
--------------------------------------------------------*/
#define SCROLLBAR_COLOR1 COLOR16( 0, 2, 8)
#define SCROLLBAR_COLOR2 COLOR16(15,15,15)

void scrollbar(u32 sx, u32 sy, u32 ex, u32 ey, u32 all,u32 view,u32 now)
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

    box(sx, sy, ex, ey, COLOR_BLACK);
    boxfill(sx + 1, sy + 1, ex - 1, ey - 1, SCROLLBAR_COLOR1);
    boxfill(sx + 1, scrollbar_sy, ex - 1, scrollbar_ey, SCROLLBAR_COLOR2);
}

/*------------------------------------------------------
  VRAMへのテクスチャ転送(拡大縮小つき)
------------------------------------------------------*/
#if 0
void bitblt(u32 vram_adr,u32 pitch, u32 sx, u32 sy, u32 ex, u32 ey, u32 x_size, u32 y_size, u32 *data)
{
   SPRITE *vertices = (SPRITE *)temp_vertex;

  sceGuStart(GU_DIRECT, display_list);
  sceGuDrawBufferList(GU_PSM_5551, (void *) vram_adr, pitch);
  sceGuScissor(0, 0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
  sceGuEnable(GU_BLEND);
  sceGuTexMode(GU_PSM_5551, 0, 0, GU_FALSE);
  sceGuTexImage(0, 512, 512, x_size, data);
  sceGuTexFilter(GU_LINEAR, GU_LINEAR);

  vertices[0].u1 = (float)0;
  vertices[0].v1 = (float)0;
  vertices[0].x1 = (float)sx;
  vertices[0].y1 = (float)sy;

  vertices[0].u2 = (float)x_size;
  vertices[0].v2 = (float)y_size;
  vertices[0].x2 = (float)ex;
  vertices[0].y2 = (float)ey;

  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_32BITF, 2, NULL, vertices);

  sceGuDisable(GU_BLEND);
  sceGuFinish();
  sceGuSync(0, GU_SYNC_FINISH);
}
#endif

/******************************************************************************
  显示文字用的简单格式（只菜单）
******************************************************************************/

#define MAX_LINES 21
#define MIN_X   24
#define MIN_Y   45
#define INC_Y   10

static int cy;
static int linefeed;
static int text_color = COLOR16(0, 0, 0);
static char msg_lines[MAX_LINES][128];
static char msg_title[256];
static int msg_color[MAX_LINES];

#define COLOR_MSG_TITLE COLOR16(0, 0, 0)

/*--------------------------------------------------------
  信息显示
--------------------------------------------------------*/
void msg_screen_draw()
{
  draw_dialog(14, 15, 465, 38);
  draw_dialog(14, 37, 465, 259);
  print_string_center(22, COLOR_MSG_TITLE, COLOR_DIALOG, msg_title);
}

/*--------------------------------------------------------
  信息屏幕初始化
--------------------------------------------------------*/
void msg_screen_init(const char *title)
{
  cy = 0;
  linefeed = 1;
  memset(msg_lines, 0, sizeof(msg_lines));
  strcpy(msg_title, title);

  msg_screen_draw();
}

/*--------------------------------------------------------
  删除信息
--------------------------------------------------------*/
void msg_screen_clear(void)
{
  cy = 0;
  linefeed = 1;
}

/*--------------------------------------------------------
  设置文字颜色
--------------------------------------------------------*/
void msg_set_text_color(u32 color)
{
  text_color = color;
}

/*--------------------------------------------------------
  打印信息 
--------------------------------------------------------*/
void msg_printf(const char *text, ...)
{
  int y;
  char buf[128];
  va_list arg;

  va_start(arg, text);
  vsprintf(buf, text, arg);
  va_end(arg);

  if (linefeed)
  {
    if (cy == MAX_LINES)
    {
      for (y = 1; y < MAX_LINES; y++)
      {
        strcpy(msg_lines[y - 1], msg_lines[y]);
        msg_color[y - 1] = msg_color[y];
      }
      cy = MAX_LINES - 1;
    }
    strcpy(msg_lines[cy], buf);
  }
  else
  {
    strcat(msg_lines[cy], buf);
  }

  msg_color[cy] = text_color;

  msg_screen_draw();

  for (y = 0; y <= cy; y++)

    PRINT_STRING(msg_lines[y], msg_color[y], MIN_X, MIN_Y + y * 10);

  if (buf[strlen(buf) - 1] == '\n')
  {
    linefeed = 1;
    cy++;
  }
  else
  {
    if (buf[strlen(buf) - 1] == '\r')
      msg_lines[cy][0] = '\0';
    linefeed = 0;
  }
  flip_screen();
}

/************************************************************
*
*************************************************************/
int show_background(void *screen, char *bgname)
{
    int ret;

    if(strcasecmp(bgname, back_ground.bgname))
    {
        char *buff, *src;
        int x, y;        
        unsigned short *dst;

        buff= (char*)malloc(256*192*4);
            
        ret= BMP_read(bgname, buff, 256, 192);
        if(ret != BMP_OK)
        {
            free((int)buff);
            return(-1);
        }

        src = buff;
        for(y= 191; y>= 0; y--)
        {
            dst=(unsigned short*) (back_ground.bgbuffer + y*256*2);
            for(x= 0; x< 256; x++)
            {
                *dst++= RGB24_15(buff);
                buff += 4;
            }   
        }

        free((int)src);
        strcpy(back_ground.bgname, bgname);
    }

    memcpy((char*)screen, back_ground.bgbuffer, 256*192*2);

    return 0;    
}

/*************************************************************/
int gui_change_icon(u32 language_id)
{
    char path[128];
    char fpath[8];
    u32  i, item;
    int err, ret; 
    char *buff, *src;
    u32 x, y;
    char *icondst;

    item= sizeof(gui_icon_list)/16;
    buff= (char*)malloc(256*192*4);
    if(buff == NULL)
        return -1;

    src= buff; 
    ret= 0;
    icondst= gui_picture;

    sprintf(fpath, "%d.bmp", language_id);
    for(i= 0; i< item; i++)
    {
        sprintf(path, "%s\\%s\\%s%s", main_path, GUI_SOURCE_PATH, gui_icon_list[i].iconname, fpath);

        err= BMP_read(path, buff, gui_icon_list[i].x, gui_icon_list[i].y);
        if(err != BMP_OK)
        {
            sprintf(path, "%s\\%s\\%s%s", main_path, GUI_SOURCE_PATH, gui_icon_list[i].iconname, ".bmp");
            err= BMP_read(path, buff, gui_icon_list[i].x, gui_icon_list[i].y);
        }

        if(err == BMP_OK)
        {
            unsigned short *dst;
            
            if(icondst >= gui_picture + GUI_PIC_BUFSIZE -1)
            {
                ret = 1;
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
            
            gui_icon_list[i].iconbuff= icondst;
            icondst += gui_icon_list[i].x*gui_icon_list[i].y*2;
        }
        else
        {
            if(!ret) ret = -(i+1);
            gui_icon_list[i].iconbuff= NULL;
        }
    }

    free((int)src);
    return ret;
}

/*************************************************************/
int gui_init(u32 language_id)
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
void show_icon(u16 *screen, struct gui_iconlist icon, u32 x, u32 y)
{
    u32 i;
    char *src, *dst;    

    src= icon.iconbuff;
    dst = (char*)screen + (y*NDS_SCREEN_WIDTH + x) *2;
    for(i= 0; i < icon.y; i++)
    {
        memcpy(dst, src, icon.x*2);
        src += (icon.x)*2;
        dst += NDS_SCREEN_WIDTH*2;
    }
}

/*************************************************************/
void show_Vscrollbar(char *screen, u32 x, u32 y, u32 part, u32 total)
{
    show_icon((u16*)screen, ICON_VSCROL_UPAROW, x+235, y+55);
    show_icon((u16*)screen, ICON_VSCROL_DWAROW, x+235, y+167);
    show_icon((u16*)screen, ICON_VSCROL_SLIDER, x+239, y+64);
    if(total <= 1)
        show_icon((u16*)screen, ICON_VSCROL_BAR, x+236, y+64);
    else
        show_icon((u16*)screen, ICON_VSCROL_BAR, x+236, y+64+(part*90)/(total-1));
}

/*************************************************************/
void show_log()
{
    char tmp_path[MAX_PATH];
    sprintf(tmp_path, "%s\\%s", main_path, BOOTLOGO);
    show_background(screen_address, tmp_path);
}

/*************************************************************/
void show_err()
{
    memcpy((char*)screen_address, (char*)err_info, 256*192*2);
}

