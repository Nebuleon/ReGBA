/* bdf_font.c
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

#include <string.h>
#include "ds2_types.h"
#include "ds2_malloc.h"
#include "ds2io.h"
#include "fs_api.h"
#include "bdf_font.h"
#include "gui.h"


#define BDF_PICTOCHAT "SYSTEM/Pictochat-16.bdf"
#define BDF_SONG "SYSTEM/song.bdf"
#define ODF_PICTOCHAT "SYSTEM/Pictochat-16.odf"
#define ODF_SONG "SYSTEM/song.odf"

#define HAVE_ODF // Define this if you have generated Pictochat-16.odf [Neb]
// #define DUMP_ODF // Define this if you want to regenerate Pictochat-16.odf [Neb]

#define BDF_LIB_NUM 2
#define ODF_VERSION "1.0"

struct bdflibinfo bdflib_info[BDF_LIB_NUM];
struct bdffont *bdf_font;           //ASCII charactor
struct bdffont *bdf_nasci;          //non-ASCII charactor
static u32 font_height;
static u32 fonts_max_height;

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static u32 bitmap_code(unsigned char *code, unsigned char *bitmap)
{
    unsigned char *map;
    u8 a, b;
	u32 len;

    len= 0;
    map= (unsigned char*)bitmap;
    while(*map)
    {
        // One hex character represents the state of 4 successive pixels
        if(*map != 0x0A)
        {
            if      (*map <= '9') a= *map - '0';
            else if (*map <= 'F') a= *map - 'A' + 10;
			else if (*map <= 'f') a= *map - 'a' + 10;
            map++;

            if      (*map <= '9') b= *map - '0';
            else if (*map <= 'F') b= *map - 'A' + 10;
			else if (*map <= 'f') b= *map - 'a' + 10;

            *code++ = (a << 4) | b;
            len++;
        }
        map++;
    }

    return len;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
/*
* example
*
* STARTCHAR <arbitrary number or name>
* ENCODING 8804
* SWIDTH 840 0
* DWIDTH 14 0
* BBX 10 12 2 1
* BITMAP
* 00C0
* 0300
* 1C00
* 6000
* 8000
* 6000
* 1C00
* 0300
* 00C0
* 0000
* 0000
* FFC0
* ENDCHAR
*/

