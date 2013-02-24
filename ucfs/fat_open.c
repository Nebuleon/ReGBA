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
File        : fat_open.c
Purpose     : FAT routines for open/delete files
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
#include "fs_dev.h"
#include "fs_api.h"
#include "fs_fsl.h"
#include "fs_int.h"
#include "fs_os.h"
#include "fs_lbl.h"
#include "lfn.h"
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


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/



static int is_all_uppercase(const char *pFileName,int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		if(*(pFileName+i)>='a' && *(pFileName+i)<='z')
			return 0;
/*
		if(*(pFileName+i)=='.')
			continue;
		if(*(pFileName+i)>='A' && *(pFileName+i)<='Z' )
			continue;
		else if(*(pFileName+i)>='0' && *(pFileName+i)<='9')
			continue;
		else
			return 0;
*/
	} 
	return 1;
}

static int is_all_lowercase(const char *pFileName,int len)
{
	int i;
	for(i=0;i<len;i++)
	{
        if(*(pFileName+i)>='A' && *(pFileName+i)<='Z' )
			return 0;
/*
		if(*(pFileName+i)=='.')
			continue;
		if(*(pFileName+i)>='a' && *(pFileName+i)<='z')
			continue;
		else if(*(pFileName+i)>='0' && *(pFileName+i)<='9')
			continue;
		else
			return 0;
*/
	} 
	return 1;
}

/* Calculate the 8-bit checksum for the long file name from its */
/* corresponding short name.                                    */
/* Called by split_lfn and find (find.c).                       */
unsigned char lfn_checksum(FS__fat_dentry_type *D)
{
    FS_u32 i;
    unsigned char Sum;
    unsigned char *pt;

    /* NOTE: the operation is an unsigned char rotate right*/
    Sum= 0;
    pt= (unsigned char*)D;
    for (i = 0; i < 11; i++)
    {
        Sum = (((Sum & 1) << 7) | ((Sum & 0xFE) >> 1)) + *pt++;
    }
    return Sum;
}

void dump_sec(char* sec)
{
    unsigned int i;
    unsigned char *pt;

    pt= (unsigned char*)sec;
    for(i= 0; i < 512; i++)
    {
        if((i%16) == 0)
        {
            printf("\n%d: ", i/16);
        }
        printf("%02x ", pt[i]);
    }
    printf("\n");
}

/*********************************************************************
*
*             _FS_fat_longname_read
*
  Description:
  FS internal function. Find the file with name pFileName in directory
  DirStart. Copy its directory entry to pDirEntry.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  DirStart    - The first clust of the directory
  cur_sec     - Pointer to Current entry of the directory
  DirSize     - Sector (not cluster) size of the directory
  s           - Pointer of pointer to an FS__fat_dentry_type data structure of current entry
  entryname   - The long file name of the entry
  pbuff       - Pointer to a sector buffer
 
  Return value:
  >0          - A long name find(including name containing non-asic code), 
                 Number of sub-component of the long entry, includeing short name entry
  =0          - Current long entry may wrong
  <0          - An error has occured.
*/

static int _FS_fat_longname_read(int Idx, FS_u32 Unit, FS_u32 DirStart, FS_u32 *cur_sec,
    FS_u32 DirSize, FS__fat_dentry_type **s, char *entryname, char *pbuff)
{
    FS_u32 i, k, j;
    FS_u32 dsec;
    FS__fat_dentry_type *ss;
    int err;
    FS_size_t EntryOrd;
    unsigned char Checksum;
    unsigned short lfname[FS_FAT_SEC_SIZE/2];
    unsigned char *pt;
    FS_i32 entry_size;

//printf("find long name\n");
    entryname[0] = '\0';
    ss = *s;
    EntryOrd = (ss -> data[0]) & 0x7F;
    if((EntryOrd & 0x40) == 0)  // Not the start of the long file name
        return 0;

    entry_size = (EntryOrd & 0x40) +1;
    j= ((unsigned int)ss - (unsigned int)pbuff) / FS_FAT_DENTRY_SIZE;
    Checksum = ss -> data[13];
    lfname[FS_FAT_SEC_SIZE/2-1] = 0;
    pt = ((unsigned char*)&lfname[FS_FAT_SEC_SIZE/2-1]) - 1;
    i= *cur_sec;
    goto _longname_read_skip;
    for(; i < DirSize; i++)
    {
        dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	    if (dsec == 0) {
            return -1;
        }
	    err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)pbuff);
	    if (err < 0) {
            return -2;
        }
	    ss = (FS__fat_dentry_type*)pbuff;
        j = 0;
        *cur_sec = i;

        while(j < FS_FAT_SEC_SIZE/FS_FAT_DENTRY_SIZE)
        {
_longname_read_skip:
            if (ss -> data[0] == 0) {
                *s = ss;
                entryname[0] = '\0';
                return 0;
            }
            if (ss -> data[0] == 0xe5) {
                *s = ss;
                entryname[0] = '\0';
                return 0;
            }
//dumpen((unsigned char*)ss);
            if((ss->data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME)
            {
                if (Checksum != ss -> data[13]) {
                    *s = ss -1; /* This entry may be another new long name entry */
                    entryname[0] = '\0';
                    return 0;
                }

                for (k = 28 + 4 -1; k >= 28; )
                {
                    *pt-- = ss -> data[k--];
                }

                for (k = 14 + 12 -1; k >= 14; )
                {
                    *pt-- = ss -> data[k--];
                }

                for (k = 1 + 10 -1; k >= 1; )
                {
                    *pt-- = ss -> data[k--];
                }

                EntryOrd = (ss -> data[0]) & 0x7F & (~0x40);
            }
            else {
                *s = ss;
                entryname[0] = '\0';
                if (EntryOrd == 0x01)
                if (ss->data[11] & (FS_FAT_ATTR_ARCHIVE | FS_FAT_ATTR_DIRECTORY))
                if (Checksum == lfn_checksum(ss)) {
                    UniToUtf8(((unsigned short*)(pt+1)), entryname);
                    return entry_size;
                }

                *s = ss -1; /* This entry may be another new long name entry */
                return 0;
            }
            ss++, j++;
        } //end while(1)
    } //end for

    return -3;
}


/*********************************************************************
*
*             _FS_fat_find_file
*
  Description:
  FS internal function. Find the file with name pFileName in directory
  DirStart. Copy its directory entry to pDirEntry.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  pFileName   - File name. 
  pDirEntry   - Pointer to an FS__fat_dentry_type data structure.
  DirStart    - 1st cluster of the directory.
  DirSize     - Sector (not cluster) size of the directory.
 
  Return value:
  >=0         - File found. Value is the first cluster of the file.
  <0          - An error has occured.
*/

