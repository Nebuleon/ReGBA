/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "fat.h" - Driver's constants, structures and prototypes        *
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

#ifndef __FD32_FAT_H
#define __FD32_FAT_H


#include <nls.h>
#include <lfn.h>
/* Use the following defines to add features to the FAT driver */
/* TODO: The FAT driver currently doesn't work with buffers disabled! */
/* TODO: The name cache is totally broken! */
/* TODO: Sharing support can't be currently enabled */
#define FATBUFFERS   /* Uncomment this to use the buffered I/O        */
#define FATLFN       /* Define this to use Long File Names            */
#define FATWRITE     /* Define this to enable writing facilities      */
//#define FATSHARE     /* Define this to enable sharing support         */
//#define FATNAMECACHE /* Define this to enable the opening name cache  */
#define FATREMOVABLE /* Define this to enable removable media support */
#define FAT_FD32DEV  /* Define this to enable FD32 devices support    */

/* 4-characters signatures to identify correct FAT driver structures */
#define FAT_VOLSIG 0x46415456 /* "FATV": FAT volume signature */
#define FAT_FILSIG 0x46415446 /* "FATF": FAT file signature   */

/* FIX ME: These should be read from CONFIG.SYS (or similar) */
#define FAT_MAX_FILES   20
#define FAT_MAX_BUFFERS 30

/* FAT Types */
typedef enum { FAT12, FAT16, FAT32 } tFatType;

/* EOC (End Of Clusterchain) check macros.                               */
/* These expressions are true (nonzero) if the value of a FAT entry is   */
/* an EOC for the FAT type. An EOC indicates the last cluster of a file. */
#define  FAT12_EOC(EntryValue)  (EntryValue >= 0x0FF8)
#define  FAT16_EOC(EntryValue)  (EntryValue >= 0xFFF8)
#define  FAT32_EOC(EntryValue)  (EntryValue >= 0x0FFFFFF8)

/* Bad cluster marks.                                                    */
/* Set a FAT entry to the FATxx_BAD value to mark the cluster as bad.    */
/*                                                                       */
/* The FAT file system specification says that to avoid confusion, no    */
/* FAT32 volume should ever be configured such that 0x0FFFFFF7 is an     */
/* allocatable cluster number. In fact an entry that would point to the  */
/* cluster 0x0FFFFFF7 would be recognised as Bad instead. Since values   */
/* greater or equal than 0x0FFFFFF8 are interpreted as EOC, I think we   */
/* can assume that the max cluster for a FAT32 volume is 0x0FFFFFF6.     */
/* That problem doesn't exist on FAT12 and FAT16 volumes, in fact:       */
/* 0x0FF7 = 4087 is greater than 4086 (max cluster for a FAT12 volume)   */
/* 0xFFF7 = 65527 is greater than 65526 (max cluster for a FAT16 volume) */
#define FAT12_BAD 0x0FF7
#define FAT16_BAD 0xFFF7
#define FAT32_BAD 0x0FFFFFF7

/* Special codes for the first byte of a directory entry */
#define FREEENT  0xE5 /* The directory entry is free             */
#define ENDOFDIR 0x00 /* This and the following entries are free */

/* FAT Return Codes                                          */
/* FIX ME: These should be removed. Use FD32 errors instead. */
#define FAT_RET_EOF -0xF7

/* Additional allowable attributes mask for lookup */
#define FAT_FANOVOLID 0x37 /* All attributes but volume label */

