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
File        : lb_misc.c
Purpose     : Logical Block Layer
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
#include "fs_conf.h"
#include "fs_dev.h"
#include "fs_api.h"
#include "fs_lbl.h"
#include "fs_os.h"
#include "fs_fsl.h"
#include "fs_int.h"
#include "fs_clib.h"


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

#if FS_USE_LB_READCACHE

/*********************************************************************
*
*             _FS_LB_GetDriverIndex
*
  Description:
  FS internal function. Get index of a driver in the device information
  table referred by FS__pDevInfo.

  Parameters:
  pDriver     - Pointer to a device driver structure
 
  Return value:
  =>0         - Index of pDriver in the device information table.
  <0          - An error has occured.
*/

static int _FS_LB_GetDriverIndex(const FS__device_type *pDriver) {
  unsigned int i;

  i = 0;
  while (1) {
    if (i >= FS__maxdev) {
      break;  /* Driver not found */
    }
    if (FS__pDevInfo[i].devdriver == pDriver) {
      break;  /* Driver found */
    }
    i++;
  }
  if (i >= FS__maxdev) {
    return -1;
  }
  return i;
}


/*********************************************************************
*
*             _FS_LB_GetFromCache
*
  Description:
  FS internal function. Copy sector from cache, if available.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
  Sector      - Sector to be read from the device's cache.
  pBuffer     - Pointer to a data buffer for storing data of the cache. 
 
  Return value:
  ==0         - Sector is in cache and has been copied to pBuffer.
  <0          - An error has occured.
*/

static int _FS_LB_GetFromCache(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int i;
  int idx;

  idx = _FS_LB_GetDriverIndex(pDriver);
  if (idx < 0) {
    return -1;
  }
  if (FS__pDevInfo[idx].pDevCacheInfo) {
    i = 0;
    while (1) {
      if (i >= FS__pDevInfo[idx].pDevCacheInfo[Unit].MaxCacheNum) {
        break;  /* Sector not in cache */
      }
      if (Sector == FS__pDevInfo[idx].pDevCacheInfo[Unit].pCache[i].BlockId) {
        break;  /* Sector found */
      }
      i++;
    }
    if (i < FS__pDevInfo[idx].pDevCacheInfo[Unit].MaxCacheNum) {
      FS__CLIB_memcpy(pBuffer, FS__pDevInfo[idx].pDevCacheInfo[Unit].pCache[i].aBlockData, FS_LB_BLOCKSIZE);
      return 0;
    }
  }
  return -1;  
}


/*********************************************************************
*
*             _FS_LB_CopyToCache
*
  Description:
  FS internal function. Copy a sector to the cache of the device.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
  Sector      - Sector to be copied to the device's cache.
  pBuffer     - Pointer to a data buffer to be stored in the cache. 
 
  Return value:
  None.
*/

static void _FS_LB_CopyToCache(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int idx;

  idx = _FS_LB_GetDriverIndex(pDriver);
  if (idx < 0) {
    return;
  }
  if (FS__pDevInfo[idx].pDevCacheInfo) {
    FS__CLIB_memcpy(FS__pDevInfo[idx].pDevCacheInfo[Unit].pCache[FS__pDevInfo[idx].pDevCacheInfo[Unit].CacheIndex].aBlockData,
            pBuffer, FS_LB_BLOCKSIZE);
    FS__pDevInfo[idx].pDevCacheInfo[Unit].pCache[FS__pDevInfo[idx].pDevCacheInfo[Unit].CacheIndex].BlockId = Sector;
    FS__pDevInfo[idx].pDevCacheInfo[Unit].CacheIndex++;
    if (FS__pDevInfo[idx].pDevCacheInfo[Unit].CacheIndex >= FS__pDevInfo[idx].pDevCacheInfo[Unit].MaxCacheNum) {
      FS__pDevInfo[idx].pDevCacheInfo[Unit].CacheIndex = 0;
    }
  }
}


/*********************************************************************
*
*             _FS_LB_UpdateInCache
*
  Description:
  FS internal function. Update sector in cache, if it is there.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
  Sector      - Sector to be updated in the device's cache.
  pBuffer     - Pointer to a data buffer to be stored in the cache. 
 
  Return value:
  None.
*/

static void _FS_LB_UpdateInCache(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int i;
  int idx;

  idx = _FS_LB_GetDriverIndex(pDriver);
  if (idx < 0) {
    return;
  }
  if (FS__pDevInfo[idx].pDevCacheInfo) {
    i = 0;
    while (1) {
      if (i >= FS__pDevInfo[idx].pDevCacheInfo[Unit].MaxCacheNum) {
        break; /* Sector not in cache */
      }
      if (Sector == FS__pDevInfo[idx].pDevCacheInfo[Unit].pCache[i].BlockId) {
        break; /* Sector found */
      }
      i++;
    }
    if (i < FS__pDevInfo[idx].pDevCacheInfo[Unit].MaxCacheNum) {
      FS__CLIB_memcpy(FS__pDevInfo[idx].pDevCacheInfo[Unit].pCache[i].aBlockData, pBuffer, FS_LB_BLOCKSIZE);
    }
  }
}


/*********************************************************************
*
*             _FS_LB_ClearCache
*
  Description:
  FS internal function. Clear cache of a device.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
 
  Return value:
  None.
*/

