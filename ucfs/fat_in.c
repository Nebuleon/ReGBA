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
File        : fat_in.c
Purpose     : FAT read routines
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


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__fat_fread
*
  Description:
  FS internal function. Read data from a file.

  Parameters:
  pData       - Pointer to a data buffer for storing data transferred
                from file. 
  Size        - Size of an element to be transferred from file to data
                buffer
  N           - Number of elements to be transferred from the file.
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  Number of elements read.
*/
#if 1
FS_size_t FS__fat_fread(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile) {
  FS_size_t todo;
  FS_u32 i;
  FS_u32 j;
  FS_u32 fatsize;
  FS_u32 fileclustnum;
  FS_u32 diskclustnum;
  FS_u32 prevclust;
  FS_u32 dstart;
  FS_u32 dsize;
  FS_u32 datastart;
  char *buffer;
  int err;
                                        
  if (!pFile) {
      return 0;  /* No valid pointer to a FS_FILE structure */
  }
  /* Check if media is OK */
  err = FS__lb_status(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo);
  if (err == FS_LBL_MEDIACHANGED) {
    /* Media has changed */
    pFile->error = FS_ERR_DISKCHANGED;
    return 0;
  }
  else if (err < 0) {
    /* Media cannot be accessed */
    pFile->error = FS_ERR_READERROR;
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
  dstart    = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RsvdSecCnt + FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].NumFATs * fatsize;
  dsize     = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RootEntCnt) * FS_FAT_DENTRY_SIZE) / FS_FAT_SEC_SIZE;
  datastart = dstart + dsize;
  prevclust = 0;
  todo = N * Size;
  while (todo) 
  {
	  if (pFile->filepos >= pFile->size) {
		  /* EOF has been reached */
		  pFile->error = FS_ERR_EOF;
		  FS__fat_free(buffer);
		  return ((N * Size - todo) / Size);
	  }
	  fileclustnum = pFile->filepos / (FS_FAT_SEC_SIZE * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus);
	  if (prevclust == 0) {
		  diskclustnum = pFile->CurClust; 
		  if (diskclustnum == 0) {
			  /* Find current cluster by starting at 1st cluster of the file */
			  diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, fileclustnum);
		  }
	  }
	  else {
		  /* Get next cluster of the file */
		  diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
	  }
	  prevclust       = diskclustnum;
	  pFile->CurClust = diskclustnum;
	  if (diskclustnum == 0) {
		  /* Could not find current cluster */
		  pFile->error = FS_ERR_READERROR;
		  FS__fat_free(buffer);
		  return ((N * Size - todo) / Size);
	  }
	  diskclustnum -= 2;
	  j = (pFile->filepos % (FS_FAT_SEC_SIZE * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus))/ FS_FAT_SEC_SIZE;
		
	  while (1) 
	  {
		  if (!todo) {
			  break;  /* Nothing more to write */
		  }
		  if (j >= (FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
			  break;  /* End of the cluster reached */
		  }
		  if (pFile->filepos >= pFile->size) {
			  break;  /* End of the file reached */
		  }
		  err = FS__lb_read(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,
				    datastart +
				    diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
				    (void*)buffer);
		  if (err < 0) {
			  pFile->error = FS_ERR_READERROR;
			  FS__fat_free(buffer);
			  return ((N * Size - todo) / Size);
		  }
		  i = pFile->filepos % FS_FAT_SEC_SIZE;
		  if(FS_FAT_SEC_SIZE-i > todo)
		  {
		  	FS__CLIB_memcpy((char *)((unsigned int)pData + N * Size - todo), &buffer[i],todo);
			  pFile->filepos += todo;
			  todo = 0;
			  i += todo;
		  }
		  else
		  {
		  	FS__CLIB_memcpy((char *)((unsigned int)pData +  N * Size - todo), &buffer[i],FS_FAT_SEC_SIZE-i);
			  pFile->filepos += (FS_FAT_SEC_SIZE-i);
			  todo -= (FS_FAT_SEC_SIZE-i);
			  i = FS_FAT_SEC_SIZE;
		  }

		  j++;
	  }  /* Sector loop */
  }  /* Cluster loop */
  if (i >= FS_FAT_SEC_SIZE) {
	  if (j >= FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
		  pFile->CurClust = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
	  }
  }
  FS__fat_free(buffer);
  return ((N * Size - todo) / Size);
}


