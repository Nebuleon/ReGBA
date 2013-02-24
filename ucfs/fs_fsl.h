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
File        : fs_fsl.h
Purpose     : Define structures for File-System-Layer
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_FSL_H_
#define _FS_FSL_H_

/*********************************************************************
*
*             Global data types
*
**********************************************************************
*/

typedef struct {
  FS_FARCHARPTR       name;
  FS_FILE *           (*fsl_fopen)(const char *pFileName, const char *pMode, FS_FILE *pFile);
  void                (*fsl_fclose)(FS_FILE *pFile);
  FS_size_t           (*fsl_fread)(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);
  FS_size_t           (*fsl_fread_block)(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);
  FS_size_t           (*fsl_fwrite)(const void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);
  long                (*fsl_ftell)(FS_FILE *pFile);
  int                 (*fsl_fseek)(FS_FILE *pFile, long int Offset, int Whence);
  int                 (*fsl_ioctl)(int Idx, FS_u32 Id, FS_i32 Cmd, FS_i32 Aux, void *pBuffer);
#if FS_POSIX_DIR_SUPPORT
  FS_DIR *            (*fsl_opendir)(const char *pDirName, FS_DIR *pDir);
  int                 (*fsl_closedir)(FS_DIR *pDir);
  struct FS_DIRENT *  (*fsl_readdir)(FS_DIR *pDir);
  struct FS_DIRENT *  (*fsl_readdir_back)(FS_DIR *pDir);
  void                (*fsl_rewinddir)(FS_DIR *pDir);
  int                 (*fsl_mkdir)(const char *pDirName, int DevIndex, char Aux);
  int                 (*fsl_rmdir)(const char *pDirName, int DevIndex, char Aux);
#endif  /* FS_POSIX_DIR_SUPPORT */
} FS__fsl_type;


#endif  /* _FS_FSL_H_ */

