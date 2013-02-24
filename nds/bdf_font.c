/******************************************************************************
*
-------------------------------------------------------------------------------
* File name: bdf_font.c
* Discription: printf BDF font
* Current version: 1.0
-------------------------------------------------------------------------------
* Primery author: DengYong
* Date: 2009.04.03
* Version: 
-------------------------------------------------------------------------------
* Modified by: 
* Date: 
* Comment:
* Version:
*******************************************************************************/
#include "stdlib.h"
#include "bdf_font.h"
#include "common.h"

#define BDF_VERDANA "SYSTEM\\verdana.bdf"
#define BDF_SONG "SYSTEM\\song.bdf"
#define ODF_VERDANA "SYSTEM\\verdana.odf"
#define ODF_SONG "SYSTEM\\song.odf"

#define HAVE_ODF 
//#define DUMP_ODF

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
    u32 a, b, len;

    len= 0;
    map= (unsigned char*)bitmap;
    while(*map)
    {
        //character to number, we assume the character can convert to number!
        if(*map != 0x0A)
        {
            if(*map <= 0x39) a= *map - 0x30;
            else             a= *map - 0x37;
            map++;

            if(*map <= 0x39) b= *map - 0x30;
            else             b= *map - 0x37;

            *code++ = (a << 4) | b;
            len++;
        }
        map++;
    }

    return len;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static u32 hatoi(char *string)
{
    char *pt;
    u32 ret, num;
    
    pt= string;
    ret= 0;
    while(*pt)
    {
        num= (((u32)*pt) & 0xFF) - 0x30;
        if(num <= 0x9)
            ret= (ret<<4) | num;
        else if(num <= 0x16)
        {
            if(num >= 0x11)
                ret= (ret<<4) | (num-0x7);
            else
                break;
        }
        else
            break;
        pt++;
    }

    return ret;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
/*
* example
*
* STARTCHAR 2264
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

    bdflibinfop -> start= start;
    bdflibinfop -> span= span;

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
            i= hatoi(pt +10);
            if(i < start) continue;
            else if(i < end) break;
            else    //Not found the start
            {
                ret= -7;
                goto parse_bdf_error;
            }
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
        if(index >= end) break;

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
            free((unsigned int)bdflibinfop -> fonts);
        if(bdflibinfop -> mapmem != NULL)
            free((unsigned int)bdflibinfop -> mapmem);
        bdflibinfop -> fonts = NULL;
        bdflibinfop -> mapmem = NULL;
    }
    else
    {
        bdflibinfop -> maplen = length;
        bdflibinfop -> mapmem = (unsigned char*)realloc((unsigned int)bdflibinfop -> mapmem, length);
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
        free((unsigned int)bdffontp);
        fclose(fp);
        return -7;
    }

    pt= (char*)malloc(maplen);
    len= fread(pt, 1, maplen, fp);
    if(len != maplen)
    {
        free((unsigned int)bdffontp);
        free((unsigned int)pt);
        fclose(fp);
        return -8;
    }

    bdflibinfop -> mapmem = pt;
    bdflibinfop -> fonts = bdffontp;

    u32 i, j;
    j= bdflibinfop -> mapmem;
    for(i= 0; i < span; i++)
        bdffontp[i].bitmap += j;

    fclose(fp);
    return 0;
}

#if 0
int BDF_font_init(void)
{
    FILE *fp;
    char string[256];
    char map[256];
    char *pt;
    unsigned char *bitbuff;
    int i, num, x_off, y_off;
    int lib_i;
    u32 tmp;

    lib_i =0;
    fp= fopen(BDF_LIB, "r");    //Open bdf font library
    if(fp == NULL)
        return -1;

    while(1)
    {
        pt= fgets(string, 255, fp);
        if(pt == NULL)
        {
            fclose(fp);
            return -1;
        }
        if(!(strncasecmp(string, "SIZE ", 5)))
            break;
    }

    //FONTBOUNDINGBOX
    pt= fgets(string, 255, fp);
    pt += 16;
    bdflib_info[lib_i].width= atoi(pt);
    pt = 1 + strchr(pt, ' ');
    bdflib_info[lib_i].height= atoi(pt);
    pt = 1 + strchr(pt, ' ');
    x_off= atoi(pt);
    bdflib_info[lib_i].xoff= x_off;
    pt = 1 + strchr(pt, ' ');
    y_off= atoi(pt);
    bdflib_info[lib_i].yoff= y_off;
    
    font_width= bdflib_info[lib_i].width;
    font_height= bdflib_info[lib_i].height;

    while(1)
    {
        pt= fgets(string, 255, fp);
        if(pt == NULL)
        {
            fclose(fp);
            return -1;
        }
        if(!(strncasecmp(string, "CHARS ", 6)))
            break;
    }

    bitbuff= (unsigned char*)malloc(2048);
    bdf_font[0].bitmap= bitbuff;

    for(i= 0; i<128; i++)
    {
        bdf_font[i].dwidth= font_width<<16;
        bdf_font[i].bbx= 0;
    }

    while(1)
    {
        //STARTCHAR
        while(1)
        {
            pt= fgets(string, 255, fp);
            if(pt == NULL)
                break;
            if(!(strncasecmp(string, "STARTCHAR ", 10)))
                break;
        }
        //ENCODING
        pt= fgets(string, 255, fp);
        if(pt == NULL)
            break;
        pt= string + 9;
        i= atoi(pt);
        if(i > 127)
            break;
        //SWIDTH
        pt= fgets(string, 255, fp);
        if(pt == NULL)
            break;
        //DWIDTH
        pt= fgets(string, 255, fp);
        if(pt == NULL)
            break;
        pt += 7;
        num= atoi(pt);
        pt= 1+ strchr(pt, ' ');
        num= num*65536 + atoi(pt);

        bdf_font[i].dwidth= num;
        //BBX
        pt= fgets(string, 255, fp);
        if(pt == NULL)
            break;

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

        bdf_font[i].bbx= tmp;
        //BITMAP
        pt= fgets(string, 255, fp);
        if(pt == NULL)
            break;

        map[0]= '\0';
        while(1)
        {
            pt= fgets(string, 255, fp);
            if(pt == NULL)
                break;
            if(!strncasecmp(pt, "ENDCHAR", 7))
                break;
            strcat(map, pt);
        }

        tmp = bitmap_code(bitbuff, map);

        if(tmp)
            bdf_font[i].bitmap = bitbuff;
        else
            bdf_font[i].bitmap = NULL;

        bitbuff += tmp;
    }
    
    fclose(fp);

//BDF_render_string((char*)screen_address, 10, 10, 0x0, 0x8888, "Hello A B CDEFGHIJKLMNOPQRSTUVWX YZ\
//abcdefghijklmnopqrstuvwxyz `~!@#$%^&*()_+-={[}]:;\"0'|\\<,>.?/!012345678908");
//flip_screen();
//while(1);

    return 0;
}
#endif

//char ucs_string[32]= {0x31, 0x32, 0x33, 0x2d, 0xe8, 0xb6, 0x85, 0xe7, 0xba, 
//  0xa7, 0xe8, 0xb5, 0x9b, 0xe8, 0xbd, 0xa6, 0x2e, 0x67, 0x62, 0x61, 0};

int BDF_font_init(void)
{
    int err;
    char tmp_path[MAX_PATH];

    fonts_max_height= 0;
#ifndef HAVE_ODF
    sprintf(tmp_path, "%s\\%s", main_path, BDF_VERDANA);
    err= parse_bdf(tmp_path, 0, 128, &bdflib_info[0], 0);
    if(err < 0)
    {
        printf("BDF 0 initial error: %d\n", err);
        return -1;
    }
#else
    sprintf(tmp_path, "%s\\%s", main_path, ODF_VERDANA);
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
    sprintf(tmp_path, "%s\\%s", main_path, BDF_VERDANA);
    err= dump2odf(tmp_path, &bdflib_info[0]);
    if(err < 0)
    {
        printf("BDF dump odf 0 error: %d\n", err);
    }
#endif

#ifndef HAVE_ODF
    sprintf(tmp_path, "%s\\%s", main_path, BDF_SONG);
    err= parse_bdf(tmp_path, 0x4E00, 20902, &bdflib_info[1], 1);
    if(err < 0)
    {
        printf("BDF 1 initial error: %d\n", err);
        return -1;
    }
#else
    sprintf(tmp_path, "%s\\%s", main_path, ODF_SONG);
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
    sprintf(tmp_path, "%s\\%s", main_path, BDF_SONG);
    err= dump2odf(tmp_path, &bdflib_info[1]);
    if(err < 0)
    {
        printf("BDF dump odf 1 error: %d\n", err);
    }
#endif


//BDF_render_mix((char*)screen_address, 10, 10, 0x0, 0x8888, ucs_string);
//flip_screen();
//while(1);

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
            free((int)bdflib_info[i].fonts);
        if(bdflib_info[i].mapmem)
            free((int)bdflib_info[i].mapmem);
    }
}

/*-----------------------------------------------------------------------------
//16-bit color
------------------------------------------------------------------------------*/
u32 BDF_render16_ucs(u16 *screen_address, u32 screen_w, u32 v_align, u32 back, u32 front, u16 ch)
{
    unsigned short *screen, *screenp;
    unsigned char *map;
    u32 width, height, x_off, y_off, i, k, m, ret, fonts_height;
    unsigned char cc;
    struct bdffont *bdffontp;

    if(ch < 128)
    {
        bdffontp= bdflib_info[0].fonts;
        fonts_height= bdflib_info[0].height;
    }
    else if(bdflib_info[1].fonts != NULL)
    {
        k= bdflib_info[1].start;
        m= k + bdflib_info[1].span;
        if(ch >= k && ch < m)
        {
            ch -= k;
            bdffontp= bdflib_info[1].fonts;
            fonts_height= bdflib_info[0].height;
        }
        else
            return 8;
    }
    else 
        return 8;

    width= bdffontp[ch].dwidth >> 16;
    ret= width;
    height= fonts_max_height;
    //if charactor is not transparent
    if(!(back & 0x8000))
    {
        for(k= 0; k < height; k++)
        {
            screenp= screen_address + k *screen_w;
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
        screen= screen_address + x_off + (fonts_max_height - height - y_off) *screen_w;
    else if(v_align== 1) //v align center
        screen= screen_address + x_off + (fonts_max_height - height - y_off)/2 *screen_w;
    else //v align top
        screen= screen_address + x_off;

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

/*-----------------------------------------------------------------------------
//16-bit color
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

//printf("******************************\n");
//printf("%d %d %d %d\n", width, height, x_off, y_off);
//printf("ch %02x\n", ch);
    map= bdf_font[ch].bitmap;
    for(k= 0; k < height; k++)
    {
        screenp = screen + k *SCREEN_WIDTH;
        i= x_off;
        while(i--)
        {
            m= 0x80;
            cc= *map++;
//printf("%02x ", cc);
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
//printf("%02x ", cc);
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

//printf("\n");
    return ret;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void BDF_render_string(char *screen_address, u32 x, u32 y, u32 back, u32 front, char *string)
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
const char* utf8decode(const char *utf8, u16 *ucs)
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

static u8 utf8_ucs2(const char *utf8, u16 *ucs)
{
  while(*utf8 !='\0')
  {
    utf8 = utf8decode(utf8, ucs++);
  }
  *ucs = '\0';
  return 0;
}

static u32 ucslen(const u16 *ucs)
{
  u32 len = 0;
  while(ucs[len] != '\0')
    len++;
  return len;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void BDF_render_mix(char *screen_address, u32 screen_w, u32 x, u32 y, u32 v_align, 
        u32 back, u32 front, char *string)
{
    char *pt;
    u32 screenp, line_start;
    u32 width, line, cmp;
    u16 unicode;

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

        cmp = (bdf_font[(u32)(*pt)].dwidth >> 16) << 1;
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
- return the total font's width
------------------------------------------------------------------------------*/
/*u32 BDF_string_width(char *string, u32 *len)
{
    char *pt;
    u32 width, width_max;
    u16 unicode;
    struct bdffont *bdf_fontp[2];

    bdf_fontp[0]= bdflib_info[0].fonts;
    bdf_fontp[1]= bdflib_info[1].fonts;
    pt = string;
    width= 0;
    width_max= 0;
    *len= 0;
    while(*pt)
    {
        pt= utf8decode(pt, &unicode);
        *len += 1;
        //when return line, count the new line's width
        if(unicode == 0x0A)
        if(width_max < width)
        {
            width_max = width;
            width = 0;
            continue;
        }

        if(unicode > 127)
            width += bdf_fontp[0][unicode].dwidth>>16;
        else
            width += bdf_fontp[1][unicode].dwidth>>16;
    }
}*/

/*-----------------------------------------------------------------------------
- 计算 width 像素宽度包含的 UNICODE 字符个数, 但是输入是 string, 不是UNICODE
- direction 0: 计算 width 像素宽度包含的 UNICODE 字符个数, 反向, 返回字节数
- direction 1: 计算 width 像素宽度包含的 UNICODE 字符个数, 正向, 返回字节数
- direction 2: 计算 string 的像素宽度
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
        if(unicode != 0x0A)
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
        xw= len - xw;
        pt= string;
        while(xw)
        {
            pt= utf8decode(pt, unicodep);
            if(unicode != 0x0A) xw--;
        }

        xw= pt -string;
    }

    return xw;
}

/*-----------------------------------------------------------------------------
- 计算 width 像素宽度包含的 UNICODE 字符个数
- direction 0: 计算 width 像素宽度包含的 UNICODE 字符个数, 反向
- direction 1: 计算 width 像素宽度包含的 UNICODE 字符个数, 正向
- direction 2: 计算 len 个 UNICODE 字符的像素宽度, 反向
- direction 3: 计算 len 个 UNICODE 字符的像素宽度, 正向
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
    bdf_fontp[1]= bdflib_info[1].fonts - start;

    if(direction < 2)
    {
        if(direction < 1)   direction = -1;

        i= 0;
        xw = 0;
        num= len;
        while(len-- > 0)
        {
            unicode= unicodes[i];
            if(unicode < 128)
                xw += bdf_fontp[0][unicode].dwidth>>16;
            else if(unicode >= start && unicode < end)
                xw += bdf_fontp[1][unicode].dwidth>>16;
    
            if(xw >= width) break;
            i += direction;
        }
    
        num -= len;
        if(len) num -= 1;
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
            if(unicode < 128)
                xw += bdf_fontp[0][unicode].dwidth>>16;
            else if(unicode >= start && unicode < end)
                xw += bdf_fontp[1][unicode].dwidth>>16;
            i += direction;
        }

        num= xw;
    }

    return num;
}


