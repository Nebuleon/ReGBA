/*
**********************************************************************
*                          Micrium, Inc.
*                      949 Crestview Circle
*                     Weston,  FL 33327-1848
*
*                            uC/FS
*
*             (c) Copyright 2001 - 2003, Micrium, Inc.
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : fs_fat.h
Purpose     : FAT File System Layer header
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_FAT_H_
#define _FS_FAT_H_

/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

#define FS_FAT_ATTR_READ_ONLY   0x01
#define FS_FAT_ATTR_HIDDEN      0x02
#define FS_FAT_ATTR_SYSTEM      0x04
#define FS_FAT_ATTR_DB          0x06
#define FS_FAT_VOLUME_ID        0x08
#define FS_FAT_ATTR_ARCHIVE     0x20
#define FS_FAT_ATTR_DIRECTORY   0x10
#define FS_FAT_ATTR_LFNAME	0x0F		// this is a long filename entry

#define FS_FAT_DENTRY_SIZE      0x20

/*
const unsigned char _ShortNameValid[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, '!',  0x00, '#',  '$',  '%',  '&',  ''', 
  '(',  ')',  0x00, 0x00, 0x00, '-',  0x00, 0x00,
  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7', 
  '8',  '9',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  '@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
  'X',  'Y',  'Z',  0x00, 0x00, 0x00, '^',  '_',
  '`',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
  'X',  'Y',  'Z',  '{',  0x00, '}',  '~',  0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //0x80
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
*/

/*********************************************************************
*
*             Global data types
*
**********************************************************************
*/

/* BIOS parameter block (FAT12/FAT16) */
typedef struct {
  FS_u32          TotSec32;         /* RSVD + FAT + ROOT + FATA (>=64k) */
  FS_u32          FATSz32;          /* number of FAT sectors          */
  FS_u32          RootClus;         /* root dir clus for FAT32        */
  FS_u16          BytesPerSec;      /* _512_,1024,2048,4096           */
  FS_u16          RootEntCnt;       /* number of root dir entries     */
  FS_u16          TotSec16;         /* RSVD + FAT + ROOT + FATA (<64k) */
  FS_u16          FATSz16;          /* number of FAT sectors          */
  FS_u16          ExtFlags;         /* mirroring info                 */
  FS_u16          FSInfo;           /* position of FSInfo structure   */
  FS_u16          Signature;        /* 0xAA55 Signature               */
  FS_u16          RsvdSecCnt;       /* 1 for FAT12 & FAT16            */
  unsigned char   SecPerClus;       /* sec in allocation unit         */
  unsigned char   NumFATs;          /* 2                              */
} FS__FAT_BPB;


/* FAT directory entry */
typedef struct {
  unsigned char   data[32];
} FS__fat_dentry_type;




/*********************************************************************
*
*             Externals
*
**********************************************************************
*/

extern FS__FAT_BPB FS__FAT_aBPBUnit[FS_MAXDEV][FS_FAT_MAXUNIT];


/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

/*********************************************************************
*
*             fat_misc
*/

void                FS__fat_block_init(void);
char                *FS__fat_malloc(unsigned int Size);
void                FS__fat_free(void *pBuffer);
FS_i32              FS__fat_diskclust(int Idx, FS_u32 Unit, FS_i32 StrtClst, FS_i32 ClstNum);
FS_i32              FS__fat_FAT_alloc(int Idx, FS_u32 Unit, FS_i32 LastClust);
FS_i32              FS__fat_FAT_find_eof(int Idx, FS_u32 Unit, FS_i32 StrtClst, FS_u32 *pClstCnt);
int                 FS__fat_which_type(int Idx, FS_u32 Unit);
int                 FS__fat_checkunit(int Idx, FS_u32 Unit);


/*********************************************************************
*
*             fat_in
*/

FS_size_t           FS__fat_fread(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);
FS_size_t           FS__fat_fread_block(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);

/*********************************************************************
*
*             fat_out
*/

FS_size_t           FS__fat_fwrite(const void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);
void                FS__fat_fclose(FS_FILE *pFile);


/*********************************************************************
*
*             fat_open
*/

FS_FILE             *FS__fat_fopen(const char *pFileName, const char *pMode, FS_FILE *pFile);
FS_u32              FS__fat_dir_size(int Idx, FS_u32 Unit, FS_u32 DirStart);
FS_u32              FS__fat_dir_realsec(int Idx, FS_u32 Unit, FS_u32 DirStart, FS_u32 DirSec);
void                FS__fat_make_realname(char *pEntryName, const char *pOrgName);
FS_u32              FS__fat_find_dir(int Idx, FS_u32 Unit, char *pDirName, FS_u32 DirStart, FS_u32 DirSize);
FS_u32              FS__fat_findpath(int Idx, const char *pFullName, FS_FARCHARPTR *pFileName, FS_u32 *pUnit, FS_u32 *pDirStart);
FS_i32              FS__fat_DeleteFileOrDir(int Idx, FS_u32 Unit,  const char *pName, FS_u32 DirStart, FS_u32 DirSize, char RmFile);


/*********************************************************************
*
*             fat_ioctl
*/

int                 FS__fat_ioctl(int Idx, FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer);


/*********************************************************************
*
*             fat_dir
*/

#if FS_POSIX_DIR_SUPPORT
  FS_DIR              *FS__fat_opendir(const char *pDirName, FS_DIR *pDir);
  int                 FS__fat_closedir(FS_DIR *pDir);
  struct FS_DIRENT    *FS__fat_readdir(FS_DIR *pDir);
  struct FS_DIRENT    *FS__fat_readdir_back(FS_DIR *pDir);
  int                 FS__fat_MkRmDir(const char *pDirName, int Idx, char MkDir);
#endif /* FS_POSIX_DIR_SUPPORT */


#endif  /* _FS_FAT_H_ */


