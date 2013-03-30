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

#include "fs_api.h"
#include "bitmap.h"
#include "ds2_malloc.h"

int BMP_read(char* filename, char *buf, unsigned int width, unsigned int height, unsigned int* type)
{
	FILE* fp;
	BMPHEADER bmp_header;
	int	flag;
	u32 bytepixel;
	u32	x, y, sx, sy, m;
	unsigned char *dest;
	s32	fpos;
	unsigned short st[54/2];

	fp= fopen(filename, "rb");
    if(fp == NULL)
		return BMP_ERR_OPENFAILURE;

	flag= fread(st, sizeof(st), 1, fp);
	if(!flag) {
		fclose(fp);
		return BMP_ERR_FORMATE;
	}

	bmp_header.bfType= st[0];
    bmp_header.bfSize= st[1] | (st[2]<<16);
    bmp_header.bfReserved0= st[3];
    bmp_header.bfReserved1= st[4];
    bmp_header.bfImgoffst= st[5] | (st[6]<<16);
    bmp_header.bfImghead.imHeadsize= st[7] | (st[8]<<16);
	bmp_header.bfImghead.imBitmapW= st[9] | (st[10]<<16);
	bmp_header.bfImghead.imBitmapH= st[11] | (st[12]<<16);
	bmp_header.bfImghead.imPlanes= st[13];
	bmp_header.bfImghead.imBitpixel= st[14];
	bmp_header.bfImghead.imCompess= st[15] | (st[16]<<16);
	bmp_header.bfImghead.imImgsize= st[17] | (st[18]<<16);
	bmp_header.bfImghead.imHres= st[19] | (st[20]<<16);
	bmp_header.bfImghead.imVres= st[21] | (st[22]<<16);
	bmp_header.bfImghead.imColnum= st[23] | (st[24]<<16);
	bmp_header.bfImghead.imImcolnum= st[25] | (st[26]<<16);

	if(bmp_header.bfType != 0x4D42) {	//"BM"
		fclose(fp);
		return BMP_ERR_FORMATE;
	}

	if(bmp_header.bfImghead.imCompess != BI_RGB && 
		bmp_header.bfImghead.imCompess != BI_BITFIELDS) {
		fclose(fp);
		return BMP_ERR_NEED_GO_ON;		//This funciton now not support...
	}

	bytepixel= bmp_header.bfImghead.imBitpixel >> 3;
	if(bytepixel < 2) {				//byte per pixel >= 2
		fclose(fp);
		return BMP_ERR_NEED_GO_ON;		//This funciton now not support...
	}

	*type = bytepixel;

	x= width;
	y= height;
	sx= bmp_header.bfImghead.imBitmapW;
	sy= bmp_header.bfImghead.imBitmapH;
	if(x > sx)
		x= sx;
	if(y > sy)
		y= sy;

	// Expect a certain amount of bytes and read them all at once.
	unsigned int BytesPerLine = (sx * bytepixel + 3) & ~3;
	char* FileBuffer = (char*) malloc(BytesPerLine * (sy - 1) + sx * bytepixel);
	if (FileBuffer == NULL) {
		fclose(fp);
		return BMP_ERR_NEED_GO_ON; // Memory allocation error
	}

	fseek(fp, (s32) bmp_header.bfImgoffst, SEEK_SET);
	m = fread(FileBuffer, 1, BytesPerLine * (sy - 1) + sx * bytepixel, fp);

	if (m < BytesPerLine * (sy - 1) + sx * bytepixel) {
		free(FileBuffer);
		fclose(fp);
		return BMP_ERR_FORMATE; // incomplete file
	}

	// Reorder all the bytes, because scanlines are from bottom to top.
	for (m = 0; m < y; m++) {
		memcpy(buf + m * x * bytepixel, FileBuffer + (sy - m - 1) * BytesPerLine, x * bytepixel);
	}

	free(FileBuffer);
	fclose(fp);

	return BMP_OK;
}

