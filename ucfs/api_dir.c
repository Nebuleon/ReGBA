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
File        : api_dir.c
Purpose     : POSIX 1003.1 like directory support
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
#include "fs_os.h"
#include "fs_fsl.h"
#include "fs_int.h"
#include "api_int.h"


#if FS_POSIX_DIR_SUPPORT

/*********************************************************************
*
*             Local variables        
*
**********************************************************************
*/

static const unsigned int _FS_dir_maxopen = FS_DIR_MAXOPEN;
static FS_DIR             _FS_dirhandle[FS_DIR_MAXOPEN];


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS_OpenDir
*
  Description:
  API function. Open an existing directory for reading.

  Parameters:
  pDirName    - Fully qualified directory name. 
  
  Return value:
  ==0         - Unable to open the directory.
  !=0         - Address of an FS_DIR data structure.
*/

FS_DIR *FS_OpenDir(const char *pDirName) {
  FS_DIR *handle;
  unsigned int i;
  int idx;
  char *s;

  /* Find correct FSL (device:unit:name) */
  idx = FS__find_fsl(pDirName, &s);
  if (idx < 0) {
    return 0;  /* Device not found */
  }
  if (FS__pDevInfo[idx].fs_ptr->fsl_opendir) {
    /*  Find next free entry in _FS_dirhandle */
    FS_X_OS_LockDirHandle();
    i = 0;
    while (1) {
      if (i >= _FS_dir_maxopen) {
        break;  /* No free entry in _FS_dirhandle */
      }
      if (!_FS_dirhandle[i].inuse) {
        break;  /* Free entry found */
      }
      i++;
    }
    if (i < _FS_dir_maxopen) {
      /* Execute open function of the found FSL */
      _FS_dirhandle[i].dev_index = idx; 
      handle = (FS__pDevInfo[idx].fs_ptr->fsl_opendir)(s, &_FS_dirhandle[i]);
      FS_X_OS_UnlockDirHandle();
      return handle;
    }
    FS_X_OS_UnlockDirHandle();
  }
  return 0;
}


/*********************************************************************
*
*             FS_CloseDir
*
  Description:
  API function. Close a directory referred by pDir.

  Parameters:
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  ==0         - Directory has been closed.
  ==-1        - Unable to close directory.
*/

int FS_CloseDir(FS_DIR *pDir) {
  int i;

  if (!pDir) {
    return -1;  /* No pointer to a FS_DIR data structure */
  }
  FS_X_OS_LockDirHandle();
  if (!pDir->inuse) {
    /* FS_DIR structure is not in use and cannot be closed */
    FS_X_OS_UnlockDirHandle();
    return -1;
  }
  i = -1;
  if (pDir->dev_index >= 0) {
    if (FS__pDevInfo[pDir->dev_index].fs_ptr->fsl_closedir) {
      /* Execute close function of the corresponding FSL */
      i = (FS__pDevInfo[pDir->dev_index].fs_ptr->fsl_closedir)(pDir);
    }
  }
  FS_X_OS_UnlockDirHandle();
  return i;
}


/*********************************************************************
*
*             FS_ReadDir
*
  Description:
  API function. Read next directory entry in directory specified by 
  pDir.

  Parameters:
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  ==0         - No more directory entries or error.
  !=0         - Pointer to a directory entry.
*/

struct FS_DIRENT *FS_ReadDir(FS_DIR *pDir) {
  struct FS_DIRENT *entry;

  if (!pDir) {
    return 0;  /* No pointer to a FS_DIR data structure */
  }
  FS_X_OS_LockDirOp(pDir);
  entry = 0;
  if (pDir->dev_index >= 0) {
    if (FS__pDevInfo[pDir->dev_index].fs_ptr->fsl_readdir) {
      /* Execute FSL function */
      entry = (FS__pDevInfo[pDir->dev_index].fs_ptr->fsl_readdir)(pDir);
    }
  }
  FS_X_OS_UnlockDirOp(pDir);
  return entry;  
}

