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
File        : fs_conf.h 
Purpose     : File system configuration
              Usually all configuration of the file system for your
              system can be done by changing this file.
              If you are using a big endian system or a totally
              different architecture, you may have to modify the file
              "fs_port.h". 
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_CONF_H_
#define _FS_CONF_H_

/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/


/*********************************************************************
*
*             #define long file name
*
**********************************************************************
*/

#define FS_USE_LONG_FILE_NAME 1

/*********************************************************************
*
*             Number of file handles
*
    Set the maximum number of simultaneously open files in your system.
    Please be aware, that the file system requires one FS_FILE structure
    for each open file. If you are using FAT, each file will also require
    two sector buffers.
*/

#define FS_MAXOPEN             15    /* Maximum number of file handles */

/*********************************************************************
*
*             POSIX 1003.1 like directory functions support
*
    Enables functions FS_OpenDir, FS_CloseDir, FS_ReadDir, FS_RewindDir,
    FS_MkDir and FS_RmDir.
*/

#define FS_POSIX_DIR_SUPPORT    1    /* POSIX 1003.1 like directory support */
#if FS_POSIX_DIR_SUPPORT
  #define FS_DIR_MAXOPEN        15    /* Maximum number of directory handles */
#endif

/*********************************************************************
*
*             OS Layer 
*
    Set all to 0, if you do not want OS support.
*/

#define FS_OS_EMBOS             0    /* 1 = use embOS */
#define FS_OS_UCOS_II           1    /* 1 = use uC/OS-II */
#define FS_OS_WINDOWS           0    /* 1 = use WINDOWS */
#if ((FS_OS_WINDOWS==1) && ((FS_OS_EMBOS==1) || (FS_OS_UCOS_II==1)))
  #error You must not use Windows at the same time as embOS or uC/OS-II!
#endif


/*********************************************************************
*
*             Time/Date support
*
  If your system does support ANSI C library functions for time/date,
  you can set this to 1. If it is set to 0, functions FS_OS_Get_Date
  and FS_OS_Get_Time will return the date 1.01.1980 0:00 unless you
  modify them.
*/

#define FS_OS_TIME_SUPPORT      0    /* 1 = time()/date supported */


/*********************************************************************
*
*             File System Layer
*
  You can turn on/off the file system layers used in your system.
  At least one layer has to be active. Because currently there is only
  one file system layer supported (FAT), you will usually not change
  this setting.
*/

#define FS_USE_FAT_FSL          1    /* FAT12/FAT16 file system */


/*********************************************************************
*
*             Logical Block Layer
*
  You can turn on/off cache of the Logical Block Layer. To use the
  cache on specific device, you also have to set its size in the
  device driver settings below.
*/

#define FS_USE_LB_READCACHE     1  /* 1 enables cache */


/*********************************************************************
*
*             Device Driver Support
*
  You can turn on/off device driver supported in your system.
  If you turn on a driver, please check also its settings in this
  file below.
*/

#define FS_USE_SMC_DRIVER       0    /* SmartMedia card driver       */
#define FS_USE_IDE_DRIVER       0    /* IDE driver                   */
#define FS_USE_WINDRIVE_DRIVER  0    /* Windows Logical Drive driver */
#define FS_USE_RAMDISK_DRIVER   0    /* RAM Disk driver              */

#ifdef MMC
#if MMC
   #define FS_USE_MMC_DRIVER       1    /* MMC/SD card driver           */
#else
   #define FS_USE_MMC_DRIVER       0    /* MMC/SD card driver           */
#endif
#else
   #define FS_USE_MMC_DRIVER       0    /* MMC/SD card driver           */
#endif

#define FS_USE_FLASH_DRIVER     0    /* Generic flash driver         */
#ifdef NAND
#if NAND
#define FS_USE_NAND_FLASH_DRIVER   1    /* Generic nand flash driver         */
//#undef FS_USE_LB_READCACHE
//#define FS_USE_LB_READCACHE        0    /* 1 enables cache */

#else
#define FS_USE_NAND_FLASH_DRIVER   0    /* Generic nand flash driver         */
#endif
#else
#define FS_USE_NAND_FLASH_DRIVER   0    /* Generic nand flash driver         */
#endif
#if (!defined(_WIN32) && (FS_USE_WINDRIVE_DRIVER))
  #error Windows Logical Drive driver needs Windows API
#endif

/*********************************************************************
*
*             FAT File System Layer defines
*/

#if FS_USE_FAT_FSL
  /*
     For each media in your system using FAT, the file system reserves
     memory to keep required information of the boot sector of that media.
     FS_MAXDEV is the number of device drivers in your system used
     with FAT, FS_FAT_MAXUNIT is the maximum number of logical units
     supported by one of the activated drivers.
  */
  #define FS_MAXDEV                 (FS_USE_SMC_DRIVER + FS_USE_WINDRIVE_DRIVER + FS_USE_RAMDISK_DRIVER + FS_USE_MMC_DRIVER + FS_USE_IDE_DRIVER + FS_USE_FLASH_DRIVER + FS_USE_NAND_FLASH_DRIVER)
  #define FS_FAT_MAXUNIT            1         /* max number of medias per device */
  #define FS_FAT_NOFAT32            0         /* 1 disables FAT32 support */
  #define FS_FAT_NOFORMAT           0         /* 1 disables code for formatting a media */
  #define FS_FAT_DISKINFO           1         /* 0 disables command FS_CMD_GET_DISKFREE */
  #define FS_FAT_FWRITE_UPDATE_DIR  1         /* 1 FS_FWrite modifies directory (default), 0 directory 
                                                 is modified by FS_FClose */
  /*
    Do not change following defines !
    They may become configurable in future versions of the file system.
  */
  #define FS_FAT_SEC_SIZE       0x200     /* do not change for FAT */
