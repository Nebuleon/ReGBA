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
File        : fs_lbl.h 
Purpose     : Header file for the file system's Logical Block Layer
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_LBL_H_
#define _FS_LBL_H_

/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

#define FS_LBL_MEDIACHANGED     0x0001


/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

int                 FS__lb_status(const FS__device_type *pDriver, FS_u32 Unit);
int                 FS__lb_read(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector, void *pBuffer);
int                 FS__lb_multi_read(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector,FS_u32 SectorNum, void *pBuffer);
int                 FS__lb_write(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector, void *pBuffer);
int                 FS__lb_multi_write(const FS__device_type *pDriver, FS_u32 Unit, FS_u32 Sector,FS_u32 SectorNum, void *pBuffer);
int                 FS__lb_ioctl(const FS__device_type *pDriver, FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer);

#endif  /* _FS_LBL_H_  */