static void _FS_LB_ClearCache(const FS__device_type *pDriver, FS_u32 Unit) {
  int i;
  int idx;

  idx = _FS_LB_GetDriverIndex(pDriver);
  if (idx<0) {
    return;
  }
  if (FS__pDevInfo[idx].pDevCacheInfo) {
    FS__pDevInfo[idx].pDevCacheInfo[Unit].CacheIndex = 0;
    for (i = 0; i < FS__pDevInfo[idx].pDevCacheInfo[Unit].MaxCacheNum; i++) {
      FS__pDevInfo[idx].pDevCacheInfo[Unit].pCache[i].BlockId = 0xffffffffUL;
    }
  }
}

#endif  /* FS_USE_LB_READCACHE */


/*********************************************************************
*
*             Global functions
*
**********************************************************************

  Functions here are global, although their names indicate a local
  scope. They should not be called by user application.
*/

/*********************************************************************
*
*             FS__lb_status
*
  Description:
  FS internal function. Get status of a device.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
 
  Return value:
  ==1 (FS_LBL_MEDIACHANGED) - The media of the device has changed.
  ==0                       - Device okay and ready for operation.
  <0                        - An error has occured.
*/

int FS__lb_status(const FS__device_type *pDriver, FS_u32 Unit) {
  int x;

  if (pDriver->dev_status) {
    FS_X_OS_LockDeviceOp(pDriver, Unit);
    x = (pDriver->dev_status)(Unit);
#if FS_USE_LB_READCACHE
    if (x != 0) {
      _FS_LB_ClearCache(pDriver, Unit);
    }
#endif  /* FS_USE_LB_READCACHE */
    FS_X_OS_UnlockDeviceOp(pDriver, Unit);
    return x;
  }
  return -1;
}


/*********************************************************************
*
*             FS__lb_read
*
  Description:
  FS internal function. Read sector from device.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

int FS__lb_read(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int x;

  if (pDriver->dev_read) {
    FS_X_OS_LockDeviceOp(pDriver, Unit);
#if FS_USE_LB_READCACHE
    x = _FS_LB_GetFromCache(pDriver, Unit, Sector, pBuffer);
    if (x != 0) {
      x = (pDriver->dev_read)(Unit, Sector, pBuffer);
      if (x == 0) {
        _FS_LB_CopyToCache(pDriver, Unit, Sector, pBuffer);
      }
    }
#else
    x = (pDriver->dev_read)(Unit, Sector, pBuffer);
#endif  /* FS_USE_LB_READCACHE */
    FS_X_OS_UnlockDeviceOp(pDriver, Unit);
    return  x;
  }
  return -1;
}


/*********************************************************************
*
*             FS__lb_multi_read
*
  Description:
  FS internal function. Read multiple sectors from device.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

int FS__lb_multi_read(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector,FS_u32 SectorNum, void *pBuffer) {
  int x;

  if (pDriver->dev_multi_read) {
    FS_X_OS_LockDeviceOp(pDriver, Unit);

    x = (pDriver->dev_multi_read)(Unit, Sector,SectorNum, pBuffer);

    FS_X_OS_UnlockDeviceOp(pDriver, Unit);
    return  x;
  }
  return -1;
}


/*********************************************************************
*
*             FS__lb_write
*
  Description:
  FS internal function. Write sector to device.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
  Sector      - Sector to be written to the device.
  pBuffer     - Pointer to data to be stored.
 
  Return value:
  ==0         - Sector has been written to the device.
  <0          - An error has occured.
*/

int FS__lb_write(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  int x;
  if (pDriver->dev_write) {
    FS_X_OS_LockDeviceOp(pDriver, Unit);
    x = (pDriver->dev_write)(Unit, Sector, pBuffer);
#if FS_USE_LB_READCACHE
    if (x==0) {
      _FS_LB_UpdateInCache(pDriver, Unit, Sector, pBuffer);
    }
    else {
      _FS_LB_ClearCache(pDriver, Unit);
    }
#endif  /* FS_USE_LB_READCACHE */
    FS_X_OS_UnlockDeviceOp(pDriver, Unit);
    return x;
  }
  return -1;
}

/*********************************************************************
*
*             FS__lb_multi_write
*
  Description:
  FS internal function. write multiple sectors to device.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

int FS__lb_multi_write(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector,FS_u32 SectorNum, void *pBuffer) {
  int x;

  if (pDriver->dev_multi_write) {
    FS_X_OS_LockDeviceOp(pDriver, Unit);

    x = (pDriver->dev_multi_write)(Unit, Sector,SectorNum, pBuffer);

    FS_X_OS_UnlockDeviceOp(pDriver, Unit);
    return  x;
  }
  return -1;
}


/*********************************************************************
*
*             FS__lb_ioctl
*
  Description:
  FS internal function. Execute device command.

  Parameters:
  pDriver     - Pointer to a device driver structure.
  Unit        - Unit number.
  Cmd         - Command to be executed.
  Aux         - Parameter depending on command.
  pBuffer     - Pointer to a buffer used for the command.
 
  Return value:
  Command specific. In general a negative value means an error.
*/

int FS__lb_ioctl(const FS__device_type *pDriver, FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer) {
  int x;

  if (pDriver->dev_ioctl) {
    FS_X_OS_LockDeviceOp(pDriver, Unit);
    x = (pDriver->dev_ioctl)(Unit, Cmd, Aux, pBuffer);
    FS_X_OS_UnlockDeviceOp(pDriver, Unit);
    return x;
  }
  return -1;
}

