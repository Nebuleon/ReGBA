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
File        : api_misc.c
Purpose     : Misc. API functions
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
#ifndef FS_FARCHARPTR
  #define FS_FARCHARPTR char *
#endif
#include "fs_conf.h"
#include "fs_dev.h"
#include "fs_api.h"
#include "fs_os.h"
#include "fs_fsl.h"
#include "fs_int.h"
#include "api_int.h"

#if FS_USE_FAT_FSL
  #include "fs_fat.h"
#endif

#include "fs_clib.h"


/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

#define FS_VALID_MODE_NUM     (sizeof(_FS_valid_modes) / sizeof(_FS_mode_type))


/*********************************************************************
*
*             Local data types
*
**********************************************************************
*/

typedef struct {
  FS_FARCHARPTR mode;
  unsigned char mode_r;     /* mode READ                    */
  unsigned char mode_w;     /* mode WRITE                   */
  unsigned char mode_a;     /* mode APPEND                  */
  unsigned char mode_c;     /* mode CREATE                  */
  unsigned char mode_b;     /* mode BINARY                  */
} _FS_mode_type;


/*********************************************************************
*
*             Local variables        
*
**********************************************************************
*/

static const _FS_mode_type _FS_valid_modes[] = {
  /*       READ  WRITE  APPEND  CREATE  BINARY */
  { "r"   ,  1,    0,     0,       0,     0 },
  { "w"   ,  0,    1,     0,       1,     0 },
  { "a"   ,  0,    1,     1,       1,     0 },
  { "rb"  ,  1,    0,     0,       0,     1 },
  { "wb"  ,  0,    1,     0,       1,     1 },
  { "ab"  ,  0,    1,     1,       1,     1 },
  { "r+"  ,  1,    1,     0,       0,     0 },
  { "w+"  ,  1,    1,     0,       1,     0 },
  { "a+"  ,  1,    1,     1,       1,     0 },
  { "r+b" ,  1,    1,     0,       0,     1 },
  { "rb+" ,  1,    1,     0,       0,     1 },
  { "w+b" ,  1,    1,     0,       1,     1 },
  { "wb+" ,  1,    1,     0,       1,     1 },
  { "a+b" ,  1,    1,     1,       1,     1 },
  { "ab+" ,  1,    1,     1,       1,     1 }
};

static const unsigned int _FS_maxopen = FS_MAXOPEN;
static FS_FILE            _FS_filehandle[FS_MAXOPEN];


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__find_fsl
*
  Description:
  FS internal function. Find correct index in the device information
  table referred by FS__pDevInfo for a given fully qualified name.

  Parameters:
  pFullName   - Fully qualified name. 
  pFilename   - Address of a pointer, which is modified to point to
                the file name part of pFullName.

  Return value:
  <0          - Unable to find the device.
  >=0         - Index of the device in the device information table.
*/

int FS__find_fsl(const char *pFullName, FS_FARCHARPTR *pFileName) {
  int idx;
  int i;
  int j;
  int m;
  FS_FARCHARPTR s;

  /* Find correct FSL (device:unit:name) */
  s = (FS_FARCHARPTR)FS__CLIB_strchr(pFullName, ':');
  if (s) {
    /* Scan for device name */
    idx = 0;
    m = (int)((FS_u32)(s) - (FS_u32)(pFullName));
    while (1) {
      j = FS__CLIB_strlen(FS__pDevInfo[idx].devname);
      if (m > j) {
        j = m;
      }
      i = FS__CLIB_strncmp(FS__pDevInfo[idx].devname, pFullName, j);
      idx++;
      if (idx >= (int)FS__maxdev) {
        break;  /* End of device information table reached */
      }
      if (i == 0) {
        break;  /* Device found */
      }
    }
    if (i == 0) {
      idx--;  /* Correct index */
    }
    else {
      return -1;  /* Device not found */
    }
    s++;
  }
  else {
    /* use 1st FSL as default */
    idx = 0;
    s = (FS_FARCHARPTR) pFullName;
  }
  *pFileName = s;
  return idx;
}


/*********************************************************************
*
*             FS_FOpen
*
  Description:
  API function. Open an existing file or create a new one.

  Parameters:
  pFileName   - Fully qualified file name. 
  pMode       - Mode for opening the file.
  
  Return value:
  ==0         - Unable to open the file.
  !=0         - Address of an FS_FILE data structure.
*/

