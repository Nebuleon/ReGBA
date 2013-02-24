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
File        : fat_misc.c
Purpose     : File system's FAT File System Layer misc routines
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

#ifndef FS_FAT_NOFAT32
  #define FS_FAT_NOFAT32        0
#endif /* FS_FAT_NOFAT32 */

#ifndef FS_DIR_MAXOPEN
  #define FS_DIR_MAXOPEN        0
#endif /* FS_DIR_MAXOPEN */

#define FS_MEMBLOCK_NUM   (FS_MAXOPEN+FS_DIR_MAXOPEN)*2


/*********************************************************************
*
*             Local data types
*
**********************************************************************
*/

typedef struct {
  int status;
  char memory[FS_FAT_SEC_SIZE];
} _FS_FAT_block_type;


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static _FS_FAT_block_type          _FS_memblock[FS_MEMBLOCK_NUM];


/*********************************************************************
*
*             Local functions section
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_ReadBPB
*
  Description:
  FS internal function. Read Bios-Parameter-Block from a device and
  copy the relevant data to FS__FAT_aBPBUnit.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
 
  Return value:
  ==0         - BPB successfully read.
  <0          - An error has occured.
*/

static int _FS_ReadBPB(int Idx, FS_u32 Unit) {
  int err;
  unsigned char *buffer;
  
  buffer = (unsigned char*)FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }
  err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, 0, (void*)buffer);
  if (err < 0) {
    FS__fat_free(buffer);
    return -1;
  }
  /* Assign FS__FAT_aBPBUnit */
  FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec   = buffer[11] + 256 * buffer[12];      /* _512_,1024,2048,4096           */
  FS__FAT_aBPBUnit[Idx][Unit].SecPerClus    = buffer[13];                         /* sec in allocation unit         */
  FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt    = buffer[14] + 256 * buffer[15];      /* 1 for FAT12 & FAT16            */
  FS__FAT_aBPBUnit[Idx][Unit].NumFATs       = buffer[16];                         /* 2                              */
  FS__FAT_aBPBUnit[Idx][Unit].RootEntCnt    = buffer[17] + 256 * buffer[18];      /* number of root dir entries     */
  FS__FAT_aBPBUnit[Idx][Unit].TotSec16      = buffer[19] + 256 * buffer[20];      /* RSVD + FAT + ROOT + FATA (<64k) */
  FS__FAT_aBPBUnit[Idx][Unit].FATSz16       = buffer[22] + 256 * buffer[23];      /* number of FAT sectors          */
  FS__FAT_aBPBUnit[Idx][Unit].TotSec32      = buffer[32] + 0x100UL * buffer[33]   /* RSVD + FAT + ROOT + FATA (>=64k) */
                                              + 0x10000UL * buffer[34] 
                                              + 0x1000000UL * buffer[35];
  if (FS__FAT_aBPBUnit[Idx][Unit].FATSz16 == 0) {
    FS__FAT_aBPBUnit[Idx][Unit].FATSz32       = buffer[36] + 0x100UL * buffer[37]   /* number of FAT sectors          */
                                                + 0x10000UL * buffer[38] 
                                                + 0x1000000UL * buffer[39];
    FS__FAT_aBPBUnit[Idx][Unit].ExtFlags      = buffer[40] + 256 * buffer[41];      /* mirroring info                 */
    FS__FAT_aBPBUnit[Idx][Unit].RootClus      = buffer[44] + 0x100UL * buffer[45]   /* root dir clus for FAT32        */
                                                + 0x10000UL * buffer[46] 
                                                + 0x1000000UL * buffer[47];
    FS__FAT_aBPBUnit[Idx][Unit].FSInfo        = buffer[48] + 256 * buffer[49];      /* position of FSInfo structure   */
  }
  else {
    FS__FAT_aBPBUnit[Idx][Unit].FATSz32       = 0;
    FS__FAT_aBPBUnit[Idx][Unit].ExtFlags      = 0;
    FS__FAT_aBPBUnit[Idx][Unit].RootClus      = 0;
    FS__FAT_aBPBUnit[Idx][Unit].FSInfo        = 0;
  }
  FS__FAT_aBPBUnit[Idx][Unit].Signature     = buffer[FS_FAT_SEC_SIZE-2] 
                                              + 256 * buffer[FS_FAT_SEC_SIZE-1];
  FS__fat_free(buffer);
  return err;
}


