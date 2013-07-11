#ifndef __IO_DS2_H__
#define __IO_DS2_H__

// 'DS2F'
#define DEVICE_TYPE_DS2_MMCF 0x46434D4D

#include "disc_io.h"
#include "ds2_mmc_api.h"

// export interface
extern const IO_INTERFACE _io_ds2_mmcf ;

/* initialize MMC/SD card */
static inline bool _MMC_StartUp(void)
{
	return MMC_Initialize();
}

/* read multi blocks from MMC/SD card */
/* read a single block from MMC/SD card */
static inline bool _MMC_ReadSectors(u32 sector, u32 numSectors, void* buffer)
{
	int flag;

	if(numSectors > 1)
		flag= MMC_ReadMultiBlock(sector, numSectors, (unsigned char*)buffer);
	else
		flag= MMC_ReadBlock(sector, (unsigned char*)buffer);
	return (flag==MMC_NO_ERROR);
}

/* write multi blocks from MMC/SD card */
/* write a single block from MMC/SD card */
static inline bool _MMC_WriteSectors(u32 sector, u32 numSectors, const void* buffer)
{
	int flag;

	if(numSectors > 1)
		flag= MMC_WriteMultiBlock(sector, numSectors, (unsigned char*)buffer);
	else
		flag= MMC_WriteBlock(sector, (unsigned char*)buffer);

	return (flag==MMC_NO_ERROR);
}

static inline bool _MMC_ClearStatus(void)
{
	return true;
}

static inline bool _MMC_ShutDown(void)
{
	return true;
}

static inline bool _MMC_IsInserted(void)
{
	return true;
}


#endif //__IO_DS2_H__

