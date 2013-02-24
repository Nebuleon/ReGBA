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
File        : fs_int.h 
Purpose     : Internals used accross different layers of the file system
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_INT_H_
#define _FS_INT_H_

/*********************************************************************
*
*             Global data types
*
**********************************************************************
*/

#ifndef FS_USE_LB_READCACHE
  #define FS_USE_LB_READCACHE 0
#endif

#if FS_USE_LB_READCACHE
  #ifndef FS_LB_BLOCKSIZE
    #define FS_LB_BLOCKSIZE 0x200
  #endif
  typedef struct {
    FS_u32            BlockId;
    char              aBlockData[FS_LB_BLOCKSIZE];
  } FS__CACHE_BUFFER;


  typedef struct {
    const int         MaxCacheNum;
    int               CacheIndex;
    FS__CACHE_BUFFER  *const pCache;
  } FS__LB_CACHE;
#endif  /* FS_USE_LB_READCACHE */


typedef struct {
  const char             *const devname;
  const FS__fsl_type     *const fs_ptr;
  const FS__device_type  *const devdriver;
#if FS_USE_LB_READCACHE
  FS__LB_CACHE           *const pDevCacheInfo;
#endif /* FS_USE_LB_READCACHE */
  const void             *const data;
} FS__devinfo_type;


/*********************************************************************
*
*             Externals
*
**********************************************************************
*/

/* fs_info.c */
extern const FS__devinfo_type   *const FS__pDevInfo;
extern const unsigned int       FS__maxdev;
extern const unsigned int       FS__fat_maxunit;

#endif  /* _FS_INT_H_ */