/*********************************************************************
*
*             FS_ReadDir_Back
*
  Description:
  API function. Read last directory entry in directory specified by 
  pDir.

  Parameters:
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  ==0         - No more directory entries or error.
  !=0         - Pointer to a directory entry.
*/

struct FS_DIRENT *FS_ReadDir_Back(FS_DIR *pDir) {
  struct FS_DIRENT *entry;

  if (!pDir) {
    return 0;  /* No pointer to a FS_DIR data structure */
  }
  FS_X_OS_LockDirOp(pDir);
  entry = 0;
  if (pDir->dev_index >= 0) {
    if (FS__pDevInfo[pDir->dev_index].fs_ptr->fsl_readdir_back) {
      /* Execute FSL function */
      entry = (FS__pDevInfo[pDir->dev_index].fs_ptr->fsl_readdir_back)(pDir);
    }
  }
  FS_X_OS_UnlockDirOp(pDir);
  return entry;  
}


/*********************************************************************
*
*             FS_RewindDir
*
  Description:
  API function. Set pointer for reading the next directory entry to 
  the first entry in the directory.

  Parameters:
  pDir        - Pointer to a FS_DIR data structure. 
  
  Return value:
  None.
*/

void FS_RewindDir(FS_DIR *pDir) {
  if (!pDir) {
    return;  /* No pointer to a FS_DIR data structure */
  }
  pDir->dirpos = 0;
}


/*********************************************************************
*
*             FS_MkDir
*
  Description:
  API function. Create a directory.

  Parameters:
  pDirName    - Fully qualified directory name. 
  
  Return value:
  ==0         - Directory has been created.
  ==-1        - An error has occured.
*/

int FS_MkDir(const char *pDirName) {
  int idx;
  int i;
  char *s;

  /* Find correct FSL (device:unit:name) */
  idx = FS__find_fsl(pDirName, &s);
  if (idx < 0) {
    return -1;  /* Device not found */
  }
  if (FS__pDevInfo[idx].fs_ptr->fsl_mkdir) {
    /* Execute the FSL function */
    FS_X_OS_LockDirHandle();
    i = (FS__pDevInfo[idx].fs_ptr->fsl_mkdir)(s, idx, 1);
    FS_X_OS_UnlockDirHandle();
    return i;
  }
  return -1;
}


/*********************************************************************
*
*             FS_RmDir
*
  Description:
  API function. Remove a directory.

  Parameters:
  pDirName    - Fully qualified directory name. 
  
  Return value:
  ==0         - Directory has been removed.
  ==-1        - An error has occured.
*/

int FS_RmDir(const char *pDirName) {
  FS_DIR *dirp;
  struct FS_DIRENT *direntp;
  int idx;
  int i;
  char *s;
  
  /* Check if directory is empty */
  dirp = FS_OpenDir(pDirName);
  if (!dirp) {
    /* Directory not found */
    return -1;
  } 
  i=0;
  while (1) {
    direntp = FS_ReadDir(dirp);
    i++;
    if (i >= 4) {
      break;  /* There is more than '..' and '.' */
    }
    if (!direntp) {
      break;  /* There is no more entry in this directory. */
    }
  }
  FS_CloseDir(dirp);
  if (i >= 4) {
    /* 
        There is more than '..' and '.' in the directory, so you 
        must not delete it.
    */
    return -1;
  }
  /* Find correct FSL (device:unit:name) */
  idx = FS__find_fsl(pDirName, &s);
  if (idx < 0) {
    return -1;  /* Device not found */
  }
  if (FS__pDevInfo[idx].fs_ptr->fsl_rmdir) {
    /* Execute the FSL function */
    FS_X_OS_LockDirHandle();
    i = (FS__pDevInfo[idx].fs_ptr->fsl_rmdir)(s, idx, 0);
    FS_X_OS_UnlockDirHandle();
    return i;
  }
  return -1;
}


#endif  /* FS_POSIX_DIR_SUPPORT */

