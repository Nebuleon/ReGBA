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
File        : r_misc.c 
Purpose     : Device Driver for simple array in RAM
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

#if FS_USE_RAMDISK_DRIVER

#include "fs_api.h"
#include "fs_clib.h"


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static char _array[(long)FS_RR_BLOCKNUM * FS_RR_BLOCKSIZE];


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_RAM_DevStatus
*
  Description:
  FS driver function. Get status of the RAM disk.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  ==1 (FS_LBL_MEDIACHANGED) - The media of the device has changed.
  ==0                       - Device okay and ready for operation.
  <0                        - An error has occured.
*/

static int _FS_RAM_DevStatus(FS_u32 Unit) {
  static int online[1];

  if (Unit != 0) {
    return -1;  /* Invalid unit number */
  }
  if (!online[Unit]) {
    /* 
       Make sure, the function returns FS_LBL_MEDIACHANGED when it is
       called the first time
    */
    online[Unit] = 1;
    return FS_LBL_MEDIACHANGED;
  }
  return 0;
}


/*********************************************************************
*
*             _FS_RAM_DevRead
*
  Description:
  FS driver function. Read a sector from the RAM disk.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

static int _FS_RAM_DevRead(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  if (Unit != 0) {
    return -1;  /* Invalid unit number */
  }
  if (Sector >= FS_RR_BLOCKNUM) {
    return -1;  /* Out of physical range */
  }
  FS__CLIB_memcpy(pBuffer, ((char*)&_array[0]) + Sector * FS_RR_BLOCKSIZE,
                  (FS_size_t)FS_RR_BLOCKSIZE);
  return 0;
}


/*********************************************************************
*
*             _FS_RAM_DevWrite
*
  Description:
  FS driver function. Write sector to the RAM disk.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be written to the device.
  pBuffer     - Pointer to data to be stored.
 
  Return value:
  ==0         - Sector has been written to the device.
  <0          - An error has occured.
*/

static int _FS_RAM_DevWrite(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  if (Unit != 0) {
    return -1;  /* Invalid unit number */
  }
  if (Sector >= FS_RR_BLOCKNUM) {
    return -1;  /* Out of physical range */
  }
  FS__CLIB_memcpy(((char*)&_array[0]) + Sector * FS_RR_BLOCKSIZE, pBuffer,
                  (FS_size_t)FS_RR_BLOCKSIZE);
  return 0;
}


/*********************************************************************
*
*             _FS_RAM_DevIoCtl
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

static int _FS_RAM_DevIoCtl(FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer) {
  FS_u32 *info;

  Aux = Aux;  /* Get rid of compiler warning */
  if (Unit != 0) {
    return -1;  /* Invalid unit number */
  }
  switch (Cmd) {
    case FS_CMD_GET_DEVINFO:
      if (!pBuffer) {
        return -1;
      }
      info = pBuffer;
      *info = 0;  /* hidden */
      info++;
      *info = 2;  /* head */
      info++;
      *info = 4;  /* sec per track */
      info++;
      *info = FS_RR_BLOCKNUM;
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

const FS__device_type FS__ramdevice_driver = {
  "RAMDISK device",
  _FS_RAM_DevStatus,
  _FS_RAM_DevRead,
  _FS_RAM_DevWrite,
  _FS_RAM_DevIoCtl
};

#endif /* FS_USE_RAMDISK_DRIVER */