#endif
#if 0
FS_size_t FS__fat_fread(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile) {
  FS_size_t todo;
  FS_u32 i;
  FS_u32 j;
  FS_u32 fatsize;
  FS_u32 fileclustnum;
  FS_u32 diskclustnum;
  FS_u32 prevclust;
  FS_u32 dstart;
  FS_u32 dsize;
  FS_u32 datastart;
  char *buffer;
  int err;
                                            
  if (!pFile) {
      return 0;  /* No valid pointer to a FS_FILE structure */
  }
  /* Check if media is OK */
  err = FS__lb_status(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo);
  if (err == FS_LBL_MEDIACHANGED) {
    /* Media has changed */
    pFile->error = FS_ERR_DISKCHANGED;
    return 0;
  }
  else if (err < 0) {
    /* Media cannot be accessed */
    pFile->error = FS_ERR_READERROR;
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
  dstart    = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RsvdSecCnt + FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].NumFATs * fatsize;
  dsize     = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RootEntCnt) * FS_FAT_DENTRY_SIZE) / FS_FAT_SEC_SIZE;
  datastart = dstart + dsize;
  prevclust = 0;
  todo = N * Size;
  while (todo) 
  {
	  if (pFile->filepos >= pFile->size) {
		  /* EOF has been reached */
		  pFile->error = FS_ERR_EOF;
		  FS__fat_free(buffer);
		  return ((N * Size - todo) / Size);
	  }
	  fileclustnum = pFile->filepos / (FS_FAT_SEC_SIZE * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus);
	  if (prevclust == 0) {
		  diskclustnum = pFile->CurClust; 
		  if (diskclustnum == 0) {
			  /* Find current cluster by starting at 1st cluster of the file */
			  diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, fileclustnum);
		  }
	  }
	  else {
		  /* Get next cluster of the file */
		  diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
	  }
	  prevclust       = diskclustnum;
	  pFile->CurClust = diskclustnum;
	  if (diskclustnum == 0) {
		  /* Could not find current cluster */
		  pFile->error = FS_ERR_READERROR;
		  FS__fat_free(buffer);
		  return ((N * Size - todo) / Size);
	  }
	  diskclustnum -= 2;
	  j = (pFile->filepos % (FS_FAT_SEC_SIZE * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus))/ FS_FAT_SEC_SIZE;

	  while (1) 
	  {
		  if (!todo) {
			  break;  /* Nothing more to write */
		  }
		  if (j >= (FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
			  break;  /* End of the cluster reached */
		  }
		  if (pFile->filepos >= pFile->size) {
			  break;  /* End of the file reached */
		  }
		  err = FS__lb_read(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo,
				    datastart +
				    diskclustnum * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus + j,
				    (void*)buffer);
		  if (err < 0) {
			  pFile->error = FS_ERR_READERROR;
			  FS__fat_free(buffer);
			  return ((N * Size - todo) / Size);
		  }
		  i = pFile->filepos % FS_FAT_SEC_SIZE;
		  while (1) 
		  {
			  if (!todo) {
				  break;  /* Nothing more to write */
			  }
			  if (i >= FS_FAT_SEC_SIZE) {
				  break;  /* End of the sector reached */
			  }
			  if (pFile->filepos >= pFile->size) {
				  break;  /* End of the file reached */
			  }
			  *((char*)(((char*)pData) + N * Size - todo)) = buffer[i];
			  i++;
			  pFile->filepos++;
			  todo--;
		  }
		  j++;
	  }  /* Sector loop */
  }  /* Cluster loop */
  if (i >= FS_FAT_SEC_SIZE) {
	  if (j >= FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus) {
		  pFile->CurClust = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
	  }
  }
  FS__fat_free(buffer);
  return ((N * Size - todo) / Size);
}
#endif



/*********************************************************************
*
*             FS__fat_integret_read
*
  Description:
  FS internal function. Read multiple integret sectors of data from a file.

  Parameters:
  pData       - Pointer to a data buffer for storing data transferred                from file. 

  sec_num     - Number of sectors to be read from file to buffer

  pFile       - Pointer to a FS_FILE data structure.

  pInfo       - Pointer to FS_READ_INFO struct
  Return value:
              - < 0 when failure
              - >= 0 when success*/