/*-----------------------------------------------------------------------------
* filename: bdf file's name, including path
* start: the coding of first font to parse
* span: number of fonts begin at start to parse
* *bdflibinfop: font library information
* method: font index method; 0-absolut sequence; 1-relative sequence; 2-compact;
*           others reserved
* return: if error return < 0; else return= char numbers 
------------------------------------------------------------------------------*/
static int parse_bdf(char *filename, u32 start, u32 span, struct bdflibinfo *bdflibinfop, u32 method)
{
    FILE *fp;
    char string[256];
    char map[256];
    char *pt;
    unsigned char *bitbuff;
    int num, x_off, y_off, ret;
    u32 tmp, i, end, length, index;
    struct bdffont *bdffontp;

    //initial bdflibinfo
    bdflibinfop -> width= 0;
    bdflibinfop -> height= 0;
    bdflibinfop -> start= 0;
    bdflibinfop -> span= 0;
    bdflibinfop -> maplen= 0;
    bdflibinfop -> mapmem= NULL;
    bdflibinfop -> fonts= NULL;

    fp= fopen(filename, "r");    //Open bdf font library
    if(fp == NULL)
        return -1;
    
    ret= 0;
    //SIZE
    while(1)
    {
        pt= fgets(string, 255, fp);
        if(pt == NULL)
        {
            ret= -2;
            goto parse_bdf_error;
        }
        if(!(strncasecmp(string, "SIZE ", 5)))
            break;
    }

    //FONTBOUNDINGBOX
    pt= fgets(string, 255, fp);
    pt += 16;
    bdflibinfop -> width= atoi(pt);
    pt = 1 + strchr(pt, ' ');
    bdflibinfop -> height= atoi(pt);
    pt = 1 + strchr(pt, ' ');
    x_off= atoi(pt);
    pt = 1 + strchr(pt, ' ');
    y_off= atoi(pt);

    //CHARS
    while(1)
    {
        pt= fgets(string, 255, fp);
        if(pt == NULL)
        {
            ret= -3;
            goto parse_bdf_error;
        }
        if(!(strncasecmp(string, "CHARS ", 6)))
            break;
    }
    pt += 6;
    ret= atoi(pt);

	if (method == 1)
    	bdflibinfop -> start= start;
    switch (method) {
	case 0:
	default:
		bdflibinfop -> span= span + start;
		break;
	case 1:
		bdflibinfop -> span= span;
		break;
	}

    //construct bdf font information
    bdffontp= (struct bdffont*)malloc(span * sizeof(struct bdffont));
    if(bdffontp == NULL)
    {
        ret= -4;
        goto parse_bdf_error;
    }
    bdflibinfop -> fonts= bdffontp;

    bitbuff= (unsigned char*)malloc((bdflibinfop -> width * bdflibinfop -> height * span) >> 3);
    if(bitbuff == NULL)
    {
        ret= -5;
        goto parse_bdf_error;
    }
    bdflibinfop -> mapmem= bitbuff;

    tmp= bdflibinfop -> width << 16;
    for(i= 0; i < span; i++)
    {
        bdffontp[i].dwidth= tmp;
        bdffontp[i].bbx= 0;
    }

    end= start + span;
    //STARTCHAR START
    while(1)
    {
        pt= fgets(string, 255, fp);
        if(pt == NULL)
        {
            ret= -6;
            goto parse_bdf_error;
        }
        if(!(strncasecmp(string, "STARTCHAR ", 10)))
        {
			break;
        }
    }

    i= 0;
    length= 0;
    while(1)
    {
        //ENCODING
        while(1)
        {
            pt= fgets(string, 255, fp);
            if(pt == NULL) goto parse_bdf_error;
            if(!(strncasecmp(string, "ENCODING ", 9))) break;
        }

        pt= string + 9;
        index= atoi(pt);
        if(index < start || index >= end) break;

        if(method == 0) i= index;
        else if(method == 1) i= index-start;
        else i++;

        //SWIDTH
        pt= fgets(string, 255, fp);
        if(pt == NULL) {ret= -8; goto parse_bdf_error;}

        //DWIDTH
        pt= fgets(string, 255, fp);
        if(pt == NULL) {ret= -9; goto parse_bdf_error;}

        pt += 7;
        num= atoi(pt);
        tmp= num << 16;
        pt= 1+ strchr(pt, ' ');
        num= atoi(pt);
        tmp |= num & 0xFFFF;

        bdffontp[i].dwidth= tmp;

        //BBX
        pt= fgets(string, 255, fp);
        if(pt == NULL) {ret= -10; goto parse_bdf_error;}

        pt += 4;
        num= atoi(pt);
        tmp= num & 0xFF;

        pt= 1+ strchr(pt, ' ');        
        num= atoi(pt);
        tmp= tmp<<8 | (num & 0xFF);

        pt= 1+ strchr(pt, ' ');
        num= atoi(pt);
        num= num - x_off;
        tmp= tmp<<8 | (num & 0xFF);

        pt= 1+ strchr(pt, ' ');
        num= atoi(pt);
        num= num - y_off;
        tmp= tmp <<8 | (num & 0xFF);

        bdffontp[i].bbx= tmp;

        //BITMAP
        pt= fgets(string, 255, fp);
        if(pt == NULL) {ret= -11; goto parse_bdf_error;}

        map[0]= '\0';
        while(1)
        {
            pt= fgets(string, 255, fp);
            if(pt == NULL) {ret= -12; goto parse_bdf_error;}
            if(!strncasecmp(pt, "ENDCHAR", 7)) break;
            strcat(map, pt);
        }

        tmp = bitmap_code(bitbuff, (unsigned char*)map);

        if(tmp)
            bdffontp[i].bitmap = bitbuff;
        else
            bdffontp[i].bitmap = NULL;

        bitbuff += tmp;
        length += tmp;
    }

parse_bdf_error:
    fclose(fp);
    if(ret < 0)
    {
        if(bdflibinfop -> fonts != NULL)
            free((void*)bdflibinfop -> fonts);
        if(bdflibinfop -> mapmem != NULL)
            free((void*)bdflibinfop -> mapmem);
        bdflibinfop -> fonts = NULL;
        bdflibinfop -> mapmem = NULL;
    }
    else
    {
        bdflibinfop -> maplen = length;
        bdflibinfop -> mapmem = (unsigned char*)realloc((void*)bdflibinfop -> mapmem, length);
    }

    return ret;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int dump2odf(char *filename, struct bdflibinfo *bdflibinfop)
{
    char *pt;
    char string[256];
    FILE *fp;
    u32 mapaddr;
    u32 fontaddr;
    u32 num;
    char buff[1024];
    u32 i, j;


    strcpy(string, filename);
    pt= strrchr(string, '.');
    if(!strcasecmp(pt, ".bdf"))
        strcpy(pt, ".odf");
    else
        return -1;

    fp= fopen(string, "wb");
    if(fp == NULL)
        return -2;

    pt= buff;
    strcpy(pt, "ODF");
    pt += 4;
    strcpy(pt, ODF_VERSION);
    pt += 4;

    struct bdflibinfo *bdflibinfo_i;

    memcpy(pt, (char*)bdflibinfop, sizeof(struct bdflibinfo));
    bdflibinfo_i= (struct bdflibinfo *)pt;
    bdflibinfo_i -> mapmem= NULL;
    bdflibinfo_i -> fonts= NULL;
    pt += sizeof(struct bdflibinfo);

    num= pt-buff;
    fwrite(buff, num, 1, fp);     //write odf file header

    num= (u32)bdflibinfop -> span;
    mapaddr= (u32)bdflibinfop -> mapmem;
    fontaddr= (u32)bdflibinfop -> fonts;

    while(num)
    {
        struct bdffont *bdffontp;

        i= 1024/sizeof(struct bdffont);
        if(num > i) num -= i;
        else i= num, num= 0;
        
        memcpy(buff, (char*)fontaddr, i*sizeof(struct bdffont));
        fontaddr += i*sizeof(struct bdffont);
        bdffontp= (struct bdffont*)buff;
        
        for(j= 0; j< i; j++)
            bdffontp[j].bitmap -= mapaddr;

        fwrite(buff, i*sizeof(struct bdffont), 1, fp);
    }
    
    fwrite((char*)mapaddr, bdflibinfop -> maplen, 1, fp);

    fclose(fp);
    return 0;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int init_from_odf(char *filename, struct bdflibinfo *bdflibinfop)
{
    FILE *fp;
    char buff[512];
    char *pt;
    u32 len, tmp;
    u32 span, maplen;
    struct bdffont *bdffontp;

    //initial bdflibinfo
    bdflibinfop -> width= 0;
    bdflibinfop -> height= 0;
    bdflibinfop -> start= 0;
    bdflibinfop -> span= 0;
    bdflibinfop -> maplen= 0;
    bdflibinfop -> mapmem= NULL;
    bdflibinfop -> fonts= NULL;

    fp= fopen(filename, "rb");
    if(fp == NULL)
        return -1;

    tmp= 8 + sizeof(struct bdflibinfo);
    len= fread(buff, 1, tmp, fp);
    if(len < tmp)
    {
        fclose(fp);
        return -2;
    }

    pt= buff;
    if(strcmp(pt, "ODF"))
    {
        fclose(fp);
        return -2;
    }

    pt += 4;
    if(strcmp(pt, ODF_VERSION))
    {
        fclose(fp);
        return -3;
    }

    pt += 4;
    memcpy((char*)bdflibinfop, pt, sizeof(struct bdflibinfo));

    span= bdflibinfop -> span;
    if(span == 0)
    {
        fclose(fp);
        return -4;
    }

    maplen= bdflibinfop -> maplen;
    if(maplen == 0)
    {
        fclose(fp);
        return -5;
    }

    bdffontp= (struct bdffont*)malloc(span * sizeof(struct bdffont));
    if(bdffontp == NULL)
    {
        fclose(fp);
        return -6;
    }

    len= fread((char*)bdffontp, 1, span * sizeof(struct bdffont), fp);
    if(len != span * sizeof(struct bdffont))
    {
        free((void*)bdffontp);
        fclose(fp);
        return -7;
    }

    pt= (char*)malloc(maplen);
    len= fread(pt, 1, maplen, fp);
    if(len != maplen)
    {
        free((void*)bdffontp);
        free((void*)pt);
        fclose(fp);
        return -8;
    }

    bdflibinfop -> mapmem = (unsigned char*)pt;
    bdflibinfop -> fonts = bdffontp;

    u32 i, j;
    j= (u32)bdflibinfop -> mapmem;
    for(i= 0; i < span; i++)
        bdffontp[i].bitmap += j;

    fclose(fp);
    return 0;
}

int BDF_font_init(void)
{
    int err;
    char tmp_path[MAX_PATH];

    fonts_max_height= 0;
#ifndef HAVE_ODF
    sprintf(tmp_path, "%s/%s", main_path, BDF_PICTOCHAT);
    err= parse_bdf(tmp_path, 32 /* from SPACE */, 8564 /* to one past the last character, "DOWNWARDS ARROW" */, &bdflib_info[0], 1);
    if(err < 0)
    {
        printf("BDF 0 initial error: %d\n", err);
        return -1;
    }
#else
    sprintf(tmp_path, "%s/%s", main_path, ODF_PICTOCHAT);
    err= init_from_odf(tmp_path, &bdflib_info[0]);
    if(err < 0)
    {
        printf("ODF 0 initial error: %d\n", err);
        return -1;
    }
#endif
    bdf_font= bdflib_info[0].fonts;
    font_height= bdflib_info[0].height;
    if(fonts_max_height < bdflib_info[0].height)
        fonts_max_height = bdflib_info[0].height;

#ifdef DUMP_ODF
    sprintf(tmp_path, "%s/%s", main_path, BDF_PICTOCHAT);
    err= dump2odf(tmp_path, &bdflib_info[0]);
    if(err < 0)
    {
        printf("BDF dump odf 0 error: %d\n", err);
    }
#endif

#ifndef HAVE_ODF
    sprintf(tmp_path, "%s/%s", main_path, BDF_SONG);
    err= parse_bdf(tmp_path, 0x4E00, 20902, &bdflib_info[1], 1);
    if(err < 0)
    {
        printf("BDF 1 initial error: %d\n", err);
        return -1;
    }
#else
    sprintf(tmp_path, "%s/%s", main_path, ODF_SONG);
    err= init_from_odf(tmp_path, &bdflib_info[1]);
    if(err < 0)
    {
        printf("ODF 1 initial error: %d\n", err);
        return -1;
    }
#endif
    bdf_nasci= bdflib_info[1].fonts;
    if(fonts_max_height < bdflib_info[1].height)
        fonts_max_height = bdflib_info[1].height;

#ifdef DUMP_ODF
    sprintf(tmp_path, "%s/%s", main_path, BDF_SONG);
    err= dump2odf(tmp_path, &bdflib_info[1]);
    if(err < 0)
    {
        printf("BDF dump odf 1 error: %d\n", err);
    }
#endif

    return 0;
}

/*-----------------------------------------------------------------------------
// release resource of BDF fonts
------------------------------------------------------------------------------*/
void BDF_font_release(void)
{
    u32 i;

    for(i= 0; i < BDF_LIB_NUM; i++)
    {
        if(bdflib_info[i].fonts)
            free((void*)bdflib_info[i].fonts);
        if(bdflib_info[i].mapmem)
            free((void*)bdflib_info[i].mapmem);
    }
}

/*----------------------------------------------------------------------------
//16-bit color
// Unicode Character
// back is background, 0x8000 is transparence, other are visable colors
------------------------------------------------------------------------------*/
u32 BDF_render16_ucs(void* screen_address, u32 screen_w, u32 v_align, u32 back, u32 front, u16 ch)
{
    unsigned short *screen, *screenp;
    unsigned char *map;
    u32 width, height, x_off, y_off, i, k, m, ret, fonts_height;
    unsigned char cc;
    struct bdffont *bdffontp;

	int font_num;
	bool found = 0;
	for (font_num = 0; font_num < BDF_LIB_NUM && !found; font_num++) {
    	if(bdflib_info[font_num].fonts != NULL)
    	{
			k = bdflib_info[font_num].start;
			if (ch < k)
				continue;
			m = k + bdflib_info[font_num].span;
	        if (ch >= m)
				continue;
            ch -= k;
           	bdffontp= bdflib_info[font_num].fonts;
           	fonts_height= bdflib_info[font_num].height;
			found = 1;
	    }
	}
	if (!found)
		return 8; // the width of an undefined character, not an error code

    width= bdffontp[ch].dwidth >> 16;
    ret= width;
    height= fonts_max_height;
    //if charactor is not transparent
    if(!(back & 0x8000))
    {
        for(k= 0; k < height; k++)
        {
            screenp= (unsigned short*)screen_address + k *screen_w;
            for(i= 0; i < width; i++)
                *screenp++ = back;
        }
    }

    width= bdffontp[ch].bbx >> 24;
    if(width == 0)
        return ret;

    height= (bdffontp[ch].bbx >> 16) & 0xFF;
    x_off= (bdffontp[ch].bbx >> 8) & 0xFF;
    y_off= bdffontp[ch].bbx & 0xFF;

    if(v_align== 0) //v align bottom
        screen= (unsigned short*)screen_address + x_off + (fonts_max_height - height - y_off) *screen_w;
    else if(v_align== 1) //v align center
        screen= (unsigned short*)screen_address + x_off + (fonts_max_height - height - y_off)/2 *screen_w;
    else //v align top
        screen= (unsigned short*)screen_address + x_off;

    x_off= width >> 3;
    y_off= width & 7;

    map= bdffontp[ch].bitmap;
    for(k= 0; k < height; k++)
    {
        screenp = screen + k *screen_w;
        i= x_off;
        while(i--)
        {
            m= 0x80;
            cc= *map++;
            while(m)
            {
                if(m & cc) *screenp = front;
                screenp++;
                m >>= 1;
            }
        }

        i= y_off;
        if(i)
        {
            i= 8 - y_off;
            cc= *map++;
            cc >>= i;
            m= 0x80 >> i;
            while(m)
            {
                if(m & cc) *screenp = front;
                screenp++;
                m >>= 1;
            }
        }
    }

    return ret;
}

/* Returns the width, in pixels, of a character given its UCS-16 codepoint. */
u32 BDF_width16_ucs(u16 ch)
{
    u32 k, ret;

	int font_num;
	for (font_num = 0; font_num < BDF_LIB_NUM; font_num++) {
    	if(bdflib_info[font_num].fonts != NULL)
    	{
			k = bdflib_info[font_num].start;
			if (ch < k)
				continue;
			if (ch > k + bdflib_info[font_num].span)
				continue;
            ch -= k;
           	return bdflib_info[font_num].fonts[ch].dwidth >> 16;
	    }
	}
	return 8; // the width of an undefined character, not an error code
}

/*-----------------------------------------------------------------------------
//16-bit color
// ASCII Character
// back is background, 0x8000 is transparence, other are visable colors
------------------------------------------------------------------------------*/
static u32 BDF_render16_font(char *screen_address, u32 back, u32 front, u16 ch)
{
    unsigned short *screen, *screenp;
    unsigned char *map;
    u32 width, height, x_off, y_off, i, k, m, ret;
    unsigned char cc;

    if(ch > 127)
        return 8;

    width= bdf_font[ch].dwidth >> 16;
    ret= width;
    height= font_height;
    //if charactor is not transparent
    if(!(back & 0x8000))
    {
        for(k= 0; k < height; k++)
        {
            screenp= (unsigned short*)screen_address + k *SCREEN_WIDTH;
            for(i= 0; i < width; i++)
                *screenp++ = back;
        }
    }

    width= bdf_font[ch].bbx >> 24;
    if(width == 0)
        return ret;
    height= (bdf_font[ch].bbx >> 16) & 0xFF;
    x_off= (bdf_font[ch].bbx >> 8) & 0xFF;
    y_off= bdf_font[ch].bbx & 0xFF;
    screen= (unsigned short*)screen_address + x_off + (font_height - height -y_off) *SCREEN_WIDTH;

    x_off= width >> 3;
    y_off= width & 7;

    map= bdf_font[ch].bitmap;
    for(k= 0; k < height; k++)
    {
        screenp = screen + k *SCREEN_WIDTH;
        i= x_off;
        while(i--)
        {
            m= 0x80;
            cc= *map++;
            while(m)
            {
                if(m & cc) *screenp = front;
                screenp++;
                m >>= 1;
            }
        }

        i= y_off;
        if(i)
        {
            i= 8 - y_off;
            cc= *map++;
            cc >>= i;
            m= 0x80 >> i;
            while(m)
            {
                if(m & cc) *screenp = front;
                screenp++;
                m >>= 1;
            }
        }
    }

    return ret;
}

/*-----------------------------------------------------------------------------
// ASCII Code Only
------------------------------------------------------------------------------*/
void BDF_render_string(void* screen_address, u32 x, u32 y, u32 back, u32 front, char *string)
{
    char *pt;
    u32 screenp, line_start;
    u32 width, line, cmp;

    pt= string;
    screenp= (u32)screen_address + (x + y *SCREEN_WIDTH)*2;
    line= 1 + y;
    line_start= (u32)screen_address + line *SCREEN_WIDTH *2;

    width= 0;
    while(*pt)
    {
        if(*pt == 0x0D)
        {
            pt++;
            continue;
        }
        if(*pt == 0x0A)
        {
            line += font_height;
            line_start= (u32)screen_address + line *SCREEN_WIDTH *2;
            screenp = line_start - SCREEN_WIDTH *2;
            pt++;
            continue;
        }

        cmp = (bdf_font[(u32)(*pt)].dwidth >> 16) << 1;
        if((screenp+cmp) >= line_start)
        {
            line += font_height;
            line_start= (u32)screen_address + line *SCREEN_WIDTH *2;
            screenp = line_start - SCREEN_WIDTH *2;
        }
        width= BDF_render16_font((char*)screenp, back, front, (u32)(*pt));
        screenp += width*2;
        pt++;
    }
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
char* utf8decode(char *utf8, u16 *ucs)
{
    unsigned char c = *utf8++;
    unsigned long code;
    int tail = 0;

    if ((c <= 0x7f) || (c >= 0xc2)) {
        /* Start of new character. */
        if (c < 0x80) {        /* U-00000000 - U-0000007F, 1 byte */
            code = c;
        } else if (c < 0xe0) { /* U-00000080 - U-000007FF, 2 bytes */
            tail = 1;
            code = c & 0x1f;
        } else if (c < 0xf0) { /* U-00000800 - U-0000FFFF, 3 bytes */
            tail = 2;
            code = c & 0x0f;
        } else if (c < 0xf5) { /* U-00010000 - U-001FFFFF, 4 bytes */
            tail = 3;
            code = c & 0x07;
        } else {
            /* Invalid size. */
            code = 0;
        }

        while (tail-- && ((c = *utf8++) != 0)) {
            if ((c & 0xc0) == 0x80) {
                /* Valid continuation character. */
                code = (code << 6) | (c & 0x3f);

            } else {
                /* Invalid continuation char */
                code = 0xfffd;
                utf8--;
                break;
            }
        }
    } else {
        /* Invalid UTF-8 char */
        code = 0;
    }
    /* currently we don't support chars above U-FFFF */
    *ucs = (code < 0x10000) ? code : 0;
    return utf8;
}

unsigned char* skip_utf8_unit(unsigned char* utf8, unsigned int num)
{
	while(num--)
	{
	    unsigned char c = *utf8++;
	    int tail = 0;
	    if ((c <= 0x7f) || (c >= 0xc2)) {
	        /* Start of new character. */
	        if (c < 0x80) {        /* U-00000000 - U-0000007F, 1 byte */
	        } else if (c < 0xe0) { /* U-00000080 - U-000007FF, 2 bytes */
	            tail = 1;
	        } else if (c < 0xf0) { /* U-00000800 - U-0000FFFF, 3 bytes */
	            tail = 2;
	        } else if (c < 0xf5) { /* U-00010000 - U-001FFFFF, 4 bytes */
	            tail = 3;
	        } else {			   /* Invalid size. */
	        }

	        while (tail-- && ((c = *utf8++) != 0)) {
	            if ((c & 0xc0) != 0x80) {
	                /* Invalid continuation char */
	                utf8--;
	                break;
	            }
	        }
	    }
	}

    /* currently we don't support chars above U-FFFF */
    return utf8;
}

/*-----------------------------------------------------------------------------
// UTF8 Code String
------------------------------------------------------------------------------*/
void BDF_render_mix(void* screen_address, u32 screen_w, u32 x, u32 y, u32 v_align, 
        u32 back, u32 front, char *string)
{
    char *pt;
    u32 screenp, line_start;
    u32 width, line, cmp, start, end;
    u16 unicode;
	struct bdffont *bdf_fontp[2];

    bdf_fontp[0]= bdflib_info[0].fonts;
    start= bdflib_info[1].start;
    end= start + bdflib_info[1].span;
    bdf_fontp[1]= bdflib_info[1].fonts;

    pt= string;
    screenp= (u32)screen_address + (x + y *screen_w)*2;
    line= 1 + y;
    line_start= (u32)screen_address + line *screen_w *2;

    width= 0;
    while(*pt)
    {
        pt= utf8decode(pt, &unicode);

        if(unicode == 0x0D) continue;
        if(unicode == 0x0A)
        {
            line += font_height;
            line_start= (u32)screen_address + line *screen_w *2;
            screenp = line_start - screen_w *2;
            continue;
        }

		/* If the text would go beyond the end of the line, go back to the
		 * start instead. */
		cmp = BDF_width16_ucs(unicode);

        if((screenp+cmp) >= line_start)
        {
            line += font_height;
            line_start= (u32)screen_address + line *screen_w *2;
            screenp = line_start - screen_w *2;
        }

        width= BDF_render16_ucs((unsigned short*)screenp, screen_w, v_align, back, front, unicode);
        screenp += width*2;
    }
}

/*-----------------------------------------------------------------------------
- count UNICODE charactor numbers in width pixels, input are UTF8, not UNICODE-16
- direction 0: count UNICODE charactor numbers in width pixels, from end, 
-	return bytes numbers
- direction 1: count UNICODE charactor numbers in width pixels, from front, 
-	return bytes numbers
- direction 2: count total pixel width of the string
------------------------------------------------------------------------------*/
u32 BDF_cut_string(char *string, u32 width, u32 direction)
{
    char *pt;
    u16 unicode[256];
    u32 len, xw;

    if(direction > 2) return -1;

    pt= string;
    len= 0;
    while(*pt)
    {
        pt= utf8decode(pt, &unicode[len]);
        if(unicode[len] != 0x0A)
        {
            len++;
            if(len >= 256) break;
        }
    }

    if(len >= 256) return -1;

    u16 *unicodep;
    if(direction == 0)
        unicodep= &unicode[len-1];
    else
        unicodep= &unicode[0];

    if(direction == 2) direction = 3;
    xw= BDF_cut_unicode(unicodep, len, width, direction);

    if(direction < 2)
    {
		if(direction < 1)
			xw= len - xw;

        pt= string;
        while(xw)
        {
            pt= utf8decode(pt, unicodep);
            if(unicode[xw] != 0x0A) xw--;
        }

        xw= pt -string;
    }

    return xw;
}

/*-----------------------------------------------------------------------------
- count UNICODE charactor numbers in width pixels
- direction 0: count UNICODE charactor numbers in width pixels, from front
- direction 1: count UNICODE charactor numbers in width pixels, from end
- direction 2: conut total pixel width of len UNICODE charachtors, from end
- direction 3: conut total pixel width of len UNICODE charachtors, from front
------------------------------------------------------------------------------*/
u32 BDF_cut_unicode(u16 *unicodes, u32 len, u32 width, u32 direction)
{
    u32 i, xw, num;
    u16 unicode;
    u32 start, end;
    struct bdffont *bdf_fontp[2];

    bdf_fontp[0]= bdflib_info[0].fonts;
    start= bdflib_info[1].start;
    end= start + bdflib_info[1].span;
    bdf_fontp[1]= bdflib_info[1].fonts;

    if(direction < 2)
    {
        if(direction < 1)   direction = -1;

        i= 0;
        xw = 0;
        num= len;
		while(len > 0)
		{
			unicode= unicodes[i];
			xw += BDF_width16_ucs(unicode);

			if(xw >= width) break;
			i += direction;
			len--;
		}

		num -= len;
	}
    else
    {
        if(direction < 3)   direction = -1;
        else    direction = 1;

        i= 0;
        xw = 0;
        while(len-- > 0)
        {
            unicode= unicodes[i];
			xw += BDF_width16_ucs(unicode);
            i += direction;
        }

        num= xw;
    }

    return num;
}


