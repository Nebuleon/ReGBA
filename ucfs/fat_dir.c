
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
File        : fat_dir.c
Purpose     : POSIX 1003.1 like directory support
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
#include "lfn.h"

#if FS_POSIX_DIR_SUPPORT


/*********************************************************************
*
*             _FS_fat_create_directory
*
  Description:
  FS internal function. Create a directory in the directory specified
  with DirStart. Do not call, if you have not checked before for 
  existing directory with name pDirName.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number, which is passed to the device driver.
  pDirName    - Directory name. 
  DirStart    - Start of directory, where to create pDirName.
  DirSize     - Size of the directory starting at DirStart.
  
  Return value:
  >=0         - Directory has been created.
  <0          - An error has occured.
*/

static int _FS_fat_create_directory(int Idx, FS_u32 Unit, const char *pDirName,
                                    FS_u32 DirStart, FS_u32 DirSize) {
  char *buffer;
  FS__fat_dentry_type *s;
  FS_u32 dirindex;
  FS_u32 dsec;
  FS_i32 cluster;
  FS_u16 val_time;
  FS_u16 val_date;
  int err;
  int len;
  int j;

  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }
  len = FS__CLIB_strlen(pDirName);
//  printf("pDirName in creat:%s\n",pDirName);
  if (len > 11) {
    len = 11;
  }
  /* Read directory */
  for (dirindex = 0; dirindex < DirSize; dirindex++) {
    dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, dirindex);
    if (dsec == 0) {
      /* Translation of relativ directory sector to an absolute sector failed */
      FS__fat_free(buffer);
      return -1;
    }
    err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer); /* Read directory sector */
    if (err < 0) {
      /* Read error */
      FS__fat_free(buffer);
      return -1;
    }
    /* Scan the directory sector for a free or deleted entry */
    s = (FS__fat_dentry_type*)buffer;
    while (1) {
      if (s >= (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) {
        break;  /* End of sector reached */
      }
      if (s->data[0] == 0x00) {
        break;  /* Found a free entry */
      }
      if (s->data[0] == (unsigned char)0xe5) {
        break;  /* Found a deleted entry */
      }
      s++;
    }
    if (s < (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) {
      /* Free entry found. Make entry and return 1st block of the file. */
//      FS__CLIB_strncpy((char*)s->data, pDirName, len);
	    FS__CLIB_strncpy((char*)s->data, pDirName, len);
//	    FS__CLIB_memset((char*)&s->data[8], 0x20,3);
        s->data[8]     = 0x20;                                /* Res */
        s->data[9]     = 0x20;                                /* CrtTimeTenth (optional, not supported) */
        s->data[10]     = 0x20;
//      printf("pDirName:%s-- len:%d\n",s->data,len);
       s->data[11] = FS_FAT_ATTR_DIRECTORY;
      cluster = FS__fat_FAT_alloc(Idx, Unit, -1);              /* Alloc block in FAT */
      if (cluster >= 0) {
        s->data[12]     = 0x00;                                /* Res */
        s->data[13]     = 0x00;                                /* CrtTimeTenth (optional, not supported) */
        s->data[14]     = 0x00;                                /* CrtTime (optional, not supported) */
        s->data[15]     = 0x00;
        s->data[16]     = 0x00;                                /* CrtDate (optional, not supported) */
        s->data[17]     = 0x00;
        s->data[18]     = 0x00;                                /* LstAccDate (optional, not supported) */
        s->data[19]     = 0x00;
        val_time        = FS_X_OS_GetTime();
        s->data[22]     = (unsigned char)(val_time & 0xff);   /* WrtTime */
        s->data[23]     = (unsigned char)(val_time / 256);
        val_date        = FS_X_OS_GetDate();
        s->data[24]     = (unsigned char)(val_date & 0xff);   /* WrtDate */
        s->data[25]     = (unsigned char)(val_date / 256);
        s->data[26]     = (unsigned char)(cluster & 0xff);    /* FstClusLo / FstClusHi */ 
        s->data[27]     = (unsigned char)((cluster / 256) & 0xff);
        s->data[20]     = (unsigned char)((cluster / 0x10000L) & 0xff);
        s->data[21]     = (unsigned char)((cluster / 0x1000000L) & 0xff);
        s->data[28]     = 0x00;                                /* FileSize */
        s->data[29]     = 0x00;
        s->data[30]     = 0x00;
        s->data[31]     = 0x00;
        err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer); /* Write the modified directory sector */
        if (err < 0) {
          FS__fat_free(buffer);
          return -1;
        }
        /* Clear new directory and make '.' and '..' entries */
        /* Make "." entry */
        FS__CLIB_memset(buffer, 0x00, (FS_size_t)FS_FAT_SEC_SIZE);
        s = (FS__fat_dentry_type*)buffer;
        FS__CLIB_strncpy((char*)s->data, ".          ", 11);
        s->data[11]     = FS_FAT_ATTR_DIRECTORY;
        s->data[22]     = (unsigned char)(val_time & 0xff);   /* WrtTime */
        s->data[23]     = (unsigned char)(val_time / 256);
        s->data[24]     = (unsigned char)(val_date & 0xff);   /* WrtDate */
        s->data[25]     = (unsigned char)(val_date / 256);
        s->data[26]     = (unsigned char)(cluster & 0xff);    /* FstClusLo / FstClusHi */ 
        s->data[27]     = (unsigned char)((cluster / 256) & 0xff);
        s->data[20]     = (unsigned char)((cluster / 0x10000L) & 0xff);
        s->data[21]     = (unsigned char)((cluster / 0x1000000L) & 0xff);
        /* Make entry ".." */
        s++;
        FS__CLIB_strncpy((char*)s->data, "..         ", 11);
        s->data[11]     = FS_FAT_ATTR_DIRECTORY;
        s->data[22]     = (unsigned char)(val_time & 0xff);   /* WrtTime */
        s->data[23]     = (unsigned char)(val_time / 256);
        s->data[24]     = (unsigned char)(val_date & 0xff);   /* WrtDate */
        s->data[25]     = (unsigned char)(val_date / 256);
        s->data[26]     = (unsigned char)(DirStart & 0xff);    /* FstClusLo / FstClusHi */ 
        s->data[27]     = (unsigned char)((DirStart / 256) & 0xff);
        s->data[20]     = (unsigned char)((DirStart / 0x10000L) & 0xff);
        s->data[21]     = (unsigned char)((DirStart / 0x1000000L) & 0xff);
        dsec = FS__fat_dir_realsec(Idx, Unit, cluster, 0); /* Find 1st absolute sector of the new directory */
        if (dsec == 0) {
          FS__fat_free(buffer);
          return -1;
        }
        /* Write "." & ".." entries into the new directory */
        err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
        if (err < 0) {
          FS__fat_free(buffer);
          return -1;
        }
        /* Clear rest of the directory cluster */
        FS__CLIB_memset(buffer, 0x00, (FS_size_t)FS_FAT_SEC_SIZE);
        for (j = 1; j < FS__FAT_aBPBUnit[Idx][Unit].SecPerClus; j++) {
          dsec = FS__fat_dir_realsec(Idx, Unit, cluster, j);
          err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
          if (err < 0) {
            FS__fat_free(buffer);
            return -1;
          }
        }
        FS__fat_free(buffer);
        return 1;

      }
      FS__fat_free(buffer);
      return -1;
    }
  }
  FS__fat_free(buffer);
  return -2;  /* Directory is full */
}


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__fat_opendir
*
  Description:
  FS internal function. Open an existing directory for reading.

  Parameters:
  pDirName    - Directory name. 
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  ==0         - Unable to open the directory.
  !=0         - Address of an FS_DIR data structure.
