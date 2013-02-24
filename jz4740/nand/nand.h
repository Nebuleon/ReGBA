//
// Copyright (c) Ingenic Semiconductor Co., Ltd. 2007.
//

#ifndef __NAND_H__
#define __NAND_H__

#include <windows.h>
#include <types.h>
#include <excpt.h>
#include <tchar.h>
#include <devload.h>
#include <diskio.h>
#include <storemgr.h>


#ifdef __cplusplus
extern "C" {
#endif

#define RD_SIZE 0x00100000
#define BYTES_PER_SECTOR 512

//
// Structure to keep track of a disk.  NUM_MEM_WINDOWS are maintained and
// remapped as needed.  One of these will remain fixed at location 0 to make
// FAT file system faster (the FAT is at the beginning of the disk).
//
typedef struct _DISK {
    struct _DISK * d_next;
    CRITICAL_SECTION d_DiskCardCrit;// guard access to global state and card    
    DISK_INFO d_DiskInfo;    // for DISK_IOCTL_GET/SETINFO
    PBYTE pbRAM;
    LPWSTR d_ActivePath;    // registry path to active key for this device
} DISK, * PDISK; 

//
// Global Variables
//
extern CRITICAL_SECTION v_DiskCrit;    // guard access to disk structure list
extern PDISK v_DiskList;


//
// Global functions
//
DWORD DoDiskIO(PDISK pDisk, DWORD Opcode, PSG_REQ pSG);
//DWORD GetDiskInfo(PDISK pDisk, PDISK_INFO pInfo);
DWORD SetDiskInfo(PDISK pDisk, PDISK_INFO pInfo);
DWORD GetDiskStateError(DWORD DiskState);
VOID CloseDisk(PDISK pDisk);
BOOL GetFolderName(PDISK pDisk, LPWSTR pOutBuf, DWORD nOutBufSize, DWORD * pBytesReturned);
//BOOL GetDeviceInfo(PDISK pDisk, PSTORAGEDEVICEINFO pInfo);
BOOL RegGetValue(PDISK pDisk, LPWSTR pwsName, PDWORD pdwValue);
DWORD GetSectorAddr (PDISK pDisk, DWORD dwSector);

#ifdef DEBUG
//
// Debug zones
//
#define ZONE_ERROR      DEBUGZONE(0)
#define ZONE_WARNING    DEBUGZONE(1)
#define ZONE_FUNCTION   DEBUGZONE(2)
#define ZONE_INIT       DEBUGZONE(3)
#define ZONE_PCMCIA     DEBUGZONE(4)
#define ZONE_IO         DEBUGZONE(5)
#define ZONE_MISC       DEBUGZONE(6)

#endif  // DEBUG

#ifdef __cplusplus
}
#endif

#endif // __NAND_H__

