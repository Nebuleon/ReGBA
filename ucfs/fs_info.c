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
File        : fs_conf.c
Purpose     : File system configurable global data
              Unless you are going to add own device drivers, or you
              would like to modify e.g. order in the device table, you
              do not have to modify this file. Usually all configuration
              can be made in fs_conf.h.
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
#include "fs_fsl.h"
#include "fs_int.h"
#include "fs_conf.h"


/*********************************************************************
*
*             Externals
*
**********************************************************************
*/

/*********************************************************************
*
*             File System Layer Function Tables
*/

/* FAT function table */
#ifdef FS_USE_FAT_FSL
 extern const FS__fsl_type FS__fat_functable;
#endif /* FS_USE_FAT_FSL */


/*********************************************************************
*
*             Device Driver Function Tables
*
  If you add an own device driver, you will have to add an external
  for its function table here.
*/

/* RAMDISK_DRIVER function table */
#if FS_USE_RAMDISK_DRIVER
  extern const FS__device_type    FS__ramdevice_driver;
#endif  /* FS_USE_RAMDISK_DRIVER */

/* WINDRIVE_DRIVER function table */
#if FS_USE_WINDRIVE_DRIVER
  extern const FS__device_type    FS__windrive_driver;
#endif  /* FS_USE_WINDRIVE_DRIVER */

/* SMC_DRIVER function table */
#if FS_USE_SMC_DRIVER
  extern const FS__device_type    FS__smcdevice_driver;
#endif /* FS_USE_SMC_DRIVER */

/* MMC_DRIVER function table */
#if FS_USE_MMC_DRIVER
  extern const FS__device_type    FS__mmcdevice_driver;
#endif /* FS_USE_SMC_DRIVER */

#if FS_USE_IDE_DRIVER
  extern const FS__device_type    FS__idedevice_driver;
#endif /* FS_USE_IDE_DRIVER */

#if FS_USE_FLASH_DRIVER
  extern const FS__device_type    FS__flashdevice_driver;
#endif /* FS_USE_FLASH_DRIVER */

#if FS_USE_NAND_FLASH_DRIVER
  extern const FS__device_type    FS__nanddevice_driver;
#endif /* FS_USE_FLASH_DRIVER */


/*********************************************************************
*
*             Local variables        
*
**********************************************************************
*/

/*********************************************************************
*
*             Logical Block Layer Cache
*
  If FS_USE_LB_READCACHE is enabled, then the buffer definition
  for each media is done here according to the settings in
  fs_conf.h.
*/

