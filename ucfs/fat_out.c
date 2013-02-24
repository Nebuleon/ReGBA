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
File        : fat_out.c
Purpose     : FAT12/FAT16/FAT32 Filesystem file write routines
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
#ifndef FS_FARCHARPTR
  #define FS_FARCHARPTR char *
#endif
#ifndef FS_FAT_FWRITE_UPDATE_DIR
  #define FS_FAT_FWRITE_UPDATE_DIR 1
#endif
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
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_fat_write_dentry
*
  Description:
  FS internal function. Write a directory entry.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  FirstClust  - First cluster of the file, which's directory entry 
                will be written.
  pDirEntry   - Pointer to an FS__fat_dentry_type structure, which 
                contains the new directory entry.
  DirSec      - Sector, which contains the directory entry.
  pBuffer     - Pointer to a buffer, which contains the sector with 
                the old directory entry.
 
  Return value:
  ==1         - Directory entry has been written.
  ==0         - An error has occured.
*/

static int _FS_fat_write_dentry(int Idx, FS_u32 Unit, FS_u32 FirstClust, FS__fat_dentry_type *pDirEntry, 
                                FS_u32 DirSec, char *pBuffer) {
  FS__fat_dentry_type *s;
  FS_u32 value;
  int err;

  if (DirSec == 0) {
    return 0;  /* Not a valid directory sector */
  }
  if (pBuffer == 0) {
    return 0;  /* No buffer */
  }
  /* Scan for the directory entry with FirstClust in the directory sector */
  s = (FS__fat_dentry_type*)pBuffer;
  while (1) {
    if (s >= (FS__fat_dentry_type*)(pBuffer + FS_FAT_SEC_SIZE)) {
      break;  /* End of sector reached */
    }
    value = (FS_u32)s->data[26] + 0x100UL * s->data[27] + 0x10000UL * s->data[20] + 0x1000000UL * s->data[21];
    if (value == FirstClust) {
      break;  /* Entry found */
    }
    s++;
  }
  if (s < (FS__fat_dentry_type*)(pBuffer + FS_FAT_SEC_SIZE)) {
    if (pDirEntry) {
      FS__CLIB_memcpy(s, pDirEntry, sizeof(FS__fat_dentry_type));
      err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, DirSec, (void*)pBuffer);
      
      if (err < 0) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}


/*********************************************************************
*
*             _FS_fat_read_dentry
*
  Description:
  FS internal function. Read a directory entry.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  FirstClust  - First cluster of the file, which's directory entry 
                will be read.
  DirStart    - Start of directory, where to read the entry.
  pDirEntry   - Pointer to an FS__fat_dentry_type structure, which is 
                used to read the directory entry.
  pDirSec     - Pointer to an FS_u32, which is used to store the sector
                number, in which the directory entry has been read.
  pBuffer     - Pointer to a buffer, which is used for reading the
                directory.
 
  Return value:
  ==1         - Directory entry has been read.
  ==0         - An error has occured.
*/

static int _FS_fat_read_dentry(int Idx, FS_u32 Unit, FS_u32 FirstClust, 
                              FS_u32 DirStart, FS__fat_dentry_type *pDirEntry, FS_u32 *pDirSec, char *pBuffer) {
  FS_u32 i;
  FS_u32 dsize;
  FS_u32 value;
  FS__fat_dentry_type *s;
  int err;

  if (pBuffer == 0) {
    return 0;
  }
  dsize  =  FS__fat_dir_size(Idx, Unit, DirStart);
  /* Read the directory */
  for (i = 0; i < dsize; i++) {
    *pDirSec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
    if (*pDirSec == 0) {
      return 0;  /* Unable to translate relative directory sector to absolute setor */
    }
    err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, *pDirSec, (void*)pBuffer);
    if (err < 0) {
      return 0;
    }
    /* Scan for entry with FirstClus in the sector */
    s = (FS__fat_dentry_type*)pBuffer;
    while (1) {
      if (s >= (FS__fat_dentry_type*)(pBuffer + FS_FAT_SEC_SIZE)) {
        break;  /* End of sector reached */
      }
      value = (FS_u32)s->data[26] + 0x100UL * s->data[27] + 0x10000UL * s->data[20] + 0x1000000UL * s->data[21];
      if (value == FirstClust) {
        break;  /* Entry found */
      }
      s++;
    }
    if (s < (FS__fat_dentry_type*)(pBuffer + FS_FAT_SEC_SIZE)) {
      if (pDirEntry) {
        /* Read the complete directory entry from the buffer */
        FS__CLIB_memcpy(pDirEntry, s, sizeof(FS__fat_dentry_type));
      }
      return 1;
    }
  }
  return 0;
}


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__fat_fwrite
*
  Description:
  FS internal function. Write data to a file.

  Parameters:
  pData       - Pointer to data, which will be written to the file. 
  Size        - Size of an element to be transferred to a file.
  N           - Number of elements to be transferred to the file.
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  Number of elements written.
*/
#if 1
FS_size_t FS__fat_fwrite(const void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile) {
    FS_size_t todo;
    FS_u32 dstart;
    FS_u32 dsize;
    FS_u32 bytesperclus;
    FS_u32 datastart;
    FS_u32 fatsize;
    FS_u32 fileclustnum;
    FS_u32 diskclustnum;
    FS_u32 prevclust;
    FS_i32 last;
    FS_i32 i;
    FS_i32 j, k;
#if (FS_FAT_FWRITE_UPDATE_DIR)
    FS__fat_dentry_type s;
    FS_u32 dsec;
    FS_u16 val;
#endif /* FS_FAT_FWRITE_UPDATE_DIR */
    int err;
    int lexp;
    unsigned char *buffer;

    if (!pFile) {
        return 0;
    }
    /* Check if media is OK */
    err = FS__lb_status(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo);
    if (err == FS_LBL_MEDIACHANGED) {
        pFile->error = FS_ERR_DISKCHANGED;
        return 0;
    }
    else if (err < 0) {        pFile->error = FS_ERR_WRITEERROR;
        return 0;
    }
    buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
    if (!buffer) {
        return 0;
    }

    fatsize = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].FATSz16;
    if (fatsize == 0) {
        /* FAT32 */
        fatsize = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].FATSz32;
    }
    todo = N * Size;  /* Number of bytes to be written */
    if (!todo) {
        FS__fat_free(buffer);
        return 0;
    }

    /* Alloc new clusters if required */
    bytesperclus = (FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus *
        ((FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].BytesPerSec);
    /* Calculate number of clusters required */    i = (pFile->filepos + todo) / bytesperclus;
    if ((pFile->filepos + todo) % bytesperclus) {
        i++;
    }
    /* Calculate clusters already allocated */
    j = pFile->size / bytesperclus;
    lexp = (pFile->size % bytesperclus);
    lexp = lexp || (pFile->size == 0);
    if (lexp) {
        j++;
    }

    i -= j;
    if (i > 0)
    {
        /* Alloc new clusters */
        last = pFile->EOFClust;        if (last < 0) {
            /* Position of EOF is unknown, so we scan the whole file to find it */
            last = FS__fat_FAT_find_eof(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, 0);
            if (last < 0) {
                /* No EOF found */
                FS__fat_free(buffer);                return 0;
            }
        }

        while (i)
        {
            last = FS__fat_FAT_alloc(pFile->dev_index, pFile->fileid_lo, last);  /* Allocate new cluster */
            pFile->EOFClust = last;
            if (last < 0) {
                /* Cluster allocation failed */
                pFile->size += (N * Size - todo);                pFile->error = FS_ERR_DISKFULL;
                FS__fat_free(buffer);
                return ((N * Size - todo) / Size);
            }
            i--;
        }
    }

    /* Get absolute postion of data area on the media */
    dstart = (FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RsvdSecCnt +               FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].NumFATs * fatsize;
    dsize = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RootEntCnt) * FS_FAT_DENTRY_SIZE) / FS_FAT_SEC_SIZE;
    datastart = dstart + dsize;
    fileclustnum = pFile->filepos / bytesperclus;
    diskclustnum = pFile->CurClust;
    if (diskclustnum == 0) {        /* No known current cluster position, we have to scan from the file's start cluster */
        diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, fileclustnum);
    }
    if (diskclustnum == 0) {        /* Translation to absolute cluster failed */        pFile->error = FS_ERR_WRITEERROR;        FS__fat_free(buffer);        return ((N * Size - todo) / Size);    }

    prevclust        = diskclustnum;
    pFile->CurClust  = diskclustnum;
    diskclustnum -= 2;
    
    j = (pFile->filepos % bytesperclus) / FS_FAT_SEC_SIZE;
    i = pFile->filepos % FS_FAT_SEC_SIZE;
    /* Write the first part which less than a sector */
    if (i > 0)
    {
        err = FS__lb_read(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,
            datastart + diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
            (void*)buffer);
        if (err < 0) {
            pFile->error = FS_ERR_WRITEERROR;
            FS__fat_free(buffer);
            return ((N * Size - todo) / Size);        }

        k = FS_FAT_SEC_SIZE - i;
        if(k > todo) k = todo;
        FS__CLIB_memcpy(&buffer[i], (char *)((unsigned int)pData + N * Size - todo), k);

        err = FS__lb_write(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,            datastart + diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
            (void*)buffer);        if (err < 0) {
            pFile->error = FS_ERR_WRITEERROR;            FS__fat_free(buffer);
            return ((N * Size - todo) / Size);
        }

        pFile->filepos += k;
        if (pFile->filepos > pFile->size) {
            pFile->size = pFile->filepos;
        }
        todo -= k;
        j += 1;
    }
    /* Remainder more than a sector */
    if(todo > FS_FAT_SEC_SIZE)
    {
        i = todo / FS_FAT_SEC_SIZE;
        while(i)
        {
            if (j >= FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
                diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
                if (diskclustnum == 0) {                    /* Translation to absolute cluster failed */                    pFile->error = FS_ERR_WRITEERROR;                    FS__fat_free(buffer);                    return ((N * Size - todo) / Size);                }
                prevclust        = diskclustnum;
                pFile->CurClust  = diskclustnum;
                diskclustnum -= 2;
                j = 0;
            }

            k = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus - j;
            if(k > i) k= i;

            err = FS__lb_multi_write(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,
                datastart + diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
                k, (void*)((unsigned int)pData + N * Size - todo));
            if (err < 0) {
                pFile->error = FS_ERR_WRITEERROR;                FS__fat_free(buffer);
                return ((N * Size - todo) / Size);
            }
            pFile->filepos += FS_FAT_SEC_SIZE *k;
            if (pFile->filepos > pFile->size) {
                pFile->size = pFile->filepos;
            }
            todo -= FS_FAT_SEC_SIZE *k;
            j += k;
/*
            FS_i32 m;
            m= k;
            while(m--)
            {
                err = FS__lb_write(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,                    datastart + diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
                    (void*)((unsigned int)pData + N * Size - todo));                if (err < 0) {
                    pFile->error = FS_ERR_WRITEERROR;                    FS__fat_free(buffer);
                    return ((N * Size - todo) / Size);
                }

                pFile->filepos += FS_FAT_SEC_SIZE;
                if (pFile->filepos > pFile->size) {
                    pFile->size = pFile->filepos;
                }
                todo -= FS_FAT_SEC_SIZE;
                j += 1;
            }
*/
            i -= k;
        }
    }
    /* Remainder less than a sector */
    if(todo > 0)
    {
        if (j >= FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
            diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
            if (diskclustnum == 0) {                /* Translation to absolute cluster failed */                pFile->error = FS_ERR_WRITEERROR;                FS__fat_free(buffer);                return ((N * Size - todo) / Size);            }
            prevclust        = diskclustnum;
            pFile->CurClust  = diskclustnum;
            diskclustnum -= 2;
            j = 0;
        }

        err = FS__lb_read(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,
            datastart + diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
            (void*)buffer);
        if (err < 0) {
            pFile->error = FS_ERR_WRITEERROR;
            FS__fat_free(buffer);
            return ((N * Size - todo) / Size);        }

        k = todo;
        FS__CLIB_memcpy((char*)buffer, (char *)((unsigned int)pData + N * Size - todo), k);

        err = FS__lb_write(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,            datastart + diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
            (void*)buffer);        if (err < 0) {
            pFile->error = FS_ERR_WRITEERROR;            FS__fat_free(buffer);
            return ((N * Size - todo) / Size);
        }

        pFile->filepos += k;
        if (pFile->filepos > pFile->size) {
            pFile->size = pFile->filepos;
        }
        todo -= k;
        j += 1;
    }

    i = pFile->filepos % FS_FAT_SEC_SIZE;
    if (i == 0) {
        if (j >= FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
            /* File pointer is already in the next cluster */
            pFile->CurClust = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
        }
    }