static FS_i32 _FS_fat_find_file(int Idx, FS_u32 Unit, const char *pFileName,
                                    FS__fat_dentry_type *pDirEntry,
                                    FS_u32 DirStart, FS_u32 DirSize) {
    FS__fat_dentry_type *s;
    FS_u32 i, len;
    FS_u32 dsec;
    int err, ret;
    char *buffer;
    char d_name[FS_DIRNAME_MAX];    char realname[12];
    int non_asic_short;     //Short name contain non-asic
    int longname= 0;        //Long name flag

    buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
    if (!buffer)
        return -1;

    non_asic_short= 0;
    len = FS__CLIB_strlen(pFileName);
    if (len <= 12) {
        i = 0;
        //Check whether all charactor are asic code
        while (i < len) {
            if (pFileName[i++] > 0x7F) break;
        }
        if (i >= len) //All belong asic code
            FS__fat_make_realname(realname, pFileName);
        else
            non_asic_short = 1;
    }
    else
        non_asic_short = 1;

//printf("pFileName %s\n", pFileName);
//printf("realname %s\n", realname);
    /* Read directory */
    for (i = 0; i < DirSize; i++)
    {
        dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
        if (dsec == 0) {
            FS__fat_free(buffer);
            return -1;
        }
//printf("Real sector ---------------------%08x\n", dsec);
        err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
        if (err < 0) {
            FS__fat_free(buffer);
            return -1;
        }
        s = (FS__fat_dentry_type*)buffer;
//dump_sec(buffer);
        len= 0;
        while (len < FS_FAT_SEC_SIZE/FS_FAT_DENTRY_SIZE)
        {
            if (s->data[0] == 00) {
                /* Entrys behind this entry are empty, if not, we don't care */
                FS__fat_free(buffer);
                return -1;
            }

            if (s->data[0] != 0xe5)
            if ((s->data[11] & (FS_FAT_ATTR_ARCHIVE | FS_FAT_ATTR_DIRECTORY)) || 
                ((s -> data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME))
		    { /* not a deleted file && is file or directory*/
                longname= 0;
                if((s -> data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME)
                { /* Long name entry*/
//dumpen((unsigned char*)s);
                    ret = _FS_fat_longname_read(Idx, Unit, DirStart, &i, DirSize, &s, d_name, buffer);
//printf("Long read error: %d\n", ret);
                    if (ret > 0) {
                        longname= 1;
                    }
                    else if (ret < 0) {
//printf("Long read error: %d\n", ret);
                        /* The long name entry have some error */
                        FS__fat_free(buffer);
                        return -1;
                    }
                    
                    if ((unsigned int)s >= (unsigned int)buffer) {
                        len = ((unsigned int)s - (unsigned int)buffer)/FS_FAT_DENTRY_SIZE;
                    }
                    else {
                        len = -1;
                    }
                }
                
                ret = -1;
                if (non_asic_short == 0) {
                    FS__CLIB_memcpy(d_name, (char*)&s -> data[0], 8);
                    d_name[8]='.';
                    FS__CLIB_memcpy(&d_name[9], (char*)&s->data[8], 3);
                    ret = FS__CLIB_strncmp(d_name, realname, 12);
                }
                else {
                    if(longname)
                        ret = FS__CLIB_strcasecmp(d_name, pFileName);
                }

//printf("d_name %s\n", d_name);
                if (ret == 0) {
                    /* Entry found. Return number of 1st block of the file */
                    if (pDirEntry) {
                        FS__CLIB_memcpy(pDirEntry, s, sizeof(FS__fat_dentry_type));
                    }
                    dsec  = (FS_u32)s->data[26];
                    dsec += (FS_u32)s->data[27] * 0x100UL;
                    dsec += (FS_u32)s->data[20] * 0x10000UL;
                    dsec += (FS_u32)s->data[21] * 0x1000000UL;

                    FS__fat_free(buffer);
                    return ((FS_i32)dsec);
                }
            }
//printf("entry %d\n", len);
            s++, len++;
	    }
    }
    /* Entry not found */
    FS__fat_free(buffer);
    return -1;
}


/*********************************************************************
*
*             _FS_fat_IncDir
*
  Description:
  FS internal function. Increase directory starting at DirStart.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  DirStart    - 1st cluster of the directory.
  pDirSize    - Pointer to an FS_u32, which is used to return the new 
                sector (not cluster) size of the directory.
 
  Return value:
  ==1         - Success.
  ==-1        - An error has occured.
*/

static int _FS_fat_IncDir(int Idx, FS_u32 Unit, FS_u32 DirStart, FS_u32 *pDirSize) {
  FS_u32 i;
  FS_u32 dsec;
  FS_i32 last;
  char *buffer;
  int err;

  if (DirStart == 0) { 
    /* Increase root directory only, if not FAT12/16  */
    i = FS__FAT_aBPBUnit[Idx][Unit].RootEntCnt;
    if (i != 0) {
      return -1;  /* Not FAT32 */
    }
  }
  last = FS__fat_FAT_find_eof(Idx, Unit, DirStart, 0);
  if (last < 0) {
    return -1;  /* No EOF marker found */
  }
  last = FS__fat_FAT_alloc(Idx, Unit, last);  /* Allocate new cluster */
  if (last < 0) {
    return -1;
  }
  *pDirSize = *pDirSize + FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
  /* Clean new directory cluster */
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }
  FS__CLIB_memset(buffer, 0x00, (FS_size_t)FS_FAT_SEC_SIZE);
  for (i = *pDirSize - FS__FAT_aBPBUnit[Idx][Unit].SecPerClus; i < *pDirSize; i++) {
    dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
    if (dsec == 0) {
      FS__fat_free(buffer);
      return -1;
    }
    err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
    if (err < 0) {
      FS__fat_free(buffer);
      return -1;
    }
  }
  FS__fat_free(buffer);
  return 1;
}


/* Copies the 16-bit character Ch into the LFN slot Slot in the position */
/* SlotPos, taking care of the slot structure.                           */
/* Called by split_lfn.                                                  */
static void copy_char_in_lfn_slot(tLfnEntry *Slot, int SlotPos, uint16_t Ch)
{
  if ((SlotPos >= 0)  && (SlotPos <= 4))  Slot->Name0_4  [SlotPos]    = Ch;
  else
  if ((SlotPos >= 5)  && (SlotPos <= 10)) Slot->Name5_10 [SlotPos-5]  = Ch;
  else
  if ((SlotPos >= 11) && (SlotPos <= 12)) Slot->Name11_12[SlotPos-11] = Ch;
}


/* Splits the long file name in the string LongName to fit in up to 20 */
/* LFN slots, using the short name of the dir entry D to compute the   */
/* short name checksum.                                                */
/* On success, returns 0 and fills the Slot array with LFN entries and */
/* NumSlots with the number of entries occupied.                       */
/* On failure, returns a negative error code.                          */
/* Called by allocate_lfn_dir_entries.                                 */
static int split_lfn(tLfnEntry *Slot, FS__fat_dentry_type *D, char *LongName, int *NumSlots)
{
  int   NamePos  = 0;
  int   SlotPos  = 0;
  int   Order    = 0;
  FBYTE  Checksum = lfn_checksum(D);
  uint16_t utf16[2];
  wchar_t  wc;
  int      res, k;
  /* Initialize the first slot */
  Slot[0].Order    = 1;
  Slot[0].Attr     = FD32_ALNGNAM;
  Slot[0].Reserved = 0;
  Slot[0].Checksum = Checksum;
  Slot[0].FstClus  = 0;

  for (;;)
  {
    res = unicode_utf8towc(&wc, LongName, 6);
    if (res < 0) return res;
    if (!wc) break;
    LongName += res;
    res = unicode_wctoutf16(utf16, wc, sizeof(utf16));
    if (res < 0) return res;
    for (k = 0; k < res; k++)
    {
      if (SlotPos == 13)
      {
        SlotPos = 0;
        Order++;
        Slot[Order].Order    = Order + 1; /* 1-based numeration */
        Slot[Order].Attr     = FD32_ALNGNAM;
        Slot[Order].Reserved = 0;
        Slot[Order].Checksum = Checksum;
        Slot[Order].FstClus  = 0;
      }
      copy_char_in_lfn_slot(&Slot[Order], SlotPos++, utf16[k]);
      if (++NamePos == FD32_LFNMAX) return -ENAMETOOLONG;
    }
  }
  /* Mark the slot as last */
  Slot[Order].Order |= 0x40;
  /* Insert an Unicode NULL terminator, only if it fits into the slots */
  copy_char_in_lfn_slot(&Slot[Order], SlotPos++, 0x0000);
  /* Pad the remaining characters of the slot with FFFFh */
  while (SlotPos < 13) copy_char_in_lfn_slot(&Slot[Order], SlotPos++, 0xFFFF);

  *NumSlots = Order + 1;
  return 0;
}

/* Searches an open directory for NumEntries free consecutive entries. */
/* On success, returns the not negative byte offset of the first free  */
/* entry. Returns a negative error code on failure.                    */
/* Called by allocate_sfn_dir_entry and allocate_lfn_dir_entries.      */

static int find_empty_dir_entries(int Idx, FS_u32 Unit,FS_u32 DirStart, FS_u32 DirSize, int NumEntries,char *FileName,tLfnEntry *Slot)
{
  int       Found = 0; /* Number of consecutive free entries found */
  int       EntryOffset = 0;
  int       stepcount = 0;
  int       savecount =0;
  int       Res,i;
  char *buffer;
  FS__fat_dentry_type *ss;
  FS_u32 dsec;
  int err;
  FS_i32 cluster;
  FS_u16 val;

  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }

  /* Scans the whole directory for free entries */
  for (i = 0; i < DirSize; i++) 
  {
	  dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	  if (dsec == 0) {
		  FS__fat_free(buffer);
		  return -1;
	  }
	  err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	  if (err < 0) {
		  FS__fat_free(buffer);
		  return -1;
	  }
	  
	  ss = (FS__fat_dentry_type*)buffer;	  stepcount=0;
	  while (1) 
	  {
		  if (ss >= (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) {
			  break;  /* End of sector reached */
		  }
		  if ((ss->data[0] == 0x00) || (ss->data[0] == (unsigned char)0xe5)) 
		  { /* not an empty entry */ /* not a deleted file */
			  if (Found > 0) Found++;
			  else {
				  Found       = 1;
				  EntryOffset = i;
				  savecount = stepcount;
			  }
		  }
		  else Found = 0;
		  
		  if (Found == NumEntries) break;
		  stepcount++;
		  ss++;
	  }
	  if (Found == NumEntries) break;
  }	  /***look for entry***/

    stepcount= NumEntries-1;
    i = EntryOffset;
    dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
    if (dsec == 0) {
        FS__fat_free(buffer);
        return -1;
    }
    err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
    if (err < 0) {
        FS__fat_free(buffer);
        return -1;
    }
    ss = (FS__fat_dentry_type*)buffer;
    ss = ss + savecount;
    goto find_empty_dir_entries_skip;

  /* write entry */
  for ( ; i < DirSize; i++) 
  {
	  dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	  if (dsec == 0) {
		  FS__fat_free(buffer);
		  return -1;
	  }
	  err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	  if (err < 0) {
		  FS__fat_free(buffer);
		  return -1;
	  }
	  ss = (FS__fat_dentry_type*)buffer;

find_empty_dir_entries_skip:
	  while (1) 
	  {
		  if (ss >= (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) {
			  break;  /* End of sector reached */
		  }
		  
		  if ((ss->data[0] == 0x00) || (ss->data[0] == (unsigned char)0xe5)) 
		  { /* not an empty entry */ /* not a deleted file */
			  if (stepcount==0) 
			  {
				  /* Free entry found. Make entry and return 1st block of the file */
				  FS__CLIB_strncpy((char*)ss->data, FileName, 11);
				  ss->data[11] = FS_FAT_ATTR_ARCHIVE;

				  /* Alloc block in FAT */
				  cluster = FS__fat_FAT_alloc(Idx, Unit, -1);
				  if (cluster >= 0)
                  {
					  ss->data[12]     = 0x00;                           /* Res */
					  ss->data[13]     = 0x00;                           /* CrtTimeTenth (optional, not supported) */
					  ss->data[14]     = 0x00;                           /* CrtTime (optional, not supported) */
					  ss->data[15]     = 0x00;
					  ss->data[16]     = 0x00;                           /* CrtDate (optional, not supported) */
					  ss->data[17]     = 0x00;
					  ss->data[18]     = 0x00;                           /* LstAccDate (optional, not supported) */
					  ss->data[19]     = 0x00;
					  val             = FS_X_OS_GetTime();
					  ss->data[22]     = (unsigned char)(val & 0xff);   /* WrtTime */
					  ss->data[23]     = (unsigned char)(val / 256);
					  val             = FS_X_OS_GetDate();
					  ss->data[24]     = (unsigned char)(val & 0xff);   /* WrtDate */
					  ss->data[25]     = (unsigned char)(val / 256);
					  ss->data[26]     = (unsigned char)(cluster & 0xff);    /* FstClusLo / FstClusHi */ 
					  ss->data[27]     = (unsigned char)((cluster / 256) & 0xff);
					  ss->data[20]     = (unsigned char)((cluster / 0x10000L) & 0xff);
					  ss->data[21]     = (unsigned char)((cluster / 0x1000000L) & 0xff);
					  ss->data[28]     = 0x00;                           /* FileSize */
					  ss->data[29]     = 0x00;
					  ss->data[30]     = 0x00;
					  ss->data[31]     = 0x00;
					  err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
					  if (err < 0) {
						  FS__fat_free(buffer);
						  return -1;
					  }
				  }
				  FS__fat_free(buffer);
				  return cluster;
			  }
              else
			  {
				  FS__CLIB_memcpy(ss->data,&Slot[stepcount-1], 32);
				  err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
				  if (err < 0) {
					  FS__fat_free(buffer);
					  return -1;
				  }
			  }
		  }
		  stepcount--;
		  ss++;
	  }
  }	  /***write entry***/

  FS__fat_free(buffer);
  return -2;
}

#if 0
static int find_empty_dir_entries(int Idx, FS_u32 Unit,FS_u32 DirStart, FS_u32 DirSize, int NumEntries,char *FileName,tLfnEntry *Slot)
{
  int       Found = 0; /* Number of consecutive free entries found */
  int       EntryOffset = 0;
  int       stepcount = 0;
  int       savecount =0;
  int       Res,i;
  char *buffer;
  FS__fat_dentry_type *ss;
  FS_u32 dsec;
  int err;
  FS_i32 cluster;
  FS_u16 val;


  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }
  /* Scans the whole directory for free entries */
  for (i = 0; i < DirSize; i++) 
  {
	  dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	  if (dsec == 0) 
	  {
		  FS__fat_free(buffer);
		  return -1;
	  }
	  err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	  if (err < 0) 
	  {
		  FS__fat_free(buffer);
		  return -1;
	  }
	  
	  ss = (FS__fat_dentry_type*)buffer;
	  
	  stepcount=0;
	  while (1) 
	  {
		  if (ss >= (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) 
		  {
			  break;  /* End of sector reached */
		  }
		  
		  if ((ss->data[0] == 0x00) || (ss->data[0] == (unsigned char)0xe5)) 
		  { /* not an empty entry */ /* not a deleted file */
			  if (Found > 0) Found++;
			  else
			  {
				  Found       = 1;
				  EntryOffset = i;
				  savecount = stepcount;
//				  printf("i:%d--stepcnt:%d\n", EntryOffset,savecount);
			  }
		  }
		  else Found = 0;
		  
		  if (Found == NumEntries) break;
		  stepcount++;
		  ss++;
	  }
	  if (Found == NumEntries) break;

	  
  }	  /***look for entry***/

  stepcount=NumEntries;
  /* write entry */
  for (i = EntryOffset; i < DirSize; i++) 
  {
	  dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	  if (dsec == 0) 
	  {
		  FS__fat_free(buffer);
		  return -1;
	  }
	  err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	  if (err < 0) 
	  {
		  FS__fat_free(buffer);
		  return -1;
	  }
	  
	  ss = (FS__fat_dentry_type*)buffer;
	  ss = ss + savecount;

	  while (1) 
	  {
		  if (ss >= (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) 
		  {
			  break;  /* End of sector reached */
		  }
		  
		  if ((ss->data[0] == 0x00) || (ss->data[0] == (unsigned char)0xe5)) 
		  { /* not an empty entry */ /* not a deleted file */

			  if (stepcount==1) 
			  {
				  /* Free entry found. Make entry and return 1st block of the file */
				  FS__CLIB_strncpy((char*)ss->data, FileName, 11);
				  ss->data[11] = FS_FAT_ATTR_ARCHIVE;
				  /* Alloc block in FAT */
				  cluster = FS__fat_FAT_alloc(Idx, Unit, -1);
				  if (cluster >= 0) {
					  ss->data[12]     = 0x00;                           /* Res */
					  ss->data[13]     = 0x00;                           /* CrtTimeTenth (optional, not supported) */
					  ss->data[14]     = 0x00;                           /* CrtTime (optional, not supported) */
					  ss->data[15]     = 0x00;
					  ss->data[16]     = 0x00;                           /* CrtDate (optional, not supported) */
					  ss->data[17]     = 0x00;
					  ss->data[18]     = 0x00;                           /* LstAccDate (optional, not supported) */
					  ss->data[19]     = 0x00;
					  val             = FS_X_OS_GetTime();
					  ss->data[22]     = (unsigned char)(val & 0xff);   /* WrtTime */
					  ss->data[23]     = (unsigned char)(val / 256);
					  val             = FS_X_OS_GetDate();
					  ss->data[24]     = (unsigned char)(val & 0xff);   /* WrtDate */
					  ss->data[25]     = (unsigned char)(val / 256);
					  ss->data[26]     = (unsigned char)(cluster & 0xff);    /* FstClusLo / FstClusHi */ 
					  ss->data[27]     = (unsigned char)((cluster / 256) & 0xff);
					  ss->data[20]     = (unsigned char)((cluster / 0x10000L) & 0xff);
					  ss->data[21]     = (unsigned char)((cluster / 0x1000000L) & 0xff);
					  ss->data[28]     = 0x00;                           /* FileSize */
					  ss->data[29]     = 0x00;
					  ss->data[30]     = 0x00;
					  ss->data[31]     = 0x00;
					  err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
					  if (err < 0) {
						  FS__fat_free(buffer);
						  return -1;
					  }
				  }
				  FS__fat_free(buffer);
				  return cluster;
			  }else
			  {
				  FS__CLIB_memcpy(ss->data,&Slot[stepcount-2], 32);
//				  dumpslot(ss->data);
				  err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
				  if (err < 0) {
					  FS__fat_free(buffer);
					  return -1;
				  }
			  }
		  }
		  stepcount--;
		  ss++;
	  }
  

	  
  }	  /***write entry***/
  FS__fat_free(buffer);
  return -2;
}
#endif

/* Allocates one entry of the specified open directory and fills it    */
/* with the directory entry D.                                         */
/* On success, returns the byte offset of the allocated entry.         */
/* On failure, returns a negative error code.                          */
/* Called by allocate_lfn_dir_entry (if not LFN entries are required), */
/* and, ifndef FATLFN, by fat_creat and fat_rename.                    */
static int allocate_sfn_dir_entry(int Idx, FS_u32 Unit,FS_u32 DirStart, FS_u32 DirSize, FS__fat_dentry_type *D, char *FileName,tLfnEntry *Slot)
{
  int Res, EntryOffset;
  char  Name[11];

  /* Get the name in FCB format, wildcards not allowed */
  if (fat_build_fcb_name(Name, FileName)) return -EINVAL;

  /* Search for a free directory entry, extending the dir if needed */
  if ((EntryOffset = find_empty_dir_entries(Idx,Unit,DirStart,DirSize,1,Name,Slot)) < 0) return EntryOffset;

  return EntryOffset;
}

void dumpslot(unsigned char *s)
{
	int i,j;
	printf("Slot:\n");
	for(i=0;i<2;i++)
	{
		for(j = 0; j < 16;j++)
		{
			printf("%02x ",s[j + i*16]);
		} 
		printf("\n");
	}
}

void nametoupper(char *pEntryName, const char *pOrgName) {

  FS_FARCHARPTR s;
  int i,len;

  s = (FS_FARCHARPTR)pOrgName;
  len=FS__CLIB_strlen(s);
  while (i < len) {
        pEntryName[i] = (char)FS__CLIB_toupper(*s);
    i++;
    s++;
  }
  pEntryName[len]=0;
}

/* Allocates 32-bytes directory entries of the specified open directory */
/* to store a long file name, using D as directory entry for the short  */
/* name alias.                                                          */
/* On success, returns the byte offset of the short name entry.         */
/* On failure, returns a negative error code.                           */
/* Called fat_creat and fat_rename.                                     */
static int allocate_lfn_dir_entries(int Idx, FS_u32 Unit, FS__fat_dentry_type *D,FS_u32 DirStart, FS_u32 DirSize, char *FileName, FWORD Hint)
{
  char      ShortName[11];
  tLfnEntry Slot[20];
  int       EntryOffset;
  int       LfnEntries;
  int       Res;
  int       k;
  char d_name[FS_DIRNAME_MAX];

  /* gen_short_fname already checks if the LFN is valid */
  /* if (!lfn_is_valid(FileName)) return -EINVAL;  */
  /* TODO: gen_short_fname cross reference without fat_ prefix */

//  nametoupper(d_name,FileName);

  Res = gen_short_fname(Idx,Unit,DirStart,DirSize,FileName, ShortName, Hint);
//printf("allocate_lfn_dir_entries 1 %d\n", Res);
  if (Res < 0) return Res;
  /* If Res == 0 no long name entries are required */
  if (Res == 0) return allocate_sfn_dir_entry(Idx,Unit,DirStart,DirSize, D, FileName,Slot);
  /* Generate a 32-byte Directory Entry for the short name alias */
  FS__CLIB_memcpy(D->data, ShortName, 11);
  /*empty slot*/
  FS__CLIB_memset(Slot,0, 20*32);
  /* Split the long name into 32-byte slots */
  Res = split_lfn(Slot, D, FileName, &LfnEntries);
//printf("allocate_lfn_dir_entries 2 %d\n", Res);
  if (Res) return Res;

  /* Search for NumSlots + 1 (LFN entries plus short name entry) */
  /* free directory entries, expanding the dir if needed.        */
  EntryOffset =  find_empty_dir_entries(Idx,Unit,DirStart,DirSize,LfnEntries + 1,ShortName,Slot);
//printf("allocate_lfn_dir_entries 3 %d\n", EntryOffset);
  if (EntryOffset < 0) return EntryOffset;

  return EntryOffset;

}

/*********************************************************************
*
*             _FS_fat_create_file
*
  Description:
  FS internal function. Create a file in the directory specified
  with DirStart. Do not call, if you have not checked before for 
  existing file with name pFileName.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number, which is passed to the device driver.
  pFileName   - File name. 
  DirStart    - Start of directory, where to create pDirName.
  DirSize     - Sector size of the directory starting at DirStart.
  
  Return value:
  >=0         - 1st cluster of the new file.
  ==-1        - An error has occured.
  ==-2        - Cannot create, because directory is full.
*/

static FS_i32 _FS_fat_create_file(int Idx, FS_u32 Unit,  const char *pFileName,
                                    FS_u32 DirStart, FS_u32 DirSize) {
  FS__fat_dentry_type s;
  FS_u32 i;
  FS_u32 dsec;
  FS_i32 cluster;
  int len;
  int err;
  FS_u16 val;
  char *buffer;
  int       Res;
// printf("create file!\n");
  Res = allocate_lfn_dir_entries(Idx,Unit,&s,DirStart,DirSize,pFileName,0);
//printf("_FS_fat_create_file %d\n", Res);
  return Res;

}


/*********************************************************************
*
*             Global functions section 1
*
**********************************************************************

  Functions in this section are global, but are used inside the FAT
  File System Layer only.
  
*/

int FS__fat_del_lfn (int Idx, FS_u32 Unit,FS_u32 DirStart, 
                        int cur_i,FS__fat_dentry_type *ss,int stepcount) {
  FS__fat_dentry_type *s;
  FS_u32 dstart;
  FS_u32 i;
  FS_u32 dsec;
  FS_u32 fatsize;
  int len;
  int err;
  int c;
  char *buffer;
  FS_u16 LongNameUtf16[FD32_LFNMAX];
  int NamePos = 0,k;
  char d_name[FS_DIRNAME_MAX];
  FBYTE Checksum = lfn_checksum(ss);
  int        Order    = 0;
  int cnt=0;
    /* Find directory */
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) 
  {
	  FS__fat_free(buffer);
	  return 0;
  }
//  printf("del cur_i:%d stepcmt:%d\n",cur_i,stepcount);
  /* Read directory */
    for (i = cur_i; i >=0; i--) 
    {
	    dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	    if (dsec == 0) 
	    {
		    FS__fat_free(buffer);
		    return 0;
	    }
	    err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	    if (err < 0) 
	    {
		    FS__fat_free(buffer);
		    return 0;
	    }
	    s = (FS__fat_dentry_type*)(buffer);
	    if(savecount==0)
	    {
		    s=s+stepcount;
	    }else
	    {
		    s=s+15;
	    }
	    while (1) 
	    {

		    if (s < (FS__fat_dentry_type*)(buffer)) 
		    {
			    break;  /* End of sector reached */
		    }
		    if (s->data[11] != 0x00) 
		    { /* not an empty entry */
			    if (s->data[0] != (unsigned char)0xe5) 
			    { /* not a deleted file */
#ifdef FS_USE_LONG_FILE_NAME
				    
				    if(s->data[11] == 0x0f)
				    {
					    // we have a long name entry
					    // cast this directory entry as a "windows" (LFN: LongFileName) entry
					    tLfnEntry *LfnSlot  = (tLfnEntry *) s->data;

					    if (++Order != (LfnSlot->Order & 0x1F))  
					    {
						    FS__fat_free(buffer);
						    return 0;
					    }
//					    dumpen(s);
//					    printf("checksum:%d LfnSlot->Checksum:%d\n",Checksum,LfnSlot->Checksum);
					    if (Checksum != LfnSlot->Checksum) 
					    {
						    FS__fat_free(buffer);
						    return 0;
					    }
					    if(LfnSlot->Order & 0x40) 
					    {
						    s->data[0]  = 0xe5;
						    s->data[11] = 0;
						    err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
						    if (err < 0) 
						    {
							    FS__fat_free(buffer);
							    return -1;
						    }
//						    printf("delete lfn finished!cnt:%d  LfnSlot->Order:%d\n",cnt,LfnSlot->Order);
						    FS__fat_free(buffer);
						    return 1;

					    }
					    s->data[0]  = 0xe5;
					    s->data[11] = 0;
					    err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);

					    if (err < 0) 
					    {
						    FS__fat_free(buffer);
						    return -1;
					    }
					    cnt++;

				    }
#endif 
				    
			    }
		    }  
		    
		    s--;
	    }
	    if(i<=0) break;

    }
    FS__fat_free(buffer);
    return 0;
}



/*********************************************************************
*
*             FS__fat_DeleteFileOrDir
*
  Description:
  FS internal function. Delete a file or directory.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number, which is passed to the device driver.
  pName       - File or directory name. 
  DirStart    - Start of directory, where to create pDirName.
  DirSize     - Sector size of the directory starting at DirStart.
  RmFile      - 1 => remove a file
                0 => remove a directory
  
  Return value:
  >=0         - Success. 
  <0          - An error has occured.
*/

#if 1
FS_i32 FS__fat_DeleteFileOrDir(int Idx, FS_u32 Unit,  const char *pName,
                                    FS_u32 DirStart, FS_u32 DirSize, char RmFile) {
  FS__fat_dentry_type *s;
  FS_u32 dsec;
  FS_u32 i;
  FS_u32 value;
  FS_u32 fatsize;
  FS_u32 filesize;
  FS_i32 len,len1;
  FS_i32 bytespersec;
  FS_i32 fatindex;
  FS_i32 fatsec;
  FS_i32 fatoffs;
  FS_i32 lastsec;
  FS_i32 curclst;
  FS_i32 todo;
  char *buffer;
  int fattype;
  int err;
  int err2;
  int lexp;
  int x;
  unsigned char a;
  unsigned char b;
#if (FS_FAT_NOFAT32==0)
  unsigned char c;
  unsigned char d;
#endif /* FS_FAT_NOFAT32==0 */

 int found=0;
// int savecount=0;
 int stepcount=0;
 int NamePos = 0,k;
 char d_name[FS_DIRNAME_MAX];
 int EntryOffset=0;
 int savestepcount=0;
 int saveentrycount=0;
 int ret=0;
 int getlongname=0;
 int cnt=0;
 char realname[12];
 int lowcase_flag=0,uppercase_flag=0;
// printf("delete file!\n");
 FS_u16 LongNameUtf16[FD32_LFNMAX];

 buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
 if (!buffer) {
	 return 0;
 }
 fattype = FS__fat_which_type(Idx, Unit);
#if (FS_FAT_NOFAT32!=0)
 if (fattype == 2) {
	 FS__fat_free(buffer);
	 return -1;
 }
#endif  /* FS_FAT_NOFAT32!=0 */
 fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
 if (fatsize == 0) {
	 fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
 }
 bytespersec = (FS_i32)FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec;

 len = FS__CLIB_strlen(pName);
  if(len<=12)
  {
	  ret=is_all_uppercase(pName,len);
	  if(ret==1)
	  {
		  FS__fat_make_realname(realname,pName);
		  uppercase_flag=1;
	  }
	  ret=is_all_lowercase(pName,len);
	  if(ret==1)
	  {
		  FS__fat_make_realname(realname,pName);
		  lowcase_flag=1;
	  }
  }

 found=0;
 savecount=0;
 /* Read directory to find entry*/
 for (i = 0; i < DirSize; i++) {
	 curclst = -1;
	 dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	 if (dsec == 0) {
		 FS__fat_free(buffer);
		 return -1;
	 }
	 err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	 if (err < 0) {
		 FS__fat_free(buffer);
		 return -1;
	 }

	 stepcount=0;
	 /* Scan for pName in the directory sector */
	 s = (FS__fat_dentry_type*) buffer;
	 while (1) 
	 {
		 getlongname=0;
		 if (s >= (FS__fat_dentry_type*)(buffer + bytespersec)) 
		 {
			 break;  /* End of sector reached */
		 }
		 if (s->data[11] != 0x00) 
		 { /* not an empty entry */
			 if (s->data[0] != (unsigned char)0xe5) 
			 { /* not a deleted file */

//				 if (s->data[11] != (FS_FAT_ATTR_READ_ONLY | FS_FAT_ATTR_HIDDEN | FS_FAT_ATTR_SYSTEM | FS_FAT_VOLUME_ID|FS_FAT_ATTR_DB)) {
				 if (s->data[11] == FS_FAT_ATTR_ARCHIVE || s->data[11] == FS_FAT_ATTR_DIRECTORY){
//					 printf("111i:%d --stepcount:%d\n",i,stepcount);
					 FS__CLIB_memset(savename,0,20*sizeof(tLfnold));
//					 dumpen(s);
					 ret=FS__fat_del_readback(Idx,Unit,DirStart,i,s,stepcount);
				    if(ret)
				    {

					    FS__CLIB_memset(d_name,0,255*sizeof(char));
					    cnt=0;
					    for (k = 0; k<savecount; k++) 
					    {
						    FS__CLIB_memcpy(&d_name[cnt], &savename[k].oldname,savename[k].count);
						    cnt = cnt + savename[k].count;
					    }
					    savecount=0;
				    }
				    else
				    {
					    FS__CLIB_memcpy(d_name,s->data,8);
					    d_name[8]='.';
					    FS__CLIB_memcpy(&d_name[9],&s->data[8],3);
					    d_name[12]='\0';
				    }

				    len1 = FS__CLIB_strlen(d_name);
				    if(len1>len)    len=len1;

//				    printf("pName:%s--d_name:%s\n",pName,d_name);
				    if(lowcase_flag==1)
				    {
					    ret = FS__CLIB_strncmp(d_name, pName, len-1);
					    c = FS__CLIB_strncmp(d_name, realname, len-1);
					    if(ret==0 || c==0)
						    c=0;
				    }
				    else if(uppercase_flag==1)
				    {
					    c = FS__CLIB_strncmp(d_name, realname, len-1);
				    }
				    else
				    {
					    c = FS__CLIB_strncmp(d_name, pName, len-1);
				    }

//				    printf("c:%d\n",c);
					 if (c == 0) 
					 {  /* Name does match */
						 if (s->data[11] != 0) 
						 {
//							 printf("delete match\n");
							 ret=FS__fat_del_lfn(Idx,Unit,DirStart,i, s,stepcount);
//							 if(ret)
//							 {
								 EntryOffset=i;
								 savestepcount=stepcount;
//								 printf("i:%d --stepcount:%d\n",i,stepcount);
								 found=1;
								 break;  /* Entry found */
//							 }
						 }
						 
					 }
				 }
			 }  //deleted file
		 }//empty
	 
		 s++;
		 stepcount++;
	 } //while
	 if(found==1)
	 {
		 break;  /* Entry found */
	 }
 }//for found entry

/*write entry */
 stepcount= 1;

 for (i = EntryOffset; i < DirSize; i++) 
 {
	 curclst = -1;
	 dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	 if (dsec == 0) 
	 {
		 FS__fat_free(buffer);
		 return -1;
	 }
	 err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	 if (err < 0) 
	 {
		 FS__fat_free(buffer);
		 return -1;
	 }

	 /* Scan for pName in the directory sector */
	 s = (FS__fat_dentry_type*) buffer;
	 s = s + savestepcount;
	 while (1) 
	 {
		 if (s >= (FS__fat_dentry_type*)(buffer + bytespersec)) 
		 {
			 break;  /* End of sector reached */
		 }
		 if (s->data[11] != 0x00) 
		 { /* not an empty entry */
			 if (s->data[0] != (unsigned char)0xe5) 
			 { /* not a deleted file */
				 if(stepcount==1)
//			    if (s->data[11] != (FS_FAT_ATTR_READ_ONLY | FS_FAT_ATTR_HIDDEN | FS_FAT_ATTR_SYSTEM | FS_FAT_VOLUME_ID))
				 {
//					 printf("delete short file suc!\n");
					 /* Entry has been found, delete directory entry */
					 s->data[0]  = 0xe5;
					 s->data[11] = 0;
					 err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
					 if (err < 0) 
					 {
						 FS__fat_free(buffer);
						 return -1;
					 }
					 /* Free blocks in FAT */
					 /*
					   For normal files, there are no more clusters freed than the entrie's filesize
					   does indicate. That avoids corruption of the complete media in case there is
					   no EOF mark found for the file (FAT is corrupt!!!). 
					   If the function should remove a directory, filesize if always 0 and cannot
					   be used for that purpose. To avoid running into endless loop, todo is set
					   to 0x0ffffff8L, which is the maximum number of clusters for FAT32.
					 */
					 if (RmFile) 
					 {
						 filesize  = s->data[28] + 0x100UL * s->data[29] + 0x10000UL * s->data[30] + 0x1000000UL * s->data[31];
						 todo      = filesize / (FS__FAT_aBPBUnit[Idx][Unit].SecPerClus * bytespersec);
						 value     = filesize % (FS__FAT_aBPBUnit[Idx][Unit].SecPerClus * bytespersec);
						 if (value != 0) {
							 todo++;
						 }
					 } 
					 else 
					 {
						 todo = (FS_i32)0x0ffffff8L;
					 }
					 curclst = s->data[26] + 0x100L * s->data[27] + 0x10000L * s->data[20] + 0x1000000L * s->data[21];
					 lastsec = -1;
					 /* Free cluster loop */
					 while (todo) {
						 if (fattype == 1) {
							 fatindex = curclst + (curclst / 2);    /* FAT12 */
						 }
#if (FS_FAT_NOFAT32==0)
						 else if (fattype == 2) {
							 fatindex = curclst * 4;               /* FAT32 */
						 }
#endif  /* FS_FAT_NOFAT32==0 */
						 else {
							 fatindex = curclst * 2;               /* FAT16 */
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
								 if (curclst & 1) {
									 buffer[fatoffs] &= 0x0f;
								 }
								 else {
									 buffer[fatoffs]  = 0x00;
								 }
								 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
								 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
								 lexp = (err < 0);
								 lexp = lexp || (err2 < 0);
								 if (lexp) {
									 FS__fat_free(buffer);
									 return -1;
								 }
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
								 b = buffer[0];
								 if (curclst & 1) {
									 buffer[0]  = 0x00;;
								 }
								 else {
									 buffer[0] &= 0xf0;
								 }
								 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)buffer);
								 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec + 1, (void*)buffer);
								 lexp = (err < 0);
								 lexp = lexp || (err2 < 0);
								 if (lexp) {
									 FS__fat_free(buffer);
									 return -1;
								 }
							 }
							 else {
								 a = buffer[fatoffs];
								 b = buffer[fatoffs + 1];
								 if (curclst & 1) {
									 buffer[fatoffs]     &= 0x0f;
									 buffer[fatoffs + 1]  = 0x00;
								 }
								 else {
									 buffer[fatoffs]      = 0x00;
									 buffer[fatoffs + 1] &= 0xf0;
								 }
								 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
								 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
								 lexp = (err < 0);
								 lexp = lexp || (err2 < 0);
								 if (lexp) {
									 FS__fat_free(buffer);
									 return -1;
								 }
							 }
							 if (curclst & 1) {
								 curclst = ((a & 0xf0) >> 4) + 16 * b;
							 }
							 else {
								 curclst = a + 256 * (b & 0x0f);
							 }
							 curclst &= 0x0fff;
							 if (curclst >= 0x0ff8) {
								 FS__fat_free(buffer);
								 return 0;
							 }
						 }
#if (FS_FAT_NOFAT32==0)
						 else if (fattype == 2) { /* FAT32 */
							 a = buffer[fatoffs];
							 b = buffer[fatoffs + 1];
							 c = buffer[fatoffs + 2];
							 d = buffer[fatoffs + 3] & 0x0f;
							 buffer[fatoffs]      = 0x00;
							 buffer[fatoffs + 1]  = 0x00;
							 buffer[fatoffs + 2]  = 0x00;
							 buffer[fatoffs + 3]  = 0x00;
							 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
							 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
							 lexp = (err < 0);
							 lexp = lexp || (err2 < 0);
							 if (lexp) {
								 FS__fat_free(buffer);
								 return -1;
							 }
							 curclst = a + 0x100 * b + 0x10000L * c + 0x1000000L * d;
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
							 buffer[fatoffs]     = 0x00;
							 buffer[fatoffs + 1] = 0x00;
							 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
							 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
							 lexp = (err < 0);
							 lexp = lexp || (err2 < 0);
							 if (lexp) {
								 FS__fat_free(buffer);
								 return -1;
							 }
							 curclst  = a + 256 * b;
							 curclst &= 0xffff;
							 if (curclst >= (FS_i32)0xfff8) {
								 FS__fat_free(buffer);
								 return 0;
							 }
						 }
						 todo--;
					 } /* Free cluster loop */
					 if (curclst > 0) {
						 FS__fat_free(buffer);
						 return curclst;
					 }
				 }  //step==1
			 }//delete
		 }//empty
		 s++;
		 stepcount--;
	 }//while
	 if(stepcount==0)
	 {
		 break; 
	 }
 }//for write entry

 FS__fat_free(buffer);
 return curclst;
}
#endif

