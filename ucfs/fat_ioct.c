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
File        : fat_ioctc.c
Purpose     : FAT File System Layer IOCTL
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "fs_conf.h"
#include "fs_port.h"
#include "fs_dev.h"
#include "fs_api.h"
#include "fs_fsl.h"
#include "fs_int.h"
#include "fs_os.h"
#include "fs_lbl.h"
#include "fs_fat.h"
#include "fs_clib.h"


/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

#define FS_KNOWNMEDIA_NUM   sizeof(_FS_wd_format_media_table) / sizeof(_FS_wd_format_media_type)

#ifndef FS_FAT_NOFORMAT
  #define FS_FAT_NOFORMAT       0
#endif

#ifndef FS_FAT_DISKINFO
  #define FS_FAT_DISKINFO       1
#endif

#ifndef FS_SUPPORT_SEC_ACCESS
  #define FS_SUPPORT_SEC_ACCESS 1
#endif

/*********************************************************************
*
*             Local data types
*
**********************************************************************
*/

#if (FS_FAT_NOFORMAT==0)

typedef struct {
  FS_i32  media_id;
  FS_u32  totsec32;
  FS_u32  hiddsec;
  FS_u16  totsec16;
  FS_u16  rootentcnt;
  FS_u16  fatsz16;
  FS_u16  secpertrk;
  FS_u16  numheads;
  char    secperclus;
  char    media; 
  char    fsystype;
} _FS_wd_format_media_type;


typedef struct {
  FS_u32 SecNum;
  FS_u32 Num;
} _FS_FAT_ROOTENTCNT ;


typedef struct {
  FS_u32 SecNum;
  FS_u16 Num;
} _FS_FAT_SECPERCLUST;


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

/*  media_id              totsec32       hidsec    totsec16   rootent  fatsz16  secpertrk  numheads secperclus  media   fstype */ 
static const _FS_wd_format_media_type _FS_wd_format_media_table[] = {
  { FS_MEDIA_RAM_16KB,  0x00000000UL, 0x00000000UL, 0x0020,   0x0040,  0x0001,  0x0004,     0x0004,   0x01, (char) 0xf8, 0 },
  { FS_MEDIA_RAM_64KB,  0x00000000UL, 0x00000000UL, 0x0080,   0x0040,  0x0001,  0x0004,     0x0004,   0x01, (char) 0xf8, 0 },
  { FS_MEDIA_RAM_128KB, 0x00000000UL, 0x00000000UL, 0x0100,   0x0080,  0x0001,  0x0004,     0x0004,   0x01, (char) 0xf8, 0 },
  { FS_MEDIA_RAM_256KB, 0x00000000UL, 0x00000000UL, 0x0200,   0x0080,  0x0002,  0x0004,     0x0004,   0x01, (char) 0xf8, 0 },
  { FS_MEDIA_RAM_512KB, 0x00000000UL, 0x00000000UL, 0x0400,   0x00e0,  0x0003,  0x0004,     0x0004,   0x01, (char) 0xf8, 0 },
  { FS_MEDIA_FD_144MB,  0x00000000UL, 0x00000000UL, 0x0b40,   0x00e0,  0x0009,  0x0012,     0x0002,   0x01, (char) 0xf0, 0 },
  { FS_MEDIA_SMC_1MB,   0x00000000UL, 0x0000000DUL, 0x07c3,   0x0100,  0x0001,  0x0004,     0x0004,   0x08, (char) 0xf8, 0 },
  { FS_MEDIA_SMC_2MB,   0x00000000UL, 0x0000000bUL, 0x0f95,   0x0100,  0x0002,  0x0008,     0x0004,   0x08, (char) 0xf8, 0 },
  { FS_MEDIA_SMC_4MB,   0x00000000UL, 0x0000001bUL, 0x1f25,   0x0100,  0x0002,  0x0008,     0x0004,   0x10, (char) 0xf8, 0 },
  { FS_MEDIA_SMC_8MB,   0x00000000UL, 0x00000019UL, 0x3e67,   0x0100,  0x0003,  0x0010,     0x0004,   0x10, (char) 0xf8, 0 },
  { FS_MEDIA_SMC_16MB,  0x00000000UL, 0x00000029UL, 0x7cd7,   0x0100,  0x0003,  0x0010,     0x0004,   0x20, (char) 0xf8, 0 },
  { FS_MEDIA_SMC_32MB,  0x00000000UL, 0x00000023UL, 0xf9dd,   0x0100,  0x0006,  0x0010,     0x0008,   0x20, (char) 0xf8, 0 },
  { FS_MEDIA_SMC_64MB,  0x0001f3c9UL, 0x00000037UL, 0x0000,   0x0100,  0x000c,  0x0020,     0x0008,   0x20, (char) 0xf8, 0 },
  { FS_MEDIA_SMC_128MB, 0x0003e7d1UL, 0x0000002fUL, 0x0000,   0x0100,  0x0020,  0x0020,     0x0010,   0x20, (char) 0xf8, 1 },
  { FS_MEDIA_MMC_32MB,  0x00000000UL, 0x00000020UL, 0xf460,   0x0200,  0x003d,  0x0020,     0x0004,   0x04, (char) 0xf8, 1 },
  { FS_MEDIA_MMC_64MB,  0x0001e8e0UL, 0x00000020UL, 0x0000,   0x0200,  0x007a,  0x0020,     0x0008,   0x04, (char) 0xf8, 1 },
  { FS_MEDIA_MMC_128MB, 0x0003d2e0UL, 0x00000020UL, 0x0000,   0x0200,  0x00f5,  0x0020,     0x0008,   0x04, (char) 0xf8, 1 },
 
  { FS_MEDIA_SD_16MB,   0x00000000UL, 0x00000039UL, 0x7187,   0x0200,  0x0003,  0x0020,     0x0002,   0x20, (char) 0xf8, 0 },

  { FS_MEDIA_SD_64MB,   0x0001dbd9UL, 0x00000027UL, 0x0000,   0x0200,  0x000c,  0x0020,     0x0008,   0x20, (char) 0xf8, 0 },
  { FS_MEDIA_SD_128MB,  0x0003c09fUL, 0x00000061UL, 0x0000,   0x0200,  0x001f,  0x0020,     0x0008,   0x20, (char) 0xf8, 1 },
  { FS_MEDIA_CF_32MB,   0x00000000UL, 0x00000020UL, 0xf760,   0x0200,  0x007c,  0x0020,     0x0004,   0x02, (char) 0xf8, 1 },
  { FS_MEDIA_CF_64MB,   0x0001e860UL, 0x00000020UL, 0x0000,   0x0200,  0x007b,  0x0020,     0x0004,   0x04, (char) 0xf8, 1 }
};