FS_FILE *FS_FOpen(const char *pFileName, const char *pMode) {
  FS_FARCHARPTR s;
  FS_FILE *handle;
  unsigned int i;
  int idx;
  int j;
  int c;

  /* Find correct FSL  (device:unit:name) */
  idx = FS__find_fsl(pFileName, &s);
  if (idx < 0) {
    return 0;  /* Device not found */
  }

  if (FS__pDevInfo[idx].fs_ptr->fsl_fopen) {
   /*  Find next free entry in _FS_filehandle */
    FS_X_OS_LockFileHandle();
    i = 0;
    while (1) {
      if (i >= _FS_maxopen) {
        break;  /* No free entry found. */
      }
      if (!_FS_filehandle[i].inuse) {
        break;  /* Unused entry found */
      }
      i++;
    }
    if (i < _FS_maxopen) {
      /*
         Check for valid mode string and set flags in file
         handle
      */
      j = 0;
      while (1) {
        if (j >= FS_VALID_MODE_NUM) {
          break;  /* Not in list of valid modes */
        }
        c = FS__CLIB_strcmp(pMode, _FS_valid_modes[j].mode);
        if (c == 0) {
          break;  /* Mode found in list */
        }
        j++;
      }
      if (j < FS_VALID_MODE_NUM) {
        /* Set mode flags according to the mode string */
        _FS_filehandle[i].mode_r = _FS_valid_modes[j].mode_r;
        _FS_filehandle[i].mode_w = _FS_valid_modes[j].mode_w;
        _FS_filehandle[i].mode_a = _FS_valid_modes[j].mode_a;
        _FS_filehandle[i].mode_c = _FS_valid_modes[j].mode_c;
        _FS_filehandle[i].mode_b = _FS_valid_modes[j].mode_b;
      }
      else {
        FS_X_OS_UnlockFileHandle();
        return 0;
      }
      _FS_filehandle[i].dev_index = idx;
      /* Execute the FSL function */
      
      handle = (FS__pDevInfo[idx].fs_ptr->fsl_fopen)(s, pMode, &_FS_filehandle[i]);
      FS_X_OS_UnlockFileHandle();
      return handle;
    }
    FS_X_OS_UnlockFileHandle();
  }
  return 0;
}


/*********************************************************************
*
*             FS_FClose
*
  Description:
  API function. Close a file referred by pFile.

  Parameters:
  pFile       - Pointer to a FS_FILE data structure. 
  
  Return value:
  None.
*/

void FS_FClose(FS_FILE *pFile) {
  if (!pFile) {
    return;  /* No pointer to a FS_FILE structure */
  }
  FS_X_OS_LockFileHandle();
  if (!pFile->inuse) {
    FS_X_OS_UnlockFileHandle(); /* The FS_FILE structure is not in use */
    return;
  }
  if (pFile->dev_index >= 0) {
    if (FS__pDevInfo[pFile->dev_index].fs_ptr->fsl_fclose) {
      /* Execute the FSL function */
      (FS__pDevInfo[pFile->dev_index].fs_ptr->fsl_fclose)(pFile);
    }
  }
  FS_X_OS_UnlockFileHandle();
}


/*********************************************************************
*
*             FS_Remove
*
  Description:
  API function. Remove a file.
  There is no real 'delete' function in the FSL, but the FSL's 'open'
  function can delete a file. 

  Parameters:
  pFileName   - Fully qualified file name. 
  
  Return value:
  ==0         - File has been removed.
  ==-1        - An error has occured.
*/

int FS_Remove(const char *pFileName) {
  FS_FARCHARPTR s;
  unsigned int i;
  int idx;
  int x;

  /* Find correct FSL  (device:unit:name) */
  idx = FS__find_fsl(pFileName, &s);
  if (idx < 0) {
    return -1;  /* Device not found */
  }
  if (FS__pDevInfo[idx].fs_ptr->fsl_fopen) {
    /*  Find next free entry in _FS_filehandle */
    FS_X_OS_LockFileHandle();
    i = 0;
    while (1) {
      if (i >= _FS_maxopen) {
        break;  /* No free file handle found */
      }
      if (!_FS_filehandle[i].inuse) {
        break;  /* Free file handle found */
      }
      i++;
    }
    if (i < _FS_maxopen) {
      /* Set file open mode to write & truncate */
      _FS_filehandle[i].mode_r = 0;
      _FS_filehandle[i].mode_w = 1;
      _FS_filehandle[i].mode_a = 0;
      _FS_filehandle[i].mode_c = 0;
      _FS_filehandle[i].mode_b = 0;
      _FS_filehandle[i].dev_index = idx;
      /* 
         Call the FSL function 'open' with the parameter 'del' to indicate,
         that we want to delete the file.
      */
      (FS__pDevInfo[idx].fs_ptr->fsl_fopen)(s, "del", &_FS_filehandle[i]);
      x = _FS_filehandle[i].error;
      FS_X_OS_UnlockFileHandle();
      return x;
    }
    FS_X_OS_UnlockFileHandle();
  }
  return -1;
}


/*********************************************************************
*
*             FS_IoCtl
*
  Description:
  API function. Execute device command.

  Parameters:
  pDevName    - Fully qualified directory name. 
  Cmd         - Command to be executed.
  Aux         - Parameter depending on command.
  pBuffer     - Pointer to a buffer used for the command.
  
  Return value:
  Command specific. In general a negative value means an error.
*/