/*********************************************************************
*
*             _FS__fat_DeleteEntry
*
  Description:
  FS internal function. Delete an entry.
  
  Parameters:
  Idx         - Index of device in the device information table                 referred by FS__pDevInfo.
  Unit        - Unit number, which is passed to the device driver.
  DirStart    - Start of directory, where to create pDirName.
  cur_sec     - Current sector number
  s           - Pointer of pointer to a entry
  pbuff       - Pointer to a buffer which can hold a sector
  entry_size  - The size of the entry
  
  Return value:
  >=0         - Success. 
  <0          - An error has occured.
*/

FS_i32 _FS__fat_DeleteEntry(int Idx, FS_u32 Unit, FS_u32 DirStart, FS_u32 cur_sec, 
    FS__fat_dentry_type *s, char *pbuff, FS_u32 entry_size)
{
    FS_u32 dsec;
    int ret;
    FS_u32 i;

    dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, cur_sec);
    if(dsec == 0) return -1;

    i= entry_size;
    while(i) {
        if ((unsigned int)s >= (unsigned int)pbuff) {
            s -> data[0] = 0xE5;
            s -> data[11] = 0;
            i--, s--;
        }
        else {
            ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)pbuff);
            if(ret < 0) return -1;

            cur_sec -= 1;
            dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, cur_sec);
            if(dsec == 0) return -1;
            ret = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)pbuff);
            if (ret < 0) return -1;
            s = (FS__fat_dentry_type*)&pbuff[FS_FAT_SEC_SIZE] - 1;
        }
    }

    ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)pbuff);
    return ret;
}