/* table for getting number of root entries for a given media size */
static const _FS_FAT_ROOTENTCNT _FS_auto_rootcnt[] = {
  {         0x100,     0x40 },
  {         0x200,     0x80 },
  {      0x0b40UL,     0xe0 },
  {  0x0001f3c9UL,    0x100 },
  {  0xffffffffUL,    0x200 }
};


/* table for calculating cluster size */
static const _FS_FAT_SECPERCLUST _FS_auto_secperclust[] = {
  /* medias up to 512MB are formatted with FAT16 */
  {     0x0b40UL, 0x0001 },
  {      32680UL, 0x0002 },
  {     262144UL, 0x0004 },
  {     524288UL, 0x0008 },
  {    1048576UL, 0x0010 },
  /* medias bigger than 512MB are formatted with FAT32 */
  {   16777216UL, 0x0008 },
  {   33554432UL, 0x0010 },
  {   67108864UL, 0x0020 },
  { 0xffffffffUL, 0x0040 }
};



static _FS_FAT_SECPERCLUST DskTableFAT12 [] = {
    {    4080,   1},       /* disks up to  1.99 MB, .5k cluster */
    {   8160,   2},       /* disks up to   3.98 MB,  1k cluster */
    {  16320,   4},       /* disks up to 7.97 MB,  2k cluster */
    {  32640,   8},       /* disks up to 15.94 MB,  4k cluster */
    { 0xFFFFFFFF, 0} /* any disk greater than 15.94 MB, 0 value for SecPerClusVal trips an error */
};


/* 
* This is the table for FAT16 drives. NOTE that this table includes
* entries for disk sizes larger than 512 MB even though typically
* only the entries for disks < 512 MB in size are used.
* The way this table is accessed is to look for the first entry
* in the table for which the disk size is less than or equal
* to the DiskSize field in that table entry.  For this table to
* work properly BPB_RsvdSecCnt must be 1, BPB_NumFATs
* must be 2, and BPB_RootEntCnt must be 512. Any of these values
* being different may require the first table entries DiskSize value
* to be changed otherwise the cluster count may be to low for FAT16.
*/

static _FS_FAT_SECPERCLUST DskTableFAT16 [] = {
    {    8400,   0},       /* disks up to  4.1 MB, the 0 value for SecPerClusVal trips an error */
    {   32680,   2},       /* disks up to   16 MB,  1k cluster */
    {  262144,   4},       /* disks up to 128 MB,  2k cluster */
    {  524288,   8},       /* disks up to 256 MB,  4k cluster */
    { 1048576,  16},       /* disks up to 512 MB,  8k cluster */ 
        
    /* The entries after this point are not used unless FAT16 is forced */
    { 2097152,  32},       /* disks up to     1 GB, 16k cluster */
    { 4194304,  64},       /* disks up to     2 GB, 32k cluster */
    { 0xFFFFFFFF, 0} /* any disk greater than 2GB, 0 value for SecPerClusVal trips an error */
};


/* 
* This is the table for FAT32 drives. NOTE that this table includes
* entries for disk sizes smaller than 512 MB even though typically
* only the entries for disks >= 512 MB in size are used.
* The way this table is accessed is to look for the first entry
* in the table for which the disk size is less than or equal
* to the DiskSize field in that table entry. For this table to
* work properly BPB_RsvdSecCnt must be 32, and BPB_NumFATs
* must be 2. Any of these values being different may require the first 
* table entries DiskSize value to be changed otherwise the cluster count 
* may be to low for FAT32.
*/
static _FS_FAT_SECPERCLUST DskTableFAT32 [] = {
    {    66600,  0},       /* disks up to 32.5 MB, the 0 value for SecPerClusVal trips an error */
    {   532480,  1},       /* disks up to 260 MB,  .5k cluster */
    { 16777216,  8},       /* disks up to     8 GB,    4k cluster */
    { 33554432, 16},       /* disks up to   16 GB,    8k cluster */
    { 67108864, 32},       /* disks up to   32 GB,  16k cluster */
    { 0xFFFFFFFF, 64}      /* disks greater than 32GB, 32k cluster */
};


#endif /* FS_FAT_NOFORMAT==0 */

/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

#if (FS_FAT_NOFORMAT==0)