/* FAT32 Boot Sector and BIOS Parameter Block                         */
/* This structure is used in all driver functions, even if the volume */
/* is FAT12 or FAT16. In fact some FAT32 fields are checked anyway    */
/* like BPB_FATSz32). So we load the boot sector from the disk, we    */
/* detect the FAT type and if it's a FAT32, we fill the following     */
/* struct as is, while if the volume is FAT12/FAT16 we copy in the    */
/* right position the appropriate (common) fields.                    */
typedef struct
{
  FBYTE  BS_jmpBoot[3];
  FBYTE  BS_OEMName[8];
  FWORD  BPB_BytsPerSec;
  FBYTE  BPB_SecPerClus;
  FWORD  BPB_ResvdSecCnt;
  FBYTE  BPB_NumFATs;
  FWORD  BPB_RootEntCnt;
  FWORD  BPB_TotSec16;
  FBYTE  BPB_Media;
  FWORD  BPB_FATSz16;
  FWORD  BPB_SecPerTrk;
  FWORD  BPB_NumHeads;
  DWORD BPB_HiddSec;
  DWORD BPB_TotSec32;
  /* Here start the FAT32 specific fields (offset 36) */
  DWORD BPB_FATSz32;
  FWORD  BPB_ExtFlags;
  FWORD  BPB_FSVer;
  DWORD BPB_RootClus;
  FWORD  BPB_FSInfo;
  WORD  BPB_BkBootSec;
  FBYTE  BPB_Reserved[12];
  /* The following fields are present also in a FAT12/FAT16 BPB,     */
  /* but at offset 36. In a FAT32 BPB they are at offset 64 instead. */
  FBYTE  BS_DrvNum;
  FBYTE  BS_Reserved1;
  FBYTE  BS_BootSig;
  DWORD BS_VolID;
  FBYTE  BS_VolLab[11];
  FBYTE  BS_FilSysType[8];
}
__attribute__ ((packed)) tBpb;


/* FAT32 FSInfo Sector structure */
typedef struct
{
  DWORD LeadSig;
  BYTE  Reserved1[480];
  DWORD StrucSig;
  DWORD Free_Count;
  DWORD Nxt_Free;
  BYTE  Reserved2[12];
  DWORD TrailSig;
}
__attribute__ ((packed)) tFSInfo;


/* FAT 32-byte Directory Entry structure */
typedef struct
{
  BYTE  Name[11];
  BYTE  Attr;
  BYTE  NTRes;
  BYTE  CrtTimeTenth;
  WORD  CrtTime;
  WORD  CrtDate;
  WORD  LstAccDate;
  WORD  FstClusHI;
  WORD  WrtTime;
  WORD  WrtDate;
  WORD  FstClusLO;
  DWORD FileSize;
}
__attribute__ ((packed)) tDirEntry;



#ifdef FATBUFFERS
/* The buffer structure stores the status of a volume buffer */
typedef struct
{
  DWORD  StartingSector;
  DWORD  LastAccess;
  DWORD  Flags;
  BYTE  *Data;
}
tBuffer;
#endif


/* This structure stores all informations about a FAT volume.       */
/* It is the P structure (private data) for file system volume ops. */
typedef struct
{
  /* Data referring to the hosting block device */
  DWORD           hBlkDev; /* Handle of the block device hosting the volume   */
//  fd32_request_t *blkreq;  /* Request function of the block device driver     */
//  void           *BlkDev;  /* DeviceId of the block device hosting the volume */

  /* Some precalculated data */
  DWORD      VolSig;          /* Must be FAT_VOLSIG for a valid volume   */
  tFatType   FatType;         /* Can be FAT12, FAT16 or FAT32            */
  DWORD      DataClusters;    /* The total number of valid data clusters */
  DWORD      FirstDataSector; /* The first sector of the data region     */
  DWORD      FirstRootSector; /* The first sector of FAT12/FAT16 root    */
  DWORD      FSI_Free_Count;  /* The count of free clusters              */
  DWORD      FSI_Nxt_Free;    /* The cluster number from which to start  */
                              /* to search for free clusters, if known   */
  struct nls_operations *nls; /* NLS operations for short file names     */

  /* Buffers */
  #ifdef FATBUFFERS
  DWORD      BufferAccess; /* Counter of buffered read operations */
  DWORD      BufferMiss;   /* Counter of buffer misses            */
  DWORD      BufferHit;    /* Counter of buffer hits              */
  int        NumBuffers;   /* Number of allocated buffers         */
  tBuffer   *Buffers;      /* Dynamic array of buffers            */
  #endif

  /* The BIOS Parameter Block of the volume (the long entry) */
  tBpb       Bpb;
}
tVolume;


