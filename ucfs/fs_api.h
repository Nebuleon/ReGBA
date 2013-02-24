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
File        : fs_api.h 
Purpose     : Define global functions and types to be used by an
              application using the file system.

              This file needs to be included by any modul using the
              file system.
----------------------------------------------------------------------
Version-Date-----Author-Explanation
----------------------------------------------------------------------
1.00.00 20020815 TB     First release
1.00.01 20020821 TB     Internal modifications
1.10.00 20020927 TB     MMC & SD card driver added
1.20.00 20021010 TB     IDE & CF driver added
1.21.00 20021025 TB     FAT32 added / POSIX like directory support added
1.22.00 20021028 TB     Internal modifications
1.22.01 20021031 TB     Experimental DSP support added (internal version)
1.23.00 20021202 TB     Intermediate release with switches FS_FAT_NOFAT32
                        and FS_FAT_NOFORMAT. Old manual V1.20 is provided,
                        FS_RmDir is not supported.
1.24.00 20021206 TB     Official successor of 1.20.00
1.24.01 20030110 TB     Internal test version
1.26.00 20030113 TB     Support for trial version added.
                        FS__fat_malloc returns cleared buffer.
                        MMC driver deactivates CS whenever allowed by 
                        specification.
1.28.00 20030207 TB     FS_RmDir support added.
                        FS_IoCtl command FS_CMD_GET_DISKFREE added.
1.28.01 20030311 TB     FS_FWrite/FS_FRead preformance improved.
1.30.00 20030530 TB     Switch FS_FAT_DISKINFO added.
                        Logical Block Read Cache implemented.
                        No open file bug when using FS_FOpen(name,"r+") 
                        corrected.
                        Additional cluster allocation bug when writing 
                        data with size of a cluster corrected.
                        Command FS_CMD_FORMAT_AUTO added.
1.31.00 20030807 TB     Internal test version
                        FAT32 formatting added.
                        Directory entries with attribute ATTR_LONG_NAME 
                        are no longer shown by FS_ReadDir.
                        FS_FWrite performance improved by optimizing
                        directory access.
                        FS_CMD_GET_DISKFREE can now be executed as first 
                        command after a disk change took place.
                        Pointer to device information table is located
                        in ROM now.
                        FS_Remove reports error when file does not
                        exist or cannot be removed.
                        FS_IoCtl commands FS_CMD_READ_SECTOR and
                        FS_CMD_WRITE_SECTOR added.
                        Non ANSI C conform construct in RAM disk and
                        generic FLASH driver removed.
1.31.01 20030815 TB     Internal test version
                        Improved function comments.
                        FS__fat_fread return value corrected for element
                        size bigger than 1.
                        FS__fat_fwrite return value corrected for element
                        size bigger than 1.
                        FS__fat_FAT_alloc valid cluster check improved.
                        Switch FS_FAT_FWRITE_UPDATE_DIR added; default on.
1.31.02 20030818 TB     Internal test version
                        Standard library function calls removed.
1.31.03 20030826 TB     Internal test version
                        Expressions in code simplified.
                        SMC write performance improved by optional 
                        switch FS_SMC_PAGE_BUFFER.
1.32.00 20030910 TB/TW  Official successor of 1.30
                        New cluster allocation performance improved.
                        Element FAT_DirAttr added in structure FS_DIRENT.
                        FS_FWrite performance improved by avoiding 
                        unnecessary read operations.
1.34.00 20031009 TW     Official successor of 1.32
                        Hardware interface routines of MMC/SD simplified.
                        MMC/SD partition table no longer needed.
                        Several improvements and bug fixes in t MMC/SD driver.
1.34.01 20031015 TW     FS_FWrite cluster allocation failed.

----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
 - Total path length of a short name is currently not limited to 80
   characters; in fact it is unlimited.
 - VFAT (long filenames) are currently not supported for legal reasons;
   there is a patent on it.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_API_H_
