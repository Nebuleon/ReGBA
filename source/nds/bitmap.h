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

#ifndef __BITMAP_H__
#define __BITMAP_H__
#include "ds2_types.h"
#include "fs_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _pixelmapheader{
	u32		imHeadsize;	//Bitmap information header size
	u32		imBitmapW;	//bitmap width in pixel
	u32		imBitmapH;	//bitmap height in pixel
	u16		imPlanes;	//bitmap planes numbers, must be set to 1
	u16		imBitpixel;	//bits per pixel
	u32		imCompess;	//compress method
	u32		imImgsize;	//image size, times of 4-byte
	u32		imHres;		//horizontal resolution, pixel/metel
	u32		imVres;		//vertical resolution, pixel/metel
	u32		imColnum;	//number of colors in color palette, 0 to exp(2)
	u32		imImcolnum;	//important colors numbers used
} IMAGEHEADER;


typedef struct _bitmapfileheader{
	u16		bfType;		//BMP file types
	u32		bfSize;		//BMP file size(Not the pixel image size)
	u16		bfReserved0;//reserved area0
	u16		bfReserved1;//reserved area1
	u32		bfImgoffst;	//pixel data area offset
	IMAGEHEADER bfImghead;
} BMPHEADER;


typedef struct _bitmapInfo{
	FILE* fp;
	BMPHEADER bmpHead;
} BMPINFO;

//#define NULL 0

//compression method
/* Value Identified by 	Compression method 		Comments
*	0 		BI_RGB 			none 				Most common
*	1 		BI_RLE8 		RLE 8-bit/pixel 	Can be used only with 8-bit/pixel bitmaps
*	2 		BI_RLE4 		RLE 4-bit/pixel 	Can be used only with 4-bit/pixel bitmaps
*	3 		BI_BITFIELDS 	Bit field 			Can be used only with 16 and 32-bit/pixel bitmaps.
*	4 		BI_JPEG 		JPEG 				The bitmap contains a JPEG image
*	5 		BI_PNG 			PNG 				The bitmap contains a PNG image
*/
#define BI_RGB			0
#define BI_RLE8			1
#define BI_RLE4			2
#define BI_BITFIELDS 	3
#define BI_JPEG			4
#define BI_PNG			5

//error message
#define BMP_OK				0
#define BMP_ERR_OPENFAILURE	1
#define BMP_ERR_FORMATE		2
#define BMP_ERR_NOTSUPPORT	3
#define BMP_ERR_NEED_GO_ON	4


#define FILEOPENCHECK(fp)		(fp!=NULL)
		

extern int BMP_read(char* filename, char *buf, unsigned int width, 
		unsigned int height, unsigned int *type);

/*
*	open BMP file
*/
extern int openBMP(BMPINFO* bmpInfo, const char* file);

/*
*	read pixel form BMP file
*/
extern int readBMP(BMPINFO* bmpInfo, unsigned int start_x, unsigned int start_y, 
		unsigned int width, unsigned int height, void* buffer);

/*
*	close BMP file
*/
extern void closeBMP(BMPINFO* bmpInfo);

#ifdef __cplusplus
}
#endif

#endif //__BITMAP_H__