#if (FS_FAT_FWRITE_UPDATE_DIR)
    /* Modify directory entry */
    err = _FS_fat_read_dentry(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, pFile->fileid_ex, &s, &dsec, buffer);    if (err == 0) {
        pFile->error = FS_ERR_WRITEERROR;
        FS__fat_free(buffer);        return ((N * Size - todo) / Size);
    }
    
    s.data[28] = (unsigned char)(pFile->size & 0xff);   /* FileSize */    s.data[29] = (unsigned char)((pFile->size / 0x100UL) & 0xff);
    s.data[30] = (unsigned char)((pFile->size / 0x10000UL) & 0xff);
    s.data[31] = (unsigned char)((pFile->size / 0x1000000UL) & 0xff);
    val = FS_X_OS_GetTime();
    s.data[22] = (unsigned char)(val & 0xff);
    s.data[23] = (unsigned char)((val / 0x100) & 0xff);
    val = FS_X_OS_GetDate();
    s.data[24] = (unsigned char)(val & 0xff);
    s.data[25] = (unsigned char)((val / 0x100) & 0xff);
    err = _FS_fat_write_dentry(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, &s, dsec, buffer);
    if (err == 0) {
        pFile->error = FS_ERR_WRITEERROR;    }
#endif /* FS_FAT_FWRITE_UPDATE_DIR */

    FS__fat_free(buffer);    return ((N * Size - todo) / Size);
}
#endif