#endif /* FS_USE_FAT_FSL */


/*********************************************************************
*
*             RAMDISK_DRIVER defines
*/

#if FS_USE_RAMDISK_DRIVER
  /*
    Define size of your RAM disk here.
    You specify the number of sectors (512 bytes) here.
  */

  #define FS_RR_BLOCKNUM      64*1024        /* 16KB RAM */

  /*
    Do not change following define !
    It may become configurable in future versions of the file system.
  */
  #define FS_RR_BLOCKSIZE     0x200     /* do not change for FAT */
#endif  /* FS_USE_RAMDISK_DRIVER */


/*********************************************************************
*
*             SMC_DRIVER defines
*
  Settings of the generic Smartmedia Card driver.
  For using SMC in your system, you will have to provide basic
  hardware access functions. Please check device\smc\hardware\XXX\smc_x_hw.c
  and device\smc\hardware\XXX\smc_x_hw.h for samples.
*/

#if FS_USE_SMC_DRIVER
  /*
    Number of card readers in your system.
    Please note, that even if your system does have more than one
    SMC slot, it might be unable to access them both at the same
    time, because they share resources (e.g. same data port). If that
    is true for your system, you must ensure, that your implementation
    of "FS_OS_Lock_deviceop(&_FS_smcdevice_driver,id)" blocks the device
    driver for all values of "id"; the default implementation of the
    file system's OS Layer does so.
  */
  #define FS_SMC_MAXUNIT                    2  /* EP7312 SMC + on board NAND */
  /*
    The following define does tell the generic driver, if your
    system can directly check the SMC RY/BY line. 
  */
  #define FS_SMC_HW_SUPPORT_BSYLINE_CHECK   0  /* EP7312 does not support */
  /*
    FS_SMC_HW_NEEDS_POLL has to be set, if your SMC hardware driver
    needs to be called periodically for status check (e.g. diskchange).
    In such a case, the generic driver does provide a function
    "void FS_smc_check_card(FS_u32 id)", which has to be called
    periodically by your system for each card reader.
  */
  #define FS_SMC_HW_NEEDS_POLL              1  /* EP7312 needs poll for diskchange */
  /*
    Logical Block Read Cache Settings for SMC driver.
    Options are not used if FS_USE_LB_READCACHE is 0.
  */
  #define FS_SMC_CACHENUM         10     /* Number of sector buffers; 0 disables cache for this device. */
  /*
    The minimum erase unit on an SMC is a physical block, which consists
    of 16 or 32 pages. A page is 256+8 or 512+16 bytes. If the file system
    is going to change a single sector (512 bytes) on the media, the SMC
    driver usually has to copy the whole corresponding physical block to
    a new block. To avoid that time consuming task for every single sector
    write, you can provide the SMC driver with a number of sector caches
    (512 bytes each). The driver will use them to cache sequential writes 
    to the media. If you set the value to 0, the SMC driver will not cache 
    any write access. A value of 1 makes no sense. The maximum of the media's 
    cluster size (number of logical sectors) and the number of bytes you 
    write with FS_FWrites divided by 512 is typically the best speed 
    performance value. Bigger values usually make sense only, if you plan to 
    bypass the FAT system by e.g. using the FS_IoCtl command
    FS_CMD_WRITE_SECTOR for a large number of sequential sectors.
  */
  #define FS_SMC_PAGE_BUFFER 0         /* Number of sector write caches; 0 disables write cache function. */
#endif /* FS_USE_SMC_DRIVER */


/*********************************************************************
*
*             MMC_DRIVER defines
*
  Settings of the generic MMC/SD card driver.
  For using MMC in your system, you will have to provide basic
  hardware access functions. Please check device\mmc_sd\hardware\XXX\mmc_x_hw.c
  and device\mmc_sd\hardware\XXX\mmc_x_hw.h for samples.
*/

#if FS_USE_MMC_DRIVER
  /*
    Number of card readers in your system.
    Please note, that even if your system does have more than one
    MMC interface, it might be unable to access them both at the same
    time, because they share resources (e.g. same data line). If that
    is true for your system, you must ensure, that your implementation
    of "FS_OS_Lock_deviceop(&_FS_smcdevice_driver,id)" blocks the device
    driver for all values of "id"; the default implementation of the
    file system's OS Layer does so.
  */
  #define FS_MMC_MAXUNIT                    1  
  /*
    FS_MMC_HW_NEEDS_POLL has to be set, if your MMC hardware driver
    needs to be called periodically for status check (e.g. diskchange).
  */
  #define FS_MMC_HW_NEEDS_POLL              1
  /*
    Logical Block Read Cache Settings for MMC driver.
    Options are not used if FS_USE_LB_READCACHE is 0.
  */
  #define FS_MMC_CACHENUM                   10  /* Number of sector buffers; 0 disables cache for this device. */