/*********************************************************************
*
*             _FS__fat_FindFreeCluster
*
  Description:
  FS internal function. Find the next free entry in the FAT.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  pFATSector  - Returns the sector number of the free entry. 
  pLastSector - Returns the sector number of the sector in pBuffer.
  pFATOffset  - Returns the offset of the free FAT entry within the
                sector pFATSector.
  LastClust   - Cluster, which will be used to link the new allocated
                cluster to. Here it is used at hint for where to start
                in the FAT.
  pBuffer     - Pointer to a sector buffer.
  FSysType    - ==1 => FAT12
                ==0 => FAT16
                ==2 => FAT32
  FATSize     - Size of one FAT ind sectors.
  BytesPerSec - Number of bytes in each sector.
 
  Return value:
  >=0         - Number of the free cluster.
  <0          - An error has occured.
*/

static FS_i32 _FS__fat_FindFreeCluster(int Idx, FS_u32 Unit, FS_i32 *pFATSector, 
                                       FS_i32 *pLastSector, FS_i32 *pFATOffset, 
                                       FS_i32 LastClust, unsigned char *pBuffer, 
                                       int FSysType, FS_u32 FATSize, FS_i32 BytesPerSec) {
  FS_u32 totclst;
  FS_u32 rootdirsize;
  FS_i32 curclst;
  FS_i32 fatindex;
  int err;
  int scan;
  unsigned char fatentry;
  unsigned char a;
  unsigned char b;
#if (FS_FAT_NOFAT32==0)  
  unsigned char c;
  unsigned char d;
#endif  
  
  if (LastClust > 0) {
    curclst = LastClust + 1;  /* Start scan after the previous allocated sector */
  }
  else {
    curclst = 0;  /*  Start scan at the beginning of the media */
  }
  scan          = 0;
  *pFATSector   = 0;
  *pLastSector  = -1;
  fatentry      = 0xff;
  /* Calculate total number of data clusters on the media */
  totclst = (FS_u32)FS__FAT_aBPBUnit[Idx][Unit].TotSec16;
  if (totclst == 0) {
    totclst = FS__FAT_aBPBUnit[Idx][Unit].TotSec32;
  }
  rootdirsize = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[Idx][Unit].RootEntCnt) * FS_FAT_DENTRY_SIZE) / BytesPerSec;
  totclst     = totclst - (FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + FS__FAT_aBPBUnit[Idx][Unit].NumFATs * FATSize + rootdirsize);
  totclst    /= FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
  while (1) {
    if (curclst >= (FS_i32)totclst) {
      scan++;
      if (scan > 1) {
        break;  /* End of clusters reached after 2nd scan */
      }
      if (LastClust <= 0) {
        break;  /* 1st scan started already at zero */
      }
      curclst   = 0;  /* Try again starting at the beginning of the FAT */
      fatentry  = 0xff;
    }
    if (fatentry == 0) {
      break;  /* Free entry found */
    }
    if (FSysType == 1) {
      fatindex = curclst + (curclst / 2);    /* FAT12 */
    }
    else if (FSysType == 2) {
      fatindex = curclst * 4;               /* FAT32 */
    }
    else {
      fatindex = curclst * 2;               /* FAT16 */
    }
    *pFATSector = FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + (fatindex / BytesPerSec);
    *pFATOffset = fatindex % BytesPerSec;
    if (*pFATSector != *pLastSector) {
      err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, *pFATSector, (void*)pBuffer);
      if (err < 0) {
        err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, FATSize + *pFATSector, (void*)pBuffer);
        if (err < 0) {
          return -1;
        }
        /* Try to repair original FAT sector with contents of copy */
        FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, *pFATSector, (void*)pBuffer);
      }
      *pLastSector = *pFATSector;
    }
    if (FSysType == 1) {
      if (*pFATOffset == (BytesPerSec - 1)) {
        a = pBuffer[*pFATOffset];
        err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, *pFATSector + 1, (void*)pBuffer);
        if (err < 0) {
          err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, FATSize + *pFATSector + 1, (void*)pBuffer);
          if (err < 0) {
            return -1;
          }
          /* Try to repair original FAT sector with contents of copy */
          FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, *pFATSector + 1, (void*)pBuffer);
        }
        *pLastSector = *pFATSector + 1;
        b = pBuffer[0];
      }
      else {
        a = pBuffer[*pFATOffset];
        b = pBuffer[*pFATOffset + 1];
      }
      if (curclst & 1) {
        fatentry = ((a & 0xf0) >> 4 ) | b;
      }
      else {
        fatentry = a | (b & 0x0f);
      }
    }
#if (FS_FAT_NOFAT32==0)
    else if (FSysType == 2) {
      a = pBuffer[*pFATOffset];
      b = pBuffer[*pFATOffset + 1];
      c = pBuffer[*pFATOffset + 2];
      d = pBuffer[*pFATOffset + 3];
      fatentry = a | b | c | d;
    }