static inline FS_u32 GetSectorsPerCluster (FS_u32 dwFatVersion, FS_u32 dwTotalSectors)
{
    FS_u32 dwSectorsPerCluster = 0;
    int i;
   if (dwFatVersion != 12 && dwFatVersion != 16 && dwFatVersion != 32)
        dwFatVersion = 16;
    
    if (dwTotalSectors <= DskTableFAT16[0].SecNum)
        dwFatVersion = 12;
    else if (dwFatVersion == 32 && dwTotalSectors <= DskTableFAT32[0].SecNum)
        dwFatVersion = 16;
    else if (dwTotalSectors > DskTableFAT16[6].SecNum)
        dwFatVersion = 32;
    else if (dwFatVersion == 12 && dwTotalSectors > DskTableFAT12[3].SecNum)
        dwFatVersion = 16;

    if (dwFatVersion == 12) {
        for (i = 0; 1; i++) {
            if (dwTotalSectors <= DskTableFAT12[i].SecNum) {
                dwSectorsPerCluster = DskTableFAT12[i].Num;
                break;
            }
        }
    }
    else if (dwFatVersion == 16) {
        for (i = 0; 1; i++) {
            if (dwTotalSectors <= DskTableFAT16[i].SecNum) {
                dwSectorsPerCluster = DskTableFAT16[i].Num;
                break;
            }
        }
    }
    else {
        for (i = 0; 1; i++) {
            if (dwTotalSectors <= DskTableFAT32[i].SecNum) {
                dwSectorsPerCluster = DskTableFAT32[i].Num;
                break;
            }
        }
    }
    printf("***dwSectorsPerCluster = %d ,%d \r\n",dwSectorsPerCluster,dwTotalSectors);
    return dwSectorsPerCluster;
}

/*********************************************************************
*
*             _FS_fat_format
*
  Description:
  FS internal function. Format a media using specified parameters.
  Currently this function needs many parameters. The function will be
  improved. 

  Parameters:
  pDriver     - Pointer to a device driver function table.
  Unit        - Unit number.
  SecPerClus  - Number of sector per allocation unit.
  RootEntCnt  - For FAT12/FAT16, this is the number of 32 byte root
                directory entries. 0 for FAT32.
  TotSec16    - 16-bit total count of sectors. If zero, TotSec32 must 
                be none-zero.
  TotSec32    - 32-bit total count of sectors. If zero, TotSec16 must 
                be none-zero.
  Media       - Media byte.
  FATSz16     - 16-bit count of sectors occupied by one FAT. 0 for
                FAT32 volumes, which use FATSz32.
  FATSz32     - 32-bit count of sectors occupied by one FAT. This is
                valid for FAT32 medias only.
  SecPerTrk   - Sectors per track.
  NumHeads    - Number of heads.
  HiddSec     - Count of hidden sectors preceding the partition.
  FSysType    - ==0 => FAT12
                ==1 => FAT16
                ==2 => FAT32
  
  Return value:
  >=0         - Media has been formatted.
  <0          - An error has occured.
*/


static int _FS_fat_format(const FS__device_type *pDriver,FS_u32 Unit, char SecPerClus, 
                          FS_u16 RootEntCnt, FS_u16 TotSec16, FS_u32 TotSec32, char Media, 
                          FS_u16 FATSz16, FS_u32 FATSz32, FS_u16 SecPerTrk,FS_u16 NumHeads, 
                          FS_u32 HiddSec, char FSysType) {
  FS__FAT_BPB bpb;
  FS_u32 sector;
  FS_u32 value;
  char *buffer;
  int i;
  int j;
  int n;
//  printf("_FS_fat_format:\r\n");

//  printf("secperclust = %d\r\n",SecPerClus);
//  printf("rootentcnt = %d\r\n",RootEntCnt);
//  printf("totsec16 = %d\r\n",TotSec16);
//  printf("totsec32 = %d\r\n",TotSec32);
//  printf("Media = %d\r\n", Media);
  
  


//  printf("FATSz16 = %d\r\n",FATSz16);
//  printf("FATSz32 = %d\r\n",FATSz32);
//  printf("SecPerTrk = %d\r\n",SecPerTrk);
//  printf("NumHeads = %d\r\n",NumHeads);

//  printf("HiddSec = %d\r\n",HiddSec);
//  printf("FSysType = %d\r\n",FSysType);

  
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }
  FS__CLIB_memset(buffer, 0x00, (FS_size_t)FS_FAT_SEC_SIZE);
  /* Setup BPB */
  FS__CLIB_memset(&bpb, 0x00, (FS_size_t)sizeof(bpb));
  bpb.BytesPerSec = 0x0200;       /* _512_,1024,2048,4096           */
  bpb.SecPerClus  = SecPerClus;   /* sec in allocation unit         */
  if (FSysType != 2) {
    bpb.RsvdSecCnt  = 0x0001;       /* 1 for FAT12 & FAT16            */
  }
#if (FS_FAT_NOFAT32==0)
  else {
    bpb.RsvdSecCnt  = 0x0020;       /* 32 for FAT32                   */
  }
#else
  /* FAT32 disabled */
  else {
    FS__fat_free(buffer);
    return -1;
  }