#endif /* FS_USE_MMC_DRIVER */


/*********************************************************************
*
*             IDE_DRIVER defines
*
  Settings of the generic IDE driver. This driver is also used to
  access CF cards. For using IDE in your system, you will have to provide basic
  hardware access functions. Please check device\ide\hardware\XXX\ide_x_hw.c
  and device\ide\hardware\XXX\ide_x_hw.h for samples.
*/

#if FS_USE_IDE_DRIVER
  /*
    Number of card readers in your system.
    Please note, that even if your system does have more than one
    IDE interface, it might be unable to access them both at the same
    time, because they share resources (e.g. same data line). If that
    is true for your system, you must ensure, that your implementation
    of "FS_OS_Lock_deviceop(&_FS_idedevice_driver,id)" blocks the device
    driver for all values of "id"; the default implementation of the
    file system's OS Layer does so.
  */
  #define FS_IDE_MAXUNIT                    1
  /*
    FS_IDE_HW_NEEDS_POLL has to be set, if your IDE hardware driver
    needs to be called periodically for status check (e.g. diskchange).
  */
  #define FS_IDE_HW_NEEDS_POLL              1  
  /*
    Logical Block Read Cache Settings for IDE driver.
    Options are not used if FS_USE_LB_READCACHE is 0.
  */
  #define FS_IDE_CACHENUM         10     /* Number of sector buffers; 0 disables cache for this device. */
#endif /* FS_USE_IDE_DRIVER */


/*********************************************************************
*
*             FLASH_DRIVER defines
*/

#if FS_USE_FLASH_DRIVER
  /* Driver is currently not released */
  /* Size of the used ram buffer, set to 0 if no ram buffer is used */
  #define FS_FLASH_RAMBUFFER      0x4000
  /* Enable/disable wear leveling for flash memory */
  #define FS_FLASHWEARLEVELING 1 /* 1 = on, 0 = off */
#endif


/*********************************************************************
*
*             WINDRIVE_DRIVER defines
*
  This driver does allow to use any Windows logical driver on a
  Windows NT system with the file system. Please be aware, that Win9X
  is not supported, because it cannot access logical drives with
  "CreateFile".
*/

#if FS_USE_WINDRIVE_DRIVER
  /*
    The following define tells WINDRIVE, how many logical drives
    of your NT system you are going to access with the file system.
    if your are going to use more than 2 logical drives, you
    will have to modify function "_FS_wd_devstatus" of module
    device\windriver\wd_misc.c.
  */
  #define FS_WD_MAXUNIT         2             /* number of windows drives */
  /*
    Specify names of logical Windows drives used with the file system. For example,
    "\\\\.\\A:" is usually the floppy of your computer.
  */
  #define FS_WD_DEV0NAME        "\\\\.\\A:"   /* Windows drive name for "windrv:0:" */
  #define FS_WD_DEV1NAME        "\\\\.\\B:"   /* Windows drive name for "windrv:1:" */
  /*
    To improve performance of WINDRIVE, it does use sector caches
    for read/write operations to avoid real device operations for
    each read/write access. The number of caches can be specified
    below and must not be smaller than 1.
  */
  #define FS_WD_CACHENUM        40            /* number of read caches per drive */
  #define FS_WD_WBUFFNUM        20            /* number of write caches per drive */
  /*
    Do not change following define !
    It may become configurable in future versions of the file system.
  */
  #define FS_WD_BLOCKSIZE       0x200         /* do not change for FAT */
#endif  /* FS_USE_WINDRIVE_DRIVER */

/*********************************************************************
*
*             NAND_FLASH_DRIVER defines
*
*/

#if FS_USE_NAND_FLASH_DRIVER
  /*
    Number of card readers in your system.
    Please note, that even if your system does have more than one
    MMC interface, it might be unable to access them both at the same
    time, because they share resources (e.g. same data line). If that
    is true for your system, you must ensure, that your implementation
    of "FS_OS_Lock_deviceop(&_FS_smcdevice_driver,id)" blocks the device
    driver for all values of "id"; the default implementation of the
    file system's OS Layer does so.
  */
  #define FS_NAND_FLASH_MAXUNIT                    1  
  /*
    FS_MMC_HW_NEEDS_POLL has to be set, if your MMC hardware driver
    needs to be called periodically for status check (e.g. diskchange).
  */
  #define FS_NAND_FLASH_HW_NEEDS_POLL              1
  /*
    Logical Block Read Cache Settings for MMC driver.
    Options are not used if FS_USE_LB_READCACHE is 0.
  */
  #define FS_NAND_FLASH_CACHENUM                   10  /* Number of sector buffers; 0 disables cache for this device. */
#endif /* FS_USE_NAND_FLASH_DRIVER */

#endif  /* _FS_CONF_H_ */