#if 0
FS_size_t FS__fat_fwrite(const void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile) {
  FS_size_t todo;
  FS_u32 dstart;
  FS_u32 dsize;
  FS_u32 bytesperclus;
  FS_u32 datastart;
  FS_u32 fatsize;
  FS_u32 fileclustnum;
  FS_u32 diskclustnum;
  FS_u32 prevclust;
  FS_i32 last;
  FS_i32 i;
  FS_i32 j;
#if (FS_FAT_FWRITE_UPDATE_DIR)
  FS__fat_dentry_type s;
  FS_u32 dsec;
  FS_u16 val;
#endif /* FS_FAT_FWRITE_UPDATE_DIR */
  int err;
  int lexp;
  unsigned char *buffer;

  if (!pFile) {
      return 0;
  }
  /* Check if media is OK */
  err = FS__lb_status(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo);
  if (err == FS_LBL_MEDIACHANGED) {
    pFile->error = FS_ERR_DISKCHANGED;
    return 0;
  }
  else if (err < 0) {
    pFile->error = FS_ERR_WRITEERROR;
    return 0;
  }
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return 0;
  }
  fatsize = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].FATSz16;
  if (fatsize == 0) {
    /* FAT32 */
    fatsize = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].FATSz32;
  }
  todo = N * Size;  /* Number of bytes to be written */
  if (!todo) {
    FS__fat_free(buffer);
    return 0;
  }
  /* Alloc new clusters if required */
  bytesperclus = (FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus *
                 ((FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].BytesPerSec);
  /* Calculate number of clusters required */
  i = (pFile->filepos + todo) / bytesperclus;
  if ((pFile->filepos + todo) % bytesperclus) {
    i++;
  }
  /* Calculate clusters already allocated */
  j = pFile->size / bytesperclus;
  lexp = (pFile->size % bytesperclus);
  lexp = lexp || (pFile->size == 0);
  if (lexp) {
    j++;
  }
  i -= j;
  if (i > 0) {
    /* Alloc new clusters */
    last = pFile->EOFClust;
    if (last < 0) {
      /* Position of EOF is unknown, so we scan the whole file to find it */
      last = FS__fat_FAT_find_eof(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, 0);
    }
    if (last < 0) {
      /* No EOF found */
      FS__fat_free(buffer);
      return 0;
    }
    while (i) {
      last = FS__fat_FAT_alloc(pFile->dev_index, pFile->fileid_lo, last);  /* Allocate new cluster */
      pFile->EOFClust = last;
      if (last < 0) {
        /* Cluster allocation failed */
        pFile->size += (N * Size - todo);
        pFile->error = FS_ERR_DISKFULL;
        FS__fat_free(buffer);
        return ((N * Size - todo) / Size);
      }
      i--;
    }
  }
  /* Get absolute postion of data area on the media */
  dstart    = (FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RsvdSecCnt + 
              FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].NumFATs * fatsize;
  dsize     = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RootEntCnt) * FS_FAT_DENTRY_SIZE) / FS_FAT_SEC_SIZE;
  datastart = dstart + dsize;
  /* Write data to clusters */
  prevclust = 0;
  while (todo) {  /* Write data loop */
    /* Translate file ppinter position to cluster position*/
    fileclustnum = pFile->filepos / bytesperclus;
    /* 
       Translate the file relative cluster position to an absolute cluster
       position on the media. To avoid scanning the whole FAT of the file,
       we remember the current cluster position in the FS_FILE data structure.
    */
    if (prevclust == 0) {
      diskclustnum = pFile->CurClust;
      if (diskclustnum == 0) {
        /* No known current cluster position, we have to scan from the file's start cluster */
        diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, fileclustnum);
      }
    } 
    else {
      /* Get next cluster of the file starting at the current cluster */
      diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
    }
    prevclust        = diskclustnum;
    pFile->CurClust  = diskclustnum;
    if (diskclustnum == 0) {
      /* Translation to absolute cluster failed */
      pFile->error = FS_ERR_WRITEERROR;
      FS__fat_free(buffer);
      return ((N * Size - todo) / Size);
    }
    diskclustnum -= 2;
    j = (pFile->filepos % bytesperclus) / FS_FAT_SEC_SIZE;
    while (1) {  /* Cluster loop */
      if (!todo) {
        break;  /* Nothing more to write */
      }
      if (j >= FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
        break; /* End of cluster reached */
      }
      i = pFile->filepos % FS_FAT_SEC_SIZE;
      /* 
         We only have to read the sector from the media, if we do not
         modify the whole sector. That is the case if

         a) Writing starts not at the first byte of the sector
         b) Less data than the sector contains is written
      */
      lexp = (i != 0);
      lexp = lexp || (todo < FS_FAT_SEC_SIZE);
      if (lexp) {
        /* We have to read the old sector */
        err = FS__lb_read(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,
                      datastart +
                      diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
                      (void*)buffer);
        if (err < 0) {
          pFile->error = FS_ERR_WRITEERROR;
          FS__fat_free(buffer);
          return ((N * Size - todo) / Size);
        }
      }
#if 1
      if(FS_FAT_SEC_SIZE - i> todo)
      {
	      FS__CLIB_memcpy(&buffer[i],(char *)((unsigned int)pData + N * Size - todo),todo);
	      pFile->filepos += todo;
	      if (pFile->filepos > pFile->size) {
		      pFile->size = pFile->filepos;
	      }
	      todo = 0;
      }
      else
      {
      	
	      FS__CLIB_memcpy(&buffer[i],(char *)((unsigned int)pData +  N * Size - todo),FS_FAT_SEC_SIZE - i);
	      pFile->filepos += (FS_FAT_SEC_SIZE - i);
	      if (pFile->filepos > pFile->size) {
		      pFile->size = pFile->filepos;
	      }
	      todo -= (FS_FAT_SEC_SIZE - i);
	      i = FS_FAT_SEC_SIZE;
      }
#endif
#if 0  //old write
      while (1) {  /* Sector loop */
        if (!todo) {
          break;  /* Nothing more to write */
        }
        if (i >= FS_FAT_SEC_SIZE) {
          break;  /* End of sector reached */
        }
        buffer[i] = *((FS_FARCHARPTR)(((FS_FARCHARPTR)pData) + N * Size - todo));
        i++;
        pFile->filepos++;
        if (pFile->filepos > pFile->size) {
          pFile->size = pFile->filepos;
        }
        todo--;
      }  /* Sector loop */
#endif
      /* Write the modified sector */
      //printf("write data %x sec\r\n",datastart +
      //              diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j);
      err = FS__lb_write(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,
                    datastart +
                    diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
                    (void*)buffer);
      if (err < 0) {
        pFile->error = FS_ERR_WRITEERROR;
        FS__fat_free(buffer);
        return ((N * Size - todo) / Size);
      }
      j++;
    }  /* Cluster loop */
  } /* Write data loop */
  if (i >= FS_FAT_SEC_SIZE) {
    if (j >= FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
      /* File pointer is already in the next cluster */
      pFile->CurClust = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
    }
  }