#endif /* FS_FAT_NOFAT32==0 */
    else {
      a = pBuffer[*pFATOffset];
      b = pBuffer[*pFATOffset + 1];
      fatentry = a | b;
    }
    if (fatentry != 0) {
      curclst++;  /* Cluster is in use or defect, so try the next one */
    }
  }
  if (fatentry == 0) {
    return curclst;  /* Free cluster found */
  }
  return -1;
}


/*********************************************************************
*
*             _FS__fat_SetEOFMark
*
  Description:
  FS internal function. Set the EOF mark in the FAT for a cluster.
  The function does not write the FAT sector. An exception is FAT12,
  if the FAT entry is in two sectors. 

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  FATSector   - FAT sector, where the cluster is located. 
  pLastSector - Pointer to an FS_i32, which contains the number of the 
                sector in pBuffer.
  FATOffset   - Offset of the cluster in the FAT sector.
  Cluster     - Cluster number, where to set the EOF mark.
  pBuffer     - Pointer to a sector buffer.
  FSysType    - ==1 => FAT12
                ==0 => FAT16
                ==2 => FAT32
  FATSize     - Size of one FAT ind sectors.
  BytesPerSec - Number of bytes in each sector.
 
  Return value:
  >=0         - EOF mark set.
  <0          - An error has occured.
*/

static int _FS__fat_SetEOFMark(int Idx, FS_u32 Unit, FS_i32 FATSector, 
                               FS_i32 *pLastSector, FS_i32 FATOffset, 
                               FS_i32 Cluster, unsigned char *pBuffer, 
                               int FSysType, FS_u32 FATSize, FS_i32 BytesPerSec) {
  int err1;
  int err2;
  int lexp;
  
  if (FSysType == 1) {
    if (FATOffset == (BytesPerSec - 1)) {
      /* Entry in 2 sectors (we have 2nd sector in buffer) */
      if (Cluster & 1) {
        pBuffer[0]  = (char)0xff;
      }
      else {
        pBuffer[0] |= (char)0x0f;
      }
      err1 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FATSector + 1, (void*)pBuffer);
      err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FATSize + FATSector + 1, (void*)pBuffer);
      lexp = (err1 < 0);
      lexp = lexp || (err2 < 0);
      if (lexp) {
        return -1;
      }
      err1 = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, FATSector, (void*)pBuffer);
      if (err1 < 0) {
        err1 = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, FATSize + FATSector, (void*)pBuffer);
        if (err1 < 0) {
          return -1;
        }
        /* Try to repair original FAT sector with contents of copy */
        FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FATSector, (void*)pBuffer);
      }
      *pLastSector = FATSector;
      if (Cluster & 1) {
        pBuffer[FATOffset] |= (char)0xf0;
      }
      else {
        pBuffer[FATOffset]  = (char)0xff;
      }
    }
    else {
      if (Cluster & 1) {
        pBuffer[FATOffset]   |= (char)0xf0;
        pBuffer[FATOffset+1]  = (char)0xff;
      }
      else {
        pBuffer[FATOffset]    = (char)0xff;
        pBuffer[FATOffset+1] |= (char)0x0f;
      }
    }
  }
#if (FS_FAT_NOFAT32==0)
  else if (FSysType == 2) { /* FAT32 */
    pBuffer[FATOffset]      = (char)0xff;
    pBuffer[FATOffset + 1]  = (char)0xff;
    pBuffer[FATOffset + 2]  = (char)0xff;
    pBuffer[FATOffset + 3]  = (char)0x0f;
  }
#endif /* FS_FAT_NOFAT32==0 */
  else { /* FAT16 */
    pBuffer[FATOffset]      = (char)0xff;
    pBuffer[FATOffset + 1]  = (char)0xff;
  }
  return 0;
}


/*********************************************************************
*
*             _FS__fat_LinkCluster
*
  Description:
  FS internal function. Link the new cluster with the EOF mark to the 
  cluster list.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  pLastSector - Pointer to an FS_i32, which contains the number of the 
                sector in pBuffer.
  Cluster     - Cluster number of the new cluster with the EOF mark.
  LastClust   - Number of cluster, to which the new allocated cluster
                is linked to.
  pBuffer     - Pointer to a sector buffer.
  FSysType    - ==1 => FAT12
                ==0 => FAT16
                ==2 => FAT32
  FATSize     - Size of one FAT ind sectors.
  BytesPerSec - Number of bytes in each sector.
 
  Return value:
  >=0         - Link has been made.
  <0          - An error has occured.
*/