#define _FS_API_H_

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/
#ifdef __cplusplus
extern "C"{
#endif


#include "fs_port.h"
#include "fs_conf.h"

/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

/* File system version */
#define FS_VERSION                0x00013401UL   /* 1.34.01 */

/* Maximum size of a directory name */
#define FS_DIRNAME_MAX            255

/* Global error codes */
#define FS_ERR_OK                 (FS_i16)0x0000
#define FS_ERR_EOF                (FS_i16)0xfff0
#define FS_ERR_DISKFULL           (FS_i16)0xffe0
#define FS_ERR_INVALIDPAR         (FS_i16)0xffd0
#define FS_ERR_WRITEONLY          (FS_i16)0xffc0
#define FS_ERR_READONLY           (FS_i16)0xffb0
#define FS_ERR_READERROR          (FS_i16)0xffa0
#define FS_ERR_WRITEERROR         (FS_i16)0xff90
#define FS_ERR_DISKCHANGED        (FS_i16)0xff80
#define FS_ERR_CLOSE              (FS_i16)0xff70

/* Global constants*/
#define FS_SEEK_CUR         1
#define FS_SEEK_END         2
#define FS_SEEK_SET         0

/* I/O commands */
#define FS_CMD_FLUSH_CACHE        1000L
#define FS_CMD_CHK_DSKCHANGE      1010L
#define FS_CMD_READ_SECTOR        1100L
#define FS_CMD_WRITE_SECTOR       1110L
#define FS_CMD_FORMAT_MEDIA       2222L
#define FS_CMD_FORMAT_AUTO        2333L
#define FS_CMD_INC_BUSYCNT        3001L
#define FS_CMD_DEC_BUSYCNT        3002L
#define FS_CMD_GET_DISKFREE       4000L
#define FS_CMD_GET_DEVINFO        4011L
#define FS_CMD_FLASH_ERASE_CHIP   9001L
#define FS_CMD_MOUNT              9002L
#define FS_CMD_UNMOUNT            9003L

/* known media */
#define FS_MEDIA_SMC_1MB          1010L
#define FS_MEDIA_SMC_2MB          1020L
#define FS_MEDIA_SMC_4MB          1030L
#define FS_MEDIA_SMC_8MB          1040L
#define FS_MEDIA_SMC_16MB         1050L
#define FS_MEDIA_SMC_32MB         1060L
#define FS_MEDIA_SMC_64MB         1070L
#define FS_MEDIA_SMC_128MB        1080L

#define FS_MEDIA_RAM_16KB         2010L
#define FS_MEDIA_RAM_64KB         2020L
#define FS_MEDIA_RAM_128KB        2030L
#define FS_MEDIA_RAM_256KB        2040L
#define FS_MEDIA_RAM_512KB        2050L

#define FS_MEDIA_FD_144MB         3040L

#define FS_MEDIA_MMC_32MB         4060L
#define FS_MEDIA_MMC_64MB         4070L
#define FS_MEDIA_MMC_128MB        4080L

#define FS_MEDIA_SD_16MB          5050L
#define FS_MEDIA_SD_64MB          5070L
#define FS_MEDIA_SD_128MB         5080L

#define FS_MEDIA_CF_32MB          6060L
#define FS_MEDIA_CF_64MB          6070L

/* device states */
#define FS_MEDIA_ISNOTPRESENT     0 
#define FS_MEDIA_ISPRESENT        1
#define FS_MEDIA_STATEUNKNOWN     2

/*********************************************************************
*
*             Global data types
*
**********************************************************************
*/

typedef struct {
	FS_u32 fileid_lo;          /* unique id for file (lo)      */
	FS_u32 fileid_hi;          /* unique id for file (hi)      */
	FS_u32 fileid_ex;          /* unique id for file (ex)      */
	FS_i32 EOFClust;           /* used by FAT FSL only         */
	FS_u32 CurClust;           /* used by FAT FSL only         */
	FS_i32 filepos;            /* current position in file     */
	FS_i32 size;               /* size of file                 */
	int dev_index;             /* index in _FS_devinfo[]       */
	FS_i16 error;              /* error code                   */
	unsigned char inuse;       /* handle in use mark           */
	unsigned char mode_r;      /* mode READ                    */
	unsigned char mode_w;      /* mode WRITE                   */
	unsigned char mode_a;      /* mode APPEND                  */
	unsigned char mode_c;      /* mode CREATE                  */
	unsigned char mode_b;      /* mode BINARY                  */
	FS_u16 creat_time;
	FS_u16 creat_date;
} FS_FILE;


typedef struct {
  FS_u32 total_clusters;
  FS_u32 avail_clusters;
  FS_u16 sectors_per_cluster;
  FS_u16 bytes_per_sector;
} FS_DISKFREE_T;


/*********************************************************************
*
*             directory types
*/

#if FS_POSIX_DIR_SUPPORT

#define FS_ino_t  int

struct FS_DIRENT {
  FS_ino_t  d_ino;                      /* to be POSIX conform */
  char      d_name[FS_DIRNAME_MAX];
  FS_u16    l_name[FS_DIRNAME_MAX];
  char      FAT_DirAttr;                /* FAT only. Contains the "DIR_Attr" field of an entry. */
};

typedef struct {
  struct FS_DIRENT  dirent;  /* cunrrent directory entry     */
  FS_u32 dirid_lo;           /* unique id for file (lo)      */
  FS_u32 dirid_hi;           /* unique id for file (hi)      */
  FS_u32 dirid_ex;           /* unique id for file (ex)      */
  FS_i32 dirpos;             /* current position in file     */
  FS_i32 size;               /* size of file                 */
  int dev_index;             /* index in _FS_devinfo[]       */
  FS_i16 error;              /* error code                   */
  unsigned char inuse;       /* handle in use mark           */
} FS_DIR;

#endif  /* FS_POSIX_DIR_SUPPORT */


/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

/*********************************************************************
*
*             STD file I/O functions
*/

FS_FILE             *FS_FOpen(const char *pFileName, const char *pMode);
void                FS_FClose(FS_FILE *pFile);
FS_size_t           FS_FRead(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);
FS_size_t           FS_FWrite(const void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);


/*********************************************************************
*
*             file pointer handling
*/

int                 FS_FSeek(FS_FILE *pFile, FS_i32 Offset, int Whence);
FS_i32              FS_FTell(FS_FILE *pFile);


/*********************************************************************
*
*             I/O error handling
*/

FS_i16              FS_FError(FS_FILE *pFile);
void                FS_ClearErr(FS_FILE *pFile);


/*********************************************************************
*
*             file functions
*/

int                 FS_Remove(const char *pFileName);


/*********************************************************************
*
*             IOCTL
*/

int                 FS_IoCtl(const char *pDevName, FS_i32 Cmd, FS_i32 Aux, void *pBuffer);



/*********************************************************************
*
*             directory functions
*/

#if FS_POSIX_DIR_SUPPORT

FS_DIR              *FS_OpenDir(const char *pDirName);
int                 FS_CloseDir(FS_DIR *pDir);
struct FS_DIRENT    *FS_ReadDir(FS_DIR *pDir);
struct FS_DIRENT    *FS_ReadDir_Back(FS_DIR *pDir);
void                FS_RewindDir(FS_DIR *pDir);
int                 FS_MkDir(const char *pDirName);
int                 FS_RmDir(const char *pDirName);

#endif  /* FS_POSIX_DIR_SUPPORT */


/*********************************************************************
*
*             file system control functions
*/

int                 FS_Init(void);
int                 FS_Exit(void);

#ifdef __cplusplus
}
#endif
#endif  /* _FS_API_H_ */