/*********************************************************************
*
*             _FS__fat_freeClustor
*
  Description:
  FS internal function. Delete an entry.
  
  Parameters:
  Idx         - Index of device in the device information table                 referred by FS__pDevInfo.
  Unit        - Unit number, which is passed to the device driver.
  s           - Pointer of pointer to a entry
  buffer      - Pointer to a secter buffer
  
  Return value:
  >=0         - Success. 
  <0          - An error has occured.
*/

FS_i32 _FS__fat_freeClustor(int Idx, FS_u32 Unit, FS__fat_dentry_type *s, char *buffer)
{
    FS_u32 fattype;
    FS_u32 fatsize;
    FS_u32 bytesperclust;
    FS_u32 bytespersec;
    FS_i32 i;
    FS_u32 curclst;
    FS_u32 lastsec;
    FS_u32 fatindex;
    FS_u32 fatsec;
    FS_u32 fatoffs;
    FS_i32 ret;

    fattype = FS__fat_which_type(Idx, Unit);
#if (FS_FAT_NOFAT32!=0)
    if (fattype == 2) {        return -1;
    }
#endif  /* FS_FAT_NOFAT32!=0 */
    fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
    if (fatsize == 0) {
        fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
    }
    bytespersec = FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec;
    bytesperclust = FS__FAT_aBPBUnit[Idx][Unit].SecPerClus * bytespersec;

    curclst = s->data[26] + 0x100L * s->data[27] + 0x10000L * s->data[20] + 0x1000000L * s->data[21];
    if (s -> data[11] & FS_FAT_ATTR_DIRECTORY) {
        /* Free a directory clustor */
        i= (FS_i32)0x0ffffff8L;
    }
    else {
        unsigned int m;

        m = s->data[28] + 0x100UL * s->data[29] + 0x10000UL * s->data[30] + 0x1000000UL * s->data[31];        i = (FS_i32)((m + bytesperclust -1) / bytesperclust);
    }

    lastsec = -1;
    while (i)
    {
        unsigned int a, b, c, d;

        if (fattype == 1) {/* FAT12 */
            fatindex = curclst + (curclst / 2);     
        }
        else if (fattype == 2) {/* FAT32 */
            fatindex = curclst * 4;
        }
        else {/* FAT16 */
            fatindex = curclst * 2;
        }

        fatsec = FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + (fatindex / bytespersec);
        fatoffs = fatindex % bytespersec;
        if (fatsec != lastsec) {
            ret = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);            if (ret < 0) {
                ret = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
                if (ret < 0) return -1;
                /* Try to repair original FAT sector with contents of copy */
                FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
            }
            lastsec = fatsec;
        }
        if (fattype == 1)
        {/* FAT12 */
            if (fatoffs == (bytespersec - 1))
            {
                a = buffer[fatoffs];
                if (curclst & 1)    buffer[fatoffs] &= 0x0f;
                else                buffer[fatoffs]  = 0x00;
                ret  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
                if (ret < 0) return -1;
                ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
                if (ret < 0) return -1;
                ret = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)buffer);
                if (ret < 0) {
                    ret = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec + 1, (void*)buffer);
                    if (ret < 0) return -1;
                    /* Try to repair original FAT sector with contents of copy */
                    FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)buffer);
                }
                lastsec = fatsec + 1;
                b = buffer[0];
                if (curclst & 1)    buffer[0]  = 0x00;
                else                buffer[0] &= 0xf0;
                ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)buffer);
                if (ret < 0) return -1;
                ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec + 1, (void*)buffer);
                if (ret < 0) return -1;
            }
            else
            {
                a = buffer[fatoffs];
                b = buffer[fatoffs + 1];
                if (curclst & 1) {
                    buffer[fatoffs] &= 0x0f;
                    buffer[fatoffs + 1] = 0x00;
                }
                else {
                    buffer[fatoffs] = 0x00;
                    buffer[fatoffs + 1] &= 0xf0;
                }
                ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
                if (ret < 0) return -1;
                ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
                if (ret < 0) return -1;
            }

            if (curclst & 1)    curclst = ((a & 0xf0) >> 4) + 16 * b;
            else                curclst = a + 256 * (b & 0x0f);

            curclst &= 0x0fff;
            if (curclst >= 0x0ff8) return 0;
        }
