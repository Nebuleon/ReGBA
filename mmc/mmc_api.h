/*
**********************************************************************
*
*                            uC/MMC
*
*             (c) Copyright 2005 - 2007, Ingenic Semiconductor, Inc
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : mmc_api.h 
Purpose     : Define global functions and types to be used by an
              application using the mmc/sd card.

----------------------------------------------------------------------
Version-Date-----Author-Explanation
----------------------------------------------------------------------
1.00.00 20060831 WeiJianli     First release

----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
(none)
---------------------------END-OF-HEADER------------------------------
*/

#ifndef __MMC_API__
#define __MMC_API__

#include "mmc_config.h"

/* Error codes */
enum mmc_result_t {
	MMC_NO_RESPONSE        = -1,
	MMC_NO_ERROR           = 0,
	MMC_ERROR_OUT_OF_RANGE,
	MMC_ERROR_ADDRESS,
	MMC_ERROR_BLOCK_LEN,
	MMC_ERROR_ERASE_SEQ,
	MMC_ERROR_ERASE_PARAM,
	MMC_ERROR_WP_VIOLATION,
	MMC_ERROR_CARD_IS_LOCKED,
	MMC_ERROR_LOCK_UNLOCK_FAILED,
	MMC_ERROR_COM_CRC,
	MMC_ERROR_ILLEGAL_COMMAND,
	MMC_ERROR_CARD_ECC_FAILED,
	MMC_ERROR_CC,
	MMC_ERROR_GENERAL,
	MMC_ERROR_UNDERRUN,
	MMC_ERROR_OVERRUN,
	MMC_ERROR_CID_CSD_OVERWRITE,
	MMC_ERROR_STATE_MISMATCH,
	MMC_ERROR_HEADER_MISMATCH,
	MMC_ERROR_TIMEOUT,
	MMC_ERROR_CRC,
	MMC_ERROR_DRIVER_FAILURE,
};

/* initialize MMC/SD card */
extern int MMC_Initialize(void);

extern unsigned int MMC_GetSize(void);
/* read a single block from MMC/SD card */
extern int MMC_ReadBlock(unsigned int blockaddr, unsigned char *recbuf);

/* read multi blocks from MMC/SD card */
extern int MMC_ReadMultiBlock(unsigned int blockaddr, unsigned int blocknum, unsigned char *recbuf);
 
/* write a block to MMC/SD card */
extern int MMC_WriteBlock(unsigned int blockaddr, unsigned char *recbuf);

/* write multi blocks to MMC/SD card */
extern int MMC_WriteMultiBlock(unsigned int blockaddr, unsigned int blocknum, unsigned char *recbuf);

/* detect MMC/SD card */
extern int MMC_DetectStatus(void);

#endif /* __MMC_API__ */
