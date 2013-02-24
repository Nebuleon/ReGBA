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
File        : mmc_drv.c
Purpose     : File system generic MMC/SD driver
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

#include "fs_port.h"
#include "fs_dev.h" 
#include "fs_lbl.h" 
#include "fs_conf.h"
#include "dospart.h"
#if FS_USE_MMC_DRIVER
#include "fs_api.h"
#include "mmc_api.h"
/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static FS_u32   _FS_mmc_logicalstart[FS_MMC_MAXUNIT];     /* start of partition */
static char     _FS_mmc_mbrbuffer[0x200];                 /* buffer for reading MBR */
#define         MULTI_BLOCK_NUM   4  // MULTI_BLOCK_NUM = 2 ^ n 
static char     _FS_mmc_diskchange[FS_MMC_MAXUNIT];       /* signal flag for driver */
static PARTENTRY _FS_mPart;
//static unsigned int _FS_TotalSector = 0;

static int init_mmc = 0;
/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

static inline int FS_MMC_Update()
{
	MMC_LB_FLASHCACHE();
	return 0;
}
static inline int FS__MMC_Init(FS_u32 Unit)
{
    printf("Initial mmc\n");
	return MMC_LB_Init();
}

static int FS__MMC_ReadSector(FS_u32 Unit,unsigned long Sector,unsigned char *pBuffer)
{
	int x = MMC_LB_Read(Sector,pBuffer);
	return x;
}

static int FS__MMC_WriteSector(FS_u32 Unit,unsigned long Sector,unsigned char *pBuffer)
{
	int x =  MMC_LB_Write(Sector,pBuffer);
	return x;
        
}
static inline int FS__MMC_ReadMultiSector(FS_u32 Unit,unsigned long Sector,unsigned long Count,unsigned char *pBuffer)
{
	return MMC_LB_MultiRead(Sector,pBuffer,Count);
}

static inline int FS__MMC_WriteMultiSector(FS_u32 Unit,unsigned long Sector,unsigned long Count,unsigned char *pBuffer)
{
	return MMC_LB_MultiWrite(Sector,pBuffer,Count);;
}

static inline int FS__MMC_DetectStatus()
{
	return MMC_DetectStatus();
}

static int FS__MMC_MOUNT()
{
	FS__fat_block_init();
	FS__MMC_DetectStatus();

	return 0;
}

static int FS__MMC_UNMOUNT()
{
	FS_MMC_Update();
	FS__MMC_DetectStatus();
	init_mmc = 0;

	return 0;
}

static int dumpsect(FS_u32 *dat)
{
	int i,j;
	printf("sect:\n");
	for(j = 0;j < 32;j++)
	{
		printf("%02x:",j * 16);
		for(i = 0;i < 4 ;i++)
		{
			printf(" %02x %02x %02x %02x",(dat[i + j *4] >> 0)& 0xff,
																		 (dat[i + j *4] >> 8)& 0xff,
																		 (dat[i + j *4] >> 16)& 0xff,
																		 (dat[i + j *4] >> 24)& 0xff);
		}
		printf("\n");
	}
}
/*********************************************************************
*
*             _FS_MMC_DevStatus
*
  Description:
  FS driver function. Get status of the media.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  ==1 (FS_LBL_MEDIACHANGED) - The media of the device has changed.
  ==0                       - Device okay and ready for operation.
  <0                        - An error has occured.
*/