#if (FS_FAT_NOFAT32==0)
        else if (fattype == 2)
        {/* FAT32 */            a = buffer[fatoffs];
            b = buffer[fatoffs + 1];
            c = buffer[fatoffs + 2];
            d = buffer[fatoffs + 3] & 0x0f;
            buffer[fatoffs]      = 0x00;
            buffer[fatoffs + 1]  = 0x00;
            buffer[fatoffs + 2]  = 0x00;
            buffer[fatoffs + 3]  = 0x00;
            ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
            if (ret < 0) return -1;
            ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
            if (ret < 0) return -1;

            curclst = a + 0x100 * b + 0x10000L * c + 0x1000000L * d;
            curclst &= 0x0fffffffL;
            if (curclst >= 0x0ffffff8L) return 0;
        }
#endif /* FS_FAT_NOFAT32==0 */
        else
        {
            a = buffer[fatoffs];
            b = buffer[fatoffs + 1];
            buffer[fatoffs]     = 0x00;
            buffer[fatoffs + 1] = 0x00;
            ret  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
            if (ret < 0) return -1;
            ret = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
            if (ret < 0) return -1;
            curclst  = a + 256 * b;
            curclst &= 0xffff;
            if (curclst >= 0xfff8) return 0;
        }
        i--;
    } /* Free cluster loop */

    return 0;
}


/*********************************************************************
*
*             FS__fat_DeleteFile
*
  Description:
  FS internal function. Delete an file.
  
  Parameters:
  Idx         - Index of device in the device information table                 referred by FS__pDevInfo.
  Unit        - Unit number, which is passed to the device driver.
  pName       - Pointer to a file name
  DirStart    - Start of directory, where to create pDirName.
  DirSize     - Sector size of the directory starting at DirStart.
  
  Return value:
  >=0         - Success. 
  <0          - An error has occured.
*/