/* The file structure stores all the informations about an open file. */
typedef struct
{
  tVolume   *V;               /* Pointer to the volume hosting the file */
  DWORD      ParentFstClus;   /* First cluster of the parent directory  */
  DWORD      DirEntryOffset;  /* Offset of the dir entry in the parent  */
  DWORD      DirEntrySector;  /* Sector containing the directory entry  */
  DWORD      DirEntrySecOff;  /* Byte offset of the dir entry in sector */
  tDirEntry  DirEntry;        /* The file's directory entry             */
  DWORD      Mode;            /* File opening mode                      */
  DWORD      References;      /* Number of times the file is open       */
  DWORD      FilSig;          /* Must be FAT_FILSIG for a valid file    */
  int        DirEntryChanged; /* Nonzero if dir entry should be written */

  /* The following fields refer to the byte position into the file */
  long long int TargetPos;       /* The target position for an I/O         */
  long long int FilePos;         /* The current position into the file     */
  DWORD      PrevCluster;     /* The cluster we were before the current */
  DWORD      Cluster;         /* The current cluster                    */
  DWORD      SectorInCluster; /* The sector in the current cluster      */
  DWORD      ByteInSector;    /* The byte in the current sector         */

  #ifdef FATNAMECACHE
  /* The following fields are used to implement the opening name cache */
  DWORD      CacheLastAccess;
  char       CacheFileName[FD32_LFNPMAX];
  #endif
}
tFile;


/* A unique file identifier. Since FAT does not support hard links,  */
/* we can identify a file from its volume and the location of its    */
/* directory in the file's parent directory.                         */
/* This structure matches the first 24 bytes of the tFile structure. */
typedef struct
{
  tVolume   *V;              /* Pointer to the volume hosting the file */
  DWORD      ParentFstClus;  /* First cluster of the parent directory  */
  DWORD      DirEntryOffset; /* Offset of the dir entry in the parent  */
}
tFileId;


/* Finddata block for the internal "readdir" function (fat_readdir) */
typedef struct
{
  tDirEntry SfnEntry;
  DWORD     EntryOffset;
  int       LfnEntries;
  char      LongName[260];
  char      ShortName[14];
}
tFatFind;


/* The FAT driver format used for the 8 reserved bytes of the find data */
typedef struct
{
  WORD  EntryCount;      /* Entry count position within the directory   */
  DWORD FirstDirCluster; /* First cluster of the search directory       */
  WORD  Reserved;        /* Currently not used by the FAT driver        */
}
__attribute__ ((packed)) tFindRes;


/* Packs the high and low word of a tDirEntry's first cluster in a DWORD */
#define FIRSTCLUSTER(D) (((DWORD) D.FstClusHI << 16) + (DWORD) D.FstClusLO)

/* Nonzero if two tFile or tFileId structs refer to the same file */
#define SAMEFILE(F1, F2) (!memcmp(F1, F2, sizeof(tFileId)))

/* Nonzero if the file is a FAT12/FAT16 root directory */
#define ISROOT(F) (!FIRSTCLUSTER(F->DirEntry) && !F->DirEntrySector && (F->V->FatType != FAT32))


/* ATTRIB.C - Files' date and time procedures */
//int fat_get_attr(tFile *F, fd32_fs_attr_t *A);
#ifdef FATWRITE
//int fat_set_attr(tFile *F, fd32_fs_attr_t *A);
#endif

/* BLOCKIO.C - Services to access the hosting block device */
#ifdef FATWRITE
int fat_flushall   (tVolume *V);
int fat_writebuf   (tVolume *V, int NumBuf);
#endif
int fat_readbuf    (tVolume *V, DWORD Sector);
int fat_trashbuf   (tVolume *V);

/* CREAT.C - File creation and deletion services */
#ifdef FATWRITE
int fat_creat(tFile *Fp, tFile *Ff, char *Name, BYTE Attr, WORD AliasHint);
int fat_rename(tVolume *V, char *OldFullName, char *NewFullName);
int fat_unlink(tVolume *V, char *FileName, DWORD Flags);
#endif

/* DIR.C - Directory management services */
#ifdef FATWRITE
int fat_rmdir(tVolume *V, char *DirName);
int fat_mkdir(tVolume *V, char *DirName);
#endif