static int _FS_MMC_DevStatus(FS_u32 Unit) {
    int x;
    char a;
  
	PPARTENTRY pPart = &_FS_mPart;
	unsigned char dospart;
    unsigned int _FS_TotalSector;

  //printf("mmc init\r\n");
  if (!init_mmc) {
    /* 
       The function is called the first time. For each unit,
       the flag for 'diskchange' is set. That makes sure, that
       FS_LBL_MEDIACHANGED is returned, if there is already a
       media in the reader.
    */
    for (x = 0; x < FS_MMC_MAXUNIT; x++) {
      _FS_mmc_diskchange[x] = 1;
    }
    init_mmc = 1;
  }
  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  a = FS__MMC_DetectStatus(Unit);  /* Check if a card is present */
  if (a) {
	_FS_mmc_diskchange[Unit] = 1;  
	printf("sd detect error!\r\n");  
    return -1;  /* No card in reader */
  }
  /* When you get here, then there is a card in the reader */
  a = _FS_mmc_diskchange[Unit];  /* Check if the media has changed */
  if (a) {
    /* 
       A diskchange took place. The following code reads the MBR of the
       card to get its partition information.
    */
    _FS_mmc_diskchange[Unit] = 0;  /* Reset 'diskchange' flag */
    FS__MMC_Init(Unit);            /* Initialize MMC/SD hw */
    x = FS__MMC_ReadSector(Unit, 0, (unsigned char*)&_FS_mmc_mbrbuffer[0]);
    
    if (x != 0) {
        printf("read sector error!\r\n");
		return -1;
    }

	//dumpsect(_FS_mmc_mbrbuffer);
/*
	if(FS__CLIB_strncmp(&_FS_mmc_mbrbuffer[54],"FAT",3) == 0)
	{	
		_FS_mmc_logicalstart[Unit] = 0;
		
    }
    else if(FS__CLIB_strncmp(&_FS_mmc_mbrbuffer[82],"FAT",3) == 0)	
	{
		_FS_mmc_logicalstart[Unit] = 0;
	}
	else
*/
	{
		_FS_TotalSector = MMC_GetSize();
        //printf("total sect = %d\n", m);
		dospart = GetDOSPartition((unsigned char*)_FS_mmc_mbrbuffer, _FS_TotalSector, 0, pPart);
		if(dospart == INVALID)
		{
			if(!(
					  (FS__CLIB_strncmp(&_FS_mmc_mbrbuffer[54],"FAT",3) == 0) || 
					  (FS__CLIB_strncmp(&_FS_mmc_mbrbuffer[82],"FAT",3) == 0)
				  ))
				  printf("error File System!\n");
            _FS_mmc_logicalstart[Unit] = 0;
		}
        else
        {	
            while(dospart == PART_EXTENDED)
            {
                x = FS__MMC_ReadSector(Unit, pPart->Part_StartSector, (unsigned char*)&_FS_mmc_mbrbuffer[0]);
				if (x != 0) {
				    printf("read sector error!\r\n");
				    return -1;
				}
                dospart = GetDOSPartition((unsigned char*)_FS_mmc_mbrbuffer, _FS_TotalSector, 0, pPart);
                if(dospart == INVALID)
                {
                    if(!(
                            (FS__CLIB_strncmp(&_FS_mmc_mbrbuffer[54],"FAT",3) == 0) || 
                            (FS__CLIB_strncmp(&_FS_mmc_mbrbuffer[82],"FAT",3) == 0)
						  ))
                        printf("error File System!\n");
                    _FS_mmc_logicalstart[Unit] = 0;
                    break;
                }
            }

            if(dospart != INVALID)
                _FS_mmc_logicalstart[Unit] = pPart->Part_StartSector; 
        }
    }
	//printf("_FS_mmc_logicalstart = %d\r\n",_FS_mmc_logicalstart[0]);
	
	return FS_LBL_MEDIACHANGED;
  }
  return 0;
}

/*********************************************************************
*
*             _FS_MMC_DevRead
*
  Description:
  FS driver function. Read a sector from the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

static int _FS_MMC_DevRead(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int x = 0;
   
  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  //printf("_FS_MMC_DevRead %d %d\r\n",Sector,_FS_mmc_logicalstart[Unit]);
  x = FS__MMC_ReadSector(Unit, Sector + _FS_mmc_logicalstart[Unit], (unsigned char*)pBuffer);  
  	  
  
  if (x != 0) {
    x = -1;
  }
  return x;
}


/*********************************************************************
*
*             _FS_MMC_DevWrite
*
  Description:
  FS driver function. Write sector to the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be written to the device.
  pBuffer     - Pointer to data to be stored.
 
  Return value:
  ==0         - Sector has been written to the device.
  <0          - An error has occured.
*/

static int _FS_MMC_DevWrite(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int x;

  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  //printf("FS__MMC_WriteSector = %d %d\r\n",Sector,_FS_mmc_logicalstart[Unit]);
  x = FS__MMC_WriteSector(Unit, Sector + _FS_mmc_logicalstart[Unit], (unsigned char*)pBuffer);
  if (x != 0) {
    x = -1;
  }
  return x;
}

