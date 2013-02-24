/*
**********************************************************************
*                          Micrium, Inc.
*                      949 Crestview Circle
*                     Weston,  FL 33327-1848
*
*                            uC/FS
*
*                (c) Copyright 2002, Micrium, Inc.
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : fs_dev.h
Purpose     : Define structures for Device Drivers
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_DEV_H_
#define _FS_DEV_H_

/*********************************************************************
*
*             Global data types
*
**********************************************************************
*/

typedef struct {
  FS_FARCHARPTR           name;
  int                     (*dev_status)(FS_u32 id);
  int                     (*dev_read)(FS_u32 id, FS_u32 block, void *buffer);
  int                     (*dev_multi_read)(FS_u32 id, FS_u32 block,FS_u32 number, void *buffer);
  int                     (*dev_write)(FS_u32 id, FS_u32 block, void *buffer);
  int                     (*dev_multi_write)(FS_u32 id, FS_u32 block,FS_u32 number, void *buffer);
  int                     (*dev_ioctl)(FS_u32 id, FS_i32 cmd, FS_i32 aux, void *buffer);
} FS__device_type;


#endif  /* _FS_DEV_H_ */

