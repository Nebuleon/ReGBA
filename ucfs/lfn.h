/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "lfn.h" - Jim gao, structures and prototypes        *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS 32 FAT Driver.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is free software; you can redistribute it    *
 * and/or modify it under the terms of the GNU General Public License     *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is distributed in the hope that it will be   *
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS 32 FAT Driver; see the file COPYING;            *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#ifndef __LFN_H
#define __LFN_H

#define FD32_LFNPMAX 260 /* Max length for a long file name path  */
#define FD32_LFNMAX  255 /* Max length for a long file name       */
#define FD32_SFNPMAX 64  /* Max length for a short file name path */
#define FD32_SFNMAX  14  /* Max length for a short file name      */

#define EINVAL 22	/* Invalid argument */

//typedef unsigned long  DWORD;
typedef unsigned short FWORD;
typedef unsigned char  FBYTE;

#define size_t DWORD
#define ssize_t long int
typedef int wchar_t;
#define FD32_ALNGNAM 0x0F
#define ENAMETOOLONG 91	

#define va_list void*
typedef  unsigned short      uint16_t;

#define u_int unsigned int
#define u_char FBYTE
#define u_short FWORD
#define u_long DWORD


/*********************************************************************
*
*             Long File Name
*
**********************************************************************
*/
/* FAT 32-byte Long File Name Directory Entry structure */
typedef struct
{
	unsigned char Order;        /* Sequence number for slot        */
	FS_u16 Name0_4[5];   /* First 5 Unicode characters      */
	unsigned char Attr;         /* Attributes, always 0x0F         */
	unsigned char Reserved;     /* Reserved, always 0x00           */
	unsigned char Checksum;     /* Checksum of 8.3 name            */
	FS_u16 Name5_10[6];  /* 6 more Unicode characters       */
	FS_u16 FstClus;      /* First cluster number, must be 0 */
	FS_u16 Name11_12[2]; /* Last 2 Unicode characters       */
}__attribute__ ((packed)) tLfnEntry;

typedef struct
{
	char oldname[39];        /* temp name save        */
	int  count;        /* temp name save        */
} tLfnold;

tLfnold savename[20];

typedef struct
{
	FS_u16 oldname16[13];        /* temp name save        */
} tLfnold16;
tLfnold16 savename16[20];

int savecount=0;

int UniToUtf8(FS_u16* src, char* putf8);
int gen_short_fname(int Idx, FS_u32 Unit,FS_u32 DirStart,FS_u32 DirSize,char *LongName, FBYTE *ShortName, FWORD Hint);

char temp_dir[12];
int  create_dir=0; 

#endif /* #ifndef __LFN_H */