/*********************************************************************
*
*             _FS_MMC_DevMultiRead
*
  Description:
  FS driver function. Read a sector from the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  Count       - Sector Number to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

static int _FS_MMC_DevMultiRead(FS_u32 Unit, FS_u32 Sector,FS_u32 Count,void *pBuffer) {
  int x;
/*
unsigned char pbuf[32];
unsigned int xi;
unsigned char *ppt;
ppt= (unsigned char*)((unsigned int)pBuffer-32);
memcpy(pbuf, ppt, 32);
printf("\n----------------pData %08x\n", (unsigned int)pBuffer);
*/
  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }

  x = FS__MMC_ReadMultiSector(Unit, Sector + _FS_mmc_logicalstart[Unit],Count, (unsigned char*)pBuffer);
/*
for(xi= 0; xi < 32; xi++)
{
    if(pbuf[xi] != ppt[xi]) printf("------------------------read block bak at %d\n", xi);
}
*/
  if (x != 0) {
    x = -1;
  }
  return x;
}


/*********************************************************************
*
*             _FS_MMC_DevMultiWrite
*
  Description:
  FS driver function. Write sector to the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be written to the device.
  Count       - Sector Number to be written to the device.
  pBuffer     - Pointer to data to be stored.
 
  Return value:
  ==0         - Sector has been written to the device.
  <0          - An error has occured.
*/

static int _FS_MMC_DevMultiWrite(FS_u32 Unit, FS_u32 Sector, FS_u32 Count,void *pBuffer) {
  int x;

  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  x = FS__MMC_WriteMultiSector(Unit, Sector + _FS_mmc_logicalstart[Unit],Count, (unsigned char*)pBuffer);
  //printf("_FS_MMC_DevWrite = %d \r\n",Sector + _FS_mmc_logicalstart[Unit]);
  if (x != 0) {
    x = -1;
  }
  return x;
}


/*********************************************************************
*
*             _FS_MMC_DevIoCtl
*
  Description:
  FS driver function. Execute device command.

  Parameters:
  Unit        - Unit number.
  Cmd         - Command to be executed.
  Aux         - Parameter depending on command.
  pBuffer     - Pointer to a buffer used for the command.
 
  Return value:
  Command specific. In general a negative value means an error.
*/

static int _FS_MMC_DevIoCtl(FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer) {
  FS_u32 *info;
  int x;
  char a;

  Aux = Aux;  /* Get rid of compiler warning */
  if (Unit >= FS_MMC_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  //printf("MMC IOCTRL = %x %x\r\n",FS_CMD_FLUSH_CACHE,Cmd);
  switch (Cmd) {
  case FS_CMD_CHK_DSKCHANGE:
	    break;
  case FS_CMD_MOUNT:
	    FS__MMC_MOUNT();
	    break;

  case FS_CMD_UNMOUNT:
	    FS__MMC_UNMOUNT();
	    break;

  case FS_CMD_GET_DEVINFO:
      if (!pBuffer) {
        return -1;
      }
      info = pBuffer;
      FS__MMC_Init(Unit);
      x = FS__MMC_ReadSector(Unit, _FS_mmc_logicalstart[Unit], (unsigned char*)&_FS_mmc_mbrbuffer[0]);
      
      if (x != 0) {
        return -1;
      }
      /* hidden */
      *info = _FS_mmc_mbrbuffer[0x1c6];
      *info += (0x100UL * _FS_mmc_mbrbuffer[0x1c7]);
      *info += (0x10000UL * _FS_mmc_mbrbuffer[0x1c8]);
      *info += (0x1000000UL * _FS_mmc_mbrbuffer[0x1c9]);
      info++;
      /* head */
      *info = _FS_mmc_mbrbuffer[0x1c3]; 
      info++;
      /* sec per track */
      *info = _FS_mmc_mbrbuffer[0x1c4]; 
      info++;
      /* size */
      *info = _FS_mmc_mbrbuffer[0x1ca];
      *info += (0x100UL * _FS_mmc_mbrbuffer[0x1cb]);
      *info += (0x10000UL * _FS_mmc_mbrbuffer[0x1cc]);
      *info += (0x1000000UL * _FS_mmc_mbrbuffer[0x1cd]);
      break;
  case FS_CMD_FLUSH_CACHE:
	  FS_MMC_Update();
	  
	  break;
    default:
      break;
  }
  return 0;
}


/*********************************************************************
*
*             Global variables
*
**********************************************************************
*/

const FS__device_type FS__mmcdevice_driver = {
  "MMC device",
  _FS_MMC_DevStatus,
  _FS_MMC_DevRead,
  _FS_MMC_DevMultiRead,
  _FS_MMC_DevWrite,
  _FS_MMC_DevMultiWrite,
  _FS_MMC_DevIoCtl
};

#endif /* FS_USE_MMC_DRIVER */