static int _FS__fat_LinkCluster(int Idx, FS_u32 Unit, FS_i32 *pLastSector, FS_i32 Cluster,
                                FS_i32 LastClust, unsigned char *pBuffer, int FSysType, 
                                FS_u32 FATSize, FS_i32 BytesPerSec) {
  FS_i32 fatindex;
  FS_i32 fatoffs;
  FS_i32 fatsec;
  int lexp;
  int err;
  int err2;
  unsigned char a;
  unsigned char b;
#if (FS_FAT_NOFAT32==0)  
  unsigned char c;
  unsigned char d;
#endif

  /* Link old last cluster to this one */
  if (FSysType == 1) {
    fatindex = LastClust + (LastClust / 2); /* FAT12 */
  }
  else if (FSysType == 2) {
    fatindex = LastClust * 4;               /* FAT32 */
  }
  else {
    fatindex = LastClust * 2;               /* FAT16 */
  }
  fatsec = FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + (fatindex / BytesPerSec);
  fatoffs = fatindex % BytesPerSec;
  if (fatsec != *pLastSector) {
    /* 
       FAT entry, which has to be modified is not in the same FAT sector, which is
       currently in the buffer. So write it to the media now.
    */
    err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, *pLastSector, (void*)pBuffer);
    err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, *pLastSector + FATSize, (void*)pBuffer);
    lexp = (err < 0);
    lexp = lexp || (err2 < 0);
    if (lexp) {
      return -1;
    }
    err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)pBuffer);
    if (err < 0) {
      err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, FATSize + fatsec, (void*)pBuffer);
      if (err<0) {
        return -1;
      }
      /* Try to repair original FAT sector with contents of copy */
      FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)pBuffer);
    }
    *pLastSector = fatsec;
  }
  a = Cluster & 0xff;
  b = (Cluster / 0x100L) & 0xff;
#if (FS_FAT_NOFAT32==0)
  c = (Cluster / 0x10000L) & 0xff;
  d = (Cluster / 0x1000000L) & 0x0f;
#endif      
  if (FSysType == 1) {
    if (fatoffs == (BytesPerSec - 1)) {
      /* Entry in 2 sectors (we have 2nd sector in buffer) */
      if (LastClust & 1) {
        pBuffer[fatoffs]   &= (char)0x0f;
        pBuffer[fatoffs]   |= (char)((a << 4) & 0xf0);
      }
      else {
        pBuffer[fatoffs]    = (char)(a & 0xff);
      }
      err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)pBuffer);
      err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FATSize + fatsec, (void*)pBuffer);
      lexp = (err < 0);
      lexp = lexp || (err2 < 0);
      if (lexp) {
        return -1;
      }
      err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)pBuffer);
      if (err < 0) {
        return -1;
      }
      *pLastSector = fatsec + 1;
      if (LastClust & 1) {
        pBuffer[0]  = (char)(((a >> 4) & 0x0f) | ((b << 4) & 0xf0));
      }
      else {
        pBuffer[0] &= (char)0xf0;
        pBuffer[0] |= (char)(b & 0x0f);
      }
      err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)pBuffer);
      err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FATSize + fatsec + 1, (void*)pBuffer);
      lexp = (err < 0);
      lexp = lexp || (err2 < 0);
      if (lexp) {
        return -1;
      }
    }
    else {
      if (LastClust & 1) {
        pBuffer[fatoffs]     &= (char)0x0f;
        pBuffer[fatoffs]     |= (char)((a << 4) & 0xf0);
        pBuffer[fatoffs + 1]  = (char)(((a >> 4) & 0x0f) | ((b << 4) & 0xf0));
      }
      else {
        pBuffer[fatoffs]      = (char)(a & 0xff);
        pBuffer[fatoffs + 1] &= (char)0xf0;
        pBuffer[fatoffs + 1] |= (char)(b & 0x0f);
      }
      err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)pBuffer);
      err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FATSize + fatsec, (void*)pBuffer);
      lexp = (err < 0);
      lexp = lexp || (err2 < 0);
      if (lexp) {
        return -1;
      }
    }
  }
#if (FS_FAT_NOFAT32==0)
  else if (FSysType == 2) { /* FAT32 */
    pBuffer[fatoffs]      = a;
    pBuffer[fatoffs + 1]  = b;
    pBuffer[fatoffs + 2]  = c;
    pBuffer[fatoffs + 3]  = d;
    err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)pBuffer);
    err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FATSize + fatsec, (void*)pBuffer);
    lexp = (err < 0) ;
    lexp = lexp || (err2 < 0);
    if (lexp) {
      return -1;
    }
  }
#endif /* FS_FAT_NOFAT32==0 */
  else { /* FAT16 */
    pBuffer[fatoffs]      = a;
    pBuffer[fatoffs + 1]  = b;
    err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)pBuffer);
    err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FATSize + fatsec, (void*)pBuffer);
    lexp = (err < 0);
    lexp = lexp || (err2 < 0);
    if (lexp) {
      return -1;
    }
  }
  return 0;
}


/*********************************************************************
*
*             Global functions section
*
**********************************************************************

  Functions in this section are used by FAT File System layer only
  
*/