/* FAT12.C - FAT12 table access procedures */
int fat12_read_entry (tVolume *V, DWORD N, int FatNum, DWORD *Value);
#ifdef FATWRITE
int fat12_write_entry(tVolume *V, DWORD N, int FatNum, DWORD  Value);
int fat12_unlink     (tVolume *V, DWORD Cluster);
#endif

/* FAT16.C - FAT16 table access procedures */
int fat16_read_entry (tVolume *V, DWORD N, int FatNum, DWORD *Value);
#ifdef FATWRITE
int fat16_write_entry(tVolume *V, DWORD N, int FatNum, DWORD  Value);
int fat16_unlink     (tVolume *V, DWORD Cluster);
#endif

/* FAT32.C - FAT32 table access procedures */
int fat32_read_entry (tVolume *V, DWORD N, int FatNum, DWORD *Value);
#ifdef FATWRITE
int fat32_write_entry(tVolume *V, DWORD N, int FatNum, DWORD  Value);
int fat32_unlink     (tVolume *V, DWORD Cluster);
#endif

/* FATINIT.C - Driver nitialization for the FD32 kernel */
int fat_init();

/* FATREQ.C - FD32 driver request function */
//fd32_request_t fat_request;

/* LFN.C - Long file names support procedures */
//BYTE lfn_checksum   (tDirEntry *D);
#ifdef FATWRITE
//int  gen_short_fname(tFile *Dir, char *LongName, BYTE *ShortName, WORD Hint);
#endif
//int fat_build_fcb_name   (const struct nls_operations *nls, BYTE *Dest, char *Source);
int fat_expand_fcb_name  (const struct nls_operations *nls, char *Dest, const BYTE *Source, size_t size);
int fat_compare_fcb_names(BYTE *Name1, BYTE *Name2); /* was from the FS layer */


/* MOUNT.C - Mount a FAT volume initializing all its data */
int fat_unmount  (tVolume *V);
int fat_mount    (DWORD hDev, tVolume **NewV);
int fat_partcheck(BYTE PartSig);
#ifdef FATREMOVABLE
int fat_mediachange(tVolume *V);
#endif

/* OPEN.C - File opening and creation procedures */
int    fat_openfiles(tVolume *V);
int    fat_isopen   (tFileId *Fid);
int    fat_syncentry(tFile *F);
void   fat_syncpos  (tFile *F);
tFile *fat_getfile  (DWORD FileId);
void   fat_split_path(const char *FullPath, char *Path, char *Name);
int    fat_open     (tVolume *V, char *FileName, DWORD Mode, WORD Attr,
                     WORD AliasHint, tFile **F);
int    fat_reopendir(tVolume *V, tFindRes *Id, tFile **F);
#ifdef FATWRITE
int    fat_fflush   (tFile *F);
#endif
int    fat_reopen   (tFile *F);
int    fat_close    (tFile *F);

/* SUPPORT.C - Small support procedures */
DWORD fat_first_sector_of_cluster(DWORD N, tVolume *V);
void  fat_timestamps(WORD *Time, WORD *Date, BYTE *Hund);
int   fat_lseek(tFile *F, long long int *Offset, int Origin);
//int   fat_get_fsinfo(fd32_fs_info_t *Fsi);
//int   fat_get_fsfree(fd32_getfsfree_t *F);


/* READDIR.C - File find procedures */
int fat_find     (tFile *F, char *FileSpec, DWORD Flags, tFatFind *FindData);
//int fat_readdir  (tFile *P, fd32_fs_lfnfind_t *Entry);
//int fat_findfirst(tVolume *v, const char *path, int attributes, fd32_fs_dosfind_t *find_data);
//int fat_findnext (tVolume *v, fd32_fs_dosfind_t *find_data);
//int fat_findfile (tFile *f, const char *name, int flags, fd32_fs_lfnfind_t *find_data);


/* READWRIT.C - Write a block of data into a file truncating or extending it */
int fat_read (tFile *F, void *Buffer, int Size);
#ifdef FATWRITE
int fat_write(tFile *F, void *Buffer, int Size);
#endif

/* WILDCARD.C - Compare UTG-8 file names with wildcards */
int fat_fnameicmp(const char *s1, const char *s2);

#endif /* #ifndef __FD32_FAT_H */