int FS_IoCtl(const char *pDevName, FS_i32 Cmd, FS_i32 Aux, void *pBuffer) {
  int idx;
  int unit;
  FS_FARCHARPTR s;
  FS_FARCHARPTR t;

  idx = FS__find_fsl(pDevName, &s);
  if (idx < 0) {
    return -1;  /* Device not found */
  }
  t = FS__CLIB_strchr(s, ':');  /* Find correct unit (unit:name) */
  if (t) {
    unit = FS__CLIB_atoi(s);  /* Scan for unit number */
  }
  else {
    unit = 0;  /* Use 1st unit as default */
  }

  if (FS__pDevInfo[idx].fs_ptr->fsl_ioctl) {
    /* Execute the FSL function */
    idx = (FS__pDevInfo[idx].fs_ptr->fsl_ioctl)(idx, unit, Cmd, Aux, pBuffer);
  }
  else {
    idx = -1;
  }
  return idx;
}


/*********************************************************************
*
*             FS_FSeek
*
  Description:
  API function. Set current position of a file pointer.
  FS_fseek does not support to position the fp behind end of a file. 

  Parameters:
  pFile       - Pointer to a FS_FILE data structure.
  Offset      - Offset for setting the file pointer position.
  Whence      - Mode for positioning the file pointer.
  
  Return value:
  ==0         - File pointer has been positioned according to the
                parameters.
  ==-1        - An error has occured.
*/

int FS_FSeek(FS_FILE *pFile, FS_i32 Offset, int Whence) {
  FS_i32 value,ret;
  //printf("eeee %x %x\n",Offset,Whence);
  if (!pFile) {
    return -1;
  }
  pFile->error     = FS_ERR_OK;    /* Clear any previous error */
  pFile->CurClust  = 0;            /* Invalidate current cluster */
  FS_X_OS_LockFileOp(pFile);
  if (Whence == FS_SEEK_SET) {
    if (Offset <= pFile->size) {
      ret = pFile->filepos = Offset;
      
    }
    else {
      /* New position would be behind EOF */
      pFile->error = FS_ERR_INVALIDPAR;
      ret = -1;
    }
  }
  else if (Whence == FS_SEEK_CUR) {
    value = pFile->filepos + Offset;
    if (value <= pFile->size) {
      pFile->filepos += Offset;
      ret = pFile->filepos;
    }
    else {
      /* New position would be behind EOF */
      pFile->error = FS_ERR_INVALIDPAR;
      ret = -1;
    }
  }
  else if (Whence == FS_SEEK_END) {
    /* The file system does not support this */
    pFile->filepos = pFile->size + Offset;
    
    ret = pFile->filepos;
  }
  else {
    /* Parameter 'Whence' is invalid */
    pFile->error = FS_ERR_INVALIDPAR;
    ret = -1;
  }
  FS_X_OS_UnlockFileOp(pFile);
//  printf("%s ret = %d",__FILE__,ret);
  return ret;
}


/*********************************************************************
*
*             FS_FTell
*
  Description:
  API function. Return position of a file pointer.

  Parameters:
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  >=0         - Current position of the file pointer.
  ==-1        - An error has occured.
*/

FS_i32 FS_FTell(FS_FILE *pFile) {
  if (!pFile) {
    return -1;
  }
  return pFile->filepos;
}


/*********************************************************************
*
*             FS_FError
*
  Description:
  API function. Return error status of a file.

  Parameters:
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  ==FS_ERR_OK - No error.
  !=FS_ERR_OK - An error has occured.
*/

FS_i16 FS_FError(FS_FILE *pFile) {
  if (!pFile) {
    return 0;
  }
  return pFile->error;
}


/*********************************************************************
*
*             FS_ClearErr
*
  Description:
  API function. Clear error status of a file.

  Parameters:
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  None.
*/

void FS_ClearErr(FS_FILE *pFile) {
  if (!pFile) {
    return;
  }
  pFile->error = FS_ERR_OK;
}


/*********************************************************************
*
*             FS_Init
*
  Description:
  API function. Start the file system.

  Parameters:
  None.
  
  Return value:
  ==0         - File system has been started.
  !=0         - An error has occured.
*/

int FS_Init(void) {
  int x;
  
  x = FS_X_OS_Init();  /* Init the OS, e.g. create semaphores  */
#if FS_USE_FAT_FSL
  if (x == 0) {
    FS__fat_block_init(); /* Init the FAT layers memory pool */
  }
#endif
  return x;
}

/*********************************************************************
*
*             FS_Exit
*
  Description:
  API function. Stop the file system.

  Parameters:
  None.
  
  Return value:
  ==0         - File system has been stopped.
  !=0         - An error has occured.
*/

int FS_Exit(void) {
  return FS_X_OS_Exit();
}