/*********************************************************************
*
*             FS__fat_block_init
*
  Description:
  FS internal function. Init FAT block memory management.

  Parameters:
  None.
 
  Return value:
  None.
*/

void FS__fat_block_init(void) {
  int i;

  FS_X_OS_LockMem();
  for (i = 0; i < FS_MEMBLOCK_NUM; i++) {
    _FS_memblock[i].status = 0;
  }
  FS_X_OS_UnlockMem();
}


/*********************************************************************
*
*             FS__fat_malloc
*
  Description:
  FS internal function. Allocate a sector buffer.

  Parameters:
  Size        - Size of the sector buffer. Normally this is 512.
                Parameter is for future extension.
 
  Return value:
  ==0         - Cannot allocate a buffer.
  !=0         - Address of a buffer.
*/


char *FS__fat_malloc(unsigned int Size) {
  int i;

  FS_X_OS_LockMem();
  if (Size <= FS_FAT_SEC_SIZE) {
    for (i = 0; i < FS_MEMBLOCK_NUM; i++) {
      if (_FS_memblock[i].status == 0) {
        _FS_memblock[i].status = 1;
        FS__CLIB_memset((void*)_FS_memblock[i].memory, 0, (FS_size_t)FS_FAT_SEC_SIZE);
        FS_X_OS_UnlockMem();
        return ((void*)_FS_memblock[i].memory);
      }
    }
  }
  FS_X_OS_UnlockMem();
  return 0;
}


/*********************************************************************
*
*             FS__fat_free
*
  Description:
  FS internal function. Free sector buffer.

  Parameters:
  pBuffer     - Pointer to a buffer, which has to be set free.
 
  Return value:
  None.
*/

void FS__fat_free(void *pBuffer) {
  int i;

  FS_X_OS_LockMem();
  for (i = 0; i < FS_MEMBLOCK_NUM; i++) {
    if (((void*)_FS_memblock[i].memory) == pBuffer) {
      _FS_memblock[i].status = 0;
      FS_X_OS_UnlockMem();
      return;
    }
  }
  FS_X_OS_UnlockMem();
}


/*********************************************************************
*
*             FS__fat_checkunit
*
  Description:
  FS internal function. Read Bios-Parameter-Block from a device and
  check, if it contains valid data.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
 
  Return value:
  ==1         - BPB is okay.
  ==0         - An error has occured.
*/

int FS__fat_checkunit(int Idx, FS_u32 Unit) {
  int err;
  int status;
  int lexp;
  
  status = FS__lb_status(FS__pDevInfo[Idx].devdriver, Unit);
  if (status < 0) {
    return 0;
  }
  if (status == FS_LBL_MEDIACHANGED) {
    /* Mount new volume */
    err = _FS_ReadBPB(Idx, Unit);
    if (err < 0) {
      return 0;
    }
  }
  if (FS__FAT_aBPBUnit[Idx][Unit].Signature != 0xaa55) {
    err = _FS_ReadBPB(Idx, Unit);
    lexp = (err < 0);
    lexp = lexp || (FS__FAT_aBPBUnit[Idx][Unit].Signature != 0xaa55);
    if (lexp) {
      return 0;
    }
  }
  if (FS__FAT_aBPBUnit[Idx][Unit].NumFATs != 2) {
    return 0;  /* Only 2 FATs are supported */
  }
  if (FS__FAT_aBPBUnit[Idx][Unit].FATSz16 == 0) {
    if (FS__FAT_aBPBUnit[Idx][Unit].ExtFlags & 0x0080) {
      return 0;  /* Only mirroring at runtime supported */
    }
  }
  return 1;
}


/*********************************************************************
*
*             FS__fat_which_type
*
  Description:
  FS internal function. Determine FAT type used on a media. This
  function is following the MS specification very closely.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
 
  Return value:
  ==0         - FAT16.
  ==1         - FAT12.
  ==2         - FAT32
*/

int FS__fat_which_type(int Idx, FS_u32 Unit) {
  FS_u32 coc;
  FS_u32 fatsize;
  FS_u32 totsec;
  FS_u32 datasec;
  FS_u32 bytespersec;
  FS_u32 dsize;

  bytespersec   = (FS_u32)FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec;
  dsize         = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[Idx][Unit].RootEntCnt) * FS_FAT_DENTRY_SIZE) / bytespersec;
  fatsize       = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
  if (fatsize == 0) {
    fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
  }
  totsec = (FS_u32)FS__FAT_aBPBUnit[Idx][Unit].TotSec16;
  if (totsec == 0) {
    totsec = FS__FAT_aBPBUnit[Idx][Unit].TotSec32;
  }
  datasec = totsec - (FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt +
                      FS__FAT_aBPBUnit[Idx][Unit].NumFATs * fatsize + dsize);
  coc     = datasec / FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
  if (coc < 4085) {
    return 1;  /* FAT12 */
  }
  else if (coc < 65525) {
    return 0;  /* FAT16 */
  }
  return 2;  /* FAT32 */
}


