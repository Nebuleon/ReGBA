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

struct bdffont{
    unsigned int dwidth;    //byte 3:2 x-distance; 1:0 y-distance
    unsigned int bbx;       //byte 3 x-width; 2 y-height; 1 x-offset; 0 y-offset
    unsigned char *bitmap;
};

struct bdflibinfo{
    unsigned int width;
    unsigned int height;
    unsigned int start;
    unsigned int span;
    unsigned int maplen;
    unsigned char *mapmem;
    struct bdffont *fonts;
};


/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
extern int BDF_font_init(void);
extern void BDF_render_string(void* screen_address, unsigned int x, unsigned int y, unsigned int back, 
    unsigned int front, char *string);
extern unsigned int BDF_render16_ucs(void* screen_address, unsigned int screen_w, 
    unsigned int v_align, unsigned int back, unsigned int front, unsigned short ch);
extern void BDF_render_mix(void* screen_address, unsigned int screen_w, unsigned int x, 
    unsigned int y, unsigned int v_align, unsigned int back, unsigned int front, char *string);
//extern unsigned int BDF_string_width(char *string, unsigned int *len);
extern char* utf8decode(char *utf8, unsigned short *ucs);
extern unsigned char* skip_utf8_unit(unsigned char* utf8, unsigned int num);
extern unsigned int BDF_cut_unicode(unsigned short *unicodes, unsigned int len, unsigned int width, unsigned int direction);
extern unsigned int BDF_cut_string(char *string, unsigned int width, unsigned int direction);
extern void BDF_font_release(void);

#ifdef __cplusplus
}
#endif

#endif //__BDF_FONT_H__
