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
File        : ntflt_drv.c
Purpose     : File system generic NAND driver
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

#if FS_USE_NAND_FLASH_DRIVER

#include "fs_api.h"
#include "header.h"
/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static FS_u32   _FS_nand_logicalstart[FS_NAND_FLASH_MAXUNIT];     /* start of partition */
static FS_u32   _FS_nand_logicaltotal[FS_NAND_FLASH_MAXUNIT];     /* start of partition */

//static char     _FS_nand_mbrbuffer[0x200];                 /* buffer for reading MBR */
static char     _FS_nand_diskchange[FS_NAND_FLASH_MAXUNIT];       /* signal flag for driver */
static int init_nand = 0;

/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/
static inline int FS__NAND_Update()
{

	NAND_LB_FLASHCACHE();
	return 0;
}
static inline int FS__NAND_Init(FS_u32 Unit)
{
	NAND_LB_Init();
	return 0;
}

static int FS__NAND_ReadSector(FS_u32 Unit,unsigned long Sector,unsigned char *pBuffer)
{
	    //printf("_FS_NAND_DevRead %d\r\n",Sector);
//	printf("sigle read!\n");
	int x = NAND_LB_Read(Sector,pBuffer);
	return x;
}

static int FS__NAND_WriteSector(FS_u32 Unit,unsigned long Sector,unsigned char *pBuffer)
{
//	printf("sigle write!\n");
	int x =  NAND_LB_Write(Sector,pBuffer);
	return x;
        
}

static inline int FS__NAND_ReadMultiSector(FS_u32 Unit,unsigned long Sector,unsigned long Count,unsigned char *pBuffer)
{
//	printf("mul read!\n");
	return  NAND_LB_MultiRead(Sector,pBuffer,Count);
}

static inline int FS__NAND_WriteMultiSector(FS_u32 Unit,unsigned long Sector,unsigned long Count,unsigned char *pBuffer)
{
//	printf("mul write!\n");
	return  NAND_LB_MultiWrite(Sector,pBuffer,Count);
  
}

static inline int FS__NAND_DetectStatus()
{
	return 1;
}

static int FS__NAND_MOUNT()
{
	FS__fat_block_init();
	FS__NAND_DetectStatus();

	return 0;
}

static int FS__NAND_UNMOUNT()
{
	FS__NAND_Update();
	FS__NAND_DetectStatus();
	init_nand = 0;

	return 0;
}

/*********************************************************************
*
*             _FS_NAND_DevStatus
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

static int _FS_NAND_DevStatus(FS_u32 Unit) {

  int x;
  char a;
   
  if (!init_nand) {
    /* 
       The function is called the first time. For each unit,
       the flag for 'diskchange' is set. That makes sure, that
       FS_LBL_MEDIACHANGED is returned, if there is already a
       media in the reader.
    */
    for (x = 0; x < FS_NAND_FLASH_MAXUNIT; x++) {
      _FS_nand_diskchange[x] = 1;
    }
    init_nand = 1;
  }
  
  if (Unit >= FS_NAND_FLASH_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  /* When you get here, then there is a card in the reader */
  a = _FS_nand_diskchange[Unit];  /* Check if the media has changed */
  if(a) {
    /* 
       A diskchange took place. The following code reads the MBR of the
       card to get its partition information.
    */
    _FS_nand_diskchange[Unit] = 0;  /* Reset 'diskchange' flag */
    _FS_nand_logicalstart[Unit] = 0;
    printf("nand init ok!");
	FS__NAND_Init(Unit);            /* Initialize NAND hw */
	return FS_LBL_MEDIACHANGED;
	
	
  }
  return 0;
}