#endif /* FS_FAT_NOFAT32==0 */
  bpb.NumFATs     = 0x02;         /* 2                              */
  bpb.RootEntCnt  = RootEntCnt;   /* number of root dir entries     */
  bpb.TotSec16    = TotSec16;     /* RSVD+FAT+ROOT+DATA (<64k)      */
  bpb.FATSz16     = FATSz16;      /* number of FAT sectors          */
  bpb.TotSec32    = TotSec32;     /* RSVD+FAT+ROOT+FATA (>=64k)     */
  bpb.Signature   = 0xaa55;       /* 0xAA55 Signature               */

  /* setup BPB specifics for FAT32 */
  bpb.FATSz32     = FATSz32;      /* number of FAT sectors          */
  bpb.ExtFlags    = 0x0000;       /* mirroring info                 */
  bpb.RootClus    = 0x00000002UL; /* root dir clus for FAT32        */
  bpb.FSInfo      = 0x0001;       /* position of FSInfo structure   */

  /* 
     Prepare buffer with information of the BPB 
     offset 0 - 35 is same for FAT12/FAT16 and FAT32 
  */

  /* jmpBoot = 0xe9 0x0000 */
  buffer[0]   = (char)0xe9;
  buffer[1]   = (char)0x00;
  buffer[2]   = (char)0x00;
  //buffer[0] = 0xEB;
  //buffer[1] = 0x58;
  //buffer[2] = 0x90;
  /* OEMName = '        ' */
  for (i = 3; i < 11; i++) {
    buffer[i] = (char)0x20;
  }
  /* BytesPerSec */
  buffer[11]  = (char)(bpb.BytesPerSec & 0xff);
  buffer[12]  = (char)((unsigned int)(bpb.BytesPerSec & 0xff00) >> 8);
  /* SecPerClus */
  buffer[13]  = (char)bpb.SecPerClus;
  /* RsvdSecCnt */
  buffer[14]  = (char)(bpb.RsvdSecCnt & 0xff);
  buffer[15]  = (char)((unsigned int)(bpb.RsvdSecCnt & 0xff00) >> 8);
  /* NumFATs */
  buffer[16]  = (char)bpb.NumFATs;
  /* RootEntCnt */
  buffer[17]  = (char)(bpb.RootEntCnt & 0xff);
  buffer[18]  = (char)((unsigned int)(bpb.RootEntCnt & 0xff00) >> 8);
  /* TotSec16 */
  buffer[19]  = (char)(bpb.TotSec16 & 0xff);
  buffer[20]  = (char)((unsigned int)(bpb.TotSec16 & 0xff00) >> 8);
  /* Media */
  buffer[21]  = Media;
  /* FATSz16 */
  buffer[22]  = (char)(bpb.FATSz16 & 0xff);
  buffer[23]  = (char)((unsigned int)(bpb.FATSz16 & 0xff00) >> 8);
  /* SecPerTrk */
  buffer[24]  = (char)(SecPerTrk & 0xff);
  buffer[25]  = (char)((unsigned int)(SecPerTrk & 0xff00) >> 8);
  /* NumHeads */
  buffer[26]  = (char)(NumHeads & 0xff);
  buffer[27]  = (char)((unsigned int)(NumHeads & 0xff00) >> 8);
  /* HiddSec */
  buffer[28]  = (char)(HiddSec & 0xff);
  buffer[29]  = (char)((FS_u32)(HiddSec & 0x0000ff00UL) >> 8);
  buffer[30]  = (char)((FS_u32)(HiddSec & 0x00ff0000UL) >> 16);
  buffer[31]  = (char)((FS_u32)(HiddSec & 0xff000000UL) >> 24);
  /* TotSec32 */
  buffer[32]  = (char)(bpb.TotSec32 & 0xff);
  buffer[33]  = (char)((FS_u32)(bpb.TotSec32 & 0x0000ff00UL) >> 8);
  buffer[34]  = (char)((FS_u32)(bpb.TotSec32 & 0x00ff0000UL) >> 16);
  buffer[35]  = (char)((FS_u32)(bpb.TotSec32 & 0xff000000UL) >> 24);

  /* Offset 36 and above have different meanings for FAT12/FAT16 and FAT32 */
  if (FSysType != 2) {
    /* FAT12/FAT16 */
    /* DrvNum = 0x00 (floppy) */
    buffer[36]  = (char)0x00;  
    /* Reserved1 = 0x00 (floppy) */
    buffer[37]  = (char)0x00;
    /* BootSig = 0x00 (next three fields are not 'present') */
    buffer[38]  = (char)0x00;
    /* VolID = 0x00000000 (serial number, e.g. date/time) */
    for (i = 39; i < 43; i++) {
      buffer[i] = (char)0x00;
    }
    /* VolLab = ' ' */
    for (i = 43; i < 54; i++) {
      buffer[i] = (char)0x20;
    }
    /* FilSysType = 'FAT12' */
    if (FSysType == 0) {
      FS__CLIB_strncpy(&buffer[54], "FAT12   ", 8);
    }
    else {
      FS__CLIB_strncpy(&buffer[54], "FAT16   ", 8);
    }
  }
