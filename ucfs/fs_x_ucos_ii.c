/*
*********************************************************************************************************
*                                             Micrium, Inc.
*                                         949 Crestview Circle
*                                        Weston,  FL 33327-1848
*
*                                         OS Layer for uC/FS
*
*                                   (c) Copyright 2003, Micrium, Inc.
*                                          All rights reserved.
*
* Filename    : fs_x_ucos_ii.C
* Programmers : Jean J. Labrosse
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "fs_port.h"
#include  "fs_dev.h"
#include  "fs_api.h"
#include  "fs_os.h"
#include  "fs_conf.h"

#if FS_OS_TIME_SUPPORT == 1
#include  <time.h>
#endif

#include  "ucos_ii.h"

/*
*********************************************************************************************************
*                                         LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_EVENT  *FS_SemFileHandle;
static  OS_EVENT  *FS_SemFileOps;
static  OS_EVENT  *FS_SemMemManager;
static  OS_EVENT  *FS_SemDeviceOps;

#if    FS_POSIX_DIR_SUPPORT
static OS_EVENT   *FS_SemDirHandle;
static OS_EVENT   *FS_SemDirOps;
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                              Lock global table _FS_filehandle (fs_info.c)
*********************************************************************************************************
*/

void  FS_X_OS_LockFileHandle (void) 
{
    INT8U  err;


    OSSemPend(FS_SemFileHandle, 0, &err);
}


/*
*********************************************************************************************************
*                              Unlock global table _FS_filehandle (fs_info.c)
*********************************************************************************************************
*/

void  FS_X_OS_UnlockFileHandle (void) 
{
    OSSemPost(FS_SemFileHandle);
}

/*
*********************************************************************************************************
*                                          Lock File Operations
*********************************************************************************************************
*/

void  FS_X_OS_LockFileOp (FS_FILE *fp) 
{
    INT8U  err;


    OSSemPend(FS_SemFileOps, 0, &err);
}

/*
*********************************************************************************************************
*                                         Unlock File Operations
*********************************************************************************************************
*/

void  FS_X_OS_UnlockFileOp (FS_FILE *fp) 
{
    OSSemPost(FS_SemFileOps);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                        Lock the Memory Manager
*********************************************************************************************************
*/

void  FS_X_OS_LockMem (void)
{
    INT8U  err;


    OSSemPend(FS_SemMemManager, 0, &err);
}

/*
*********************************************************************************************************
*                                       Unlock the Memory Manager
*********************************************************************************************************
*/

void  FS_X_OS_UnlockMem (void) 
{
    OSSemPost(FS_SemMemManager);
}

/*
*********************************************************************************************************
*                                        Lock Device Operations
*********************************************************************************************************
*/

void  FS_X_OS_LockDeviceOp (const FS__device_type *driver, FS_u32 id) 
{
    INT8U  err;


    OSSemPend(FS_SemDeviceOps, 0, &err);
}

/*
*********************************************************************************************************
*                                       Unlock Device Operations
*********************************************************************************************************
*/

void  FS_X_OS_UnlockDeviceOp (const FS__device_type *driver, FS_u32 id) 
{
    OSSemPost(FS_SemDeviceOps);
}



/*
*********************************************************************************************************
*                              Lock global table _FS_dirhandle (api_dir.c).
*********************************************************************************************************
*/

#if  FS_POSIX_DIR_SUPPORT
void FS_X_OS_LockDirHandle (void)
{
    INT8U  err;


    OSSemPend(FS_SemDirHandle, 0, &err);
}
#endif


/*
*********************************************************************************************************
*                              Unlock global table _FS_dirhandle (api_dir.c).
*********************************************************************************************************
*/

#if  FS_POSIX_DIR_SUPPORT
void FS_X_OS_UnlockDirHandle (void)
{
    OSSemPost(FS_SemDirHandle);
}
#endif


/*
*********************************************************************************************************
*                                          Lock Directory Operations
*********************************************************************************************************
*/

#if  FS_POSIX_DIR_SUPPORT
void FS_X_OS_LockDirOp (FS_DIR *dirp)
{
    INT8U  err;


    OSSemPend(FS_SemDirOps, 0, &err);
}
#endif


/*
*********************************************************************************************************
*                                          Unlock Directory Operations
*********************************************************************************************************
*/

#if  FS_POSIX_DIR_SUPPORT
void FS_X_OS_UnlockDirOp (FS_DIR *dirp) 
{
    OSSemPost(FS_SemDirOps);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                              Get Date
*********************************************************************************************************
*/

FS_u16  FS_X_OS_GetDate (void) 
{
#if FS_OS_TIME_SUPPORT == 1
    FS_u16      fdate;
    time_t      t;
    struct tm  *ltime;


    time(&t);
    ltime  = localtime(&t);
    fdate  = ltime->tm_mday;
    fdate += ((FS_u16)(ltime->tm_mon  +  1) << 5);
    fdate += ((FS_u16)(ltime->tm_year - 80) << 9);
#else
    FS_u16      fdate;


    fdate  = 1;
    fdate += ((FS_u16) 1 << 5);
    fdate += ((FS_u16) 0 << 9);
#endif
    return (fdate);
}

/*
*********************************************************************************************************
*                                              Get Time
*********************************************************************************************************
*/

FS_u16  FS_X_OS_GetTime (void) 
{
#if FS_OS_TIME_SUPPORT == 1
    FS_U16      ftime;
    time_t      t;
    struct tm  *ltime;


    time(&t);
    ltime  = localtime(&t);
    ftime  = ltime->tm_sec / 2;
    ftime += ((FS_u16) ltime->tm_min  <<  5);
    ftime += ((FS_u16) ltime->tm_hour << 11);
#else
    FS_u16 ftime;


    ftime  = 0;
#endif
    return (ftime);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                         Initialize OS Resources
*********************************************************************************************************
*/

int  FS_X_OS_Init (void) 
{
    FS_SemFileHandle = OSSemCreate(1);
    FS_SemFileOps    = OSSemCreate(1);
    FS_SemMemManager = OSSemCreate(1);
    FS_SemDeviceOps  = OSSemCreate(1);

#if FS_POSIX_DIR_SUPPORT    
    FS_SemDirHandle  = OSSemCreate(1);
    FS_SemDirOps     = OSSemCreate(1);
#endif

    return (0);
}

/*
*********************************************************************************************************
*                                         Reclaim OS Resources
*********************************************************************************************************
*/

int  FS_X_OS_Exit (void) 
{
    INT8U  err;


    OSSemDel(FS_SemFileHandle, OS_DEL_ALWAYS, &err);
    OSSemDel(FS_SemFileOps   , OS_DEL_ALWAYS, &err);
    OSSemDel(FS_SemMemManager, OS_DEL_ALWAYS, &err);
    OSSemDel(FS_SemDeviceOps , OS_DEL_ALWAYS, &err);

#if FS_POSIX_DIR_SUPPORT    
    OSSemDel(FS_SemDirHandle , OS_DEL_ALWAYS, &err);
    OSSemDel(FS_SemDirOps    , OS_DEL_ALWAYS, &err);
#endif

    return (0);
}