static FS_size_t FS__fat_integret_read(void *pData, FS_size_t sec_num, FS_FILE *pFile)
{
    FS_u32 fatsize;
    FS_u32 dstart;          //Unit: sector
    FS_u32 dsize;           //Unit: sector
    FS_u32 datastart;       //Unit: sector
    FS_u32 clustsize;       //Unit: byte
    FS_u32 prevclust;       //Unit: clustor
    FS_u32 fileclustnum;    //Unit: clustor
    FS_u32 diskclustnum;    //Unit: clustor
    FS_u32 secperclust;     //Unit: sector
    FS_u32 start_sec;       //Unit: sector
    FS_u32 sec_to_read;     //Unit: sector
    FS_size_t todo;         //Unit: sector
    int err;

    fatsize = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].FATSz16;
    if (fatsize == 0) {
        /* FAT32 */
        fatsize = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].FATSz32;
    }
    dstart = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RsvdSecCnt + 
        FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].NumFATs * fatsize;
    dsize = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RootEntCnt) * FS_FAT_DENTRY_SIZE) / FS_FAT_SEC_SIZE;
    secperclust = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus;
    clustsize = FS_FAT_SEC_SIZE * secperclust;
    datastart = dstart + dsize;

    fileclustnum = pFile->filepos / clustsize;
    /* Find current cluster by starting at 1st cluster of the file */
    diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, fileclustnum);
    prevclust = pFile->CurClust;

    start_sec = ((pFile->filepos % clustsize) + FS_FAT_SEC_SIZE -1)/FS_FAT_SEC_SIZE;
    if (start_sec >= secperclust) {
        sec_to_read = 0;
    }
    else {
        sec_to_read = secperclust - start_sec;
    }

    todo= 0;
    while(1)
    {
        if (todo >= sec_num) {
            return (todo *FS_FAT_SEC_SIZE);
        }

        if (sec_to_read == 0) {
            /* Get next cluster of the file */
            diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, prevclust, 1);
    
            if (diskclustnum == 0) {
                /* Could not find current cluster */
                pFile->error = FS_ERR_READERROR;
                return -1;
            }
            prevclust = diskclustnum;

            sec_to_read = secperclust;
            start_sec = 0;
        }

        if (sec_to_read > (sec_num - todo)) {
            sec_to_read = sec_num - todo;
        }

        diskclustnum -= 2;
        if(sec_to_read > 1) {
            err = FS__lb_multi_read(FS__pDevInfo[pFile->dev_index].devdriver, 
                    pFile->fileid_lo,
                    datastart + diskclustnum * secperclust + start_sec,
                    sec_to_read,
                    (char *)((unsigned int)pData + todo *FS_FAT_SEC_SIZE));
        }
        else {
            err = FS__lb_read(FS__pDevInfo[pFile->dev_index].devdriver,
                    pFile->fileid_lo,
				    datastart + diskclustnum * secperclust + start_sec,
				    (char *)((unsigned int)pData + todo *FS_FAT_SEC_SIZE));
        }

        if (err < 0) {
            pFile->error = FS_ERR_READERROR;
            return -1;
        }

        todo += sec_to_read;
        sec_to_read = 0;
    }
}