#if (FS_FAT_NOFAT32==0)
  else {
    /* FAT32 */
    /* FATSz32 */
    buffer[36]  = (char)(bpb.FATSz32 & 0xff);
    buffer[37]  = (char)((FS_u32)(bpb.FATSz32 & 0x0000ff00UL) >> 8);
    buffer[38]  = (char)((FS_u32)(bpb.FATSz32 & 0x00ff0000UL) >> 16);
    buffer[39]  = (char)((FS_u32)(bpb.FATSz32 & 0xff000000UL) >> 24);
    /* EXTFlags */
    buffer[40]  = (char)(bpb.ExtFlags & 0xff);
    buffer[41]  = (char)((unsigned int)(bpb.ExtFlags & 0xff00) >> 8);
    /* FSVer = 0:0 */
    buffer[42]  = 0x00;
    buffer[43]  = 0x00;
    /* RootClus */
    buffer[44]  = (char)(bpb.RootClus & 0xff);
    buffer[45]  = (char)((FS_u32)(bpb.RootClus & 0x0000ff00UL) >> 8);
    buffer[46]  = (char)((FS_u32)(bpb.RootClus & 0x00ff0000UL) >> 16);
    buffer[47]  = (char)((FS_u32)(bpb.RootClus & 0xff000000UL) >> 24);
    /* FSInfo */
    buffer[48]  = (char)(bpb.FSInfo & 0xff);
    buffer[49]  = (char)((unsigned int)(bpb.FSInfo & 0xff00) >> 8);
    /* BkBootSec = 0x0006; */
    buffer[50]  = 0x06;
    buffer[51]  = 0x00;
    /* Reserved */
    for (i = 52; i < 64; i++) {
      buffer[i] = (char)0x00;
    }
    /* DrvNum = 0x00 (floppy) */
    buffer[64]  = (char)0x00;
    /* Reserved1 = 0x00 (floppy) */
    buffer[65]  = (char)0x00;
    /* BootSig = 0x00 (next three fields are not 'present') */
    buffer[66]  = (char)0x00;
    /* VolID = 0x00000000 (serial number, e.g. date/time) */
    for (i = 67; i < 71; i++) {
      buffer[i] = (char)0x00;
    }
    /* VolLab = ' ' */
    for (i = 71; i < 82; i++) {
      buffer[i] = (char)0x20;
    }
    /* FilSysType = 'FAT12' */
    FS__CLIB_strncpy(&buffer[82], "FAT32   ", 8);
  }
#endif /* FS_FAT_NOFAT32==0 */
  /* Signature = 0xAA55 */
  buffer[510] = (char)0x55;
  buffer[511] = (char)0xaa;
  /* Write BPB to media */
  i = FS__lb_write(pDriver, Unit, 0, (void*)buffer);
  
  if (i < 0) {
    FS__fat_free(buffer);
    return -1;
  }
  if (FSysType == 2) {
    /* Write backup BPB */
    i = FS__lb_write(pDriver, Unit, 6, (void*)buffer);
    if (i<0) {
      FS__fat_free(buffer);
      return -1;
    }
  }
  /*  Init FAT 1 & 2   */
  FS__CLIB_memset(buffer, 0x00, (FS_size_t)FS_FAT_SEC_SIZE);
  for (sector = 0; sector < 2 * (bpb.FATSz16 + bpb.FATSz32); sector++) {
    value = sector % (bpb.FATSz16 + bpb.FATSz32);
    if (value != 0) {
      i = FS__lb_write(pDriver, Unit, bpb.RsvdSecCnt + sector, (void*)buffer);
      if (i<0) {
        FS__fat_free(buffer);
        return -1;
      }
    }
  }
  buffer[0] = (char)Media;
  buffer[1] = (char)0xff;
  buffer[2] = (char)0xff;
  if (FSysType != 0) {
    buffer[3] = (char)0xff;
  }
#if (FS_FAT_NOFAT32==0)
  if (FSysType == 2) {
    buffer[4]   = (char)0xff;
    buffer[5]   = (char)0xff;
    buffer[6]   = (char)0xff;
    buffer[7]   = (char)0x0f;
    buffer[8]   = (char)0xff;
    buffer[9]   = (char)0xff;
    buffer[10]  = (char)0xff;
    buffer[11]  = (char)0x0f;
  }
#endif /* FS_FAT_NOFAT32==0 */
  for (i = 0; i < 2; i++) {
    j = FS__lb_write(pDriver, Unit, (FS_u32)bpb.RsvdSecCnt + i * ((FS_u32)bpb.FATSz16+bpb.FATSz32), (void*)buffer);
    if (j < 0) {
      FS__fat_free(buffer);
      return -1;
    }
  }
  /* Init root directory area */
  FS__CLIB_memset(buffer, 0x00, (FS_size_t)FS_FAT_SEC_SIZE);
  if (bpb.RootEntCnt != 0) {
    /* FAT12/FAT16 */
    n = (((FS_u32)bpb.RootEntCnt * 32) / (FS_u32)512);
    for (i = 0; i < n; i++) {
      j = FS__lb_write(pDriver, Unit, bpb.RsvdSecCnt + 2 * bpb.FATSz16 + i, (void*)buffer);
      if (j < 0) {
        FS__fat_free(buffer);
        return -1;
      }
    }
  }
#if (FS_FAT_NOFAT32==0)
  else {
    /* FAT32 */
    n = bpb.SecPerClus;
    for (i = 0; i < n; i++) {
      j = FS__lb_write(pDriver, Unit, bpb.RsvdSecCnt + 2 * bpb.FATSz32 + (bpb.RootClus - 2) * n + i, (void*)buffer);
      if (j < 0) {
        FS__fat_free(buffer);
        return -1;
      }
    }
  }