#if (FS_FAT_FWRITE_UPDATE_DIR)
  /* Modify directory entry */
  err = _FS_fat_read_dentry(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, pFile->fileid_ex, &s, &dsec, buffer);
  if (err == 0) {
    pFile->error = FS_ERR_WRITEERROR;
    FS__fat_free(buffer);
    return ((N * Size - todo) / Size);
  }
  s.data[28] = (unsigned char)(pFile->size & 0xff);   /* FileSize */
  s.data[29] = (unsigned char)((pFile->size / 0x100UL) & 0xff);   
  s.data[30] = (unsigned char)((pFile->size / 0x10000UL) & 0xff);
  s.data[31] = (unsigned char)((pFile->size / 0x1000000UL) & 0xff);
  val = FS_X_OS_GetTime();
  s.data[22] = (unsigned char)(val & 0xff);
  s.data[23] = (unsigned char)((val / 0x100) & 0xff);
  val = FS_X_OS_GetDate();
  s.data[24] = (unsigned char)(val & 0xff);
  s.data[25] = (unsigned char)((val / 0x100) & 0xff);
  err = _FS_fat_write_dentry(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, &s, dsec, buffer);
  if (err == 0) {
    pFile->error = FS_ERR_WRITEERROR;
  }
#endif /* FS_FAT_FWRITE_UPDATE_DIR */
  FS__fat_free(buffer);
  return ((N * Size - todo) / Size);
}
#endif

