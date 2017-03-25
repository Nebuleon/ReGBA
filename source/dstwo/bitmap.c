/* bitmap.c
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

#include <stdbool.h>
#include <stdio.h>
#include "bitmap.h"
#include <stdlib.h>
#include <string.h>

typedef struct _pixelmapheader {
	uint32_t imHeadsize;  //Bitmap information header size
	uint32_t imBitmapW;   //bitmap width in pixel
	int32_t  imBitmapH;   //bitmap height in pixel
	uint16_t imPlanes;    //bitmap planes numbers, must be set to 1
	uint16_t imBitpixel;  //bits per pixel
	uint32_t imCompess;   //compress method
	uint32_t imImgsize;   //image size, times of 4-byte
	uint32_t imHres;      //horizontal resolution, pixel/metel
	uint32_t imVres;      //vertical resolution, pixel/metel
	uint32_t imColnum;    //number of colors in color palette, 0 to exp(2)
	uint32_t imImcolnum;  //important colors numbers used
} IMAGEHEADER;

typedef struct _bitmapfileheader {
	uint16_t bfType;       //BMP file types
	uint32_t bfSize;       //BMP file size(Not the pixel image size)
	uint16_t bfReserved0;  //reserved area0
	uint16_t bfReserved1;  //reserved area1
	uint32_t bfImgoffst;   //pixel data area offset
	IMAGEHEADER bfImghead;
} BMPHEADER;

//compression method
/* Value  Identified by  Compression method  Comments
*      0  BI_RGB         none                Most common
*      1  BI_RLE8        RLE 8-bit/pixel     Can be used only with 8-bit/pixel bitmaps
*      2  BI_RLE4        RLE 4-bit/pixel     Can be used only with 4-bit/pixel bitmaps
*      3  BI_BITFIELDS   Bit field           Can be used only with 16 and 32-bit/pixel bitmaps
*      4  BI_JPEG        JPEG                The bitmap contains a JPEG image
*      5  BI_PNG         PNG                 The bitmap contains a PNG image
*/
#define BI_RGB       0
#define BI_RLE8      1
#define BI_RLE4      2
#define BI_BITFIELDS 3
#define BI_JPEG      4
#define BI_PNG       5

#define RGB888_BGR555(pix_ptr) \
	( ((uint16_t) (*(pix_ptr) & 0xF8) << 7) \
	| ((uint16_t) (*((pix_ptr) + 1) & 0xF8) << 2) \
	| ((uint16_t) (*((pix_ptr) + 2) & 0xF8) >> 3) )