FS_i32 FS__fat_DeleteFile(int Idx, FS_u32 Unit,  const char *pName,
        FS_u32 DirStart, FS_u32 DirSize)
{
    FS__fat_dentry_type *s;
    FS__fat_dentry_type dentry;
    FS_u32 dsec;
    FS_u32 i;
    FS_u32 entry_size;
    char *buffer;
    char d_name[FS_DIRNAME_MAX];
    char realname[12];
    int j, len;
    int cmp_short_name;
    int ret;

    buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
    if (!buffer) {
        return 0;
    }

    cmp_short_name= 0;
    len = FS__CLIB_strlen(pName);
    if (len < 12) {
        //Check whether all charactor are asic code
        while (i < len) {
            if (pName[i++] > 0x7F) break;
        }
        if (i >= len) {
            cmp_short_name= 1;
            FS__fat_make_realname(realname, pName);
        }
    }
    /* Read directory to find entry*/
    for (i = 0; i < DirSize; i++)
    {
        dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
        if (dsec == 0) {
            FS__fat_free(buffer);
            return -1;        }
        ret = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
        if (ret < 0) {
            FS__fat_free(buffer);
            return -1;
        }
        /* Scan for pName in the directory sector */
        s = (FS__fat_dentry_type*)buffer;
        j= 0;
        while (j < FS_FAT_SEC_SIZE/FS_FAT_DENTRY_SIZE) 
        {
            if (s -> data[0] == 00) {
                /* Entrys behind this entry are empty, if not, we don't care */
                FS__fat_free(buffer);
                return -1;
            }

            if ((s->data[11] & (FS_FAT_ATTR_ARCHIVE | FS_FAT_ATTR_DIRECTORY)) || 
                ((s -> data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME))
		    { /* not a deleted file && is file or directory*/
                len = 0;
                if((s -> data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME) {
                    len = _FS_fat_longname_read(Idx, Unit, DirStart, &i, DirSize, &s, d_name, buffer);
                    if (len >= 0) {
                        if ((unsigned int)s >= (unsigned int)buffer) {
                            j = ((unsigned int)s - (unsigned int)buffer)/FS_FAT_DENTRY_SIZE;
                        }
                        else {
                            j = -1;
                        }
                    }
                    else {
                        FS__fat_free(buffer);
                        return -1;
                    }
                }

                ret = -1;
                if (cmp_short_name == 0) {
                    entry_size = 1;
                    FS__CLIB_memcpy(d_name, (char*)&s -> data[0], 8);
                    d_name[8]='.';
                    FS__CLIB_memcpy(&d_name[9], (char*)&s->data[8], 3);
                    ret = FS__CLIB_strncmp(d_name, realname, 12);
                }
                else {
                    if (len > 0) {
                        entry_size = len;
                        ret = FS__CLIB_strcasecmp(d_name, pName);
                    }
                }

                if (ret == 0) {
                    /* Entry found && isn't a directory */
                    if ((s->data[11] & FS_FAT_ATTR_DIRECTORY) == 0) {
                        FS__CLIB_memcpy((char*)&dentry, (char*)s, sizeof(FS__fat_dentry_type));
                        ret = _FS__fat_DeleteEntry(Idx, Unit, DirStart, i, s, buffer, entry_size);
                        if (ret < 0) {
                            FS__fat_free(buffer);
                            return -1;
                        }
                        ret = _FS__fat_freeClustor(Idx, Unit, s, buffer);
                        FS__fat_free(buffer);
                        return ret;
                    }
                }
            }
            s++, j++;
        }
    }

    FS__fat_free(buffer);
    return 0;
}



#if 0
FS_i32 FS__fat_DeleteFileOrDir(int Idx, FS_u32 Unit,  const char *pName,
                                    FS_u32 DirStart, FS_u32 DirSize, char RmFile) {
  FS__fat_dentry_type *s;
  FS_u32 dsec;
  FS_u32 i;
  FS_u32 value;
  FS_u32 fatsize;
  FS_u32 filesize;
  FS_i32 len;
  FS_i32 bytespersec;
  FS_i32 fatindex;
  FS_i32 fatsec;
  FS_i32 fatoffs;
  FS_i32 lastsec;
  FS_i32 curclst;
  FS_i32 todo;
    char *buffer;
    char d_name[FS_DIRNAME_MAX];
    char realname[12];
    int non_asic_code;
    int ret;

  int fattype;
  int err;
  int err2;
  int lexp;
  int x;
  unsigned char a;
  unsigned char b;
#if (FS_FAT_NOFAT32==0)
  unsigned char c;
  unsigned char d;
#endif /* FS_FAT_NOFAT32==0 */

 int found=0;
 int savecount=0;
 int stepcount=0;
 int NamePos = 0,k;
 int EntryOffset=0;
 int savestepcount=0;
 int saveentrycount=0;


    buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
    if (!buffer) {
        return 0;
    }
    fattype = FS__fat_which_type(Idx, Unit);
#if (FS_FAT_NOFAT32!=0)
    if (fattype == 2) {
        FS__fat_free(buffer);
        return -1;
    }
#endif  /* FS_FAT_NOFAT32!=0 */
    fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
    if (fatsize == 0) {
        fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
    }
    bytespersec = (FS_i32)FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec;

    non_asic_code = 0;
    len = FS__CLIB_strlen(pName);
    if(len < 12)
    {
        i= 0;
        while (i < len) {
            realname = 
        }
    }

    /* Read directory to find entry*/
    for (i = 0; i < DirSize; i++) {
        curclst = -1;
        dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
        if (dsec == 0) {
            FS__fat_free(buffer);
            return -1;
        }
        ret = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
        if (ret < 0) {
            FS__fat_free(buffer);
            return -1;
        }

        /* Scan for pName in the directory sector */
        s = (FS__fat_dentry_type*) buffer;
        len= 0;
        while (len < FS_FAT_SEC_SIZE/FS_FAT_DENTRY_SIZE) 
        {
            if (s -> data[0] == 00) {
                /* Entrys behind this entry are empty, if not, we don't care */
                FS__fat_free(buffer);
                return -1;
            }

            if ((s->data[11] & (FS_FAT_ATTR_ARCHIVE | FS_FAT_ATTR_DIRECTORY)) || 
                ((s -> data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME))
		    { /* not a deleted file && is file or directory*/
                if((s -> data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME) {
                    ret = _FS_fat_longname_read(Idx, Unit, DirStart, &i, DirSize, &s, d_name, buffer);
                    if (ret >= 0) {
                        if ((unsigned int)s >= (unsigned int)buffer) {
                            len = ((unsigned int)s - (unsigned int)buffer)/FS_FAT_DENTRY_SIZE;
                        }
                        else {
                            len = -1;
                        }
                    }
                    else {
                        FS__fat_free(buffer);
                        return -1;
                    }
                }

            }

            if (s->data[11] != 0x00) 
            { /* not an empty entry */
                if (s->data[0] != (unsigned char)0xe5) 
                { /* not a deleted file */
#ifdef FS_USE_LONG_FILE_NAME
				 if(s->data[11] == 0x0f)
				 {
					 NamePos = 0;
					 FS__CLIB_memset(LongNameUtf16,0,255 * 2);
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
					 utf16to8(LongNameUtf16, d_name);
					 FS__CLIB_memcpy(&savename[savecount].oldname, d_name,13);
					 savecount++;
					 if(savecount==1)
					 {
						 EntryOffset = i;
						 savestepcount = stepcount;
					 }
				 }
#endif 
//				 if (s->data[11] != (FS_FAT_ATTR_READ_ONLY | FS_FAT_ATTR_HIDDEN | FS_FAT_ATTR_SYSTEM | FS_FAT_VOLUME_ID| FS_FAT_ATTR_DB))
//				 {
				 if (s->data[11] == FS_FAT_ATTR_ARCHIVE || s->data[11] == FS_FAT_ATTR_DIRECTORY){
#ifdef FS_USE_LONG_FILE_NAME
					 if(savecount >0)
					 {
						 for (k = 0; k<savecount; k++) 
						 {
							 FS__CLIB_memcpy(&d_name[k*13], &savename[savecount-1-k].oldname,13);
						 }

						 saveentrycount=savecount;
						 savecount =0;
					 }
#endif 

					 c = FS__CLIB_strncmp(d_name, pName, len);
					 if (c == 0) 
					 {  /* Name does match */
						 if (s->data[11] != 0) 
						 {
							 found=1;
							 break;  /* Entry found */
						 }
						 
					 }
				 }
			 }  //deleted file
		 }//empty
	 
		 s++;
		 stepcount++;
	 } //while
	 if(found==1)
	 {
		 break;  /* Entry found */
	 }
 }//for found entry

/*write entry */
 stepcount= saveentrycount+1;

 for (i = EntryOffset; i < DirSize; i++) 
 {
	 curclst = -1;
	 dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	 if (dsec == 0) 
	 {
		 FS__fat_free(buffer);
		 return -1;
	 }
	 err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	 if (err < 0) 
	 {
		 FS__fat_free(buffer);
		 return -1;
	 }

	 /* Scan for pName in the directory sector */
	 s = (FS__fat_dentry_type*) buffer;
	 s = s + savestepcount;
	 while (1) 
	 {
		 if (s >= (FS__fat_dentry_type*)(buffer + bytespersec)) 
		 {
			 break;  /* End of sector reached */
		 }
		 if (s->data[11] != 0x00) 
		 { /* not an empty entry */
			 if (s->data[0] != (unsigned char)0xe5) 
			 { /* not a deleted file */
#ifdef FS_USE_LONG_FILE_NAME
				 if(s->data[11] == 0x0f)
				 {
					 /* Entry has been found, delete directory entry */
					 s->data[0]  = 0xe5;
					 s->data[11] = 0;
					 err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
					 if (err < 0) 
					 {
						 FS__fat_free(buffer);
						 return -1;
					 }
				 }
#endif 
				 if(stepcount==1)
//			    if (s->data[11] != (FS_FAT_ATTR_READ_ONLY | FS_FAT_ATTR_HIDDEN | FS_FAT_ATTR_SYSTEM | FS_FAT_VOLUME_ID))
				 {
					 /* Entry has been found, delete directory entry */
					 s->data[0]  = 0xe5;
					 s->data[11] = 0;
					 err = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
					 if (err < 0) 
					 {
						 FS__fat_free(buffer);
						 return -1;
					 }
					 /* Free blocks in FAT */
					 /*
					   For normal files, there are no more clusters freed than the entrie's filesize
					   does indicate. That avoids corruption of the complete media in case there is
					   no EOF mark found for the file (FAT is corrupt!!!). 
					   If the function should remove a directory, filesize if always 0 and cannot
					   be used for that purpose. To avoid running into endless loop, todo is set
					   to 0x0ffffff8L, which is the maximum number of clusters for FAT32.
					 */
					 if (RmFile) 
					 {
						 filesize  = s->data[28] + 0x100UL * s->data[29] + 0x10000UL * s->data[30] + 0x1000000UL * s->data[31];
						 todo      = filesize / (FS__FAT_aBPBUnit[Idx][Unit].SecPerClus * bytespersec);
						 value     = filesize % (FS__FAT_aBPBUnit[Idx][Unit].SecPerClus * bytespersec);
						 if (value != 0) {
							 todo++;
						 }
					 } 
					 else 
					 {
						 todo = (FS_i32)0x0ffffff8L;
					 }
					 curclst = s->data[26] + 0x100L * s->data[27] + 0x10000L * s->data[20] + 0x1000000L * s->data[21];
					 lastsec = -1;
					 /* Free cluster loop */
					 while (todo) {
						 if (fattype == 1) {
							 fatindex = curclst + (curclst / 2);    /* FAT12 */
						 }
#if (FS_FAT_NOFAT32==0)
						 else if (fattype == 2) {
							 fatindex = curclst * 4;               /* FAT32 */
						 }
#endif  /* FS_FAT_NOFAT32==0 */
						 else {
							 fatindex = curclst * 2;               /* FAT16 */
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
								 if (curclst & 1) {
									 buffer[fatoffs] &= 0x0f;
								 }
								 else {
									 buffer[fatoffs]  = 0x00;
								 }
								 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
								 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
								 lexp = (err < 0);
								 lexp = lexp || (err2 < 0);
								 if (lexp) {
									 FS__fat_free(buffer);
									 return -1;
								 }
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
								 b = buffer[0];
								 if (curclst & 1) {
									 buffer[0]  = 0x00;;
								 }
								 else {
									 buffer[0] &= 0xf0;
								 }
								 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec + 1, (void*)buffer);
								 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec + 1, (void*)buffer);
								 lexp = (err < 0);
								 lexp = lexp || (err2 < 0);
								 if (lexp) {
									 FS__fat_free(buffer);
									 return -1;
								 }
							 }
							 else {
								 a = buffer[fatoffs];
								 b = buffer[fatoffs + 1];
								 if (curclst & 1) {
									 buffer[fatoffs]     &= 0x0f;
									 buffer[fatoffs + 1]  = 0x00;
								 }
								 else {
									 buffer[fatoffs]      = 0x00;
									 buffer[fatoffs + 1] &= 0xf0;
								 }
								 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
								 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
								 lexp = (err < 0);
								 lexp = lexp || (err2 < 0);
								 if (lexp) {
									 FS__fat_free(buffer);
									 return -1;
								 }
							 }
							 if (curclst & 1) {
								 curclst = ((a & 0xf0) >> 4) + 16 * b;
							 }
							 else {
								 curclst = a + 256 * (b & 0x0f);
							 }
							 curclst &= 0x0fff;
							 if (curclst >= 0x0ff8) {
								 FS__fat_free(buffer);
								 return 0;
							 }
						 }
#if (FS_FAT_NOFAT32==0)
						 else if (fattype == 2) { /* FAT32 */
							 a = buffer[fatoffs];
							 b = buffer[fatoffs + 1];
							 c = buffer[fatoffs + 2];
							 d = buffer[fatoffs + 3] & 0x0f;
							 buffer[fatoffs]      = 0x00;
							 buffer[fatoffs + 1]  = 0x00;
							 buffer[fatoffs + 2]  = 0x00;
							 buffer[fatoffs + 3]  = 0x00;
							 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
							 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
							 lexp = (err < 0);
							 lexp = lexp || (err2 < 0);
							 if (lexp) {
								 FS__fat_free(buffer);
								 return -1;
							 }
							 curclst = a + 0x100 * b + 0x10000L * c + 0x1000000L * d;
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
							 buffer[fatoffs]     = 0x00;
							 buffer[fatoffs + 1] = 0x00;
							 err  = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsec, (void*)buffer);
							 err2 = FS__lb_write(FS__pDevInfo[Idx].devdriver, Unit, fatsize + fatsec, (void*)buffer);
							 lexp = (err < 0);
							 lexp = lexp || (err2 < 0);
							 if (lexp) {
								 FS__fat_free(buffer);
								 return -1;
							 }
							 curclst  = a + 256 * b;
							 curclst &= 0xffff;
							 if (curclst >= (FS_i32)0xfff8) {
								 FS__fat_free(buffer);
								 return 0;
							 }
						 }
						 todo--;
					 } /* Free cluster loop */
					 if (curclst > 0) {
						 FS__fat_free(buffer);
						 return curclst;
					 }
				 }  //step==1
			 }//delete
		 }//empty
		 s++;
		 stepcount--;
	 }//while
	 if(stepcount==0)
	 {
		 break; 
	 }
 }//for write entry

 FS__fat_free(buffer);
 return curclst;
}
#endif