#if FS_USE_LB_READCACHE
  /*
    RAMDISK cache settings.
    There is no cache required for RAM.
  */
  #define FS_CACHEINFO_RAMDISK_DRIVER     0,

  /* 
    Windrive cache settings. 
    The driver does have its own cache. Therefore we never
    use LB read cache for it.
  */
  #define FS_CACHEINFO_WINDRIVE_DRIVER    0,

  /*
    FLASH cache settings.
    There is no read cache required for flash.
  */
  #define FS_CACHEINFO_FLASH_DRIVER       0,

  /* SmartMedia cache settings */
  #ifndef FS_SMC_CACHENUM
    #define FS_SMC_CACHENUM 0
  #endif
  #if ((FS_SMC_CACHENUM) && (FS_USE_SMC_DRIVER))
    static FS__CACHE_BUFFER  _FS__SMC_Buffers[FS_SMC_MAXUNIT][FS_SMC_CACHENUM];
    static FS__LB_CACHE      _FS__SMC_Cache[FS_SMC_MAXUNIT] = 
      #if (FS_SMC_MAXUNIT==1)
        { {FS_SMC_CACHENUM, 0, &_FS__SMC_Buffers[0][0]} };
      #elif (FS_SMC_MAXUNIT==2)
        { {FS_SMC_CACHENUM, 0, &_FS__SMC_Buffers[0][0]},
          {FS_SMC_CACHENUM, 0, &_FS__SMC_Buffers[1][0]} };
      #else
        #error Please define _FS__SMC_Cache for desired number of units
      #endif
    #define FS_CACHEINFO_SMC_DRIVER     &_FS__SMC_Cache[0],
  #else
    #define FS_CACHEINFO_SMC_DRIVER     0,
  #endif
  
  /* MMC/SD cache settings */
  #ifndef FS_MMC_CACHENUM
    #define FS_MMC_CACHENUM 0
  #endif
  #if ((FS_MMC_CACHENUM) && (FS_USE_MMC_DRIVER))
    static FS__CACHE_BUFFER  _FS__MMC_Buffers[FS_MMC_MAXUNIT][FS_MMC_CACHENUM];
    static FS__LB_CACHE      _FS__MMC_Cache[FS_MMC_MAXUNIT] = 
      #if (FS_MMC_MAXUNIT==1)
        { {FS_MMC_CACHENUM, 0, &_FS__MMC_Buffers[0][0]} };
      #elif (FS_MMC_MAXUNIT==2)
        { {FS_MMC_CACHENUM, 0, &_FS__MMC_Buffers[0][0]},
          {FS_MMC_CACHENUM, 0, &_FS__MMC_Buffers[1][0]} };
      #else
        #error Please define _FS__MMC_Cache for desired number of units
      #endif
    #define FS_CACHEINFO_MMC_DRIVER     &_FS__MMC_Cache[0],
  #else
    #define FS_CACHEINFO_MMC_DRIVER     0,
  #endif

   /* NAND cache settings */
  #ifndef FS_NAND_FLASH_CACHENUM
    #define FS_NAND_FLASH_CACHENUM 0
  #endif
  #if ((FS_NAND_FLASH_CACHENUM) && (FS_USE_NAND_FLASH_DRIVER))
    static FS__CACHE_BUFFER  _FS__NAND_Buffers[FS_NAND_FLASH_MAXUNIT][FS_NAND_FLASH_CACHENUM];
    static FS__LB_CACHE      _FS__NAND_Cache[FS_NAND_FLASH_MAXUNIT] = 
      #if (FS_NAND_FLASH_MAXUNIT==1)
        { {FS_NAND_FLASH_CACHENUM, 0, &_FS__NAND_Buffers[0][0]} };
      #elif (FS_MMC_MAXUNIT==2)
        { {FS_NAND_FLASH_CACHENUM, 0, &_FS__NAND_Buffers[0][0]},
          {FS_NAND_FLASH_CACHENUM, 0, &_FS__NAND_Buffers[1][0]} };
      #else
        #error Please define _FS__NAND_FLASH_Cache for desired number of units
      #endif
    #define FS_CACHEINFO_NAND_FLASH_DRIVER     &_FS__NAND_Cache[0],
  #else
    #define FS_CACHEINFO_NAND_FLASH_DRIVER     0,
  #endif
  
  /* IDE cache settings */
  #ifndef FS_IDE_CACHENUM
    #define FS_IDE_CACHENUM 0
  #endif
  #if ((FS_IDE_CACHENUM) && (FS_USE_IDE_DRIVER))
    static FS__CACHE_BUFFER  _FS__IDE_Buffers[FS_IDE_MAXUNIT][FS_IDE_CACHENUM];
    static FS__LB_CACHE      _FS__IDE_Cache[FS_IDE_MAXUNIT] = 
      #if (FS_IDE_MAXUNIT==1)
        { {FS_IDE_CACHENUM, 0, &_FS__IDE_Buffers[0][0]} };
      #elif (FS_IDE_MAXUNIT==2)
        { {FS_IDE_CACHENUM, 0, &_FS__IDE_Buffers[0][0]},
          {FS_IDE_CACHENUM, 0, &_FS__IDE_Buffers[1][0]} };
      #else
        #error Please define _FS__IDE_Cache for desired number of units
      #endif
    #define FS_CACHEINFO_IDE_DRIVER     &_FS__IDE_Cache[0],
  #else
    #define FS_CACHEINFO_IDE_DRIVER     0,
  #endif
  