/*********************************************************************
*
*             FS__fat_FAT_find_eof
*
  Description:
  FS internal function. Find the next EOF mark in the FAT.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  StrtClst    - Starting cluster in FAT.
  pClstCnt    - If not zero, this is a pointer to an FS_u32, which
                is used to return the number of clusters found
                between StrtClst and the next EOF mark.
 
  Return value:
  >=0         - Cluster, which contains the EOF mark.
  <0          - An error has occured.
*/

FS_i32 FS__fat_FAT_find_eof(int Idx, FS_u32 Unit, FS_i32 StrtClst, FS_u32 *pClstCnt) {
  FS_u32 clstcount;
  FS_u32 fatsize;
  FS_u32 maxclst;
  FS_i32 fatindex;
  FS_i32 fatsec;
  FS_i32 fatoffs;
  FS_i32 lastsec;
  FS_i32 curclst;
  FS_i32 bytespersec;
  FS_i32 eofclst;
  int fattype;
  int err;
  char *buffer;
  unsigned char a;
  unsigned char b;
#if (FS_FAT_NOFAT32==0)
  unsigned char c;
  unsigned char d;
#endif /* FS_FAT_NOFAT32==0 */
  
  fattype = FS__fat_which_type(Idx, Unit);
  if (fattype == 1) {
    maxclst = 4085UL;       /* FAT12 */
  }
  else if (fattype == 2) {
#if (FS_FAT_NOFAT32 == 0)
    maxclst = 0x0ffffff0UL; /* FAT32 */
#else
    return -1;
#endif /* (FS_FAT_NOFAT32==0) */    
  }
  else {
    maxclst = 65525UL;      /* FAT16 */
  }
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }
  fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
  if (fatsize == 0) {
    fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
  }
  bytespersec   = (FS_i32)FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec;
  curclst       = StrtClst;
  lastsec       = -1;
  clstcount     = 0;
  while (clstcount < maxclst) {
    eofclst = curclst;
    clstcount++;
    if (fattype == 1) {
      fatindex = curclst + (curclst / 2);   /* FAT12 */
    }
#if (FS_FAT_NOFAT32==0)    
    else if (fattype == 2) {
      fatindex = curclst * 4;               /* FAT32 */
    }
#endif /* FS_FAT_NOFAT32==0 */    
    else {
      fatindex = curclst * 2;               /* FAT16 */
    }
    fatsec  = FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + (fatindex / bytespersec);
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
        a   = buffer[fatoffs];
        err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)buffer);
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
        b       = buffer[0];
      }
      else {
        a = buffer[fatoffs];
        b = buffer[fatoffs + 1];
      }
      if (curclst & 1) {
        curclst = ((a & 0xf0) >> 4 ) + 16 * b;
      }
      else {
        curclst = a + 256 * (b & 0x0f);
      }
      curclst &= 0x0fffL;
      if (curclst >= 0x0ff8L) {
        /* EOF found */
        FS__fat_free(buffer);
        if (pClstCnt) {
          *pClstCnt = clstcount;
        }
        return eofclst;
      }
    }
#if (FS_FAT_NOFAT32==0)
    else if (fattype == 2) {
      a         = buffer[fatoffs];
      b         = buffer[fatoffs + 1];
      c         = buffer[fatoffs + 2];
      d         = buffer[fatoffs + 3];
      curclst   = a + 0x100L * b + 0x10000L * c + 0x1000000L * d;
      curclst  &= 0x0fffffffL;
      if (curclst >= (FS_i32)0x0ffffff8L) {
        /* EOF found */
        FS__fat_free(buffer);
        if (pClstCnt) {
          *pClstCnt = clstcount;
        }
        return eofclst;
      }
    }
#endif /* FS_FAT_NOFAT32==0 */
    else {
      a         = buffer[fatoffs];
      b         = buffer[fatoffs + 1];
      curclst   = a + 256 * b;
      curclst  &= 0xffffL;
      if (curclst >= (FS_i32)0xfff8L) {
        /* EOF found */
        FS__fat_free(buffer);
        if (pClstCnt) {
          *pClstCnt = clstcount;
        }
        return eofclst;
      }
    }
  } /* while (clstcount<maxclst) */
  FS__fat_free(buffer);
  return -1;
}