#endif /* FS_FAT_NOFAT32==0 */
#if (FS_FAT_NOFAT32==0)
  if (FSysType == 2) {
    /* Init FSInfo */
    FS__CLIB_memset(buffer, 0x00, (FS_size_t)FS_FAT_SEC_SIZE);
    /* LeadSig = 0x41615252 */
    buffer[0]     = (char)0x52;
    buffer[1]     = (char)0x52;
    buffer[2]     = (char)0x61;
    buffer[3]     = (char)0x41;
    /* StructSig = 0x61417272 */
    buffer[484]   = (char)0x72;
    buffer[485]   = (char)0x72;
    buffer[486]   = (char)0x41;
    buffer[487]   = (char)0x61;
    /* Invalidate last known free cluster count */
    buffer[488]   = (char)0xff;
    buffer[489]   = (char)0xff;
    buffer[490]   = (char)0xff;
    buffer[491]   = (char)0xff;
    
/* Give hint for free cluster search */
    buffer[492]   = (char)0xff;
    buffer[493]   = (char)0xff;
    buffer[494]   = (char)0xff;
    buffer[495]   = (char)0xff;
    /* TrailSig = 0xaa550000 */
    buffer[508]   = (char)0x00;
    buffer[509]   = (char)0x00;
    buffer[510]   = (char)0x55;
    buffer[511]   = (char)0xaa;
    i = FS__lb_write(pDriver, Unit, bpb.FSInfo, (void*)buffer);
    if (i < 0) {
      FS__fat_free(buffer);
      return -1;
    }
  }
#endif /* FS_FAT_NOFAT32==0 */
  FS__fat_free(buffer);
  return 0;
}


/*********************************************************************
*
*             _FS_FAT_AutoFormat
*
  Description:
  FS internal function. Get information about the media from the 
  device driver. Based on that informaton, calculate parameters for
  formatting that media and call the format routine.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  
  Return value:
  >=0         - Media has been formatted.
  <0          - An error has occured.
*/

static int _FS_FAT_AutoFormat(int Idx, FS_u32 Unit) {
  struct {
    FS_u32 hiddennum;
    FS_u32 headnum;
    FS_u32 secnum;
    FS_u32 partsize;
  } devinfo;
  FS_u32 rootentcnt;
  FS_u32 fatsz;
  FS_u32 rootdirsectors;
  FS_u32 tmpval1;
  FS_u32 tmpval2;
  FS_u32 fatsz32;
  FS_u32 totsec32;
  FS_u32 resvdseccnt;
  FS_u16 totsec16;
  FS_u16 fatsz16;
  int i;
  unsigned char secperclust;
  char fsystype;

  /* Get info from device */
  devinfo.hiddennum = 0x00001111UL;
  devinfo.headnum   = 0x00002222UL;
  devinfo.secnum    = 0x00003333UL;
  devinfo.partsize  = 0x00004444UL;
  i = FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_GET_DEVINFO, 0, (void*)&devinfo);
  if (i) {
    return -1;
  }
  /* Check if the driver really supports FS_CMD_GET_DEVINFO */
  if (devinfo.hiddennum == 0x00001111UL) {
    if (devinfo.headnum == 0x00002222UL) {
      if (devinfo.secnum == 0x00003333UL) {
        if (devinfo.partsize == 0x00004444UL) {
          return -1;
        }
      }
    }
  }
  if (devinfo.partsize <= DskTableFAT16[6].SecNum) {
    fsystype = 1;   /* FAT16 */
  }
#if (FS_FAT_NOFAT32==0)
  else {
    fsystype = 2;   /* FAT32 */
  }
#else
  else {
    /* FAT32 disabled */
    return -1;
  }
#endif /* FS_FAT_NOFAT32==0 */

  /* Set correct RootEntCnt & ResvdSecCnt */ 
  if (fsystype != 2) {
    /* FAT16 */
    i = 0;
    while (_FS_auto_rootcnt[i].SecNum < devinfo.partsize) {
      i++;
    }
    rootentcnt = (FS_u16)_FS_auto_rootcnt[i].Num;
    resvdseccnt = 1;
    
     
  }
 
#if (FS_FAT_NOFAT32==0)
  else {
    /* FAT32 */
    rootentcnt = 0;
    resvdseccnt = 0x26;
  }
#endif /* FS_FAT_NOFAT32==0 */

  /* Determinate SecPerClust */
  /* 
  i = 0;
   while (_FS_auto_secperclust[i].SecNum < devinfo.partsize) {
	   i++;
   }
     
   secperclust = (unsigned char) _FS_auto_secperclust[i].Num;
   */
   secperclust = (unsigned char) GetSectorsPerCluster(0,devinfo.partsize);
  /* 
     FAT16/FAT32 FAT size calculation 
     The formula used is following the Microsoft specification
     version 1.03 very closely. Therefore the calculated FAT
     size can be up to 8 sectors bigger than required for the 
     media. This is a waste of up to 8 sectors, but not a problem
     regarding correctness of the media's data.
  */
  rootdirsectors = ((rootentcnt * 32 ) + 511) / 512;
  tmpval1 = devinfo.partsize - (resvdseccnt + rootdirsectors);
  tmpval2 = (256 * secperclust) + 2;
//  printf("AutoFormat  secperclust = %d,rootdirsectors = %d\r\n",
//	 secperclust,rootdirsectors);
#if (FS_FAT_NOFAT32==0)
  if (fsystype == 2) {
    tmpval2 = tmpval2 / 2;
  }
#endif /* FS_FAT_NOFAT32==0 */
  fatsz = (tmpval1 + (tmpval2 - 1)) / tmpval2;
  if (fsystype != 2) {
    fatsz16 = (FS_u16)fatsz;
    fatsz32 = 0;
  }
#if (FS_FAT_NOFAT32==0)
  else {
    fatsz16 = 0;
    fatsz32 = fatsz;
  }