FS_size_t FS__fat_fread_block(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile) {
    FS_u32 fatsize;
    FS_u32 secperclust;
    FS_u32 fileclustnum;
    FS_u32 diskclustnum;
    FS_u32 dstart;
    FS_u32 dsize;
    FS_u32 datastart;
    int err;
    FS_u32 clustsize;
    char *buffer;
    FS_size_t todo;
    FS_u32 i, j;

    FS_u32 pre_byte_to_read;
    FS_u32 mid_sec_to_read;
    FS_u32 post_sec_to_read;
/*
unsigned char pbuf[64];
unsigned int x;
unsigned char *ppt;
ppt= (unsigned char*)((unsigned int)pData-64);
memcpy(pbuf, ppt, 64);
*/
    if (!pFile) {
        return 0;  /* No valid pointer to a FS_FILE structure */
    }
    /* Check if media is OK */
    err = FS__lb_status(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo);
    if (err == FS_LBL_MEDIACHANGED) {
        /* Media has changed */
        pFile->error = FS_ERR_DISKCHANGED;
        return 0;
    }
    else if (err < 0) {
        /* Media cannot be accessed */
        pFile->error = FS_ERR_READERROR;
        return 0;
    }

    todo = N * Size;
    if (todo > (pFile->size - pFile->filepos)) {
        todo = pFile->size - pFile->filepos;
        pFile->error = FS_ERR_EOF;
    }
    if (todo == 0) return 0;

    buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
    if (!buffer) {
        return 0;
    }

    fatsize = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].FATSz16;
    if (fatsize == 0) {
        /* FAT32 */
        fatsize = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].FATSz32;
    }
    dstart = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RsvdSecCnt + 
        FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].NumFATs * fatsize;
    dsize = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].RootEntCnt) * FS_FAT_DENTRY_SIZE) / FS_FAT_SEC_SIZE;
    secperclust = FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus;
    clustsize = FS_FAT_SEC_SIZE * secperclust;
    datastart = dstart + dsize;

    fileclustnum = pFile->filepos / clustsize;
    diskclustnum = pFile->CurClust; 
    if (diskclustnum == 0) {
        /* Find current cluster by starting at 1st cluster of the file */
        diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, fileclustnum);
    }
    if (diskclustnum == 0) {
        /* Could not find current cluster */
        pFile->error = FS_ERR_READERROR;
        FS__fat_free(buffer);
        return 0;
    }
    pFile->CurClust = diskclustnum;
    diskclustnum -= 2;

    i = pFile->filepos % FS_FAT_SEC_SIZE;
    if (i > 0) {
        j = FS_FAT_SEC_SIZE - i;
        if (j > todo) {
            j = todo;
            pFile->error = FS_ERR_EOF;
        }
        pre_byte_to_read = j;
        mid_sec_to_read = 0;
    }
    else {
        pre_byte_to_read = 0;
        mid_sec_to_read = 1;
    }

    i = pFile->filepos/FS_FAT_SEC_SIZE;
    j = (pFile->filepos + todo)/FS_FAT_SEC_SIZE;
    if (j > i) {
        // Start and end not on the same sector
        mid_sec_to_read += j - i -1;

        post_sec_to_read = (pFile->filepos + todo)%FS_FAT_SEC_SIZE;
        //This may happened when pFile->filepos%FS_FAT_SEC_SIZE > todo
        if(post_sec_to_read > todo) post_sec_to_read = todo;
    }
    else {
        // Start and end on the same sector
        mid_sec_to_read = 0;

        post_sec_to_read = 0;
        // Start at the sector bounder
        if(pre_byte_to_read == 0)
            post_sec_to_read = todo;
    }

    if (mid_sec_to_read > 0) {
        err= FS__fat_integret_read(pData, mid_sec_to_read, pFile);
        if (err < 0) {
            pFile->error = FS_ERR_READERROR;
            FS__fat_free(buffer);
            return 0;
        }

        i = pFile->filepos % FS_FAT_SEC_SIZE;
        if (i > 0) {
            i = FS_FAT_SEC_SIZE - i;
            j = mid_sec_to_read;
            while( j-- > 0 )
            {
                FS__CLIB_memcpy(buffer, (char *)((unsigned int)pData + j*FS_FAT_SEC_SIZE), FS_FAT_SEC_SIZE);
                FS__CLIB_memcpy((char *)((unsigned int)pData + j*FS_FAT_SEC_SIZE + i), buffer, FS_FAT_SEC_SIZE);
            }
        }
    }

    if (pre_byte_to_read > 0) {
        j = (pFile->filepos % clustsize)/FS_FAT_SEC_SIZE;
        err = FS__lb_read(FS__pDevInfo[pFile->dev_index].devdriver, 
                pFile->fileid_lo,
                datastart + diskclustnum * secperclust + j,
				(void*)buffer);

        if (err < 0) {
            pFile->error = FS_ERR_READERROR;
            FS__fat_free(buffer);
            return 0;
        }
        else {
            i = pFile->filepos % FS_FAT_SEC_SIZE;
            FS__CLIB_memcpy((char *)pData, &buffer[i], pre_byte_to_read);
        }
    }

    if (post_sec_to_read > 0) {
        fileclustnum = (pFile->filepos + pre_byte_to_read + mid_sec_to_read*FS_FAT_SEC_SIZE)/clustsize;
        diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, fileclustnum);
        if (diskclustnum == 0) {
            /* Could not find current cluster */
            pFile->error = FS_ERR_READERROR;
            FS__fat_free(buffer);
            return 0;
        }
        diskclustnum -= 2;

        j = ((pFile->filepos + todo) % clustsize)/FS_FAT_SEC_SIZE;
        err = FS__lb_read(FS__pDevInfo[pFile->dev_index].devdriver, 
                pFile->fileid_lo,
                datastart + diskclustnum * secperclust + j,
				(void*)buffer);
        if (err < 0) {
            pFile->error = FS_ERR_READERROR;
            FS__fat_free(buffer);
            return 0;
        }
        else {
            i = todo - post_sec_to_read;
            FS__CLIB_memcpy((char *)((unsigned int)pData +i), &buffer[0], post_sec_to_read);
        }
    }

    pFile->filepos += todo;
    fileclustnum= pFile->filepos/clustsize;
    diskclustnum = FS__fat_diskclust(pFile->dev_index, pFile->fileid_lo, pFile->fileid_hi, fileclustnum);
    pFile->CurClust = diskclustnum;
/*
for(x= 0; x < 64; x++)
{
    if (pbuf[x] != ppt[x]) {
        printf("------------------------read block bak at %d\n", x);
        while(1);
    }
}
*/
    FS__fat_free(buffer);
    return todo/Size;
}