/*********************************************************************
*
*             FS__fat_FAT_alloc
*
  Description:
  FS internal function. Allocate a new cluster in the FAT and link it
  to LastClust. Assign an EOF mark to the new allocated cluster.
  The function has grown a lot, since it supports all FAT types (FAT12,
  FAT16 & FAT32). There is also room for performance improvement, when
  makeing the new FAT entry and the old entry is within the same FAT
  sector.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  LastClust   - Number of cluster, to which the new allocated cluster
                is linked to. If this is negative, the new cluster is
                not linked to anything and only the EOF mark is set.
 
  Return value:
  >=0         - Number of new allocated cluster, which contains the 
                EOF mark.
  <0          - An error has occured.
*/

FS_i32 FS__fat_FAT_alloc(int Idx, FS_u32 Unit, FS_i32 LastClust) {
  FS_u32 fatsize;
  FS_i32 fatoffs;
  FS_i32 bytespersec;
  FS_i32 curclst;
  FS_i32 fatsec;
  FS_i32 lastsec;
  unsigned char *buffer;
  int fattype;
  int err;
  int err2;
  int lexp;
  
  buffer = (unsigned char*)FS__fat_malloc(FS_FAT_SEC_SIZE);
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
  bytespersec   = (FS_i32)FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec;
  /* Find a free cluster in the FAT */
  curclst       = _FS__fat_FindFreeCluster(Idx, Unit, &fatsec, &lastsec, &fatoffs, LastClust, buffer, fattype, fatsize, bytespersec);
  if (curclst < 0) {
    FS__fat_free(buffer);   /* No free cluster found. */
    return -1;
  }
  /* Make an EOF entry for the new cluster */
  err = _FS__fat_SetEOFMark(Idx, Unit, fatsec, &lastsec, fatoffs, curclst, buffer, fattype, fatsize, bytespersec);
  if (err < 0) {
    FS__fat_free(buffer);
    return -1;
  }
  /* Link the new cluster to the cluster list */
  if (LastClust < 0) {
    err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, lastsec, (void*)buffer);
    err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + lastsec, (void*)buffer);
    lexp = (err < 0);
    lexp = lexp || (err2 < 0);
    if (lexp) {
      FS__fat_free(buffer);
      return -1;
    }
  }
  else {
    err = _FS__fat_LinkCluster(Idx, Unit, &lastsec, curclst, LastClust, buffer, fattype, fatsize, bytespersec);
    if (err < 0) {
      FS__fat_free(buffer);
      return -1;
    }
  }
  