#endif /* FS_FAT_NOFAT32==0 */
  //printf("AutoFormat: %d fatsz16 = %d \r\n",fsystype,fatsz16);
  /* Number of total sectors (512 byte units) of the media
     This is independent of FAT type (FAT12/FAT16/FAT32) */
  if (devinfo.partsize < 0x10000UL) {
    totsec16 = (FS_u16)devinfo.partsize;
    totsec32 = 0;
  }
  else {
    totsec16 = 0;
    totsec32 = devinfo.partsize;
  }


  /* Format media using calculated values */
  i = _FS_fat_format(FS__pDevInfo[Idx].devdriver,
                        Unit,
                        secperclust,
                        (FS_u16)rootentcnt,
                        totsec16,
                        totsec32,
                        (char)0xf8,
                        fatsz16,
                        fatsz32,
                        (FS_u16)devinfo.secnum,
                        (FS_u16)devinfo.headnum,
                        devinfo.hiddennum,
                        fsystype);
  return i;
}

#endif /* FS_FAT_NOFORMAT==0 */


#if FS_FAT_DISKINFO

/*********************************************************************
*
*             _FS_fat_GetTotalFree
*
  Description:
  FS internal function. Store information about used/unused clusters
  in a FS_DISKFREE_T data structure.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  pDiskData   - Pointer to a FS_DISKFREE_T data structure.
  
  Return value:
  ==0         - Information is stored in pDiskData.
  <0          - An error has occured.
*/

static int _FS_fat_GetTotalFree(int Idx, FS_u32 Unit, FS_DISKFREE_T *pDiskData) {
  FS_u32 freeclust;
  FS_u32 usedclust;
  FS_u32 totclust;
  FS_u32 fatentry;
  FS_u32 fatsize;
  FS_i32 fatoffs;
  FS_i32 bytespersec;
  FS_i32 cursec;
  FS_i32 fatsec;
  FS_i32 lastsec;
  FS_i32 fatindex;
  int fattype;
  int err;
  char *buffer;
  unsigned char a;
  unsigned char b;
#if (FS_FAT_NOFAT32==0)
  unsigned char c;
  unsigned char d;
#endif
  
  if (!pDiskData) {
    return -1;  /* No pointer to a FS_DISKFREE_T structure */
  }
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }
  fattype = FS__fat_which_type(Idx, Unit);

#if (FS_FAT_NOFAT32!=0)
  if (fattype == 2) {
    FS__fat_free(buffer);
    return -1;
  }
#endif /* FS_FAT_NOFAT32!=0 */
  fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
  if (fatsize == 0) {
    fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
  }
  bytespersec = (FS_i32)FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec;
  /* Calculate total allocation units on disk */
  totclust = FS__FAT_aBPBUnit[Idx][Unit].TotSec16;
  if (!totclust) {
    totclust = FS__FAT_aBPBUnit[Idx][Unit].TotSec32;
  }
  totclust  -= FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt;
  totclust  -= 2*fatsize;
  usedclust  = FS__FAT_aBPBUnit[Idx][Unit].RootEntCnt;
  usedclust *= 0x20;
  usedclust /= bytespersec;
  totclust  -= usedclust;
  totclust  /= FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
  /* Scan FAT for free and used entries */
  cursec     = 0;
  fatsec     = 0;
  lastsec    = -1;
  fatentry   = 0xffffUL;
  freeclust  = 0;
  usedclust  = 0;
  while (1) {
    if (cursec >= (FS_i32)totclust) {
      break;  /* Last cluster reached */
    }
    if (fatsec >= (FS_i32)fatsize + FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt) {
      break;  /* End of FAT reached */
    }
    if (fattype == 1) {
      fatindex = (cursec + 2) + ((cursec + 2) / 2);    /* FAT12 */
    }
    else if (fattype == 2) {
      fatindex = (cursec + 2) * 4;               /* FAT32 */
    }
    else {
      fatindex = (cursec + 2) * 2;               /* FAT16 */
    }
    fatsec = FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + (fatindex / bytespersec);
    fatoffs = fatindex % bytespersec;
    if (fatsec != lastsec) {
      err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
      if (err < 0) {
        err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
        if (err < 0) {
          FS__fat_free(buffer);
          return -1;
        }
        /* Try to repair original FAT sector with contents of copy */
        FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
      }
      lastsec = fatsec;
    }
    if (fattype == 1) {
      if (fatoffs == (bytespersec - 1)) {
        a = buffer[fatoffs];
        err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec+1, (void*)buffer);
        if (err < 0) {
          err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec + 1, (void*)buffer);
          if (err < 0) {
            FS__fat_free(buffer);
            return -1;
          }
          /* Try to repair original FAT sector with contents of copy */
          FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)buffer);
        }
        lastsec = fatsec + 1;
        b = buffer[0];
      }
      else {
        a = buffer[fatoffs];
        b = buffer[fatoffs + 1];
      }
      if (cursec & 1) {
        fatentry = ((a & 0xf0) >> 4 ) + 16 * b;
      }
      else {
        fatentry = a + 256 * (b & 0x0f);
      }
      fatentry &= 0x0fff;
    }
#if (FS_FAT_NOFAT32==0)
    else if (fattype == 2) {
      a = buffer[fatoffs];
      b = buffer[fatoffs + 1];
      c = buffer[fatoffs + 2];
      d = buffer[fatoffs + 3];
      fatentry = a + 0x100UL * b + 0x10000UL * c + 0x1000000UL * d;
      fatentry &= 0x0fffffffUL;
    }
