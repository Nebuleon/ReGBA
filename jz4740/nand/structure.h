//
// Copyright (c) Ingenic Semiconductor Co., Ltd. 2007.
//

#ifndef __STRUCTURE_H__
#define __STRUCTURE_H__

typedef enum  __FLASH_TYPE__
{
	NANDFLASH,
	NORFLASH
} FLASH_TYPE;

typedef struct __FLASH_INFO__
{
	FLASH_TYPE		flashType;
	unsigned int	dwBytesPerBlock;
	unsigned int	dwSectorsPerBlock;
	unsigned int	dwDataBytesPerSector;

	unsigned int	dwBLStartBlock;
	unsigned int	dwBLTotalBlocks;
	unsigned int	dwBLTotalSectors;
	
	unsigned int	dwFSStartBlock;
	unsigned int	dwFSTotalBlocks;
	unsigned int	dwFSTotalSectors;

	unsigned int	dwTotalPhyBlocks;
	unsigned int	dwMaxBadBlocks;

	unsigned int	dwCylinders;
	unsigned int	dwHeads;
	unsigned int	dwSectors;
	unsigned int	dwFlags;
} flash_info_t, *pflash_info_t;

typedef struct __BLOCK_CACHE__
{
	int	in_use;
	int	virt_block; 
	int	old_phys_block;
	int	new_phys_block;

	// unsigned char 8bit because max pagesize = 4(2048 / 512) bit
	unsigned char	*dirty_page;

	//unsigned short dirty_sector[CONFIG_SSFDC_NAND_PAGE_PER_BLOCK][SECTORS_PER_PAGE];
	unsigned char	*data;
	unsigned char	*ext_data;
} block_cache_t;

typedef struct __PAGE_CACHE__
{
	unsigned long	page_num;
	unsigned char	*data;
} page_cache_t;

typedef struct __BLOCK_INFO__
{
	unsigned int	lifetime;
	unsigned int	tag;
} block_info_t;

typedef struct __attribute__ ((packed)) __NAND_PAGE_INFO__ 
{
	unsigned char	block_status;
	unsigned char	bOEMReserved;

	unsigned int	dwReserved1;
	unsigned char	ecc_field[36];
	unsigned char	reserved3;
	unsigned short	wReserved2;
	unsigned char	reserved4;
	unsigned short	block_addr_field; //47

	unsigned int	lifetime; //49
	unsigned char	page_status;
	unsigned char	block_erase;// 54
	unsigned char	reserver5[NAND_OOB_SIZE - 1 - 3 - 4 - 1 - 36 - 1 - 2 - 4 - 1 - 1];
//} __attribute__ ((packed));
} nand_page_info_t ;

typedef enum NAND_ZONE_TYPE
{
	NZT_BOOT = 0x00,
	NZT_SAVE = 0x01,
	NZT_LOGO = 0x02,
	NZT_REGS = 0x03,
	NZT_KERN = 0x04,
	NZT_FATFS = 0x05
};

typedef struct __NAND_ZONE__
{
	unsigned int	dwSignature;
	unsigned int	dwStartBlock;
	unsigned int	dwTotalBlocks;
	unsigned int	dwTotalSectors;
	unsigned int	dwMaxBadBlocks;
} NAND_ZONE, *PNAND_ZONE;


typedef struct __VENUS_NAND_INFO__
{
	unsigned int	dwBytesPerBlock;
	unsigned int	dwSectorsPerBlock;
	unsigned int	dwDataBytesPerSector;
	unsigned int	dwTotalPhyBlocks;

	unsigned int	dwNumOfZone;
	NAND_ZONE		NandZoneList[1];
} VENUS_NAND_INFO, *PVENUS_NAND_INFO;

typedef struct __NAND_INFO__
{
	unsigned int	dwNandID;

	// Essential Timing
	unsigned int	Tals;				// ALE Setup Time(Unit: ns)
	unsigned int	Talh;				// ALE Hold Time(Unit: ns)
	unsigned int	Trp;				// /RE Pulse Width(Unit: ns)
	unsigned int	Twp;				// /WE Pulse Width(Unit: ns)
	unsigned int	Trhw;				// /RE High to /WE Low(Unit: ns)
	unsigned int	Twhr;				// /WE High to /RE Low(Unit: ns)

	// Essential Nand Size
	unsigned int	dwPageSize;				// 512 or 2048
	unsigned int	dwBlockSize;			// 16K or 128K
	unsigned int	dwPageAddressCycle;		// 2 or 3

	unsigned int	dwMinValidBlocks;
	unsigned int	dwMaxValidBlocks;
} NAND_INFO, *PNAND_INFO;

#endif	// __STRUCTURE_H__