int BMP_Read(const char* filename, uint16_t* buf, uint32_t width, uint32_t height)
{
	FILE* fp;
	BMPHEADER bmp_header;
	uint32_t bytepixel;
	uint32_t x = width, y = height, sx, ix, iy;
	int32_t sy;
	uint16_t masks[3];
	size_t shifts[3];
	bool top_down = false;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		return BMP_ERR_OPEN;

	{
		uint16_t st[27];
		if (fread(st, sizeof(st), 1, fp) == 0) {
			fclose(fp);
			return BMP_ERR_FORMAT;
		}

		bmp_header.bfType = st[0];
		bmp_header.bfSize = st[1] | (st[2] << 16);
		bmp_header.bfReserved0 = st[3];
		bmp_header.bfReserved1 = st[4];
		bmp_header.bfImgoffst = st[5] | (st[6] << 16);
		bmp_header.bfImghead.imHeadsize = st[7] | (st[8] << 16);
		bmp_header.bfImghead.imBitmapW = st[9] | (st[10] << 16);
		bmp_header.bfImghead.imBitmapH = st[11] | (st[12] << 16);
		bmp_header.bfImghead.imPlanes = st[13];
		bmp_header.bfImghead.imBitpixel = st[14];
		bmp_header.bfImghead.imCompess = st[15] | (st[16] << 16);
		bmp_header.bfImghead.imImgsize = st[17] | (st[18] << 16);
		bmp_header.bfImghead.imHres = st[19] | (st[20] << 16);
		bmp_header.bfImghead.imVres = st[21] | (st[22] << 16);
		bmp_header.bfImghead.imColnum = st[23] | (st[24] << 16);
		bmp_header.bfImghead.imImcolnum = st[25] | (st[26] << 16);
	}

	if (bmp_header.bfType != 0x4D42) {  // "BM"
		fclose(fp);
		return BMP_ERR_FORMAT;
	}

	if (bmp_header.bfImghead.imBitpixel < 16
	 || bmp_header.bfImghead.imBitpixel > 24
	 || bmp_header.bfImghead.imBitpixel % 8 != 0
	 || (bmp_header.bfImghead.imBitpixel == 16
	  && !(bmp_header.bfImghead.imCompess == BI_RGB
	    || bmp_header.bfImghead.imCompess == BI_BITFIELDS))
	 || (bmp_header.bfImghead.imBitpixel == 24
	  && bmp_header.bfImghead.imCompess != BI_RGB)) {
		fclose(fp);
		return BMP_ERR_UNSUPPORTED;
	}
	bytepixel = bmp_header.bfImghead.imBitpixel / 8;

	if (bytepixel == 2) {
		if (bmp_header.bfImghead.imCompess == BI_BITFIELDS) {
			uint32_t file_masks[3];
			size_t i;
			if (fread(file_masks, sizeof(file_masks), 1, fp) == 0) {
				fclose(fp);
				return BMP_ERR_FORMAT;
			}
			for (i = 0; i < 3; i++) {
				size_t shift = 0, bits = 0;
				uint16_t mask = file_masks[i];
				if (mask == 0) {
					fclose(fp);
					return BMP_ERR_FORMAT;
				}
				while (!(mask & 1)) {
					mask >>= 1;
					shift++;
				}
				while (mask & 1) {
					mask >>= 1;
					bits++;
				}
				if (mask != 0) {
					fclose(fp);
					return BMP_ERR_FORMAT;
				}
				shift += bits - 5;
				masks[i] = file_masks[i];
				shifts[i] = shift;
			}
		} else if (bmp_header.bfImghead.imCompess == BI_RGB) {
			/* Default masks for 16-bit bitmaps make up RGB555. */
			masks[0] = UINT16_C(0x7C00);
			masks[1] = UINT16_C(0x03E0);
			masks[2] = UINT16_C(0x001F);
			shifts[0] = 10;
			shifts[1] = 5;
			shifts[2] = 0;
		}
	}

	sx = bmp_header.bfImghead.imBitmapW;
	sy = bmp_header.bfImghead.imBitmapH;
	if (sy < 0) {  /* This is a top-down bitmap: row 0 is stored first. */
		top_down = true;
		sy = -sy;
	}
	if (x > sx)
		x = sx;
	if (y > (uint32_t) sy)
		y = sy;

	// Expect a certain number of bytes and read them all at once.
	size_t BytesPerLine = (sx * bytepixel + 3) & ~3;  /* with alignment */
	uint8_t* FileBuffer = malloc(BytesPerLine * ((uint32_t) sy - 1) + sx * bytepixel);
	if (FileBuffer == NULL) {
		fclose(fp);
		return BMP_ERR_MEM;
	}

	fseek(fp, bmp_header.bfImgoffst, SEEK_SET);

	if (fread(FileBuffer, BytesPerLine * ((uint32_t) sy - 1) + sx * bytepixel, 1, fp) == 0) {
		free(FileBuffer);
		fclose(fp);
		return BMP_ERR_FORMAT;  /* Incomplete file */
	}

	// Convert all the bytes while possibly reordering the scanlines.
	switch (bytepixel) {
	case 2:
		for (iy = 0; iy < y; iy++) {
			uint16_t* dst = &buf[iy * width];
			uint16_t* src = (uint16_t*) &FileBuffer[BytesPerLine * (top_down ? iy : sy - 1 - iy)];
			for (ix = 0; ix < x; ix++) {
				*dst++ =  ((*src & masks[0]) >> shifts[0])        /* Red */
				       | (((*src & masks[1]) >> shifts[1]) <<  5) /* Green */
				       | (((*src & masks[2]) >> shifts[2]) << 10) /* Blue */;
				src++;
			}
		}
		break;
	case 3:
		for (iy = 0; iy < y; iy++) {
			uint16_t* dst = &buf[iy * width];
			uint8_t* src = &FileBuffer[BytesPerLine * (top_down ? iy : sy - 1 - iy)];
			for (ix = 0; ix < x; ix++) {
				*dst++ = RGB888_BGR555(src);
				src += 3;
			}
		}
		break;
	}

	if (x < width) {
		for (iy = 0; iy < y; iy++) {
			memset(&buf[y * width + x], 0, (width - x) * sizeof(uint16_t));
		}
	}
	if (y < height) {
		memset(&buf[y * width], 0, (height - y) * width * sizeof(uint16_t));
	}

	free(FileBuffer);
	fclose(fp);

	return BMP_OK;
}