*/

FS_DIR *FS__fat_opendir(const char *pDirName, FS_DIR *pDir) {
  FS_size_t len;
  FS_u32 unit;
  FS_u32 dstart;
  FS_u32 dsize;
  FS_i32 i;
  char realname[12];
  char *filename;
  if (!pDir) {
    return 0;  /* No valid pointer to a FS_DIR structure */
  }
  /* Find path on the media and return file name part of the complete path */
  dsize = FS__fat_findpath(pDir->dev_index, pDirName, &filename, &unit, &dstart); 
//printf("dsize:%d--pdirname:%s--filename:%s\n",dsize,pDirName,filename);
  if (dsize == 0) {
    return 0;  /* Directory not found */
  }
  FS__lb_ioctl(FS__pDevInfo[pDir->dev_index].devdriver, unit, FS_CMD_INC_BUSYCNT, 0, (void*)0); /* Turn on busy signal */
  len = FS__CLIB_strlen(filename);
  if (len != 0) {
    /* There is a name in the complete path (it does not end with a '\') */
//    FS__fat_make_realname(realname, filename);  /* Convert name to FAT real name */
    i =  FS__fat_find_dir(pDir->dev_index, unit, filename, dstart, dsize);  /* Search name in the directory */
    if (i == 0) {
      /* Directory not found */
      FS__lb_ioctl(FS__pDevInfo[pDir->dev_index].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
      return 0;
    }
  }
  else {
    /* 
       There is no name in the complete path (it does end with a '\'). In that
       case, FS__fat_findpath returns already start of the directory.
    */
    i = dstart;  /* Use 'current' path */
  }
  if (i) {
    dsize  =  FS__fat_dir_size(pDir->dev_index, unit, i);  /* Get size of the directory */
  }
  if (dsize == 0) {
    /* Directory not found */
    FS__lb_ioctl(FS__pDevInfo[pDir->dev_index].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
    return 0;
  }
  pDir->dirid_lo  = unit;
  pDir->dirid_hi  = i;
  pDir->dirid_ex  = dstart;
  pDir->error     = 0;
  pDir->size      = dsize;
  pDir->dirpos    = 0;
  pDir->inuse     = 1;
  return pDir;
}


/*********************************************************************
*
*             FS__fat_closedir
*
  Description:
  FS internal function. Close a directory referred by pDir.

  Parameters:
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  ==0         - Directory has been closed.
  ==-1        - Unable to close directory.
*/

int FS__fat_closedir(FS_DIR *pDir) {
  if (!pDir) {
    return -1;  /* No valid pointer to a FS_DIR structure */
  }
  FS__lb_ioctl(FS__pDevInfo[pDir->dev_index].devdriver, pDir->dirid_lo, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
  pDir->inuse = 0;
  return 0;
}

void RevertLfn(FS_u16 *Name,int size)
{
	int i;
	FS_u16 tmp[13];
	int d = size / 13;
	
	for(i = 0;i < d / 2 ;i++)
	{
		FS__CLIB_memcpy(tmp,&Name[ i * 13],13 * sizeof(FS_u16));
		FS__CLIB_memcpy(&Name[i * 13],&Name[(d- 1 - i) * 13],13 *sizeof(FS_u16));
		FS__CLIB_memcpy(&Name[ (d - 1 - i) * 13],tmp,13 * sizeof(FS_u16));
	}
}


/* To fetch a LFN we use a circular buffer of directory entries.       */
/* A long file name is up to 255 characters long, so 20 entries are    */
/* needed, plus one additional entry for the short name alias. We scan */
/* the directory and place the entry that we read into the Buffer at   */
/* the offset BufferPos. When we find a valid short name, we backtrace */
/* the Buffer with LfnSlot to extract the long file name, if present.  */
#define LFN_FETCH_SLOTS 21
#if 0
/* Fetches a long file name before a short name entry, if present, from the */
/* specified Buffer of directory entries to a 16-bit character string.      */
/* Returns the number of directory entries used by the long name, or 0 if   */
/* no long name is present before the short name entry.                     */
/* Called by fat_readdir and find.                                          */
static int fetch_lfn(tDirEntry *Buffer, int BufferPos, WORD *Name)
{
  tLfnEntry *LfnSlot  = (tLfnEntry *) &Buffer[BufferPos];
  BYTE       Checksum = lfn_checksum(&Buffer[BufferPos]);
  int        Order    = 0;
  int        NamePos  = 0;
  int        k;

  do
  {
    if (--LfnSlot < (tLfnEntry *) Buffer) LfnSlot += LFN_FETCH_SLOTS;
    if (LfnSlot->Attr != FD32_ALNGNAM) return 0;
    if (++Order != (LfnSlot->Order & 0x1F)) return 0;
    if (Checksum != LfnSlot->Checksum) return 0;
    /* Ok, the LFN slot is valid, attach it to the long name */
    for (k = 0; k < 5; k++) Name[NamePos++] = LfnSlot->Name0_4[k];
    for (k = 0; k < 6; k++) Name[NamePos++] = LfnSlot->Name5_10[k];
    for (k = 0; k < 2; k++) Name[NamePos++] = LfnSlot->Name11_12[k];
  }
  while (!(LfnSlot->Order & 0x40));
  if (Name[NamePos - 1] != 0x0000) Name[NamePos] = 0x0000;
  return Order;
}
#endif
/* Temporary function */
static int utf16to8(const FS_u16 *source, char *dest)
{
	int wc;
	int res;
	while (*source)
	{
		res = unicode_utf16towc(&wc, source, 2);
		if (res < 0) return res;
		source += res;
		res = unicode_wctoutf8(dest, wc, 6);
		if (res < 0) return res;
		dest += res;
	}
	*dest = 0;
	return 0;
}




void dumpentry(unsigned char *s)
{
	int i,j;
	printf("Entry:\n");
	for(i=0;i<2;i++)
	{
		for(j = 0; j < 16;j++)
		{
			printf("%02x ",s[j + i*16]);
		} 
		printf("\n");
	}
}


/*********************************************************************
*
*             FS__fat_readback
*
  Description:
  FS internal function. Read next directory entry in directory 
  specified by pDir.

  Parameters:
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  ==0         - No more directory entries or error.
  !=0         - Pointer to a directory entry.
*/

extern FBYTE lfn_checksum(FS__fat_dentry_type *D);

int FS__fat_readback(FS_u32 index,FS_u32 dirpos,FS_DIR *pDir,FS__fat_dentry_type *s)
{
//  FS__fat_dentry_type *s;
  FS_u32 dirindex;
  FS_u32 dsec;
  FS_u16 bytespersec;
  char *buffer;
  int err,LfnEntries;
  FS_u16 LongNameUtf16[FD32_LFNMAX];
  int NamePos = 0,k;
  FBYTE Checksum = lfn_checksum(s);
  int        Order    = 0;
  int cnt=0;
  int sector_end=0;

  if (!pDir) {
    return 0;  /* No valid pointer to a FS_DIR structure */
  }
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return 0;
  }
  bytespersec = FS__FAT_aBPBUnit[pDir->dev_index][pDir->dirid_lo].BytesPerSec;
  if(dirpos!=0)
  {
	  dirpos=dirpos-32;
  }
  dirindex = dirpos / bytespersec;
  savecount =0;  

//  printf("readback!---index:%d--pos:%d\n", dirindex,dirpos);

  while (dirindex >= 0) {

    dsec = FS__fat_dir_realsec(pDir->dev_index, pDir->dirid_lo, pDir->dirid_hi, dirindex);
    if (dsec == 0) {
      /* Cannot convert logical sector */
      FS__fat_free(buffer);
      return 0;
    }
    /* Read directory sector */
    err = FS__lb_read(FS__pDevInfo[pDir->dev_index].devdriver, pDir->dirid_lo, dsec, (void*)buffer);
    if (err < 0) {
      FS__fat_free(buffer);
      return 0;
    }
    if(sector_end)
    {
	    dirpos+=32;
	    sector_end=0;
    }
    /* Scan for valid directory entry */
    s = (FS__fat_dentry_type*)&buffer[(dirpos) % bytespersec];
//    printf("%d\n",(dirpos) % bytespersec);

    while (1) 
    {
//	    dumpentry(s);
	    if (s < (FS__fat_dentry_type*)(buffer)) {
//		    printf("sector begain\n");
		    break;  /* Begin of sector reached */
	    }
	    if (s->data[11] != 0x00) 
	    { /* not an empty entry */
		    if (s->data[0] != (unsigned char)0xe5) 
		    { /* not a deleted file */
#ifdef FS_USE_LONG_FILE_NAME
//			    dumpentry(s);
			    if(s->data[11] == 0x0f)
			    {
//				    dumpentry(s);
				    NamePos = 0;
				    FS__CLIB_memset(LongNameUtf16,0,FD32_LFNMAX * sizeof(FS_u16));
				    // we have a long name entry
				    // cast this directory entry as a "windows" (LFN: LongFileName) entry
				    tLfnEntry *LfnSlot  = (tLfnEntry *) s->data;
				    
//				    printf("checksum:%d--order:%d!\n",Checksum,LfnSlot->Checksum);
				    if (Checksum != LfnSlot->Checksum) 
				    {
					    FS__fat_free(buffer);
					    return 0;
				    }
				    if (++Order != (LfnSlot->Order & 0x1F)) 
				    {
					    FS__fat_free(buffer);
					    return 0;
				    }



				    for (k = 0; k < 5; k++)
				    {		
					    LongNameUtf16[NamePos++] = LfnSlot->Name0_4[k];
				    }
				    for (k = 0; k < 6; k++)
				    {
					    LongNameUtf16[NamePos++] = LfnSlot->Name5_10[k];
				    }
				    for (k = 0; k < 2; k++) 
				    {
					    LongNameUtf16[NamePos++] = LfnSlot->Name11_12[k];
				    }

//				    dumpentry(s);

				    int ret;
				    ret=UniToUtf8(LongNameUtf16, pDir->dirent.d_name);
//				    utf16to8(LongNameUtf16, pDir->dirent.d_name);
				    savename[savecount].count=ret;
				    FS__CLIB_memcpy(&savename[savecount].oldname, pDir->dirent.d_name,ret);
				    FS__CLIB_memcpy(&savename16[savecount].oldname16, LongNameUtf16,13*sizeof(FS_u16));
				    savecount++;
//				    printf("long--order:%x!\n",LfnSlot->Order);
				    if(LfnSlot->Order & 0x40) 
				    {
					    FS__CLIB_memset(pDir->dirent.d_name,0,255*sizeof(char));
//					    FS__CLIB_memcpy(&pDir->dirent.l_name,LongNameUtf16,255*sizeof(FS_u16));
					    for (k = 0; k<savecount; k++) 
					    {
						    FS__CLIB_memcpy(&pDir->dirent.d_name[cnt], &savename[k].oldname,savename[k].count);
						    cnt = cnt + savename[k].count;
						    FS__CLIB_memcpy(&pDir->dirent.l_name[k*13], &savename16[k].oldname16,13*sizeof(FS_u16));
					    }
//					    printf("longname:%s\n",pDir->dirent.d_name);
					    FS__fat_free(buffer);
					    return 1;

				    }
//				    break;
			    }
#endif 
		    }//(s->data[0] != (unsigned char)0xe5)
	    }/* not an empty entry */
	    s--;
	    sector_end=1;
	    dirpos -= 32;
    }
    if(dirindex<=0)
	    break;
    dirindex--;
    dirpos -= 32;
  }
  FS__fat_free(buffer);
  return 0;
}

/*********************************************************************
*
*             FS__fat_readdir
*
  Description:
  FS internal function. Read next directory entry in directory 
  specified by pDir.

  Parameters:
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  ==0         - No more directory entries or error.
  !=0         - Pointer to a directory entry.
*/
static trim_filename(char *pEntryName,const char *filename)
{
	int i=0,j=0;
	FS_FARCHARPTR s;
	s = (FS_FARCHARPTR)filename;
	while (1) {
		if (i > 12) {
			break;  /* If there is no '.', this is the end of the name */
		}
		if (*s != (char)0x20) {
			pEntryName[j] = *s;
			j++;
		}
		i++;
		s++;
	}

}

static trim_dir(char *pEntryName,const char *filename)
{
	int i=0,j=0;
	FS_FARCHARPTR s;
	s = (FS_FARCHARPTR)filename;
	while (1) {
		if (i > 8) {
			break;  /* If there is no '.', this is the end of the name */
		}
		if (*s != (char)0x20) {
			pEntryName[j] = *s;
			j++;
		}
		i++;
		s++;
	}

}
#if 1
struct FS_DIRENT *FS__fat_readdir(FS_DIR *pDir) {
  FS__fat_dentry_type *s;
  FS_u32 dirindex;
  FS_u32 dsec;
  FS_u16 bytespersec;
  char *buffer;
  int err,LfnEntries;
  FS_u16 LongNameUtf16[FD32_LFNMAX];
  int NamePos = 0,k;
  int ret;
  int getlongname=0;
  char dname[12];

  if (!pDir) {
    return 0;  /* No valid pointer to a FS_DIR structure */
  }
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return 0;
  }
  bytespersec = FS__FAT_aBPBUnit[pDir->dev_index][pDir->dirid_lo].BytesPerSec;
  dirindex = pDir->dirpos / bytespersec;
  savecount =0;  
  getlongname=0;

  while (dirindex < (FS_u32)pDir->size) {
//	    printf("dirindex:%d size:%d\n",dirindex,pDir->size);
//	    printf("%x %x\n",pDir->dirid_lo,pDir->dirid_hi);
    dsec = FS__fat_dir_realsec(pDir->dev_index, pDir->dirid_lo, pDir->dirid_hi, dirindex);
    if (dsec == 0) {
      /* Cannot convert logical sector */
      FS__fat_free(buffer);
      return 0;
    }
    /* Read directory sector */
    err = FS__lb_read(FS__pDevInfo[pDir->dev_index].devdriver, pDir->dirid_lo, dsec, (void*)buffer);
    if (err < 0) {
      FS__fat_free(buffer);
      return 0;
    }
    /* Scan for valid directory entry */
    s = (FS__fat_dentry_type*)&buffer[pDir->dirpos % bytespersec];
    

    while (1) {
//      	    dumpentry(s);
//	    printf("%d\n",pDir->dirpos);
//	    printf("%d %d %d\n",pDir->dirpos,s->data[11],s->data[0]);
      if (s >= (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) {
//	      printf("sector end\n");
	      break;  /* End of sector reached */
      }
//      printf("Shortname1111:%x-%x\n",s->data[11],s->data[0]);
      if (s->data[11] != 0x00) { /* not an empty entry */
        if (s->data[0] != (unsigned char)0xe5) { /* not a deleted file */

//		if (s->data[11] != (FS_FAT_ATTR_READ_ONLY | FS_FAT_ATTR_HIDDEN | FS_FAT_ATTR_SYSTEM | FS_FAT_VOLUME_ID)) {
		if (s->data[11] == FS_FAT_ATTR_ARCHIVE || s->data[11] == FS_FAT_ATTR_DIRECTORY){
//	    printf("%d\n",pDir->dirpos);
#ifdef FS_USE_LONG_FILE_NAME
//			printf("savecount = %d\n",savecount);
		ret= FS__fat_readback(dirindex,pDir->dirpos,pDir, s);
		if(ret)
			getlongname=1;
#endif 
		  break;  /* Also not a long entry, so it is a valid entry */

          }
        }//(s->data[0] != (unsigned char)0xe5)
      }/* not an empty entry */
      
      s++;
      pDir->dirpos += 32;
    }
    
    if (s < (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) {
      /* Valid entry found, copy it.*/
      pDir->dirpos += 32;
      if(getlongname==0)
      {
	      if(s->data[11]!=0x10)
	      {
//		      FS__CLIB_memcpy(pDir->dirent.d_name, s->data, 8);
//		      pDir->dirent.d_name[8] = '.';
//		      FS__CLIB_memcpy(&pDir->dirent.d_name[9], &s->data[8], 3);
//		      pDir->dirent.d_name[12] = 0;
		      FS__CLIB_memset(pDir->dirent.d_name,0, 12);
		      FS__CLIB_memcpy(dname, s->data, 8);
		      dname[8] = '.';
		      FS__CLIB_memcpy(&dname[9], &s->data[8], 3);
		      dname[12] = 0;
		      trim_filename(pDir->dirent.d_name,dname);
	      }
	      else
	      {
//		      FS__CLIB_memset(pDir->dirent.d_name,0, 12);
//		      FS__CLIB_memcpy(pDir->dirent.d_name, s->data, 8);
//		      pDir->dirent.d_name[8] = 0;
		      FS__CLIB_memset(pDir->dirent.d_name,0, 12);
		      FS__CLIB_memcpy(dname, s->data, 8);
		      dname[8] = 0;
		      trim_dir(pDir->dirent.d_name,dname);
	      }
      }
      pDir->dirent.FAT_DirAttr = s->data[11];
      FS__fat_free(buffer);
      return &pDir->dirent;
    }else
    {
//	    printf("Not %x %x %d\n",buffer + FS_FAT_SEC_SIZE,s,savecount);
    }
    dirindex++;
  }
  FS__fat_free(buffer);
  return 0;
}
#endif

/*********************************************************************
*
*             FS__fat_readdir_back
*
  Description:
  FS internal function. Read next directory entry in directory 
  specified by pDir.

  Parameters:
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  ==0         - No more directory entries or error.
  !=0         - Pointer to a directory entry.
*/
#if 1
struct FS_DIRENT *FS__fat_readdir_back(FS_DIR *pDir) {
  FS__fat_dentry_type *s;
  FS_u32 dirindex;
  FS_u32 dsec;
  FS_u16 bytespersec;
  char *buffer;
  int err,LfnEntries;
  FS_u16 LongNameUtf16[FD32_LFNMAX];
  int NamePos = 0,k;
  int ret;
  int getlongname=0;

  if (!pDir) {
    return 0;  /* No valid pointer to a FS_DIR structure */
  }
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return 0;
  }

  bytespersec = FS__FAT_aBPBUnit[pDir->dev_index][pDir->dirid_lo].BytesPerSec;
  dirindex = pDir->dirpos / bytespersec;
  savecount =0;  
  getlongname=0;

  while (dirindex >= 0) {
//	    printf("size:%d\n",pDir->size);
    dsec = FS__fat_dir_realsec(pDir->dev_index, pDir->dirid_lo, pDir->dirid_hi, dirindex);
    if (dsec == 0) {
      /* Cannot convert logical sector */
      FS__fat_free(buffer);
      return 0;
    }
    /* Read directory sector */
    err = FS__lb_read(FS__pDevInfo[pDir->dev_index].devdriver, pDir->dirid_lo, dsec, (void*)buffer);
    if (err < 0) {
      FS__fat_free(buffer);
      return 0;
    }
    if(pDir->dirpos <0)
    {
	    FS__fat_free(buffer);
	    return 0;
    }
    /* Scan for valid directory entry */
    s = (FS__fat_dentry_type*)&buffer[pDir->dirpos % bytespersec];
    

    while (1) {
//      	    dumpentry(s);

//	    printf("%d\n",pDir->dirpos);
      if (s < (FS__fat_dentry_type*)buffer) {
//	      printf("sector end\n");
	      break;  /* End of sector reached */
      }
//      printf("Shortname1111:%x-%x\n",s->data[11],s->data[0]);
      if (s->data[11] != 0x00) { /* not an empty entry */
        if (s->data[0] != (unsigned char)0xe5) { /* not a deleted file */

//		if (s->data[11] != (FS_FAT_ATTR_READ_ONLY | FS_FAT_ATTR_HIDDEN | FS_FAT_ATTR_SYSTEM | FS_FAT_VOLUME_ID) && s->data[11] !=FS_FAT_ATTR_DB ) {
		if (s->data[11] == FS_FAT_ATTR_ARCHIVE || s->data[11] == FS_FAT_ATTR_DIRECTORY){
//	    printf("%d\n",pDir->dirpos);
#ifdef FS_USE_LONG_FILE_NAME
//			printf("savecount = %d\n",savecount);
		ret= FS__fat_readback(dirindex,pDir->dirpos,pDir, s);
		if(ret)
			getlongname=1;
#endif 
		  break;  /* Also not a long entry, so it is a valid entry */

          }
        }//(s->data[0] != (unsigned char)0xe5)
      }/* not an empty entry */
      
      s--;
      pDir->dirpos -= 32;
    }
    
    if (s >= (FS__fat_dentry_type*)buffer) {
      /* Valid entry found, copy it.*/
      pDir->dirpos -= 32;


      if(getlongname==0)
      {
	      if(s->data[11]!=0x10)
	      {
		      FS__CLIB_memcpy(pDir->dirent.d_name, s->data, 8);
		      pDir->dirent.d_name[8] = '.';
		      FS__CLIB_memcpy(&pDir->dirent.d_name[9], &s->data[8], 3);
		      pDir->dirent.d_name[12] = 0;
	      }
	      else
	      {
		      FS__CLIB_memset(pDir->dirent.d_name,0, 12);
		      FS__CLIB_memcpy(pDir->dirent.d_name, s->data, 8);
		      pDir->dirent.d_name[8] = 0;
	      }
      }
      pDir->dirent.FAT_DirAttr = s->data[11];
      FS__fat_free(buffer);
      return &pDir->dirent;
    }else
    {
//	    printf("Not %x %x %d\n",buffer + FS_FAT_SEC_SIZE,s,savecount);
    }
    dirindex--;
    if(dirindex <0)
    {
	    FS__fat_free(buffer);
	    return 0;
    }
  }
  FS__fat_free(buffer);
  return 0;
}
#endif


#if 0
struct FS_DIRENT *FS__fat_readdir(FS_DIR *pDir) {
  FS__fat_dentry_type *s;
  FS_u32 dirindex;
  FS_u32 dsec;
  FS_u16 bytespersec;
  char *buffer;
  int err,LfnEntries;
  FS_u16 LongNameUtf16[FD32_LFNMAX];
  int NamePos = 0,k;
  int getlongname=0;

  if (!pDir) {
    return 0;  /* No valid pointer to a FS_DIR structure */
  }
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return 0;
  }
  bytespersec = FS__FAT_aBPBUnit[pDir->dev_index][pDir->dirid_lo].BytesPerSec;
  dirindex = pDir->dirpos / bytespersec;
  savecount =0;  
  getlongname=0;

  while (dirindex < (FS_u32)pDir->size) {
//	    printf("%x\n",pDir->dirid_hi);
    dsec = FS__fat_dir_realsec(pDir->dev_index, pDir->dirid_lo, pDir->dirid_hi, dirindex);
    if (dsec == 0) {
      /* Cannot convert logical sector */
      FS__fat_free(buffer);
      return 0;
    }
    /* Read directory sector */
    err = FS__lb_read(FS__pDevInfo[pDir->dev_index].devdriver, pDir->dirid_lo, dsec, (void*)buffer);
    if (err < 0) {
      FS__fat_free(buffer);
      return 0;
    }
    /* Scan for valid directory entry */
    s = (FS__fat_dentry_type*)&buffer[pDir->dirpos % bytespersec];
    

    while (1) {

      if (s >= (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) {

        break;  /* End of sector reached */
      }
//      printf("Shortname1111:%x-%x\n",s->data[11],s->data[0]);
      if (s->data[11] != 0x00) { /* not an empty entry */
        if (s->data[0] != (unsigned char)0xe5) { /* not a deleted file */
#ifdef FS_USE_LONG_FILE_NAME
		if(s->data[11] == 0x0f){
			NamePos = 0;
			FS__CLIB_memset(LongNameUtf16,0,FD32_LFNMAX * sizeof(FS_u16));
			// we have a long name entry
			// cast this directory entry as a "windows" (LFN: LongFileName) entry
			tLfnEntry *LfnSlot  = (tLfnEntry *) s->data;
			for (k = 0; k < 5; k++)
			{		
				LongNameUtf16[NamePos++] = LfnSlot->Name0_4[k];
			}
			for (k = 0; k < 6; k++)
			{
				LongNameUtf16[NamePos++] = LfnSlot->Name5_10[k];
			}
			for (k = 0; k < 2; k++) 
			{
				LongNameUtf16[NamePos++] = LfnSlot->Name11_12[k];
			}
			utf16to8(LongNameUtf16, pDir->dirent.d_name);
//			int ret;
//			ret = utf16to8(LongNameUtf16, pDir->dirent.d_name);
//			if(ret <= 0)
//				printf("ret = %d\n",ret);
//			printf("pDir->dirent.d_name:\n");
			FS__CLIB_memcpy(&savename[savecount].oldname, pDir->dirent.d_name,13);
//			for(ret = 0;ret < 13;ret++)
//			{
//				printf("%02x ",pDir->dirent.d_name[ret]);
//			}
//			printf("\n");
			savecount++;
		}
		
#endif 
		if (s->data[11] != (FS_FAT_ATTR_READ_ONLY | FS_FAT_ATTR_HIDDEN | FS_FAT_ATTR_SYSTEM | FS_FAT_VOLUME_ID)) {
#ifdef FS_USE_LONG_FILE_NAME
//			printf("savecount = %d\n",savecount);
			if(savecount >0)
			{
//				FS__CLIB_memset(pDir->dirent.d_name,0,255*sizeof(char));
				for (k = 0; k<savecount; k++) 
				{
//					printf("%d:%s\n",k,&savename[savecount-1-k].oldname);
					FS__CLIB_memcpy(&pDir->dirent.d_name[k*13], &savename[savecount-1-k].oldname,13);
				}
				FS__CLIB_memset(savename,0, 13 * 20);
				printf("longname:%s\n",pDir->dirent.d_name);
				getlongname=1;
				savecount =0;
			}
#endif 
		  break;  /* Also not a long entry, so it is a valid entry */

          }
        }//(s->data[0] != (unsigned char)0xe5)
      }/* not an empty entry */
      
      s++;
      pDir->dirpos += 32;
    }
    
    if (s < (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) {
      /* Valid entry found, copy it.*/
      pDir->dirpos += 32;
      if(getlongname==0)
      {
	      if(s->data[11]!=0x10)
	      {
		      FS__CLIB_memcpy(pDir->dirent.d_name, s->data, 8);
		      pDir->dirent.d_name[8] = '.';
		      FS__CLIB_memcpy(&pDir->dirent.d_name[9], &s->data[8], 3);
		      pDir->dirent.d_name[12] = 0;
	      }
	      else
	      {
		      FS__CLIB_memset(pDir->dirent.d_name,0, 12);
		      FS__CLIB_memcpy(pDir->dirent.d_name, s->data, 8);
		      pDir->dirent.d_name[8] = 0;
	      }
      }
      pDir->dirent.FAT_DirAttr = s->data[11];
      FS__fat_free(buffer);
      return &pDir->dirent;
    }else
    {
//	    printf("Not %x %x %d\n",buffer + FS_FAT_SEC_SIZE,s,savecount);
    }
    dirindex++;
  }
  FS__fat_free(buffer);
  return 0;
}
#endif

/*********************************************************************
*
*             FS__fat_MkRmDir
*
  Description:
  FS internal function. Create or remove a directory. If you call this 
  function to remove a directory (MkDir==0), you must make sure, that 
  it is already empty.

  Parameters:
  pDirName    - Directory name. 
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  MkDir       - ==0 => Remove directory.
                !=0 => Create directory.
  
  Return value:
  ==0         - Directory has been created.
  ==-1        - An error has occured.
*/

int  FS__fat_MkRmDir(const char *pDirName, int Idx, char MkDir) {
  FS_size_t len;
  FS_u32 dstart;
  FS_u32 dsize;
  FS_u32 unit;
  FS_i32 i;
  int lexp_a;
  int lexp_b;
  char realname[12];
  char *filename;

  if (Idx < 0) {
    return -1; /* Not a valid index */
  }
//  printf("pDirName:%s---\n",pDirName);

  dsize = FS__fat_findpath(Idx, pDirName, &filename, &unit, &dstart);

  if (dsize == 0) {
    return -1;  /* Path not found */
  }
  FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, unit, FS_CMD_INC_BUSYCNT, 0, (void*)0); /* Turn on busy signal */
  len = FS__CLIB_strlen(filename);
  if (len != 0) {
//    FS__fat_make_realname(realname, filename);  /* Convert name to FAT real name */
    create_dir=1;
    i =  FS__fat_find_dir(Idx, unit,  filename, dstart, dsize);
    create_dir=0;
    lexp_a = (i!=0) && (MkDir);  /* We want to create a direcory , but it does already exist */
    lexp_b = (i==0) && (!MkDir); /* We want to remove a direcory , but it does not exist */
    lexp_a = lexp_a || lexp_b;
    if (lexp_a) {
      /* We want to create, but dir does already exist or we want to remove, but dir is not there */
      /* turn off busy signal */
      FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);
      return -1;
    }
  }
  else {
    FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
    return -1;
  }
  /* 
      When you get here, variables have following values:
       dstart="current"  
       dsize="size of current"  
       realname="real dir name to create" 
  */
  if (MkDir) {

    i = _FS_fat_create_directory(Idx, unit,temp_dir, dstart, dsize);  /* Create the directory */
  }
  else {
    i = FS__fat_DeleteFileOrDir(Idx, unit, temp_dir, dstart, dsize, 0);  /* Remove the directory */
  }
  if (i >= 0) {
    /* If the operation has been successfull, flush the cache.*/
    i = FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, unit, FS_CMD_FLUSH_CACHE, 2, (void*)0);
    FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn of busy signal */
    if (i < 0) {
      return -1;
    }
    return 0;
  }
  FS__lb_ioctl(FS__pDevInfo[Idx].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn of busy signal */
  return -1;
}


#endif /* FS_POSIX_DIR_SUPPORT */

