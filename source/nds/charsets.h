#ifndef __CHARSETS_H__
#define __CHARSETS_H__

extern unsigned int charsets_utf32_conv(const unsigned char * ucs, unsigned char * cjk);
extern unsigned int charsets_ucs_conv(const unsigned char * uni, unsigned char * cjk);
extern void charsets_big5_conv(const unsigned char * big5, unsigned char * cjk);
extern void charsets_sjis_conv(const unsigned char * jis, unsigned char ** cjk, unsigned int * newsize);
extern unsigned int charsets_utf8_conv(const unsigned char * ucs, unsigned char * cjk);
extern unsigned int charsets_utf16_conv(const unsigned char * ucs, unsigned char * cjk);
extern unsigned int charsets_utf16be_conv(const unsigned char * ucs, unsigned char * cjk);
extern unsigned short charsets_gbk_to_ucs(const unsigned char * cjk);

#endif //__CHARSETS_H__