/*********************************************************************
*
*             FS__fat_make_realname
*
  Description:
  FS internal function. Convert a given name to the format, which is
  used in the FAT directory.
  
  Parameters:
  pOrgName    - Pointer to name to be translated
  pEntryName  - Pointer to a buffer for storing the real name used
                in a directory.

  Return value:
  None.
*/

void FS__fat_make_realname(char *pEntryName, const char *pOrgName) {
  FS_FARCHARPTR ext;
  FS_FARCHARPTR s;
  int i;

  s = (FS_FARCHARPTR)pOrgName;
  ext = (FS_FARCHARPTR) FS__CLIB_strchr(s, '.');
  if (!ext) {
    ext = &s[FS__CLIB_strlen(s)];
  }
  i=0;
  while (1) {
    if (s >= ext) {
      break;  /* '.' reached */
    }
    if (i >= 8) {
      break;  /* If there is no '.', this is the end of the name */
    }
    if (*s == (char)0xe5) {
      pEntryName[i] = 0x05;
    }
    else {
      pEntryName[i] = (char)FS__CLIB_toupper(*s);
    }
    i++;
    s++;
  }
  while (i < 8) {
    /* Fill name with spaces*/
    pEntryName[i] = ' ';
    i++;
  }
  if (*s == '.') {
    s++;
  }

  pEntryName[8]='.';
  i++;

  while (i < 12) {
    if (*s != 0) {
      if (*s == (char)0xe5) {
        pEntryName[i] = 0x05;
      }
      else {
        pEntryName[i] = (char)FS__CLIB_toupper(*s);
      }
      s++;
    }
    else {
      pEntryName[i] = ' ';
    }
    i++;
  }
  pEntryName[12]=0;
}


/*********************************************************************
*
*             FS__fat_find_dir
*
  Description:
  FS internal function. Find the directory with name pDirName in directory
  DirStart.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  pDirName    - Directory name; if zero, return the root directory.
  DirStart    - 1st cluster of the directory.
  DirSize     - Sector (not cluster) size of the directory.
 
  Return value:
  >0          - Directory found. Value is the first cluster of the file.
  ==0         - An error has occured.
*/

FS_u32 FS__fat_find_dir(int Idx, FS_u32 Unit, char *pDirName, FS_u32 DirStart, 
                        FS_u32 DirSize) {
    FS__fat_dentry_type *s;
    FS_u32 dstart;
    FS_u32 i;
    FS_u32 dsec;
    FS_u32 fatsize;
    int len;
    int ret;
    char *buffer;
    char d_name[FS_DIRNAME_MAX];
    char realname[8];
    int cmp_long_name;      //Use long name to compare
    int longname;           //Long name flag

//printf("find dir!\n");
    if (pDirName == 0) {
        /* Return root directory */
        if (FS__FAT_aBPBUnit[Idx][Unit].FATSz16) {
            fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
            dstart  = FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + FS__FAT_aBPBUnit[Idx][Unit].NumFATs * fatsize;
        }
        else {
            fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
            dstart  = FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + FS__FAT_aBPBUnit[Idx][Unit].NumFATs * fatsize
                + (FS__FAT_aBPBUnit[Idx][Unit].RootClus - 2) * FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
        }
        return dstart;
    }

    buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
    if (!buffer) {
        return 0;
    }

    cmp_long_name = 0;
    realname[0] = '\0';
    len = FS__CLIB_strlen(pDirName);
    if (len <= 8) {
        i= 0;
        while (i < len) {
            realname[i] = (char)FS__CLIB_toupper((int)pDirName[i++]);
        }
        while (i < 8) {
            realname[i++] = 0x20;
        }
    }
    else
        cmp_long_name= 1;

    /* Read directory */
    for (i = 0; i < DirSize; i++)
    {
        dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
//printf("dsec %d; i %d; s %d\n", dsec, i, DirStart);
        if (dsec == 0) {
            FS__fat_free(buffer);
            return 0;
        }
        ret = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
        if (ret < 0) {
            FS__fat_free(buffer);
            return 0;
        }
        s = (FS__fat_dentry_type*)buffer;

        len= 0;
        while (len < FS_FAT_SEC_SIZE/FS_FAT_DENTRY_SIZE)
        {
            if (s >= (FS__fat_dentry_type*)&buffer[FS_FAT_SEC_SIZE]) {
                break;  /* End of sector reached */
            }

            if(s -> data[0] == 0) {
                /* Following are empty entry */
                FS__fat_free(buffer);
                return 0;
            }

		    if (s->data[0] != 0xe5)
            if((s->data[11] & FS_FAT_ATTR_DIRECTORY) || (s->data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME)
		    { /* not a deleted entry && is a directory or long name (?)*/
                longname = 0;
                if ((s->data[11] & FS_FAT_ATTR_LFNAME) == FS_FAT_ATTR_LFNAME) {
                    /*A long name entry */
                    ret = _FS_fat_longname_read(Idx, Unit, DirStart, &i, DirSize, &s, d_name, buffer);
                    if (ret >=0) {
                        if ((unsigned int)s >= (unsigned int)buffer) {
                            len = ((unsigned int)s - (unsigned int)buffer)/FS_FAT_DENTRY_SIZE;
                        }
                        else {
                            len = -1;
                        }
//                        len = ((unsigned int)s - (unsigned int)buffer)/FS_FAT_DENTRY_SIZE;
                        if(s->data[11] & FS_FAT_ATTR_DIRECTORY) longname = 1;
                    }
                    else {
                        /* Encountered a bad entry */
                        FS__fat_free(buffer);
//printf("a bad entry\n");
                        return 0;
                    }
                }

                ret = -1;
                if (cmp_long_name) {
                    if (longname) {
                        ret = FS__CLIB_strcasecmp(d_name, pDirName);
                    }
                }
                else {
                    if (s->data[11] & FS_FAT_ATTR_DIRECTORY) {
                        FS__CLIB_memcpy(d_name, (char*)&s->data[0], 8);
                        ret = FS__CLIB_strncasecmp(d_name, realname, 8);
                    }
                }
//printf("d_name: %s\n", d_name);
                if (ret == 0) {
                    dstart  = (FS_u32)s->data[26];
                    dstart += (FS_u32)0x100UL * s->data[27];
                    dstart += (FS_u32)0x10000UL * s->data[20];
                    dstart += (FS_u32)0x1000000UL * s->data[21];

                    FS__fat_free(buffer);
                    return dstart;
                }
            }
            s++, len++;
        } //end while
    }

    FS__fat_free(buffer);
    return 0;
}


void dumpen(unsigned char *s)
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

int FS__fat_del_readback(int Idx, FS_u32 Unit,FS_u32 DirStart, 
                        int cur_i,FS__fat_dentry_type *ss,int stepcount) {
  FS__fat_dentry_type *s;
  FS_u32 dstart;
  FS_u32 i;
  FS_u32 dsec;
  FS_u32 fatsize;
  int len;
  int err;
  int c;
  char *buffer;
  FS_u16 LongNameUtf16[FD32_LFNMAX];
  int NamePos = 0,k;
  char d_name[FS_DIRNAME_MAX];

  FBYTE Checksum = lfn_checksum(ss);
  int        Order    = 0;
  int        sector_end    = 0;

    /* Find directory */
  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) 
  {
	  FS__fat_free(buffer);
	  return 0;
  }
//  printf("cur_i:%d stepcmt:%d\n",cur_i,stepcount);
  /* Read directory */
    for (i = cur_i; i >=0; i--) 
    {
	    dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
	    if (dsec == 0) 
	    {
		    FS__fat_free(buffer);
		    return 0;
	    }
	    err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
	    if (err < 0) 
	    {
		    FS__fat_free(buffer);
		    return 0;
	    }
	    s = (FS__fat_dentry_type*)(buffer);
	    if(savecount==0)
	    {
		    s=s+stepcount;
		    if(stepcount==0 && sector_end == 1)
		    {
			    s=s+15;
			    sector_end =0;
		    }
	    }else
	    {
		    s=s+15;
	    }
	    while (1) 
	    {

		    if (s < (FS__fat_dentry_type*)(buffer)) 
		    {
			    break;  /* End of sector reached */
		    }
		    if (s->data[11] != 0x00) 
		    { /* not an empty entry */
			    if (s->data[0] != (unsigned char)0xe5) 
			    { /* not a deleted file */
#ifdef FS_USE_LONG_FILE_NAME
				    
				    if(s->data[11] == 0x0f)
				    {
//					    dumpen(s);

					    NamePos = 0;
					    FS__CLIB_memset(LongNameUtf16,0,255 * 2);
					    // we have a long name entry
					    // cast this directory entry as a "windows" (LFN: LongFileName) entry
					    tLfnEntry *LfnSlot  = (tLfnEntry *) s->data;

					    if (++Order != (LfnSlot->Order & 0x1F))  
					    {
						    FS__fat_free(buffer);
						    return 0;
					    }
//					    printf("checksum:%d LfnSlot->Checksum:%d\n",Checksum,LfnSlot->Checksum);
					    if (Checksum != LfnSlot->Checksum) 
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
					    int ret;
					    ret=UniToUtf8(LongNameUtf16,d_name);
					    udelay(1);
//					    printf("ret:%d\n",ret);
//				    utf16to8(LongNameUtf16, d_name);
					    savename[savecount].count=ret;
					    FS__CLIB_memcpy(&savename[savecount].oldname, d_name,ret);
					    savecount++;
					    if(LfnSlot->Order & 0x40) 
					    {

//					    printf("longname:%s\n",d_name);
						    FS__fat_free(buffer);
						    return 1;

					    }
				    }
#endif 
				    
			    }
		    }  
		    
		    s--;
	    }
	    sector_end    = 1;
	    if(i<=0) break;

    }
    FS__fat_free(buffer);
    return 0;
}


/*********************************************************************
*
*             FS__fat_dir_realsec
*
  Description:
  FS internal function. Translate a directory relative sector number
  to a real sector number on the media.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  DirStart    - 1st cluster of the directory. This is zero to address 
                the root directory. 
  DirSec      - Sector in the directory.
 
  Return value:
  >0          - Directory found. Value is the sector number on the media.
  ==0         - An error has occured.
*/

