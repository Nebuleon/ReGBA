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

#include "common.h"


#define BDF_PICTOCHAT "SYSTEM/Pictochat-16.bdf"
#define BDF_SONG "SYSTEM/song.bdf"
#define ODF_PICTOCHAT "SYSTEM/Pictochat-16.odf"
#define ODF_SONG "SYSTEM/song.odf"

#define HAVE_ODF // Define this if you have generated Pictochat-16.odf [Neb]
// #define DUMP_ODF // Define this if you want to regenerate Pictochat-16.odf [Neb]

#define BDF_LIB_NUM 2

static const uint8_t ODF_SIGNATURE[] = { 'O', 'D', 'F', 0 };
static const uint8_t ODF_VERSION[] = { '1', '.', '0', 0 };

struct bdflibinfo bdflib_info[BDF_LIB_NUM];
static uint32_t fonts_max_height;

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static uint32_t bitmap_code(uint8_t* code, const char* bitmap)
{
	uint8_t a, b;
	uint32_t len = 0;

	len = 0;
	while (*bitmap) {
		// One hex character represents the state of 4 successive pixels
		if (*bitmap != 0x0A) {
			if      (*bitmap <= '9') a = *bitmap - '0';
			else if (*bitmap <= 'F') a = *bitmap - 'A' + 10;
			else if (*bitmap <= 'f') a = *bitmap - 'a' + 10;
			bitmap++;

			if      (*bitmap <= '9') b = *bitmap - '0';
			else if (*bitmap <= 'F') b = *bitmap - 'A' + 10;
			else if (*bitmap <= 'f') b = *bitmap - 'a' + 10;

			*code++ = (a << 4) | b;
			len++;
		}
		bitmap++;
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
static int parse_bdf(const char* filename, uint32_t start, uint32_t span, struct bdflibinfo *bdflibinfop, uint32_t method)
{
	FILE *fp;
	char string[256];
	char map[256];
	char *pt;
	uint8_t *bitbuff;
	int num, x_off, y_off, ret = 0;
	uint32_t tmp, i, end, length, index;
	struct bdffont *bdffontp;

	bdflibinfop->width = 0;
	bdflibinfop->height = 0;
	bdflibinfop->start = 0;
	bdflibinfop->span = 0;
	bdflibinfop->maplen = 0;
	bdflibinfop->mapmem = NULL;
	bdflibinfop->fonts = NULL;

	fp = fopen(filename, "r");
	if (fp == NULL)
		return -1;

	// SIZE
	do {
		pt = fgets(string, sizeof(string), fp);
		if (pt == NULL) {
			ret = -2;
			goto parse_bdf_error;
		}
	} while (strncasecmp(string, "SIZE ", 5) != 0);

	// FONTBOUNDINGBOX (assumed after SIZE, don't know if that's correct)
	pt = fgets(string, sizeof(string), fp);
	if (pt == NULL) {
		ret = -2;
		goto parse_bdf_error;
	}
	pt += 16;
	bdflibinfop->width = atoi(pt);
	pt = strchr(pt, ' ') + 1;
	bdflibinfop->height = atoi(pt);
	pt = strchr(pt, ' ') + 1;
	x_off = atoi(pt);
	pt = strchr(pt, ' ') + 1;
	y_off = atoi(pt);

	// skip until CHARS
	do {
		pt = fgets(string, sizeof(string), fp);
		if (pt == NULL) {
			ret = -2;
			goto parse_bdf_error;
		}
	} while (strncasecmp(string, "CHARS ", 6) != 0);
	pt += 6;
	ret = atoi(pt);

	switch (method) {
	case 0:
	default:
		bdflibinfop->span = span + start;
		break;
	case 1:
		bdflibinfop->start = start;
		bdflibinfop->span = span;
		break;
	}

	// construct bdf font information
	bdffontp = malloc(span * sizeof(struct bdffont));
	if (bdffontp == NULL) {
		ret = -4;
		goto parse_bdf_error;
	}
	bdflibinfop->fonts = bdffontp;

	bitbuff = malloc((bdflibinfop->width * bdflibinfop->height * span) / 8);
	if (bitbuff == NULL) {
		ret = -5;
		goto parse_bdf_error;
	}
	bdflibinfop->mapmem = bitbuff;

	tmp = bdflibinfop->width << 16;
	for (i = 0; i < span; i++) {
		bdffontp[i].dwidth = tmp;
		bdffontp[i].bbx = 0;
	}

	end = start + span;
	// STARTCHAR
	do {
		pt = fgets(string, sizeof(string), fp);
		if (pt == NULL)
		{
			ret = -6;
			goto parse_bdf_error;
		}
	} while (strncasecmp(string, "STARTCHAR ", 10) != 0);

	i = 0;
	length = 0;
	while (1) {
		// ENCODING
		do {
			pt = fgets(string, sizeof(string), fp);
			if (pt == NULL) {
				ret = -2;
				goto parse_bdf_error;
			}
		} while (strncasecmp(string, "ENCODING ", 9) != 0);

		pt = string + 9;
		index = atoi(pt);
		if(index < start || index >= end) break;

		if (method == 0)      i = index;
		else if (method == 1) i = index-start;
		else                  i++;

		// SWIDTH
		pt = fgets(string, sizeof(string), fp);
		if (pt == NULL) {
			ret = -8;
			goto parse_bdf_error;
		}

		// DWIDTH
		pt = fgets(string, sizeof(string), fp);
		if (pt == NULL) {
			ret = -9;
			goto parse_bdf_error;
		}

		pt += 7;
		num = atoi(pt);
		tmp = num << 16;
		pt = strchr(pt, ' ') + 1;
		num = atoi(pt);
		tmp |= num & 0xFFFF;

		bdffontp[i].dwidth = tmp;

		// BBX
		pt = fgets(string, sizeof(string), fp);
		if (pt == NULL) {
			ret = -10;
			goto parse_bdf_error;
		}

		pt += 4;
		num = atoi(pt);
		tmp = num & 0xFF;

		pt = strchr(pt, ' ') + 1;
		num = atoi(pt);
		tmp = (tmp << 8) | (num & 0xFF);

		pt = strchr(pt, ' ') + 1;
		num = atoi(pt);
		num = num - x_off;
		tmp = (tmp << 8) | (num & 0xFF);

		pt = strchr(pt, ' ') + 1;
		num = atoi(pt);
		num = num - y_off;
		tmp = (tmp << 8) | (num & 0xFF);

		bdffontp[i].bbx = tmp;

		// BITMAP
		pt = fgets(string, sizeof(string), fp);
		if (pt == NULL) {
			ret= -11;
			goto parse_bdf_error;
		}

		map[0] = '\0';
		while (1) {
			pt = fgets(string, sizeof(string), fp);
			if (pt == NULL) {
				ret = -12;
				goto parse_bdf_error;
			}
			if (strncasecmp(pt, "ENDCHAR", 7) == 0)
				break;
			strcat(map, pt);
		}

		tmp = bitmap_code(bitbuff, map);

		if (tmp)
			bdffontp[i].bitmap = bitbuff;
		else
			bdffontp[i].bitmap = NULL;

		bitbuff += tmp;
		length += tmp;
	}

parse_bdf_error:
	fclose(fp);
	if (ret < 0) {
		if (bdflibinfop->fonts != NULL) {
			free(bdflibinfop->fonts);
			bdflibinfop->fonts = NULL;
		}
		if (bdflibinfop->mapmem != NULL) {
			free(bdflibinfop->mapmem);
			bdflibinfop->mapmem = NULL;
		}
	} else {
		bdflibinfop->maplen = length;
		bdflibinfop->mapmem = realloc(bdflibinfop->mapmem, length);
	}

	return ret;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static int dump2odf(const char* filename, const struct bdflibinfo* bdflibinfop)
{
	uint8_t *pt;
	char* dot;
	char new_filename[MAX_PATH];
	FILE *fp;
	uint8_t* mapaddr;
	struct bdffont* fontaddr;
	uint32_t num;
	uint8_t buff[1024];
	uint32_t i, j;

	strcpy(new_filename, filename);
	dot = strrchr(new_filename, '.');
	if (dot != NULL && strcasecmp(dot, ".bdf") == 0)
		strcpy(dot, ".odf");

	fp = fopen(new_filename, "wb");
	if (fp == NULL)
		return -2;

	pt = buff;
	memcpy(pt, ODF_SIGNATURE, sizeof(ODF_SIGNATURE));
	pt += sizeof(ODF_SIGNATURE);
	memcpy(pt, ODF_VERSION, sizeof(ODF_VERSION));
	pt += sizeof(ODF_VERSION);

	struct bdflibinfo *bdflibinfo_i;

	memcpy(pt, bdflibinfop, sizeof(struct bdflibinfo));
	bdflibinfo_i = (struct bdflibinfo*) pt;
	/* Adjust these pointers in the copy to NULL. */
	bdflibinfo_i->mapmem = NULL;
	bdflibinfo_i->fonts = NULL;
	pt += sizeof(struct bdflibinfo);

	num = pt - buff;
	fwrite(buff, num, 1, fp);     //write odf file header

	num = bdflibinfop->span;
	mapaddr = bdflibinfop->mapmem;
	fontaddr = bdflibinfop->fonts;

	while (num > 0) {
		struct bdffont *bdffontp;

		i = sizeof(buff) / sizeof(struct bdffont);
		if (num > i) num -= i;
		else         i = num, num = 0;
		
		memcpy(buff, fontaddr, i * sizeof(struct bdffont));
		fontaddr += i;
		bdffontp = (struct bdffont*) buff;
		
		for (j = 0; j < i; j++)
			/* Flatten the pointers, writing only the offset from the map. */
			bdffontp[j].bitmap -= (uintptr_t) mapaddr;

		fwrite(buff, sizeof(struct bdffont), i, fp);
	}

	fwrite(mapaddr, 1, bdflibinfop->maplen, fp);

	fclose(fp);
	return 0;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int init_from_odf(const char* filename, struct bdflibinfo* bdflibinfop)
{
	int ret = 0;
	FILE *fp;
	uint8_t buff[512];
	uint8_t *pt;
	uint32_t len, tmp;
	uint32_t span, maplen, i;
	struct bdffont *bdffontp;

	bdflibinfop->width = 0;
	bdflibinfop->height = 0;
	bdflibinfop->start = 0;
	bdflibinfop->span = 0;
	bdflibinfop->maplen = 0;
	bdflibinfop->mapmem = NULL;
	bdflibinfop->fonts = NULL;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		return -1;

	tmp = 8 + sizeof(struct bdflibinfo);
	len = fread(buff, 1, tmp, fp);
	if (len < tmp) {
		ret = -2;
		goto failed_with_file;
	}

	pt = buff;
	if (memcmp(pt, ODF_SIGNATURE, 4) != 0) {
		ret = -2;
		goto failed_with_file;
	}

	pt += 4;
	if (memcmp(pt, ODF_VERSION, 4) != 0) {
		ret = -3;
		goto failed_with_file;
	}

	pt += 4;
	memcpy(bdflibinfop, pt, sizeof(struct bdflibinfo));

	span = bdflibinfop->span;
	if (span == 0) {
		ret = -4;
		goto failed_with_file;
	}

	maplen = bdflibinfop->maplen;
	if (maplen == 0) {
		ret = -5;
		goto failed_with_file;
	}

	bdffontp = malloc(span * sizeof(struct bdffont));
	if (bdffontp == NULL) {
		ret = -6;
		goto failed_with_file;
	}

	len = fread(bdffontp, sizeof(struct bdffont), span, fp);
	if (len != span)
	{
		ret = -7;
		goto failed_with_bdffontp;
	}

    pt = malloc(maplen);
	if (bdffontp == NULL) {
		ret = -6;
		goto failed_with_bdffontp;
	}

	len = fread(pt, 1, maplen, fp);
	if (len != maplen) {
		ret = -8;
		goto failed_with_map;
	}

	bdflibinfop->mapmem = pt;
	bdflibinfop->fonts = bdffontp;

	for (i = 0; i < span; i++)
		/* Expand the pointers, adding the address of the map to the offset. */
		bdffontp[i].bitmap += (uintptr_t) bdflibinfop->mapmem;

	fclose(fp);
	return 0;

failed_with_map:
	free(pt);
failed_with_bdffontp:
	free(bdffontp);
failed_with_file:
	fclose(fp);
	return ret;
}

int BDF_font_init(void)
{
	int err;
	char tmp_path[MAX_PATH];

	fonts_max_height = 0;
#ifndef HAVE_ODF
	sprintf(tmp_path, "%s/%s", main_path, BDF_PICTOCHAT);
	err = parse_bdf(tmp_path, 32 /* from SPACE */, 8564 /* to one past the last character, "DOWNWARDS ARROW" */, &bdflib_info[0], 1);
	if (err < 0) {
		printf("BDF 0 initial error: %d\n", err);
		return -1;
	}
#else
	sprintf(tmp_path, "%s/%s", main_path, ODF_PICTOCHAT);
	err = init_from_odf(tmp_path, &bdflib_info[0]);
	if (err < 0) {
		printf("ODF 0 initial error: %d\n", err);
		return -1;
	}
#endif
	fonts_max_height = bdflib_info[0].height;

#ifdef DUMP_ODF
	sprintf(tmp_path, "%s/%s", main_path, BDF_PICTOCHAT);
	err = dump2odf(tmp_path, &bdflib_info[0]);
	if (err < 0) {
		printf("BDF dump odf 0 error: %d\n", err);
	}
#endif

#ifndef HAVE_ODF
	sprintf(tmp_path, "%s/%s", main_path, BDF_SONG);
	err = parse_bdf(tmp_path, 0x4E00, 20902, &bdflib_info[1], 1);
	if (err < 0) {
		printf("BDF 1 initial error: %d\n", err);
		return -1;
	}
#else
	sprintf(tmp_path, "%s/%s", main_path, ODF_SONG);
	err = init_from_odf(tmp_path, &bdflib_info[1]);
	if (err < 0) {
		printf("ODF 1 initial error: %d\n", err);
		return -1;
	}
#endif
	if (fonts_max_height < bdflib_info[1].height)
		fonts_max_height = bdflib_info[1].height;

#ifdef DUMP_ODF
	sprintf(tmp_path, "%s/%s", main_path, BDF_SONG);
	err = dump2odf(tmp_path, &bdflib_info[1]);
	if (err < 0) {
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
	size_t i;

	for (i = 0; i < BDF_LIB_NUM; i++)
	{
		if (bdflib_info[i].fonts != NULL) {
			free(bdflib_info[i].fonts);
			bdflib_info[i].fonts = NULL;
		}
		if (bdflib_info[i].mapmem != NULL) {
			free(bdflib_info[i].mapmem);
			bdflib_info[i].mapmem = NULL;
		}
	}
}

uint32_t BDF_render16_ucs(uint16_t* screen, size_t screen_w, uint16_t bg_color, uint16_t fg_color, uint16_t ch)
{
	uint16_t *screenp;
	uint_fast16_t width, x, y;
	uint32_t ret;
	uint_fast8_t height;
	struct bdffont* bdffontp;

	{
		int font_num;
		bool found = false;
		for (font_num = 0; font_num < BDF_LIB_NUM && !found; font_num++) {
			if (bdflib_info[font_num].fonts != NULL) {
				uint32_t start = bdflib_info[font_num].start;
				if (ch < start || ch >= start + bdflib_info[font_num].span)
					continue;
				ch -= start;
				bdffontp = bdflib_info[font_num].fonts;
				found = true;
			}
		}
		if (!found)
			return 8; // the width of an undefined character, not an error code
	}

	ret = width = (uint16_t) (bdffontp[ch].dwidth >> 16);
	if (!(bg_color & 0x8000)) {
		/* If the background is not transparent, draw it. */
		for (y = 0; y < fonts_max_height; y++) {
			screenp = screen + y * screen_w;
			for (x = 0; x < width; x++)
				*screenp++ = bg_color;
		}
	}

	width = (uint8_t) (bdffontp[ch].bbx >> 24);
	if (width == 0)
		return ret;

	{
		height = (uint8_t) (bdffontp[ch].bbx >> 16);
		uint_fast8_t x_off = (uint8_t) (bdffontp[ch].bbx >> 8);
		uint_fast8_t y_off = (uint8_t) bdffontp[ch].bbx;

		/* Align the baseline of each glyph properly. */
		screen += x_off + (fonts_max_height - height - y_off) * screen_w;
	}

	{
		uint_fast16_t bytes = width >> 3, bits = width & 7;
		uint8_t* map = bdffontp[ch].bitmap;

		for (y = 0; y < height; y++) {
			size_t byte;
			uint_fast8_t data, mask;
			screenp = screen + y * screen_w;
			for (byte = 0; byte < bytes; byte++) {
				data = *map++;
				for (mask = UINT8_C(0x80); mask != 0; mask >>= 1) {
					if (data & mask) {
						*screenp = fg_color;
					}
					screenp++;
				}
			}

			if (bits > 0) {
				/* Search only the top 'bits' bits of the last byte.
				 * Bring down the data and mask by '8 - bits' bits. */
				uint_fast8_t shift = 8 - bits;
				data = *map++ >> shift;
				for (mask = UINT8_C(0x80) >> shift; mask != 0; mask >>= 1) {
					if (data & mask) {
						*screenp = fg_color;
					}
					screenp++;
				}
			}
		}
	}

	return ret;
}

/*-----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
const char* utf8decode(const char *utf8, uint16_t *ucs)
{
	unsigned char c = *utf8++;
	uint32_t code;
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
				/* Invalid continuation character. */
				code = 0xfffd;
				utf8--;
				break;
			}
		}
	} else {
		/* Invalid UTF-8 char */
		code = 0;
	}
	/* Currently we don't support chars above U+FFFF. */
	*ucs = (code < 0x10000) ? code : 0;
	return utf8;
}

unsigned char* skip_utf8_unit(unsigned char* utf8, size_t num)
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
void BDF_render_mix(uint16_t* screen, size_t screen_w, uint32_t x, uint32_t y,
        uint16_t bg_color, uint16_t fg_color, const char* string)
{
	const char* pt = string;
	uint32_t line = y + 1, cmp;
	uint16_t unicode;
	uint16_t* screenp = screen + (y * screen_w + x);
	uint16_t* line_end = screen + line * screen_w;

	while (*pt) {
		pt = utf8decode(pt, &unicode);

		if (unicode == 0x0D)
			continue;
		else if (unicode == 0x0A) {
			line += fonts_max_height;
			line_end = screen + line * screen_w;
			/* Return to the original 'x' coordinate on a new line. */
			screenp = line_end - screen_w + x;
			continue;
		}

		/* If the text would go beyond the end of the line, go back to the
		 * original 'x' coordinate instead on a new line. */
		cmp = BDF_WidthUCS2(unicode);

		if (screenp + cmp >= line_end) {
			line += fonts_max_height;
			line_end = screen + line * screen_w;
			screenp = line_end - screen_w + x;
		}

		screenp += BDF_render16_ucs(screenp, screen_w, bg_color, fg_color, unicode);
	}
}

uint32_t BDF_GetFontHeight(void)
{
	return fonts_max_height;
}

uint32_t BDF_WidthUCS2(uint16_t ch)
{
	size_t font_num;
	for (font_num = 0; font_num < BDF_LIB_NUM; font_num++) {
		if (bdflib_info[font_num].fonts != NULL) {
			uint32_t start = bdflib_info[font_num].start;
			if (ch < start || ch >= start + bdflib_info[font_num].span)
				continue;
			ch -= start;
			return bdflib_info[font_num].fonts[ch].dwidth >> 16;
		}
	}
	return 8; // the width of an undefined character, not an error code
}

uint32_t BDF_WidthUCS2s(const uint16_t* ucs2s, size_t len)
{
	uint32_t ret = 0;

	while (len--) {
		ret += BDF_WidthUCS2(*ucs2s++);
	}

	return ret;
}

uint32_t BDF_WidthUTF8s(const char* utf8)
{
	uint32_t ret = 0;
	uint16_t ucs2;

	while (*utf8) {
		utf8 = utf8decode(utf8, &ucs2);
		ret += BDF_WidthUCS2(ucs2);
	}

	return ret;
}

uint32_t BDF_cut_unicode(uint16_t *unicodes, size_t len, uint32_t width, uint32_t direction)
{
	uint32_t xw = 0;
	ptrdiff_t i = 0, delta = (direction & 1) ? 1 : -1;

	if (direction & 2) {  /* Counting the width of 'len' characters */
		while (len > 0) {
			xw += BDF_WidthUCS2(unicodes[i]);
			i += delta;
			len--;
		}

		return xw;
	} else {  /* Counting the characters that fit in 'width' pixels */
		size_t saved_len = len, last_space = 0;

		while (len > 0) {
			if (unicodes[i] == 0x0A)
				return saved_len - len;
			else if (unicodes[i] == ' ')
				last_space = len;

			xw += BDF_WidthUCS2(unicodes[i]);

			if (xw > width)
				return saved_len - last_space;

			i += delta;
			len--;
		}

		return saved_len;
    }
}