#else
  #define FS_CACHEINFO_RAMDISK_DRIVER     
  #define FS_CACHEINFO_WINDRIVE_DRIVER    
  #define FS_CACHEINFO_SMC_DRIVER         
  #define FS_CACHEINFO_MMC_DRIVER         
  #define FS_CACHEINFO_IDE_DRIVER
  #define FS_CACHEINFO_FLASH_DRIVER 
#endif  /* FS_USE_LB_READCACHE */


/*********************************************************************
*
*             Global variables        
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_devinfo
*
  This data structure does tell the file system, which file system layer
  (currently FAT only) and device driver (e.g. SmartMedia or RAM disk) is
  used for which device name.
  If you do not specify the device name in an file system API call, the
  first entry in this table will be used.
*/

#ifndef FS_DEVINFO
  #if FS_USE_SMC_DRIVER
    #define FS_DEVINFO_DEVSMC     { "smc",    &FS__fat_functable, &FS__smcdevice_driver, FS_CACHEINFO_SMC_DRIVER 0 },
  #else
    #define FS_DEVINFO_DEVSMC
  #endif
  #if FS_USE_MMC_DRIVER
    #define FS_DEVINFO_DEVMMC     { "mmc",    &FS__fat_functable, &FS__mmcdevice_driver, FS_CACHEINFO_MMC_DRIVER 0 },
  #else
    #define FS_DEVINFO_DEVMMC
  #endif
		
  #if FS_USE_NAND_FLASH_DRIVER
    #define FS_DEVINFO_DEVNAND     { "nfl",    &FS__fat_functable, &FS__nanddevice_driver, FS_CACHEINFO_NAND_FLASH_DRIVER 0 },
  #else
    #define FS_DEVINFO_DEVNAND
  #endif
		
  #if FS_USE_IDE_DRIVER
    #define FS_DEVINFO_DEVIDE     { "ide",    &FS__fat_functable, &FS__idedevice_driver, FS_CACHEINFO_IDE_DRIVER 0 },
  #else
    #define FS_DEVINFO_DEVIDE
  #endif /* FS_USE_IDE_DRIVER */
  #if FS_USE_FLASH_DRIVER
    #define FS_DEVINFO_DEVFLASH   { "flash",    &FS__fat_functable, &FS__flashdevice_driver, FS_CACHEINFO_FLASH_DRIVER 0 },
  #else
    #define FS_DEVINFO_DEVFLASH
  #endif /* FS_USE_FLASH_DRIVER */
  #if FS_USE_WINDRIVE_DRIVER
    #define FS_DEVINFO_DEVWINDRV  { "windrv", &FS__fat_functable, &FS__windrive_driver, FS_CACHEINFO_WINDRIVE_DRIVER 0 },
  #else
    #define FS_DEVINFO_DEVWINDRV
  #endif
  #if FS_USE_RAMDISK_DRIVER
    #define FS_DEVINFO_DEVRAM     { "ram",    &FS__fat_functable, &FS__ramdevice_driver, FS_CACHEINFO_RAMDISK_DRIVER 0 },
  #else
    #define FS_DEVINFO_DEVRAM
  #endif
  #define FS_DEVINFO FS_DEVINFO_DEVSMC FS_DEVINFO_DEVMMC FS_DEVINFO_DEVIDE FS_DEVINFO_DEVFLASH FS_DEVINFO_DEVWINDRV FS_DEVINFO_DEVRAM FS_DEVINFO_DEVRAM FS_DEVINFO_DEVNAND
#endif  /* FS_DEVINFO */

const FS__devinfo_type _FS__devinfo[] = { FS_DEVINFO };

const FS__devinfo_type *const FS__pDevInfo = _FS__devinfo;

const unsigned int FS__maxdev=sizeof(_FS__devinfo)/sizeof(FS__devinfo_type);

const unsigned int FS__fat_maxunit=FS_FAT_MAXUNIT;


