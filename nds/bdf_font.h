#ifndef __BDF_FONT_H__
#define __BDF_FONT_H__

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
extern void BDF_render_string(char *screen_address, unsigned int x, unsigned int y, unsigned int back, 
    unsigned int front, char *string);
extern unsigned int BDF_render16_ucs(unsigned short *screen_address, unsigned int screen_w, 
    unsigned int v_align, unsigned int back, unsigned int front, unsigned short ch);
extern void BDF_render_mix(char *screen_address, unsigned int screen_w, unsigned int x, 
    unsigned int y, unsigned int v_align, unsigned int back, unsigned int front, char *string);
//extern unsigned int BDF_string_width(char *string, unsigned int *len);
extern const char* utf8decode(const char *utf8, unsigned short *ucs);
extern unsigned int BDF_cut_unicode(unsigned short *unicodes, unsigned int len, unsigned int width, unsigned int direction);
extern unsigned int BDF_cut_string(char *string, unsigned int width, unsigned int direction);
extern void BDF_font_release(void);

#endif //__BDF_FONT_H__