#endif /* FS_FAT_NOFAT32==0 */
    else {
      a = buffer[fatoffs];
      b = buffer[fatoffs + 1];
      fatentry = a + 256 * b;
      fatentry &= 0xffffUL;
    }
    cursec++;
    if (fatentry == 0) {
      freeclust++;
    }
    else {
      usedclust++;
    }
  }
  FS__fat_free(buffer);
  pDiskData->total_clusters      = totclust;
  pDiskData->avail_clusters      = freeclust;
  pDiskData->sectors_per_cluster = FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
  pDiskData->bytes_per_sector    = (FS_u16)bytespersec;
  return 0;
}

#endif /* FS_FAT_DISKINFO */


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__fat_ioctl
*
  Description:
  FS internal function. Execute device command. The FAT layer checks
  first, if it has to process the command (e.g. format). Any other
  command is passed to the device driver.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  Cmd         - Command to be executed.
  Aux         - Parameter depending on command.
  pBuffer     - Pointer to a buffer used for the command.
  
  Return value:
  Command specific. In general a negative value means an error.
*/

int FS__fat_ioctl(int Idx, FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer) {
  int x;
#if ((FS_SUPPORT_SEC_ACCESS) || (FS_FAT_NOFORMAT==0))
  int i;
#endif
#if (FS_FAT_NOFORMAT==0)
  int j;
#endif

  FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_INC_BUSYCNT, 0, (void*)0);  /* Turn on busy signal */
#if (FS_FAT_NOFORMAT==0)
  if (Cmd == FS_CMD_FORMAT_MEDIA) {
    j = 0;
    while (1) {
      if (j >= FS_KNOWNMEDIA_NUM) {
        break;  /* Not a known media */
      }
      if (_FS_wd_format_media_table[j].media_id == Aux) {
        break;  /* Media found in the list */
      }
      j++;
    }
    if (j >= FS_KNOWNMEDIA_NUM) {
      FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
      return -1;
    }
    i = FS__lb_status(FS__pDevInfo[Idx].devdriver, Unit);
    if (i >= 0) {
      x = _FS_fat_format(FS__pDevInfo[Idx].devdriver,
                          Unit,
                          _FS_wd_format_media_table[j].secperclus,
                          _FS_wd_format_media_table[j].rootentcnt,
                          _FS_wd_format_media_table[j].totsec16,
                          _FS_wd_format_media_table[j].totsec32,
                          _FS_wd_format_media_table[j].media,
                          _FS_wd_format_media_table[j].fatsz16,
                          0,
                          _FS_wd_format_media_table[j].secpertrk,
                          _FS_wd_format_media_table[j].numheads,
                          _FS_wd_format_media_table[j].hiddsec,
                          _FS_wd_format_media_table[j].fsystype);
      i = FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_FLUSH_CACHE, 0, (void*)0);
      if (i < 0) {
        x = i;
      }
      else {
        /* Invalidate BPB */
        for (i = 0; i < (int)FS__maxdev; i++) {
          for (j = 0; j < (int)FS__fat_maxunit; j++) {
              FS__FAT_aBPBUnit[i][j].Signature = 0x0000;
          }
        }
      }
    }
    else {
      FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
      return -1;
    }
  }
  else if (Cmd == FS_CMD_FORMAT_AUTO) {
    i = FS__lb_status(FS__pDevInfo[Idx].devdriver, Unit);
    if (i >= 0) {
      x = _FS_FAT_AutoFormat(Idx, Unit);
      i = FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_FLUSH_CACHE, 0, (void*)0);
      if (i < 0) {
        x = i;
      }
    }
    else {
      x = -1;
    }
  }
#else /* FS_FAT_NOFORMAT==0 */
  if (Cmd == FS_CMD_FORMAT_MEDIA) {
    x = -1;  /* Format command is not supported */
  }
#endif /* FS_FAT_NOFORMAT==0 */
#if FS_FAT_DISKINFO
  else if (Cmd == FS_CMD_GET_DISKFREE) {
    i = FS__fat_checkunit(Idx, Unit);
    if (i > 0) {
      x = _FS_fat_GetTotalFree(Idx, Unit, (FS_DISKFREE_T*)pBuffer);
      i = FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_FLUSH_CACHE, 0, (void*)0);
      if (i < 0) {
        x = i;
      }
    }
    else {
      x = -1;
    }
  }
#else /* FS_FAT_DISKINFO==0 */
  else if (Cmd == FS_CMD_GET_DISKFREE) {
    x = -1; /* Diskinfo command not supported */
  }
#endif /* FS_FAT_DISKINFO */
#if FS_SUPPORT_SEC_ACCESS
  else if ((Cmd == FS_CMD_READ_SECTOR) || (Cmd == FS_CMD_WRITE_SECTOR)) {
    if (!pBuffer) {
      FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);
      return -1;
    }
    i = FS__lb_status(FS__pDevInfo[Idx].devdriver, Unit);
    if (i >= 0) {
      if (Cmd == FS_CMD_READ_SECTOR) {
        x = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, Aux, pBuffer);
      }
      else {
        x = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, Aux, pBuffer);
      }
    }
    else {
      x = -1;
    }
  }
#else /* FS_SUPPORT_SEC_ACCESS */
  else if ((Cmd == FS_CMD_READ_SECTOR) || (Cmd == FS_CMD_WRITE_SECTOR)) {
    FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);
    return -1;
  }
#endif /* FS_SUPPORT_SEC_ACCESS */
  else {
    /* Maybe command for driver */
    x = FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, Cmd, Aux, (void*)pBuffer);
  }
  FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, Unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
  return x;
}


