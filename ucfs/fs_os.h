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
File        : fs_os.h 
Purpose     : File system's OS Layer header file
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_OS_H_
#define _FS_OS_H_

void     FS_X_OS_LockFileHandle(void);
void     FS_X_OS_UnlockFileHandle(void);
void     FS_X_OS_LockFileOp(FS_FILE *fp);
void     FS_X_OS_UnlockFileOp(FS_FILE *fp);
void     FS_X_OS_LockMem(void);
void     FS_X_OS_UnlockMem(void);
void     FS_X_OS_LockDeviceOp(const FS__device_type *driver, FS_u32 id);
void     FS_X_OS_UnlockDeviceOp(const FS__device_type *driver, FS_u32 id);
FS_u16   FS_X_OS_GetDate(void);
FS_u16   FS_X_OS_GetTime(void);
int      FS_X_OS_Init(void);
int      FS_X_OS_Exit(void);

#if FS_POSIX_DIR_SUPPORT
void     FS_X_OS_LockDirHandle(void);
void     FS_X_OS_UnlockDirHandle(void);
void     FS_X_OS_LockDirOp(FS_DIR *dirp);
void     FS_X_OS_UnlockDirOp(FS_DIR *dirp);
#endif  /* FS_POSIX_DIR_SUPPORT */

#endif    /*  _FS_OS_H_  */

