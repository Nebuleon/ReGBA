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
File        : api_in.c
Purpose     : FS read functions
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
#include "fs_api.h"
#include "fs_os.h"
#include "fs_fsl.h"
#include "fs_int.h"
#include "fs_fat.h"

/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS_FRead
*
  Description:
  API function. Read data from a file.

  Parameters:
  pData       - Pointer to a data buffer for storing data transferred
                from file. 
  Size        - Size of an element to be transferred from file to data
                buffer
  N           - Number of elements to be transferred from the file.
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  Number of elements read.
*/

FS_size_t FS_FRead(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile) {
  FS_size_t i;
/*
    unsigned char pbuf[32];
    unsigned int x;
    unsigned char *ppt;

    ppt= (unsigned char*)((unsigned int)pData-32);
    memcpy(pbuf, ppt, 32);
printf("\n----------------pData %08x\n", (unsigned int)pData);
*/
  if (!pFile) {
    return 0;  /* No pointer to a FS_FILE structure */
  }
  FS_X_OS_LockFileOp(pFile);
  if (!pFile->mode_r) {
    /* File open mode does not allow read ops */
    pFile->error = FS_ERR_WRITEONLY;
    FS_X_OS_UnlockFileOp(pFile);
    return 0;
  }
  i = 0;
  if (pFile->dev_index >= 0) {
//  	if(((unsigned int)pData & 31) || (N *Size < FS_FAT_SEC_SIZE * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus))
//    if(N *Size < FS_FAT_SEC_SIZE * FS__FAT_aBPBUnit[pFile->dev_index][pFile->fileid_lo].SecPerClus)
      if((unsigned int)pData & 0x3)
	  {
	  	if (FS__pDevInfo[pFile->dev_index].fs_ptr->fsl_fread) {
			  /* Execute the FSL function  */
			  i = (FS__pDevInfo[pFile->dev_index].fs_ptr->fsl_fread)(pData, Size, N, pFile);
		  }
	  }
      else {
	  	if (FS__pDevInfo[pFile->dev_index].fs_ptr->fsl_fread_block) {
			  /* Execute the FSL function  */
			  i = (FS__pDevInfo[pFile->dev_index].fs_ptr->fsl_fread_block)(pData, Size, N, pFile);
		  }
	  }
  }
  FS_X_OS_UnlockFileOp(pFile);
/*
    for(x= 0; x < 32; x++)
    {
        if(pbuf[x] != ppt[x]) printf("------------------------read block bak at %d\n", x);
    }
*/
  return i;  
}