/*********************************************************************
*
*             _FS_NAND_DevRead
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

static int _FS_NAND_DevRead(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int x = 0;
  if (Unit >= FS_NAND_FLASH_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
//  printf("_FS_NAND_DevRead %d %d %d\r\n",Sector,_FS_nand_logicalstart[Unit],Unit);
  x = FS__NAND_ReadSector(Unit, Sector + _FS_nand_logicalstart[Unit], (unsigned char*)pBuffer); 
  if(x != 512)
  	x = -1;
  //printf("%s %d\n",__FUNCTION__,x);
  return 0;
}


/*********************************************************************
*
*             _FS_NAND_DevMultiRead
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

static int _FS_NAND_DevMultiRead(FS_u32 Unit, FS_u32 Sector,FS_u32 Count,void *pBuffer) {
  int x;
  if (Unit >= FS_NAND_FLASH_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  x = FS__NAND_ReadMultiSector(Unit, Sector + _FS_nand_logicalstart[Unit],Count, (unsigned char*)pBuffer); 
  if(x != Count * 512)
  	x = -1;
  //printf("%s %d\n",__FUNCTION__,x);
  return 0;
}

/*********************************************************************
*
*             _FS_NAND_DevWrite
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

static int _FS_NAND_DevWrite(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int x;
  if (Unit >= FS_NAND_FLASH_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
//  printf("FS__NAND_WriteSector = %d %d %s---111\r\n",Sector,_FS_nand_logicalstart[Unit],(unsigned char*)pBuffer);
  x = FS__NAND_WriteSector(Unit, Sector + _FS_nand_logicalstart[Unit],(unsigned char*)pBuffer);
  
  if (x != 512) {
      printf("%s Write Error %x!\r\n",__FUNCTION__,x);
	  x = -1;
  }
  return 0;
}

/*********************************************************************
*
*             _FS_NAND_DevMulWrite
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

static int _FS_NAND_DevMultiWrite(FS_u32 Unit, FS_u32 Sector,FS_u32 Count, void *pBuffer) {
  int x;

  if (Unit >= FS_NAND_FLASH_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
  //printf("FS__NAND_WriteSector = %d %d\r\n",Sector,_FS_nand_logicalstart[Unit]);
  x = FS__NAND_WriteMultiSector(Unit, Sector + _FS_nand_logicalstart[Unit],Count, (unsigned char*)pBuffer);
  if (x != Count * 512) {
      printf("%s Write Error %x!\r\n",__FUNCTION__,x);
	  x = -1;
  }
  return 0;
}

/*********************************************************************
*
*             _FS_NAND_DevIoCtl
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

static int _FS_NAND_DevIoCtl(FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer) {
  FS_u32 *info;
  int x;
  char a;

  Aux = Aux;  /* Get rid of compiler warning */
  if (Unit >= FS_NAND_FLASH_MAXUNIT) {
    return -1;  /* No valid unit number */
  }
// printf("NAND IOCTRL = %x %x\r\n",FS_CMD_FLUSH_CACHE,Cmd);
// printf("NAND IOCTRL = %x %x %x\r\n", FS_CMD_GET_DEVINFO,FS_CMD_CHK_DSKCHANGE,_DSKCHANGE););
  switch (Cmd) {
    case FS_CMD_CHK_DSKCHANGE:
      break;

  case FS_CMD_MOUNT:
	    FS__NAND_MOUNT();
	    break;

  case FS_CMD_UNMOUNT:
	    FS__NAND_UNMOUNT();
	    break;

    case FS_CMD_GET_DEVINFO:
      if (!pBuffer) {
        return -1;
      }
      info = pBuffer;
      //FS__NAND_Init(Unit);
      
	  	flash_info_t flashinfo;
      ssfdc_nftl_getInfo(0,&flashinfo);
	 
      /* hidden */
      *info = 0;
      info++;
      /* head */
      *info = 0; 
      info++;
      /* sec per track */
      *info = 4; 
      info++;
      /* size */
	  x = flashinfo.dwFSTotalSectors;
      *info = x;
      break;
  case FS_CMD_FLUSH_CACHE:
	  FS__NAND_Update(Unit);
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

const FS__device_type FS__nanddevice_driver = {
  "NAND device",
  _FS_NAND_DevStatus,
  _FS_NAND_DevRead,
  _FS_NAND_DevMultiRead,
  _FS_NAND_DevWrite,
  _FS_NAND_DevIoCtl
};

#endif /* FS_USE_NAND_DRIVER */