#if (FS_FAT_NOFAT32==0)
  /* Update the FSInfo structure */
  if (fattype == 2) {  /* FAT32 */
    /* Modify FSInfo */
    err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, FS__FAT_aBPBUnit[Idx][Unit].FSInfo, (void*)buffer);
    if (err < 0) {
      FS__fat_free(buffer);
      return -1;
    }
    /* Check for FSInfo structure in buffer */
    if (buffer[0] == (unsigned char)0x52) {
      if (buffer[1] == (unsigned char)0x52) {
        if (buffer[2] == (unsigned char)0x61) {
          if (buffer[3] == (unsigned char)0x41) {
            if (buffer[484] == (unsigned char)0x72) {
              if (buffer[485] == (unsigned char)0x72) {
                if (buffer[486] == (unsigned char)0x41) {
                  if (buffer[487] == (unsigned char)0x61) {
                    if (buffer[508] == (unsigned char)0x00) {
                      if (buffer[509] == (unsigned char)0x00) {
                        if (buffer[510] == (unsigned char)0x55) {
                          if (buffer[511] == (unsigned char)0xaa) {
                            /* Invalidate last known free cluster count */
                            buffer[488] = (unsigned char)0xff;
                            buffer[489] = (unsigned char)0xff;
                            buffer[490] = (unsigned char)0xff;
                            buffer[491] = (unsigned char)0xff;
                            /* Give hint for free cluster search */
                            buffer[492] = curclst & 0xff;
                            buffer[493] = (curclst / 0x100L) & 0xff;
                            buffer[494] = (curclst / 0x10000L) & 0xff;
                            buffer[495] = (curclst / 0x1000000L) & 0x0f;
                            err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, FS__FAT_aBPBUnit[Idx][Unit].FSInfo, (void*)buffer);
                            if (err < 0) {
                              FS__fat_free(buffer);
                              return -1;
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    } /* buffer contains FSInfo structure */
  } /* FS_Info modification for fattype==2 */
#endif /* FS_FAT_NOFAT32==0 */
  FS__fat_free(buffer);
  return curclst;
}


/*********************************************************************
*
*             FS__fat_diskclust
*
  Description:
  FS internal function. Walk through the FAT starting at StrtClst for
  ClstNum times. Return the found cluster number of the media. This is
  very similar to FS__fat_FAT_find_eof.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  StrtClst    - Starting point for FAT walk.
  ClstNum     - Number of steps.
 
  Return value:
  > 0         - Number of cluster found after ClstNum steps.
  ==0         - An error has occured.
*/

FS_i32 FS__fat_diskclust(int Idx, FS_u32 Unit, FS_i32 StrtClst, FS_i32 ClstNum) {
  FS_u32 fatsize;
  FS_i32 fatindex;
  FS_i32 fatsec;
  FS_i32 fatoffs;
  FS_i32 lastsec;
  FS_i32 curclst;
  FS_i32 todo;
  FS_i32 bytespersec;
  int err;
  int fattype;
  char *buffer;
  unsigned char a;
  unsigned char b;
#if (FS_FAT_NOFAT32==0)
  unsigned char c;
  unsigned char d;
#endif /* FS_FAT_NOFAT32==0 */

  fattype = FS__fat_which_type(Idx, Unit);
#if (FS_FAT_NOFAT32!=0)
  if (fattype == 2) {
    return 0;
  }
#endif  /* FS_FAT_NOFAT32!=0 */
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return 0;
  }
  fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
  if (fatsize == 0) {
    fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
  }
  bytespersec = (FS_i32)FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec;
  todo        = ClstNum;
  curclst     = StrtClst;
  lastsec     = -1;
  while (todo) {
    if (fattype == 1) {
      fatindex = curclst + (curclst / 2);    /* FAT12 */
    }
#if (FS_FAT_NOFAT32==0)
    else if (fattype == 2) {
      fatindex = curclst * 4;               /* FAT32 */
    }
#endif /* FS_FAT_NOFAT32==0 */
    else {
      fatindex = curclst * 2;               /* FAT16 */
    }
    fatsec  = FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + (fatindex / bytespersec);
    fatoffs = fatindex % bytespersec;
    if (fatsec != lastsec) {
      err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
      if (err < 0) {
        err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
        if (err < 0) {
          FS__fat_free(buffer);
          return 0;
        }
        /* Try to repair original FAT sector with contents of copy */
        FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
      }
      lastsec = fatsec;
    }
    if (fattype == 1) {
      if (fatoffs == (bytespersec - 1)) {
        a = buffer[fatoffs];
        err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)buffer);
        if (err < 0) {
          err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec + 1, (void*)buffer);
          if (err < 0) {
            FS__fat_free(buffer);
            return 0;
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
      if (curclst & 1) {
        curclst = ((a & 0xf0) >> 4) + 16 * b;
      }
      else {
        curclst = a + 256 * (b & 0x0f);
      }
      curclst &= 0x0fffL;
      if (curclst >= 0x0ff8L) {
        FS__fat_free(buffer);
        return 0;
      }
    }
#if (FS_FAT_NOFAT32==0)
    else if (fattype == 2) {
      a = buffer[fatoffs];
      b = buffer[fatoffs + 1];
      c = buffer[fatoffs + 2];
      d = buffer[fatoffs + 3];
      curclst = a + 0x100L * b + 0x10000L * c + 0x1000000L * d;
      curclst &= 0x0fffffffL;
      if (curclst >= (FS_i32)0x0ffffff8L) {
        FS__fat_free(buffer);
        return 0;
      }
    }
#endif /* FS_FAT_NOFAT32==0 */
    else {
      a = buffer[fatoffs];
      b = buffer[fatoffs + 1];
      curclst  = a + 256 * b;
      curclst &= 0xffffL;
      if (curclst >= (FS_i32)0xfff8L) {
        FS__fat_free(buffer);
        return 0;
      }
    }
    todo--;
  }
  FS__fat_free(buffer);
  return curclst;
}


/*********************************************************************
*
*             Global Variables
*
**********************************************************************
*/

const FS__fsl_type FS__fat_functable = {
#if (FS_FAT_NOFAT32==0)
  "FAT12/FAT16/FAT32",
#else
  "FAT12/FAT16",
#endif /* FS_FAT_NOFAT32==0 */
  FS__fat_fopen,        /* open  */
  FS__fat_fclose,       /* close */
  FS__fat_fread,        /* read  */
  FS__fat_fread_block,        /* read block */
  FS__fat_fwrite,       /* write */
  0,                    /* tell  */
  0,                    /* seek  */
  FS__fat_ioctl,        /* ioctl */
#if FS_POSIX_DIR_SUPPORT
  FS__fat_opendir,      /* opendir   */
  FS__fat_closedir,     /* closedir  */
  FS__fat_readdir,      /* readdir   */
  FS__fat_readdir_back,      /* readdir_back   */
  0,                    /* rewinddir */
  FS__fat_MkRmDir,      /* mkdir     */
  FS__fat_MkRmDir,      /* rmdir     */
#endif  /* FS_POSIX_DIR_SUPPORT */
};