/*
*	open BMP file
*/
int openBMP(BMPINFO* bmpInfo, const char* file)
{
	FILE* fp;
	unsigned short st[54/2];
	int len;

	bmpInfo->fp = NULL;

	fp = fopen(file, "r");
	if(NULL == fp)
		return BMP_ERR_OPENFAILURE;

	len = fread((void*)st, 1, sizeof(BMPHEADER), fp);
	if(len < sizeof(BMPHEADER)) {
		fclose(fp);
		return BMP_ERR_FORMATE;
	}

	bmpInfo->bmpHead.bfType= st[0];
    bmpInfo->bmpHead.bfSize= st[1] | (st[2]<<16);
    bmpInfo->bmpHead.bfReserved0= st[3];
    bmpInfo->bmpHead.bfReserved1= st[4];
    bmpInfo->bmpHead.bfImgoffst= st[5] | (st[6]<<16);
    bmpInfo->bmpHead.bfImghead.imHeadsize= st[7] | (st[8]<<16);
	bmpInfo->bmpHead.bfImghead.imBitmapW= st[9] | (st[10]<<16);
	bmpInfo->bmpHead.bfImghead.imBitmapH= st[11] | (st[12]<<16);
	bmpInfo->bmpHead.bfImghead.imPlanes= st[13];
	bmpInfo->bmpHead.bfImghead.imBitpixel= st[14];
	bmpInfo->bmpHead.bfImghead.imCompess= st[15] | (st[16]<<16);
	bmpInfo->bmpHead.bfImghead.imImgsize= st[17] | (st[18]<<16);
	bmpInfo->bmpHead.bfImghead.imHres= st[19] | (st[20]<<16);
	bmpInfo->bmpHead.bfImghead.imVres= st[21] | (st[22]<<16);
	bmpInfo->bmpHead.bfImghead.imColnum= st[23] | (st[24]<<16);
	bmpInfo->bmpHead.bfImghead.imImcolnum= st[25] | (st[26]<<16);

	if(bmpInfo->bmpHead.bfType != 0x4D42)	//"BM"
	{
		fclose(fp);
		return BMP_ERR_FORMATE;
	}

	if(bmpInfo->bmpHead.bfImghead.imCompess != BI_RGB &&
		bmpInfo->bmpHead.bfImghead.imCompess != BI_BITFIELDS)
	{
		fclose(fp);
		return BMP_ERR_NEED_GO_ON;		//This funciton now not support...
	}

	bmpInfo->fp = fp;

	return BMP_OK;
}

/*
*	read pixel form BMP file
*/
int readBMP(BMPINFO* bmpInfo, unsigned int start_x, unsigned int start_y, 
		unsigned int width, unsigned int height, void* buffer)
{
	unsigned int m, n;
	unsigned int bmp_w, bmp_h;
	int fpos;
	unsigned char* dst;
	unsigned int bytepixel;

	bytepixel = bmpInfo->bmpHead.bfImghead.imBitpixel >> 3;
	if(bytepixel < 2)	//Not support <2 bytes per pixel now
		return -1;

	//BMP scan from down to up
	bmp_w = bmpInfo->bmpHead.bfImghead.imBitmapW;
	bmp_h = bmpInfo->bmpHead.bfImghead.imBitmapH;
	if(((start_x +1) > bmp_w) || ((start_y+1) > bmp_h)) return -1;
	n = bmp_w - start_x;
	if(n > width) n = width;			//start_x + width < bmp_w
	m = bmp_h - start_y;
	if(m > height) m = height;			//start_y + height < bmp_h

	fpos = (int)bmpInfo->bmpHead.bfImgoffst;

	fpos += (((bmp_w*bytepixel+3)>>2)<<2)*(bmp_h - start_y -1) + start_x*bytepixel;
	dst = (unsigned char*)buffer;
	n *= bytepixel;
	while(m--) {
		fseek(bmpInfo->fp, fpos, SEEK_SET);
		fread(dst, 1, n, bmpInfo->fp);
		fpos -= ((bmp_w*bytepixel+3)>>2)<<2;
		dst += width*bytepixel;
	}

	return 0;
}

/*
*	close BMP file
*/
void closeBMP(BMPINFO* bmpInfo)
{
	if(NULL != bmpInfo->fp)
		fclose(bmpInfo->fp);
}