FS_u32 FS__fat_dir_realsec(int Idx, FS_u32 Unit, FS_u32 DirStart, FS_u32 DirSec) {
  FS_u32 rootdir;
  FS_u32 rsec;
  FS_u32 dclust;
  FS_u32 fatsize;
  int fattype;
  int lexp;
  unsigned char secperclus;

  fattype = FS__fat_which_type(Idx, Unit);
  lexp = (0 == DirStart);
  lexp = lexp && (fattype != 2);
  if (lexp) {
    /* Sector in FAT12/FAT16 root directory */
    rootdir = FS__fat_find_dir(Idx, Unit, 0, 0, 0);
    rsec = rootdir + DirSec;
  }
  else {
    fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz16;
    if (fatsize == 0) {
      fatsize = FS__FAT_aBPBUnit[Idx][Unit].FATSz32;
    }
    secperclus = FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
    dclust = DirSec / secperclus;
    if (0 == DirStart) {
      /* FAT32 root directory */
//      if(dclust > 0)
//      {
//        rsec = FS__fat_diskclust(Idx, Unit, 0, dclust);
//        if (rsec == 0) {
//          return 0;
//        }
//        rsec += FS__FAT_aBPBUnit[Idx][Unit].RootClus;   //?
//      }
//      else
        rsec = FS__FAT_aBPBUnit[Idx][Unit].RootClus;
    }
    else{
      rsec = FS__fat_diskclust(Idx, Unit, DirStart, dclust);
      if (rsec == 0) {
        return 0;
      }
    }
//printf("rsec %d\n", rsec);
    rsec -= 2;  //?
    rsec *= secperclus;
    rsec += FS__FAT_aBPBUnit[Idx][Unit].RsvdSecCnt + FS__FAT_aBPBUnit[Idx][Unit].NumFATs * fatsize;
    rsec += ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[Idx][Unit].RootEntCnt) * FS_FAT_DENTRY_SIZE) / FS_FAT_SEC_SIZE;
    rsec += (DirSec % secperclus);
  }
  return rsec;
}


/*********************************************************************
*
*             FS__fat_dirsize
*
  Description:
  FS internal function. Return the sector size of the directory 
  starting at DirStart.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  DirStart    - 1st cluster of the directory. This is zero to address 
                the root directory. 
 
  Return value:
  >0          - Sector (not cluster) size of the directory.
  ==0         - An error has occured.
*/

FS_u32 FS__fat_dir_size(int Idx, FS_u32 Unit, FS_u32 DirStart) {
  FS_u32 dsize;
  FS_i32 value;

  if (DirStart == 0) {
    /* For FAT12/FAT16 root directory, the size can be found in BPB */
    dsize = ((FS_u32)((FS_u32)FS__FAT_aBPBUnit[Idx][Unit].RootEntCnt)
            * FS_FAT_DENTRY_SIZE) / ((FS_u32)FS__FAT_aBPBUnit[Idx][Unit].BytesPerSec);
    if (dsize == 0) {
      /* Size in BPB is 0, so it is a FAT32 (FAT32 does not have a real root dir) */
      value = FS__fat_FAT_find_eof(Idx, Unit, FS__FAT_aBPBUnit[Idx][Unit].RootClus, &dsize);
      if (value < 0) {
        dsize = 0;
      }
      else {
        dsize *= FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
      }
    }
  }
  else {
    /* Calc size of a sub-dir */
    value = FS__fat_FAT_find_eof(Idx, Unit, DirStart, &dsize);
    if (value < 0) {
      dsize = 0;
    }
    else {
      dsize *= FS__FAT_aBPBUnit[Idx][Unit].SecPerClus;
    }
  }
  return dsize;
}


/*********************************************************************
*
*             FS__fat_findpath
*
  Description:
  FS internal function. Return start cluster and size of the directory
  of the file name in pFileName.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  pFullName   - Fully qualified file name w/o device name.
  pFileName   - Pointer to a pointer, which is modified to point to the
                file name part of pFullName.
  pUnit       - Pointer to an FS_u32 for returning the unit number.
  pDirStart   - Pointer to an FS_u32 for returning the start cluster of
                the directory.

  Return value:
  >0          - Sector (not cluster) size of the directory.
  ==0         - An error has occured.
*/

FS_u32 FS__fat_findpath(int Idx, const char *pFullName, FS_FARCHARPTR *pFileName, 
                        FS_u32 *pUnit, FS_u32 *pDirStart) {
  FS_u32 dsize;
  FS_i32 i;
  FS_i32 j;
  FS_FARCHARPTR dname_start;
  FS_FARCHARPTR dname_stop;
  FS_FARCHARPTR chprt;
  int x;
  char dname[12];
  char realname[255];

  /* Find correct unit (unit:name) */
  *pFileName = (FS_FARCHARPTR)FS__CLIB_strchr(pFullName, ':');
  if (*pFileName) {
    /* Scan for unit number */
    *pUnit = FS__CLIB_atoi(pFullName);
    (*pFileName)++;
  }
  else {
    /* Use 1st unit as default */
    *pUnit = 0;
    *pFileName = (FS_FARCHARPTR) pFullName;
  }
  /* Check volume */
  x = !FS__fat_checkunit(Idx, *pUnit);
  if (x) {
    return 0;
  }
  /* Setup pDirStart/dsize for root directory */
  *pDirStart = 0;
  dsize      = FS__fat_dir_size(Idx, *pUnit, 0);
  /* Find correct directory */
  do {
    dname_start = (FS_FARCHARPTR)FS__CLIB_strchr(*pFileName, '\\');
    if (dname_start) {
      dname_start++;
      *pFileName = dname_start;
      dname_stop = (FS_FARCHARPTR)FS__CLIB_strchr(dname_start, '\\');
    }
    else {
      dname_stop = 0;
    }
    if (dname_stop) {
      i = dname_stop-dname_start;
//      printf("i----------------:%d--start:%d --stop:%d\n",i,dname_start,dname_stop);
      FS__CLIB_strncpy(realname, dname_start, i);
#if 0
      if (i >= 12) {
        j = 0;
        for (chprt = dname_start; chprt < dname_stop; chprt++) {
          if (*chprt == '.') {
            i--;
          }
          else if (j < 12) {
            realname[j] = *chprt;
            j++;
          }
        }
        if (i >= 12) {
          return 0;
        }
      }
      else {
        FS__CLIB_strncpy(realname, dname_start, i);
      }
#endif
      realname[i] = 0;

//      FS__fat_make_realname(dname, realname);
//      printf("dname:%s realname:%s\n",dname,realname);
      *pDirStart =  FS__fat_find_dir(Idx, *pUnit, realname, *pDirStart, dsize);
      if (*pDirStart) {
        dsize  =  FS__fat_dir_size(Idx, *pUnit, *pDirStart);
      }
      else {
        dsize = 0;    /* Directory NOT found */
      }
    }
  } while (dname_start);
  return dsize;
}


/*********************************************************************
*
*             Global functions section 2
*
**********************************************************************

  These are real global functions, which are used by the API Layer
  of the file system.
  
*/

/*********************************************************************
*
*             FS__fat_fopen
*
  Description:
  FS internal function. Open an existing file or create a new one.

  Parameters:
  pFileName   - File name. 
  pMode       - Mode for opening the file.
  pFile       - Pointer to an FS_FILE data structure.
  
  Return value:
  ==0         - Unable to open the file.
  !=0         - Address of the FS_FILE data structure.
*/

FS_FILE *FS__fat_fopen(const char *pFileName, const char *pMode, FS_FILE *pFile) {
  FS_u32 unit;
  FS_u32 dstart;
  FS_u32 dsize;
  FS_i32 i;
  FS_FARCHARPTR fname;
  FS__fat_dentry_type s;
  char realname[12];
  int lexp_a;
  int lexp_b;
  
  if (!pFile) {
    return 0;  /* Not a valid pointer to an FS_FILE structure*/
  }

  dsize = FS__fat_findpath(pFile->dev_index, pFileName, &fname, &unit, &dstart);
  if (dsize == 0) {
    return 0;  /* Directory not found */
  }

  FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, unit, FS_CMD_INC_BUSYCNT, 0, (void*)0);  /* Turn on busy signal */
//  FS__fat_make_realname(realname, fname);  /* Convert name to FAT real name */

  /* FileSize = 0 */
  s.data[28] = 0x00;      
  s.data[29] = 0x00;
  s.data[30] = 0x00;
  s.data[31] = 0x00;
  i = _FS_fat_find_file(pFile->dev_index, unit, fname, &s, dstart, dsize);
//if(i >= 0)
//printf("file find %s\n", fname);
//else
//printf("file find failure: %s\n", pFileName);

  /* Delete file */
  lexp_b = (FS__CLIB_strcmp(pMode, "del") == 0);    /* Delete file request by FS_Remove*/
  lexp_a = lexp_b && (i >= 0);                      /* File does exist */
  if (lexp_a) {
    i = FS__fat_DeleteFileOrDir(pFile->dev_index, unit, fname, dstart, dsize, 1);
//    i = FS__fat_DeleteFile(pFile->dev_index, unit,  fname, dstart, dsize);
    if (i != 0) {
      pFile->error = -1;
    }
    else {
      pFile->error = 0;
    }
    FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, pFile->fileid_lo, FS_CMD_FLUSH_CACHE, 2, (void*)0);
    FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
    return 0;
  }
  else if (lexp_b) {
    FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
    pFile->error = -1;
    return 0;
  }
  /* Check read only */
  lexp_a = ((i >= 0) && ((s.data[11] & FS_FAT_ATTR_READ_ONLY) != 0)) &&
          ((pFile->mode_w) || (pFile->mode_a) || (pFile->mode_c));
  if (lexp_a) {
    /* Files is RO and we try to create, write or append */
    FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
    return 0;
  }
  lexp_a = ( i>= 0) && (!pFile->mode_a) && (((pFile->mode_w) && (!pFile->mode_r)) || 
          ((pFile->mode_w) && (pFile->mode_c) && (pFile->mode_r)) );
  if (lexp_a) {
    /* Delete old file */
    i = FS__fat_DeleteFileOrDir(pFile->dev_index, unit, fname, dstart, dsize, 1);
/*
printf("Del %s\n", fname);
if(i >= 0)
    printf("success\n");
else
    printf("failure\n");
*/
    /* FileSize = 0 */
    s.data[28] = 0x00;      
    s.data[29] = 0x00;
    s.data[30] = 0x00;
    s.data[31] = 0x00;
    i=-1;
  }
  if ((!pFile->mode_c) && (i < 0)) {
    /* File does not exist and we must not create */
    FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
    return 0;
  }
  else if ((pFile->mode_c) && (i < 0)) {
    /* Create new file */
    i = _FS_fat_create_file(pFile->dev_index, unit, fname, dstart, dsize);
    if (i < 0) {
      /* Could not create file */
      if (i == -2) {
        /* Directory is full, try to increase */
        i = _FS_fat_IncDir(pFile->dev_index, unit, dstart, &dsize);
        if (i > 0) {
          i = _FS_fat_create_file(pFile->dev_index, unit, fname, dstart, dsize);
        }
      }
      if (i < 0) {
        FS__lb_ioctl(FS__pDevInfo[pFile->dev_index].devdriver, unit, FS_CMD_DEC_BUSYCNT, 0, (void*)0);  /* Turn off busy signal */
        return 0;
      }
    }
  }
//printf("fat opened %s\n", pFileName);
  pFile->fileid_lo  = unit;
  pFile->fileid_hi  = i;
  pFile->fileid_ex  = dstart;
  pFile->EOFClust   = -1;
  pFile->CurClust   = 0;
  pFile->error      = 0;
  pFile->size       = (FS_u32)s.data[28];   /* FileSize */
  pFile->size      += (FS_u32)0x100UL * s.data[29];
  pFile->size      += (FS_u32)0x10000UL * s.data[30];
  pFile->size      += (FS_u32)0x1000000UL * s.data[31];
  pFile->creat_time       = (FS_u32)s.data[22];   /* CreatTime */
  pFile->creat_time      += (FS_u32)0x100UL * s.data[23];
  pFile->creat_date       = (FS_u32)s.data[24];   /* CreatDate */
  pFile->creat_date      += (FS_u32)0x100UL * s.data[25];

//dumpen((unsigned char*)&s);

  if (pFile->mode_a) {
    pFile->filepos   = pFile->size;
  }
  else {
    pFile->filepos   = 0;
  }
  pFile->inuse     = 1;
  return pFile;
}