/*********************************************************************
*
*             FS__fat_fclose
*
  Description:
  FS internal function. Close a file referred by pFile.

  Parameters:
  pFile       - Pointer to a FS_FILE data structure. 
  
  Return value:
  None.
*/

void FS__fat_fclose(FS_FILE *pFile) {
#if (FS_FAT_FWRITE_UPDATE_DIR==0)
  FS__fat_dentry_type s;
  char *buffer;
  FS_u32 dsec;
  FS_u16 val;
#endif /* FS_FAT_FWRITE_UPDATE_DIR */
  int err;

  if (!pFile) {
      return;
  }
  /* Check if media is OK */
  err = FS__lb_status(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo);
  if (err == FS_LBL_MEDIACHANGED) {
    pFile->error = FS_ERR_DISKCHANGED;
    FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
    pFile->inuse = 0;
    return;
  }
  else if (err < 0) {
    pFile->error = FS_ERR_CLOSE;
    FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
    pFile->inuse = 0;
    return;
  }
#if (FS_FAT_FWRITE_UPDATE_DIR==0)
  /* Modify directory entry */
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    pFile->inuse = 0;
    pFile->error = FS_ERR_CLOSE;
    return;
  }
  err = _FS_fat_read_dentry(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, pFile->fileid_ex, &s, &dsec, buffer);
  if (err == 0) {
    pFile->inuse = 0;
    pFile->error = FS_ERR_CLOSE;
    FS__fat_free(buffer);
    return;
  }
  s.data[28] = (unsigned char)(pFile->size & 0xff);   /* FileSize */
  s.data[29] = (unsigned char)((pFile->size / 0x100UL) & 0xff);   
  s.data[30] = (unsigned char)((pFile->size / 0x10000UL) & 0xff);
  s.data[31] = (unsigned char)((pFile->size / 0x1000000UL) & 0xff);
  val = FS_X_OS_GetTime();
  s.data[22] = (unsigned char)(val & 0xff);
  s.data[23] = (unsigned char)((val / 0x100) & 0xff);
  val = FS_X_OS_GetDate();
  s.data[24] = (unsigned char)(val & 0xff);
  s.data[25] = (unsigned char)((val / 0x100) & 0xff);
  err = _FS_fat_write_dentry(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, &s, dsec, buffer);
  if (err == 0) {
    pFile->error = FS_ERR_CLOSE;
  }
  FS__fat_free(buffer);
#endif /* FS_FAT_FWRITE_UPDATE_DIR */
  err = FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo, FS_CMD_FLUSH_CACHE, 2, (void*)0);
  if (err < 0) {
    pFile->error = FS_ERR_WRITEERROR;
  }
  pFile->inuse = 0;
  FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
}


